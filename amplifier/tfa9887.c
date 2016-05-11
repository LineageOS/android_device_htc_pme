/*
 * Copyright (C) 2013, The CyanogenMod Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "tfa9887"
//#define LOG_NDEBUG 0

#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/ioctl.h>

#include <cutils/log.h>

#include <system/audio.h>
#include <tinyalsa/asoundlib.h>

#include "tfa9887.h"

#define UNUSED __attribute__((unused))

/* Module variables */

const struct mode_config_t mode_configs[TFA9887_MODE_MAX] = {
    {   /* Playback */
        .preset = PRESET_PLAYBACK,
        .eq = EQ_PLAYBACK,
        .drc = DRC_PLAYBACK,
    },
    {   /* Voice */
        .preset = PRESET_VOICE,
        .eq = EQ_VOICE,
        .drc = DRC_VOICE,
    },
    {   /* VOIP */
        .preset = PRESET_VOIP,
        .eq = EQ_VOIP,
        .drc = DRC_VOIP,
    }
};

static struct tfa9887_amp_t *main_amp = NULL;

/* Helper functions */

static int i2s_interface_en(bool enable)
{
    enum mixer_ctl_type type;
    struct mixer_ctl *ctl;
    struct mixer *mixer = mixer_open(0);

    if (mixer == NULL) {
        ALOGE("Error opening mixer 0");
        return -1;
    }

    ctl = mixer_get_ctl_by_name(mixer, I2S_MIXER_CTL);
    if (ctl == NULL) {
        mixer_close(mixer);
        ALOGE("%s: Could not find %s\n", __func__, I2S_MIXER_CTL);
        return -ENODEV;
    }

    type = mixer_ctl_get_type(ctl);
    if (type != MIXER_CTL_TYPE_BOOL) {
        ALOGE("%s: %s is not supported\n", __func__, I2S_MIXER_CTL);
        mixer_close(mixer);
        return -ENOTTY;
    }

    mixer_ctl_set_value(ctl, 0, enable);
    mixer_close(mixer);
    return 0;
}

void * write_dummy_data(void *param)
{
    struct tfa9887_amp_t *amp = (struct tfa9887_amp_t *) param;
    uint8_t *buffer;
    int size;
    struct pcm *pcm;
    struct pcm_config config = {
        .channels = 2,
        .rate = 48000,
        .period_size = 256,
        .period_count = 2,
        .format = PCM_FORMAT_S16_LE,
        .start_threshold = config.period_size * config.period_count - 1,
        .stop_threshold = UINT_MAX,
        .silence_threshold = 0,
        .avail_min = 1,
    };

    if (i2s_interface_en(true)) {
        ALOGE("%s: Failed to enable I2S interface\n", __func__);
        return NULL;
    }

    pcm = pcm_open(0, 0, PCM_OUT | PCM_MONOTONIC, &config);
    if (!pcm || !pcm_is_ready(pcm)) {
        ALOGE("pcm_open failed: %s", pcm_get_error(pcm));
        if (pcm) {
            goto err_close_pcm;
        }
        goto err_disable_i2s;
    }

    size = 1024 * 8;
    buffer = calloc(1, size);
    if (!buffer) {
        ALOGE("%s: failed to allocate buffer", __func__);
        goto err_close_pcm;
    }

    do {
        if (pcm_write(pcm, buffer, size)) {
            ALOGE("%s: pcm_write failed", __func__);
        }
        pthread_mutex_lock(&amp->mutex);
        amp->writing = true;
        pthread_cond_signal(&amp->cond);
        pthread_mutex_unlock(&amp->mutex);
    } while (amp->initializing);

err_free:
    free(buffer);
err_close_pcm:
    pcm_close(pcm);
err_disable_i2s:
    i2s_interface_en(false);
    return NULL;
}

static uint32_t get_mode(audio_mode_t mode)
{
    switch (mode) {
        case AUDIO_MODE_IN_CALL:
            return TFA9887_MODE_VOICE;
        case AUDIO_MODE_IN_COMMUNICATION:
            return TFA9887_MODE_VOIP;
        case AUDIO_MODE_NORMAL:
        default:
            return TFA9887_MODE_PLAYBACK;
    }
}

static int read_file(const char *file_name, uint8_t *buf, int sz, int seek)
{
    int ret;
    int fd;

    fd = open(file_name, O_RDONLY);
    if (fd < 0) {
        ret = errno;
        ALOGE("%s: unable to open file %s: %d", __func__, file_name, ret);
        return ret;
    }

    lseek(fd, seek, SEEK_SET);

    ret = read(fd, buf, sz);
    if (ret < 0) {
        ret = errno;
        ALOGE("%s: error reading from file %s: %d", __func__, file_name, ret);
    }

    close(fd);
    return ret;
}

static void bytes2data(const uint8_t bytes[], int num_bytes,
        uint32_t data[])
{
    int i; /* index for data */
    int k; /* index for bytes */
    uint32_t d;
    int num_data = num_bytes / 3;

    for (i = 0, k = 0; i < num_data; i++, k += 3) {
        d = (bytes[k] << 16) | (bytes[k + 1] << 8) | (bytes[k + 2]);
        /* sign bit was set*/
        if (bytes[k] & 0x80) {
            d = - ((1 << 24) - d);
        }
        data[i] = d;
    }
}

static void data2bytes(const int32_t data[], int num_data, uint8_t bytes[])
{
    int i; /* index for data */
    int k; /* index for bytes */
    uint32_t d;

    for (i = 0, k = 0; i < num_data; i++, k += 3) {
        if (data[i] >= 0) {
            d = MIN(data[i], (1 << 23) - 1);
        } else {
            d = (1 << 24) - MIN(-data[i], 1 << 23);
        }
        bytes[k] = (d >> 16) & 0xff;
        bytes[k + 1] = (d >> 8) & 0xff;
        bytes[k + 2] = d & 0xff;
    }
}

/* TFA9887 helper functions */

static int tfa9887_read_reg(struct tfa9887_amp_t *amp, uint8_t reg,
        uint16_t *val)
{
    int ret = 0;
    struct tfa9895_i2c_buffer cmd_buf;

    /* kernel uses unsigned int */
    unsigned int reg_val[2];
    uint8_t buf[2];

    if (!amp) {
        return -ENODEV;
    }

    memset(&cmd_buf, 0, sizeof(cmd_buf));
    cmd_buf.size = 1;
    cmd_buf.buffer[0] = reg;
    if ((ret = ioctl(amp->fd, TFA9895_WRITE_CONFIG(cmd_buf),
                    &cmd_buf)) != 0) {
        ALOGE("ioctl TFA9895_WRITE_CONFIG failed, ret = %d", -errno);
        goto read_reg_err;
    }

    memset(&cmd_buf, 0, sizeof(cmd_buf));
    cmd_buf.size = 2;
    if ((ret = ioctl(amp->fd, TFA9895_READ_CONFIG(cmd_buf),
                    &cmd_buf)) != 0) {
        ALOGE("ioctl TFA9895_READ_CONFIG failed, ret = %d", -errno);
        goto read_reg_err;
    }

    *val = ((cmd_buf.buffer[0] << 8) | cmd_buf.buffer[1]);

read_reg_err:
    return ret;
}

static int tfa9887_write_reg(struct tfa9887_amp_t *amp, uint8_t reg,
        uint16_t val)
{
    int ret = 0;
    struct tfa9895_i2c_buffer cmd_buf;

    if (!amp) {
        return -ENODEV;
    }

    memset(&cmd_buf, 0, sizeof(cmd_buf));
    cmd_buf.size = 4;
    cmd_buf.buffer[0] = reg;
    cmd_buf.buffer[1] = (0xFF00 & val) >> 8;
    cmd_buf.buffer[2] = (0x00FF & val);
    cmd_buf.buffer[3] = 0;
    if ((ret = ioctl(amp->fd, TFA9895_WRITE_CONFIG(cmd_buf),
                    &cmd_buf)) != 0) {
        ALOGE("ioctl TFA9895_WRITE_CONFIG failed, ret = %d", -errno);
        goto write_reg_err;
    }

write_reg_err:
    return ret;
}

static int tfa9887_read(struct tfa9887_amp_t *amp, int addr, uint8_t *buf,
        int len, int alignment)
{
    int rc = 0;
    int bytes_read = 0;
    /* Round chunk size down to nearest alignment */
    int chunk_size = ROUND_DOWN(MAX_I2C_LENGTH, alignment);
    struct tfa9895_i2c_buffer cmd_buf;

    if (!amp) {
        return -ENODEV;
    }

    memset(&cmd_buf, 0, sizeof(cmd_buf));
    cmd_buf.size = 1;
    cmd_buf.buffer[0] = (0xFF & addr);
    if ((rc = ioctl(amp->fd, TFA9895_WRITE_CONFIG(cmd_buf),
                    &cmd_buf)) != 0) {
        ALOGE("ioctl TFA9895_WRITE_CONFIG failed, ret = %d", -errno);
        goto read_err;
    }

    do {
        memset(&cmd_buf, 0, sizeof(cmd_buf));
        cmd_buf.size = MIN(len - bytes_read, chunk_size);
        if ((rc = ioctl(amp->fd, TFA9895_READ_CONFIG(cmd_buf),
                        &cmd_buf)) != 0) {
            ALOGE("ioctl TFA9895_READ_CONFIG failed, ret = %d", -errno);
            goto read_err;
        }
        memcpy(buf + bytes_read, cmd_buf.buffer, cmd_buf.size);
        bytes_read += cmd_buf.size;
    } while (bytes_read < len);

read_err:
    return rc;
}

static int tfa9887_write(struct tfa9887_amp_t *amp, int addr,
        const uint8_t *buf, int len, int alignment)
{
    int rc = 0;
    int bytes_written = 0;
    int to_write = 0;
    int chunk_size = ROUND_DOWN(MAX_I2C_LENGTH, alignment);
    struct tfa9895_i2c_buffer cmd_buf;

    if (!amp) {
        return -ENODEV;
    }

    do {
        /* Write the address out over I2C */
        memset(&cmd_buf, 0, sizeof(cmd_buf));
        to_write = MIN(len - bytes_written, chunk_size - 1);
        cmd_buf.size = to_write + 1;
        cmd_buf.buffer[0] = (0xFF & addr);
        memcpy(cmd_buf.buffer + 1, buf + bytes_written, to_write);
        if ((rc = ioctl(amp->fd, TFA9895_WRITE_CONFIG(cmd_buf),
                        &cmd_buf)) != 0) {
            ALOGE("ioctl TFA9895_WRITE_CONFIG failed, ret = %d", -errno);
            goto write_err;
        }
        bytes_written += to_write;
    } while (bytes_written < len);

write_err:
    return rc;
}

static int tfa9887_read_mem(struct tfa9887_amp_t *amp, uint16_t start_offset,
        int num_words, uint32_t *p_values)
{
    int rc = 0;
    uint16_t cf_ctrl; /* the value to sent to the CF_CONTROLS register */
    uint8_t mem[num_words * 3]; /* each word is 24 bits */

    if (!amp) {
        return -ENODEV;
    }

    /* first set DMEM and AIF, leaving other bits intact */
    rc = tfa9887_read_reg(amp, TFA9887_CF_CONTROLS, &cf_ctrl);
    if (rc) {
        ALOGE("%s: Failure reading from register CF_CONTROLS: %d", __func__, rc);
        goto read_mem_err;
    }
    cf_ctrl &= ~0x000E; /* clear AIF & DMEM */
    cf_ctrl |= (TFA9887_DMEM_XMEM << 1); /* set DMEM, leave AIF cleared for autoincrement */
    rc = tfa9887_write_reg(amp, TFA9887_CF_CONTROLS, cf_ctrl);
    if (rc) {
        ALOGE("%s: Failure writing register CF_CONTROLS: %d", __func__, rc);
        goto read_mem_err;
    }
    rc = tfa9887_write_reg(amp, TFA9887_CF_MAD, start_offset);
    if (rc) {
        ALOGE("%s: Failure writing register CF_MAD: %d", __func__, rc);
        goto read_mem_err;
    }
    rc = tfa9887_read(amp, TFA9887_CF_MEM, mem, num_words * 3, 3);
    if (rc) {
        ALOGE("%s: Failure reading from CF_MEM: %d\n", __func__, rc);
        goto read_mem_err;
    }
    /* Re-assemble 24-bit values (stored in 32-bit value) using bytes2data */
    /* Assume p_values has enough memory allocated */
    bytes2data(mem, num_words * 3, p_values);

read_mem_err:
    return rc;
}

static int tfa9887_write_mem(struct tfa9887_amp_t *amp, uint16_t address,
        int size, uint8_t *buf)
{
    int rc = 0;
    uint16_t cf_ctrl;

    if (!amp) {
        return -ENODEV;
    }

    /* first set DMEM and AIF, leaving other bits intact */
    rc = tfa9887_read_reg(amp, TFA9887_CF_CONTROLS, &cf_ctrl);
    if (rc) {
        ALOGE("%s: Failure reading from register CF_CONTROLS: %d", __func__, rc);
        goto write_mem_err;
    }
    cf_ctrl &= ~0x000E; /* clear AIF & DMEM */
    cf_ctrl |= (TFA9887_DMEM_XMEM << 1); /* set DMEM, leave AIF cleared for autoincrement */
    rc = tfa9887_write_reg(amp, TFA9887_CF_CONTROLS, cf_ctrl);
    if (rc) {
        ALOGE("%s: Failed to write register CF_CONTROLS: %d\n", __func__, rc);
        goto write_mem_err;
    }
    rc = tfa9887_write_reg(amp, TFA9887_CF_MAD, address);
    if (rc) {
        ALOGE("%s: Failed to write register CF_MAD: %d\n", __func__, rc);
        goto write_mem_err;
    }
    /* Memory is always 3-byte aligned */
    rc = tfa9887_write(amp, TFA9887_CF_MEM, buf, size, 3);
    if (rc) {
        ALOGE("%s: Unable to write to %04x: %d\n", __func__, address, rc);
        goto write_mem_err;
    }

write_mem_err:
    return rc;
}

static int tfa9887_wait_reg(struct tfa9887_amp_t *amp, uint16_t reg,
        uint16_t ready_value, bool value_set, int max_tries, int interval)
{
    int rc = 0;
    int tries = 0;
    uint16_t value;

    if (!amp) {
        return -ENODEV;
    }

    do {
        rc = tfa9887_read_reg(amp, reg, &value);
        if (rc) {
            ALOGE("%s: error reading reg: 0x%02x: %d\n", __func__, reg, rc);
            goto wait_reg_err;
        }
        usleep(interval);
        tries++;
    } while ((value & ready_value) == (value_set ? 0 : ready_value) &&
            tries < max_tries);

    if (tries == max_tries) {
        rc = -EBUSY;
        ALOGE("%s: Timed out waiting for reg: 0x%02x, current: 0x%04x, expected: 0x%04x\n",
                __func__, reg, value, ready_value);
        goto wait_reg_err;
    }

wait_reg_err:
    return rc;
}

static int tfa9887_check_rpc_status(struct tfa9887_amp_t *amp)
{
    int rc = 0;
    int tries = 0;
    uint16_t cf_ctrl = 0x0002;
    uint16_t cf_mad = 0x0000;
    uint32_t rpc_status = 0;
    uint8_t mem[3];

    rc = tfa9887_write_reg(amp, TFA9887_CF_CONTROLS, cf_ctrl);
    if (rc) {
        ALOGE("%s: Failed to write register CF_CONTROLS: %d\n", __func__, rc);
        goto check_rpc_err;
    }
    rc = tfa9887_write_reg(amp, TFA9887_CF_MAD, cf_mad);
    if (rc) {
        ALOGE("%s: Failed to write register CF_MAD: %d\n", __func__, rc);
        goto check_rpc_err;
    }
    /* wait for mem to come to stable state */
    do {
        rc = tfa9887_read(amp, TFA9887_CF_MEM, mem, 3, 3);
        if (rc) {
            ALOGE("%s: Failed to read from CF_MEM: %d\n", __func__, rc);
            goto check_rpc_err;
        }
        rpc_status = ((mem[0] << 16) | (mem[1] << 8) | (mem[2] << 0));
        tries++;
        usleep(1000);
    } while (rpc_status && tries < 100);

    if (rpc_status) {
        rc = rpc_status + 100;
        goto check_rpc_err;
    }

check_rpc_err:
    return rc;
}

static int tfa9887_get_data(struct tfa9887_amp_t *amp, uint8_t module,
        uint8_t param, uint8_t *data, int length)
{
    int rc;
    uint16_t cf_ctrl = 0x0002;
    uint16_t cf_mad = 0x0001;
    uint8_t id[3];

    if (!amp) {
        return -ENODEV;
    }

    rc = tfa9887_write_reg(amp, TFA9887_CF_CONTROLS, cf_ctrl);
    if (rc) {
        ALOGE("%s: Failed to write register TFA98XX_CF_CONTROLS: %d\n",
                __func__, rc);
        goto get_data_err;
    }
    rc = tfa9887_write_reg(amp, TFA9887_CF_MAD, cf_mad);
    if (rc) {
        ALOGE("%s: Failed to write register TFA98XX_CF_MAD: %d\n",
                __func__, rc);
        goto get_data_err;
    }

    /* Instruct DSP which module/param to get */
    id[0] = 0;
    id[1] = module + 128;
    id[2] = param;
    rc = tfa9887_write(amp, TFA9887_CF_MEM, id, 3, 3);
    if (rc) {
        ALOGE("%s: Failed to write to CF_MEM: %d\n", __func__, rc);
        goto get_data_err;
    }

    /* Wake DSP to process written data */
    cf_ctrl |= (1 << 8) | (1 << 4); /* set the cf_req1 and cf_int bits */
    rc = tfa9887_write_reg(amp, TFA9887_CF_CONTROLS, cf_ctrl);
    if (rc) {
        ALOGE("%s: Failed to write to register CF_CONTROLS: %d\n", __func__, rc);
        goto get_data_err;
    }
    /* Wait for 0x100 to be set */
    rc = tfa9887_wait_reg(amp, TFA9887_CF_STATUS, 0x100, true, 100, 1000);
    if (rc) {
        ALOGE("%s: Timed out waiting for CF_STATUS: %d\n", __func__, rc);
        goto get_data_err;
    }
    /* Check DSP RPC status */
    rc = tfa9887_check_rpc_status(amp);
    if (rc) {
        ALOGE("%s: RPC status check returned error: %d\n", __func__, rc);
        goto get_data_err;
    }

    /* Start reading data */
    cf_mad = 0x0002;
    rc = tfa9887_write_reg(amp, TFA9887_CF_MAD, cf_mad);
    if (rc) {
        ALOGE("%s: Failed to write to register CF_MAD: %d\n", __func__, rc);
        goto get_data_err;
    }
    rc = tfa9887_read(amp, TFA9887_CF_MEM, data, length, 3);
    if (rc) {
        ALOGE("%s: Failed to read from CF_MEM: %d\n", __func__, rc);
        goto get_data_err;
    }

get_data_err:
    return rc;
}

static int tfa9887_set_data(struct tfa9887_amp_t *amp, uint8_t module,
        uint8_t param, uint8_t *data, int length)
{
    int rc;
    uint16_t cf_ctrl = 0x0002; /* the value to be sent to the CF_CONTROLS register: cf_req=00000000, cf_int=0, cf_aif=0, cf_dmem=XMEM=01, cf_rst_dsp=0 */
    uint16_t cf_mad = 0x0001; /* memory address to be accessed (0 : Status, 1 : ID, 2 : parameters) */
    uint8_t id[3];

    if (!amp) {
        return -ENODEV;
    }

    rc = tfa9887_write_reg(amp, TFA9887_CF_CONTROLS, cf_ctrl);
    if (rc) {
        ALOGE("%s: Failed to write register TFA98XX_CF_CONTROLS: %d\n",
                __func__, rc);
        goto set_data_err;
    }
    rc = tfa9887_write_reg(amp, TFA9887_CF_MAD, cf_mad);
    if (rc) {
        ALOGE("%s: Failed to write register TFA98XX_CF_MAD: %d\n",
                __func__, rc);
        goto set_data_err;
    }

    /* Instruct DSP which module/param to set */
    id[0] = 0;
    id[1] = module + 128;
    id[2] = param;
    rc = tfa9887_write(amp, TFA9887_CF_MEM, id, 3, 3);
    if (rc) {
        ALOGE("%s: Failed to write to CF_MEM: %d\n", __func__, rc);
        goto set_data_err;
    }

    /* Start writing data */
    rc = tfa9887_write(amp, TFA9887_CF_MEM, data, length, 3);
    if (rc) {
        ALOGE("%s: Failed to write CF_MEM: %d\n", __func__, rc);
        goto set_data_err;
    }

    /* Wake DSP to process written data */
    cf_ctrl |= (1 << 8) | (1 << 4); /* set the cf_req1 and cf_int bit */
    rc = tfa9887_write_reg(amp, TFA9887_CF_CONTROLS, cf_ctrl);
    if (rc) {
        ALOGE("%s: Failed to write register CF_CONTROLS: %d\n", __func__, rc);
        goto set_data_err;
    }
    /* Wait for 0x100 to be set */
    rc = tfa9887_wait_reg(amp, TFA9887_CF_CONTROLS, 0x100, true, 100, 1000);
    if (rc) {
        ALOGE("%s: Timed out waiting for CF_CONTROLS: %d\n", __func__, rc);
        goto set_data_err;
    }
    /* Check DSP RPC status */
    rc = tfa9887_check_rpc_status(amp);
    if (rc) {
        ALOGE("%s: RPC status check returned error: %d\n", __func__, rc);
        goto set_data_err;
    }

set_data_err:
    return rc;
}

/* Hardware functions */

static int tfa9887_load_patch(struct tfa9887_amp_t *amp, uint8_t *bytes, int length)
{
    int rc;
    int size;
    int index = 0;
    uint8_t buffer[MAX_I2C_LENGTH];
    uint32_t value = 0;
    uint16_t status;
    uint16_t status_ok = TFA9887_STATUS_VDDS | TFA9887_STATUS_PLLS | TFA9887_STATUS_CLKS;

    if (!amp) {
        return -ENODEV;
    }

    rc = tfa9887_read_reg(amp, TFA9887_STATUS, &status);
    if (rc) {
        if ((status & status_ok) != status_ok) {
            ALOGE("%s: Checking for 0x%04x, got: 0x%04x\n",
                    __func__, status_ok, status);
            return -EIO;
        }
    }
    while (index < length) {
        /* extract little endian length */
        size = bytes[index] | (bytes[index+1] << 8);
        index += 2;
        if ((index + size) > length) {
            /* outside the buffer, error in the input data */
            ALOGE("%s: Error in input data\n", __func__);
            return -EINVAL;
        }
        memcpy(buffer, bytes + index, size);
        rc = tfa9887_write(amp, buffer[0],
                &buffer[1], size, 1);
        ALOGV("%s: %d %d", __func__, buffer[0], size);
        if (rc) {
            ALOGE("%s: error writing: %d", __func__, rc);
            return -EIO;
        }
        index += size;
    }
    return 0;
}

static int tfa9887_load_dsp(struct tfa9887_amp_t *amp, const char *param_file)
{
    int param_id, module_id;
    uint8_t data[MAX_PARAM_SIZE];
    int num_bytes, error;
    char *suffix;

    suffix = strrchr(param_file, '.');
    if (suffix == NULL) {
        ALOGE("%s: Failed to determine parameter file type", __func__);
        return -EINVAL;
    } else if (strcmp(suffix, ".speaker") == 0) {
        param_id = PARAM_SET_LSMODEL;
        module_id = MODULE_SPEAKERBOOST;
    } else if (strcmp(suffix, ".config") == 0) {
        param_id = PARAM_SET_CONFIG;
        module_id = MODULE_SPEAKERBOOST;
    } else if (strcmp(suffix, ".preset") == 0) {
        param_id = PARAM_SET_PRESET;
        module_id = MODULE_SPEAKERBOOST;
    } else if (strcmp(suffix, ".drc") == 0) {
        param_id = PARAM_SET_DRC;
        module_id = MODULE_SPEAKERBOOST;
    } else {
        ALOGE("%s: Invalid DSP param file %s", __func__, param_file);
        return -EINVAL;
    }

    num_bytes = read_file(param_file, data, MAX_PARAM_SIZE, 0);
    if (num_bytes < 0) {
        error = num_bytes;
        ALOGE("%s: Failed to load file %s: %d", __func__, param_file, error);
        return -EIO;
    }

    return tfa9887_set_data(amp, module_id, param_id, data, num_bytes);
}

static int tfa9887_load_eq(struct tfa9887_amp_t *amp, const char *eq_file)
{
    uint8_t data[MAX_EQ_SIZE];
    const float disabled[5] = { 1.0, 0.0, 0.0, 0.0, 0.0 };
    float line[5];
    int32_t line_data[6];
    float max;
    FILE *f;
    int i, j;
    int idx, space, rc;

    memset(data, 0, MAX_EQ_SIZE);

    f = fopen(eq_file, "r");
    if (!f) {
        rc = errno;
        ALOGE("%s: Unable to open file %s: %d", __func__, eq_file, rc);
        return -EIO;
    }

    for (i = 0; i < MAX_EQ_LINES; i++) {
        rc = fscanf(f, "%d %f %f %f %f %f", &idx, &line[0], &line[1],
                &line[2], &line[3], &line[4]);
        if (rc != 6) {
            ALOGE("%s: %s has bad format: line must be 6 values\n",
                    __func__, eq_file);
            fclose(f);
            return -EINVAL;
        }

        if (idx != i + 1) {
            ALOGE("%s: %s has bad format: index mismatch\n",
                    __func__, eq_file);
            fclose(f);
            return -EINVAL;
        }

        if (!memcmp(disabled, line, 5)) {
            /* skip */
            continue;
        } else {
            max = (float) fabs(line[0]);
            /* Find the max */
            for (j = 1; j < 5; j++) {
                if (fabs(line[j]) > max) {
                    max = (float) fabs(line[j]);
                }
            }
            space = (int) ceil(log(max + pow(2.0, -23)) / log(2.0));
            if (space > 8) {
                fclose(f);
                ALOGE("%s: Invalid value encountered\n", __func__);
                return -EINVAL;
            }
            if (space < 0) {
                space = 0;
            }

            /* Pack line into bytes */
            line_data[0] = space;
            line_data[1] = (int32_t) (-line[4] * (1 << (23 - space)));
            line_data[2] = (int32_t) (-line[3] * (1 << (23 - space)));
            line_data[3] = (int32_t) (line[2] * (1 << (23 - space)));
            line_data[4] = (int32_t) (line[1] * (1 << (23 - space)));
            line_data[5] = (int32_t) (line[0] * (1 << (23 - space)));
            data2bytes(line_data, MAX_EQ_LINE_SIZE,
                    &data[i * MAX_EQ_LINE_SIZE * MAX_EQ_ITEM_SIZE]);
        }
    }

    fclose(f);

    return tfa9887_set_data(amp, MODULE_BIQUADFILTERBANK, PARAM_SET_EQ, data,
            MAX_EQ_SIZE);
}

static int tfa9887_hw_power(struct tfa9887_amp_t *amp, bool on)
{
    int error;
    uint16_t value;

    error = tfa9887_read_reg(amp, TFA9887_SYSTEM_CONTROL, &value);
    if (error != 0) {
        ALOGE("Unable to read from TFA9887_SYSTEM_CONTROL");
        goto power_err;
    }

    // get powerdown bit
    value &= ~(TFA9887_SYSCTRL_POWERDOWN);
    if (!on) {
        value |= TFA9887_SYSCTRL_POWERDOWN;
    }

    error = tfa9887_write_reg(amp, TFA9887_SYSTEM_CONTROL, value);
    if (error != 0) {
        ALOGE("Unable to write TFA9887_SYSTEM_CONTROL");
    }

    if (!on) {
        usleep(1000);
    }

power_err:
    return error;
}

static int tfa9887_dsp_reset(struct tfa9887_amp_t *amp, bool reset)
{
    int rc = 0;
    uint16_t value;

    rc = tfa9887_read_reg(amp, TFA9887_CF_CONTROLS, &value);
    if (rc) {
        ALOGE("%s: Failed to read register CF_CONTROLS: %d\n", __func__, rc);
        goto dsp_reset_err;
    }

    value = reset ? (value | TFA9887_CF_CONTROLS_RST_MSK) :
        (value & ~TFA9887_CF_CONTROLS_RST_MSK);

    rc = tfa9887_write_reg(amp, TFA9887_CF_CONTROLS, value);
    if (rc) {
        ALOGE("%s: Failed to write register CF_CONTROLS: %d\n", __func__, rc);
        goto dsp_reset_err;
    }

dsp_reset_err:
    return rc;
}

#ifdef WITH_SET_VOLUME
static int tfa9887_set_volume(struct tfa9887_amp_t *amp, float volume)
{
    int error;
    uint16_t value;
    uint8_t volume_int;

    if (!amp) {
        return -ENODEV;
    }

    if (volume > 0.0) {
        return -1;
    }

    error = tfa9887_read_reg(amp, TFA9887_AUDIO_CONTROL, &value);
    if (error != 0) {
        ALOGE("Unable to read from TFA9887_AUDIO_CONTROL");
        goto set_vol_err;
    }

    volume = -2.0 * volume;
    volume_int = (((uint8_t) volume) & 0xFF);

    value = ((value & 0x00FF) | (volume_int << 8));
    error = tfa9887_write_reg(amp, TFA9887_AUDIO_CONTROL, value);
    if (error != 0) {
        ALOGE("Unable to write to TFA9887_AUDIO_CONTROL");
        goto set_vol_err;
    }

set_vol_err:
    return error;
}
#endif

static int tfa9887_mute(struct tfa9887_amp_t *amp, uint32_t mute)
{
    int error;
    uint16_t aud_value, sys_value;

    error = tfa9887_read_reg(amp, TFA9887_AUDIO_CONTROL, &aud_value);
    if (error != 0) {
        ALOGE("Unable to read from TFA9887_AUDIO_CONTROL");
        goto mute_err;
    }
    error = tfa9887_read_reg(amp, TFA9887_SYSTEM_CONTROL, &sys_value);
    if (error != 0) {
        ALOGE("Unable to read from TFA9887_SYSTEM_CONTROL");
        goto mute_err;
    }

    switch (mute) {
        case TFA9887_MUTE_OFF:
            /* clear CTRL_MUTE, set ENBL_AMP, mute none */
            aud_value &= ~(TFA9887_AUDIOCTRL_MUTE);
            sys_value |= TFA9887_SYSCTRL_ENBL_AMP;
            break;
        case TFA9887_MUTE_DIGITAL:
            /* set CTRL_MUTE, set ENBL_AMP, mute ctrl */
            aud_value |= TFA9887_AUDIOCTRL_MUTE;
            sys_value |= TFA9887_SYSCTRL_ENBL_AMP;
            break;
        case TFA9887_MUTE_AMPLIFIER:
            /* clear CTRL_MUTE, clear ENBL_AMP, only mute amp */
            aud_value &= ~(TFA9887_AUDIOCTRL_MUTE);
            sys_value &= ~(TFA9887_SYSCTRL_ENBL_AMP);
            break;
        default:
            error = -1;
            ALOGW("Unknown mute type: %d", mute);
            goto mute_err;
    }

    error = tfa9887_write_reg(amp, TFA9887_AUDIO_CONTROL, aud_value);
    if (error != 0) {
        ALOGE("Unable to write TFA9887_AUDIO_CONTROL");
        goto mute_err;
    }
    error = tfa9887_write_reg(amp, TFA9887_SYSTEM_CONTROL, sys_value);
    if (error != 0) {
        ALOGE("Unable to write TFA9887_SYSTEM_CONTROL");
        goto mute_err;
    }

mute_err:
    return error;
}

static int tfa9887_select_input(struct tfa9887_amp_t *amp, int input)
{
    int error;
    uint16_t value;

    if (!amp) {
        return -ENODEV;
    }

    error = tfa9887_read_reg(amp, TFA9887_I2S_CONTROL, &value);
    if (error != 0) {
        goto select_amp_err;
    }

    // clear 2 bits
    value &= ~(0x3 << TFA9887_I2SCTRL_INPUT_SEL_SHIFT);

    switch (input) {
        case 1:
            value |= 0x40;
            break;
        case 2:
            value |= 0x80;
            break;
        default:
            ALOGW("Invalid input selected: %d",
                    input);
            error = -1;
            goto select_amp_err;
    }
    error = tfa9887_write_reg(amp, TFA9887_I2S_CONTROL, value);

select_amp_err:
    return error;
}

static int tfa9887_select_channel(struct tfa9887_amp_t *amp, int channels)
{
    int error;
    uint16_t value;

    if (!amp) {
        return -ENODEV;
    }

    error = tfa9887_read_reg(amp, TFA9887_I2S_CONTROL, &value);
    if (error != 0) {
        ALOGE("Unable to read from TFA9887_I2S_CONTROL");
        goto select_channel_err;
    }

    // clear the 2 bits first
    value &= ~(0x3 << TFA9887_I2SCTRL_CHANSEL_SHIFT);

    switch (channels) {
        case 0:
            value |= 0x8;
            break;
        case 1:
            value |= 0x10;
            break;
        case 2:
            value |= 0x18;
            break;
        default:
            ALOGW("Too many channels requested: %d",
                    channels);
            error = -1;
            goto select_channel_err;
    }
    error = tfa9887_write_reg(amp, TFA9887_I2S_CONTROL, value);
    if (error != 0) {
        ALOGE("Unable to write to TFA9887_I2S_CONTROL");
        goto select_channel_err;
    }

select_channel_err:
    return error;
}

static int tfa9887_set_sample_rate(struct tfa9887_amp_t *amp, int sample_rate)
{
    int error;
    uint16_t value;

    if (!amp) {
        return -ENODEV;
    }

    error = tfa9887_read_reg(amp, TFA9887_I2S_CONTROL, &value);
    if (error == 0) {
        // clear the 4 bits first
        value &= (~(0xF << TFA9887_I2SCTRL_RATE_SHIFT));
        switch (sample_rate) {
            case 48000:
                value |= TFA9887_I2SCTRL_RATE_48000;
                break;
            case 44100:
                value |= TFA9887_I2SCTRL_RATE_44100;
                break;
            case 32000:
                value |= TFA9887_I2SCTRL_RATE_32000;
                break;
            case 24000:
                value |= TFA9887_I2SCTRL_RATE_24000;
                break;
            case 22050:
                value |= TFA9887_I2SCTRL_RATE_22050;
                break;
            case 16000:
                value |= TFA9887_I2SCTRL_RATE_16000;
                break;
            case 12000:
                value |= TFA9887_I2SCTRL_RATE_12000;
                break;
            case 11025:
                value |= TFA9887_I2SCTRL_RATE_11025;
                break;
            case 8000:
                value |= TFA9887_I2SCTRL_RATE_08000;
                break;
            default:
                ALOGE("Unsupported sample rate %d", sample_rate);
                error = -1;
                return error;
        }
        error = tfa9887_write_reg(amp, TFA9887_I2S_CONTROL, value);
    }

    return error;
}

static int tfa9887_set_configured(struct tfa9887_amp_t *amp)
{
    int error;
    uint16_t value;

    if (!amp) {
        return -ENODEV;
    }

    error = tfa9887_read_reg(amp, TFA9887_SYSTEM_CONTROL, &value);
    if (error != 0) {
        ALOGE("Unable to read from TFA9887_SYSTEM_CONTROL");
        goto set_conf_err;
    }
    value |= TFA9887_SYSCTRL_CONFIGURED;
    tfa9887_write_reg(amp, TFA9887_SYSTEM_CONTROL, value);
    if (error != 0) {
        ALOGE("Unable to write TFA9887_SYSTEM_CONTROL");
    }

set_conf_err:
    return error;
}

static int tfa9887_startup(struct tfa9887_amp_t *amp)
{
    int rc;
    uint16_t value;

    if (!amp) {
        return -ENODEV;
    }

    rc = tfa9887_write_reg(amp, TFA9887_SYSTEM_CONTROL, 0x0002);
    if (rc) {
        ALOGE("%s:%d: Failure writing SYSTEM_CONTROL register: %d\n",
                __func__, __LINE__, rc);
        goto startup_err;
    }

    rc = tfa9887_read_reg(amp, TFA9887_SYSTEM_CONTROL, &value);
    if (rc) {
        ALOGE("%s:%d: Failure reading SYSTEM_CONTROL register: %d\n",
                __func__, __LINE__, rc);
        goto startup_err;
    }

    value |= TFA9887_SYSCTRL_ENBL_AMP;
    rc = tfa9887_write_reg(amp, TFA9887_SYSTEM_CONTROL, value);
    if (rc) {
        ALOGE("%s:%d: Failure writing SYSTEM_CONTROL register: %d\n",
                __func__, __LINE__, rc);
        goto startup_err;
    }

    rc = tfa9887_write_reg(amp, TFA9887_BAT_PROT, 0x13AB);
    if (rc) {
        ALOGE("%s:%d: Failure writing BAT_PROT register: %d\n",
                __func__, __LINE__, rc);
        goto startup_err;
    }

    rc = tfa9887_write_reg(amp, TFA9887_AUDIO_CONTROL, 0x001F);
    if (rc) {
        ALOGE("%s:%d: Failure writing AUDIO_CONTROL register: %d\n",
                __func__, __LINE__, rc);
        goto startup_err;
    }

    rc = tfa9887_write_reg(amp, TFA9887_SPKR_CALIBRATION, 0x3C4E);
    if (rc) {
        ALOGE("%s:%d: Failure writing SPKR_CALIBRATION register: %d\n",
                __func__, __LINE__, rc);
        goto startup_err;
    }

    rc = tfa9887_write_reg(amp, TFA9887_SYSTEM_CONTROL, 0x024D);
    if (rc) {
        ALOGE("%s:%d: Failure writing SYSTEM_CONTROL register: %d\n",
                __func__, __LINE__, rc);
        goto startup_err;
    }

    rc = tfa9887_write_reg(amp, TFA9887_PWM_CONTROL, 0x0308);
    if (rc) {
        ALOGE("%s:%d: Failure writing PWM_CONTROL register: %d\n",
                __func__, __LINE__, rc);
        goto startup_err;
    }

    rc = tfa9887_write_reg(amp, TFA9887_CURRENTSENSE4, 0x0E82);
    if (rc) {
        ALOGE("%s:%d: Failure writing CURRENTSENSE4 register: %d\n",
                __func__, __LINE__, rc);
        goto startup_err;
    }

    ALOGI("%s:%d: Hardware startup complete\n", __func__, __LINE__);

startup_err:
    return rc;
}

static int tfa9887_force_coldboot(struct tfa9887_amp_t *amp)
{
    int rc = 0;
    int tries = 0;
    uint16_t value = 0;
    uint8_t dsp_patch[16] = {
        /* 6 bytes of header, 2 bytes of length */
        0xFF, 0xFF, 0xFF, 0x00,
        0x00, 0x00, 0x08, 0x00,
        /* real data starts here */
        0x70, 0x00, 0x07, 0x81,
        0x00, 0x00, 0x00, 0x01
    };

    do {
        rc = tfa9887_load_patch(amp, dsp_patch + PATCH_HEADER_LENGTH,
                sizeof(dsp_patch) - PATCH_HEADER_LENGTH);
        if (rc) {
            ALOGW("%s: Failed to load coldboot patch, try: %d\n",
                    __func__, tries);
        }
        rc = tfa9887_read_reg(amp, TFA9887_STATUS, &value);
        if (rc) {
            ALOGE("%s: Failed to read STATUS register\n", __func__);
            goto coldboot_err;
        }
        tries++;
        usleep(5000);
    } while ((value & TFA9887_STATUS_ACS) == 0 && tries < 10);

    if (tries == 10) {
        ALOGE("%s:%d: Timed out checking ACS, waiting for: 0x%04x, got 0x%04x\n",
                __func__, __LINE__, TFA9887_STATUS_ACS, value);
        /* This doesn't appear to be a critical error */
        rc = 0;
        goto coldboot_err;
    }

    ALOGI("%s: Loaded coldboot patch\n", __func__);

coldboot_err:
    return rc;
}

#ifdef WITH_OTC
static int tfa9887_otc(struct tfa9887_amp_t *amp)
{
    int rc = 0;
    uint16_t value;

    rc = tfa9887_read_reg(amp, TFA9887_MTP, &value);
    if (rc) {
        ALOGE("%s: Failed to read register MTP: %d\n", __func__, rc);
        goto otc_err;
    }

    /* Skip reset if MTPOTC is set */
    if (!(value & TFA9887_MTP_MTPOTC)) {
        rc = tfa9887_write_reg(amp, TFA9887_MTPKEY2_REG, 0x005A);
        rc = tfa9887_write_reg(amp, TFA9887_MTP, 0x0001);
        rc = tfa9887_write_reg(amp, TFA9887_MTP_COPY, 0x0800);

        /* Wait until MTPB is not set */
        rc = tfa9887_wait_reg(amp, TFA9887_STATUS, TFA9887_STATUS_MTPB, false,
                10000, 10000);
        if (rc) {
            ALOGE("%s: Timed out waiting for STATUS register to clear MTP: %d\n",
                    __func__, rc);
            goto otc_err;
        }
        ALOGI("%s: Completed OTC reset\n", __func__);
    }
otc_err:
    return rc;
}
#endif

#if defined(WITH_RESET_CALIBRATION) || defined(WITH_MFG_RESET_CALIBRATION)
static int tfa9887_reset_calibration(struct tfa9887_amp_t *amp)
{
    int rc = 0;
    uint16_t value;

    rc = tfa9887_read_reg(amp, TFA9887_MTP, &value);
    if (rc) {
        ALOGE("%s: Failed to read register MTP: %d\n", __func__, rc);
        goto reset_cal_err;
    }

    if (!(value & TFA9887_MTP_MTPOTC)) {
        rc = tfa9887_write_reg(amp, TFA9887_MTPKEY2_REG, 0x005A);
        rc = tfa9887_write_reg(amp, TFA9887_MTP, 0x0001);
        rc = tfa9887_write_reg(amp, TFA9887_MTP_COPY, 0x0800);

        /* Wait until MTPB is not set */
        rc = tfa9887_wait_reg(amp, TFA9887_STATUS, TFA9887_STATUS_MTPB, false,
                10000, 50);
        if (rc) {
            ALOGE("%s: Timed out waiting for MTPB to clear: %d\n", __func__, rc);
            goto reset_cal_err;
        }

        rc = tfa9887_read_reg(amp, TFA9887_MTP, &value);
        if (rc) {
            ALOGE("%s: Failed to read register MTP: %d\n", __func__, rc);
            goto reset_cal_err;
        }
        ALOGI("%s: reset calibration done, MTP: 0x%02x\n", __func__, value);
    }

reset_cal_err:
    return rc;
}
#endif

static int tfa9887_get_calibration_impedance(struct tfa9887_amp_t *amp,
        float *re_25c)
{
    int rc = 0;
    /* 24 bit word */
    uint32_t word = 0;
    uint8_t bytes[3];

    *re_25c = 0.0f;

    rc = tfa9887_read_mem(amp, 231, 1, &word);
    if (rc) {
        ALOGE("%s: Failed to read from memory at offset 231\n", __func__);
        goto get_cal_imp_err;
    }
    if (!word) {
        /* Calibration wasn't completed */
        ALOGE("%s: Calibration not completed\n", __func__);
        rc = -EBUSY;
        goto get_cal_imp_err;
    }
    /* get speakerboost param */
    rc = tfa9887_get_data(amp, MODULE_SPEAKERBOOST, PARAM_GET_RE0, bytes, 3);
    if (rc) {
        ALOGE("%s: Failed to read SPEAKERBOOST param: %d\n", __func__, rc);
        goto get_cal_imp_err;
    }
    word = 0;
    bytes2data(bytes, 3, &word);
    *re_25c = ((float) word) / (1 << (23 - SPKRBST_TEMP_EXP));

get_cal_imp_err:
    return rc;
}

static int tfa9887_set_calibration_impedance(struct tfa9887_amp_t *amp,
        float impedance)
{
    int rc = 0;
    uint8_t bytes[3];
    uint32_t data;

    impedance = impedance * (1 << (23 - SPKRBST_TEMP_EXP));
    data = *((uint32_t *)&impedance);
    data2bytes((int32_t *)&data, 1, bytes);

    rc = tfa9887_set_data(amp, MODULE_SETRE, PARAM_SET_RE0, bytes, 3);
    if (rc) {
        ALOGE("%s: Failed to write calibration data: %d\n", __func__, rc);
        goto set_cal_imp_err;
    }

    rc = tfa9887_dsp_reset(amp, true);
    if (rc) {
        ALOGE("%s: Failed to reset DSP: %d\n", __func__, rc);
        goto set_cal_imp_err;
    }

    rc = tfa9887_dsp_reset(amp, false);
    if (rc) {
        ALOGE("%s: Failed to bring DSP out of reset: %d\n", __func__, rc);
        goto set_cal_imp_err;
    }

set_cal_imp_err:
    return rc;
}

static int tfa9887_reset_agc(struct tfa9887_amp_t *amp)
{
    uint8_t data[3];
    int32_t agc = 0;

    data2bytes(&agc, 1, data);
    return tfa9887_set_data(amp, MODULE_SPEAKERBOOST, PARAM_SET_AGC,
            data, 3);
}

static int tfa9887_hw_init(struct tfa9887_amp_t *amp, int sample_rate)
{
    int rc = 0;
    uint8_t patch_buf[MAX_PATCH_SIZE];
    int patch_size;

    if (!amp) {
        return -ENODEV;
    }

    /* do cold boot init */
    rc = tfa9887_startup(amp);
    if (rc) {
        ALOGE("%s: Unable to cold boot: %d\n", __func__, rc);
        goto priv_init_err;
    }
    rc = tfa9887_set_sample_rate(amp, sample_rate);
    if (rc) {
        ALOGE("%s: Unable to set sample rate: %d\n", __func__, rc);
        goto priv_init_err;
    }
    rc = tfa9887_select_channel(amp, 2);
    if (rc) {
        ALOGE("%s: Unable to select channel: %d\n", __func__, rc);
        goto priv_init_err;
    }
    rc = tfa9887_select_input(amp, 2);
    if (rc) {
        ALOGE("%s: Unable to select input: %d\n", __func__, rc);
        goto priv_init_err;
    }
    rc = tfa9887_hw_power(amp, true);
    if (rc) {
        ALOGE("%s: Unable to power up: %d\n", __func__, rc);
        goto priv_init_err;
    }
    usleep(5000);

    /* Wait for PLL bit to be set */
    rc = tfa9887_wait_reg(amp, TFA9887_STATUS, TFA9887_STATUS_PLLS, true,
            10, 1000);
    if (rc) {
        ALOGE("%s: Failed to lock PLLs: %d\n", __func__, rc);
        goto priv_init_err;
    }

    rc = tfa9887_force_coldboot(amp);
    if (rc) {
        ALOGE("%s: Failed to force coldboot: %d\n", __func__, rc);
        goto priv_init_err;
    }

    /* load patch firmware */
    patch_size = read_file(PATCH_TFA9887, patch_buf, MAX_PATCH_SIZE,
            PATCH_HEADER_LENGTH);
    if (patch_size < 0) {
        ALOGE("%s: Failed to read patch file %s\n", __func__, PATCH_TFA9887);
        goto priv_init_err;
    }
    rc = tfa9887_load_patch(amp, patch_buf, patch_size);
    if (rc) {
        ALOGE("%s: Unable to load patch data: %d\n", __func__, rc);
        goto priv_init_err;
    }
    ALOGI("%s: Loaded patch file\n", __func__);

#ifdef WITH_OTC
    rc = tfa9887_otc(amp);
    if (rc) {
        ALOGE("%s: Failed to perform one time init: %d\n", __func__, rc);
        goto priv_init_err;
    }
#endif

    rc = tfa9887_load_dsp(amp, SPKR);
    if (rc) {
        ALOGE("%s: Unable to load speaker data: %d\n", __func__, rc);
        goto priv_init_err;
    }
    ALOGI("%s: Loaded speaker file\n", __func__);

    rc = tfa9887_reset_agc(amp);
    if (rc) {
        ALOGE("%s: Unable to reset AGC: %d\n", __func__, rc);
        goto priv_init_err;
    }
    ALOGI("%s: Reset AGC after speaker load\n", __func__);

    rc = tfa9887_load_dsp(amp, CONFIG_TFA9887);
    if (rc) {
        ALOGE("%s: Unable to load config data: %d\n", __func__, rc);
        goto priv_init_err;
    }
    ALOGI("%s: Loaded config file\n", __func__);

    rc = tfa9887_reset_agc(amp);
    if (rc) {
        ALOGE("%s: Unable to reset AGC: %d\n", __func__, rc);
        goto priv_init_err;
    }
    ALOGI("%s: Reset AGC after config load\n", __func__);

    ALOGI("%s: Initialized hardware", __func__);

priv_init_err:
    return rc;
}

static int tfa9887_htc_init(struct tfa9887_amp_t *amp)
{
    int rc = 0;
    uint16_t value;

    rc = tfa9887_read_reg(amp, TFA9887_DCDCBOOST, &value);
    if (rc) {
        ALOGE("%s: Failed to read from DCDCBOOST register\n", __func__);
        goto htc_init_err;
    }
    value = (value & 0xFFF8) | 5; /* Boost of 5? */
    rc = tfa9887_write_reg(amp, TFA9887_DCDCBOOST, value);
    if (rc) {
        ALOGE("%s: Failed to write to DCDCBOOST register\n", __func__);
        goto htc_init_err;
    }

    rc = tfa9887_read_reg(amp, TFA9887_SYSTEM_CONTROL, &value);
    if (rc) {
        ALOGE("%s: Failed to read from SYSTEM_CONTROL register\n", __func__);
        goto htc_init_err;
    }
    value |= 0x200;
    rc = tfa9887_write_reg(amp, TFA9887_SYSTEM_CONTROL, value);
    if (rc) {
        ALOGE("%s: Failed to write to SYSTEM_CONTROL register\n", __func__);
        goto htc_init_err;
    }
    rc = tfa9887_read_reg(amp, TFA9887_I2S_SEL, &value);
    if (rc) {
        ALOGE("%s: Failed to read from I2S_SEL register\n", __func__);
        goto htc_init_err;
    }
    value = (value & 0xF9FE) | 0x10;
    rc = tfa9887_write_reg(amp, TFA9887_I2S_SEL, value);
    if (rc) {
        ALOGE("%s: Failed to write to I2S_SEL register\n", __func__);
        goto htc_init_err;
    }

    ALOGI("%s: HTC initialization sequence complete\n", __func__);

htc_init_err:
    return rc;
}

static int tfa9887_set_dsp_mode(struct tfa9887_amp_t *amp, uint32_t mode)
{
    int error;
    uint8_t buf[3];
    const struct mode_config_t *config;

    if (!amp) {
        return -ENODEV;
    }

    config = mode_configs;
    ALOGV("%s: Setting mode to %d\n", __func__, mode);

    buf[0] = 0;
    buf[1] = 0;
    buf[2] = 0; /* Should be 0 for voice or voip cases only, 1 otherwise */
    error = tfa9887_write_mem(amp, 0x0293, 3, buf);
    if (error != 0) {
        ALOGE("%s: Unable to write memory at 0x293\n", __func__);
        goto set_dsp_err;
    }
    ALOGI("%s: Prepared memory\n", __func__);
    error = tfa9887_load_dsp(amp, config[mode].preset);
    if (error != 0) {
        ALOGE("%s: Unable to load preset data\n", __func__);
        goto set_dsp_err;
    }
    ALOGI("%s: Loaded preset\n", __func__);
    error = tfa9887_load_eq(amp, config[mode].eq);
    if (error != 0) {
        ALOGE("%s: Unable to load EQ data\n", __func__);
        goto set_dsp_err;
    }
    ALOGI("%s: Loaded EQ\n", __func__);
    error = tfa9887_load_dsp(amp, config[mode].drc);
    if (error != 0) {
        ALOGE("%s: Unable to load DRC data\n", __func__);
        goto set_dsp_err;
    }
    ALOGI("%s: Loaded DRC\n", __func__);

    ALOGI("%s: Set DSP mode to %u\n", __func__, mode);

set_dsp_err:
    return error;
}

static int tfa9887_wait_for_calibration(struct tfa9887_amp_t *amp,
        uint32_t *cal_value)
{
    int rc;
    int tries = 0;
    uint16_t reg_value = 0;
    /* 1 DSP memory word is actually 24 bits */
    uint32_t mem = 0;

    if (!amp) {
        return -ENODEV;
    }

    rc = tfa9887_read_reg(amp, TFA9887_MTP, &reg_value);
    if (rc) {
        ALOGE("%s: Failed to read MTP register: %d\n", __func__, rc);
        rc = -EBUSY;
        goto wait_cal_err;
    }

    if ((reg_value & TFA9887_MTP_MTPOTC)) {
        /* Wait for MTPEX bit to be set */
        rc = tfa9887_wait_reg(amp, TFA9887_MTP, TFA9887_MTP_MTPEX, true, 30,
                50000);
        if (rc) {
            ALOGE("%s: Timed out waiting for MTPEX\n", __func__);
            *cal_value = 0;
            goto wait_cal_err;
        } else {
            *cal_value = 1;
        }
    } else {
        do {
            rc = tfa9887_read_mem(amp, 231, 1, &mem);
            if (rc) {
                ALOGE("%s: Failed to read memory at offset 231: %d\n",
                        __func__, rc);
                goto wait_cal_err;
            }
            *cal_value = mem;
            tries++;
            usleep(1000);
        } while ((*cal_value) == 0 && tries < 30);
        if (tries == 30) {
            ALOGE("%s: Timed out waiting for mem\n", __func__);
            rc = -EBUSY;
            goto wait_cal_err;
        }
    }

    ALOGI("%s: Got calibration value: %d\n", __func__, *cal_value);

wait_cal_err:
    return rc;
}

static int tfa9887_recalibrate(struct tfa9887_amp_t *amp)
{
    int rc = 0;
    uint16_t value = 0;
    uint32_t cal_result = 0;
    float impedance = 0.0f;
    float default_cal = 0.0f;

    /* check if calibration needed */
    rc = tfa9887_read_reg(amp, TFA9887_MTP, &value);
    if (rc) {
        ALOGE("%s: Failed to read register MTP: %d\n", __func__, rc);
        goto recalibrate_done;
    }

    if (value & TFA9887_MTP_MTPEX) {
        rc = tfa9887_get_calibration_impedance(amp, &impedance);
        if (rc) {
            ALOGE("%s: Failed to get calibration: %d\n", __func__, rc);
            goto recalibrate_done;
        }
        ALOGI("%s: Stored calibration: %0.2f\n",
                __func__, impedance);
        if (impedance >= 6.8f || impedance <= 10.0f) {
            /* Properly calibrated */
            return 0;
        } else {
            ALOGW("%s: Stored calibration out of range, recalibrating\n",
                    __func__);
        }
    } else {
        ALOGI("%s: DSP not calibrated\n", __func__);
    }

#ifdef WITH_RESET_CALIBRATION
    /* TODO: actually make this work */
    rc = tfa9887_mute(amp, TFA9887_MUTE_DIGITAL);

    rc = tfa9887_wait_for_calibration(amp, &cal_result);
    if (rc) {
        ALOGW("%s: Failed waiting for calibration: %d\n", __func__, rc);
        cal_result = 0;
    }
    if (cal_result) {
        rc = tfa9887_get_calibration_impedance(amp, &impedance);
        if (rc) {
            ALOGE("%s: Failed to get calibration impedance: %d\n",
                    __func__, rc);
            goto open_i2s_shutdown;
        }
    } else {
        impedance = -1.0f;
    }
    ALOGI("%s: Current impedance: %0.2f\n", __func__, impedance);

    if (impedance < 6.8f || impedance > 10.0f) {
        if (pass == 0) {
            ALOGW("%s: Calibration out of range, resetting to default\n",
                    __func__);
            rc = tfa9887_reset_calibration(amp);
            if (rc) {
                ALOGE("%s: Failed to reset calibration: %d\n",
                        __func__, rc);
                goto open_i2s_shutdown;
            }
            rc = tfa9887_set_calibration_impedance(amp, 7.0f);
            if (rc) {
                ALOGE("%s: Failed to set calibration impedance: %d\n",
                        __func__, rc);
                goto open_i2s_shutdown;
            }
            ALOGI("%s: Set calibration to 7.0f\n", __func__);
        rc = tfa9887_mute(amp, TFA9887_MUTE_DIGITAL);
        }
    } else {
        /* Calibration within expected range, don't loop again */
        break;
    }

recalibrate_unmute:
    rc = tfa9887_mute(amp, TFA9887_MUTE_OFF);
#endif
recalibrate_done:
    return rc;
}

static int tfa9887_lock(struct tfa9887_amp_t *amp, bool lock)
{
    int rc;
    int cmd = lock ? 1 : 0;

    if (!amp) {
        return -ENODEV;
    }

    if (amp->fd < 0) {
        return -ENODEV;
    }

    rc = ioctl(amp->fd, TFA9895_KERNEL_LOCK(cmd), &cmd);
    if (rc) {
        rc = -errno;
        ALOGE("%s: Failed to lock amplifier: %d\n",
                __func__, rc);
        return rc;
    }

    return rc;
}

static int tfa9887_enable_dsp(struct tfa9887_amp_t *amp, bool enable)
{
    int rc;
    int cmd = enable ? 1 : 0;

    if (!amp) {
        return -ENODEV;
    }

    if (amp->fd < 0) {
        return -ENODEV;
    }

    rc = ioctl(amp->fd, TFA9895_ENABLE_DSP(cmd), &cmd);
    if (rc) {
        rc = -errno;
        ALOGE("%s: Failed to enable DSP mode: %d\n",
                __func__, rc);
        return rc;
    }

    ALOGI("%s: Set DSP enable to %d\n", __func__, enable);

    return rc;
}

static int tfa9887_init(struct tfa9887_amp_t *amp)
{
    int rc;

    if (!amp) {
        return -ENODEV;
    }

    memset(amp, 0, sizeof(struct tfa9887_amp_t));

    amp->mode = TFA9887_MODE_PLAYBACK;
    amp->initializing = true;
    amp->writing = false;
    pthread_mutex_init(&amp->mutex, NULL);
    pthread_cond_init(&amp->cond, NULL);

    amp->fd = open(TFA9887_DEVICE, O_RDWR);
    if (amp->fd < 0) {
        rc = -errno;
        ALOGE("%s: Failed to open amplifier device: %d\n", __func__, rc);
        return rc;
    }

    return 0;
}

/* Public functions */

int tfa9887_open(void)
{
    int rc = 0;
    uint16_t value = 0;
    struct tfa9887_amp_t *amp = NULL;

    if (main_amp) {
        ALOGE("%s: TFA9887 already opened\n", __func__);
        rc = -EBUSY;
        goto open_err;
    }

    main_amp = calloc(1, sizeof(struct tfa9887_amp_t));
    if (!main_amp) {
        ALOGE("%s: Failed to allocate TFA9887 amplifier device memory", __func__);
        rc = -ENOMEM;
        goto open_err;
    }

    amp = main_amp;
    rc = tfa9887_init(amp);
    if (rc) {
        /* Try next amp */
        goto open_err;
    }

    rc = tfa9887_enable_dsp(amp, false);
    if (rc) {
        ALOGE("%s: Failed to disable DSP mode: %d\n", __func__, rc);
        goto open_err;
    }

    /* Open I2S interface while DSP ops are occurring */
    pthread_create(&amp->write_thread, NULL, write_dummy_data, amp);
    pthread_mutex_lock(&amp->mutex);
    while (!amp->writing) {
        pthread_cond_wait(&amp->cond, &amp->mutex);
    }
    pthread_mutex_unlock(&amp->mutex);

#ifdef WITH_MFG_RESET_CALIBRATION
    /* Only for MFG ROM */
    rc = tfa9887_reset_calibration(amp);
    if (rc) {
        ALOGE("%s: Failed to reset calibration: %d\n", __func__);
        goto open_i2s_shutdown;
    }
#endif
    rc = tfa9887_read_reg(amp, TFA9887_REVISIONNUMBER, &value);
    if (rc) {
        ALOGE("%s: Unable to read register REVISIONNUMBER: %d\n",
                __func__, rc);
        goto open_i2s_shutdown;
    }
    ALOGI("%s: Revision number: 0x%04x\n", __func__, value);

    rc = tfa9887_hw_init(amp, TFA9887_DEFAULT_RATE);
    if (rc) {
        ALOGE("%s: Failed to initialize hardware: %d\n", __func__, rc);
        goto open_i2s_shutdown;
    }

    /* Set config, preset, EQ, DRC */
    rc = tfa9887_set_dsp_mode(amp, amp->mode);
    if (rc) {
        ALOGE("%s: Failed to set DSP mode: %d\n", __func__, rc);
        goto open_i2s_shutdown;
    }
    rc = tfa9887_set_configured(amp);
    if (rc) {
        ALOGE("%s: Unable to set configured: %d\n", __func__, rc);
        goto open_i2s_shutdown;
    }

    rc = tfa9887_recalibrate(amp);
    if (rc) {
        ALOGE("%s: Failed to recalibrate: %d\n", __func__, rc);
        goto open_i2s_shutdown;
    }

    rc = tfa9887_enable_dsp(amp, true);
    if (rc) {
        ALOGE("%s: Failed to enable DSP mode: %d\n", __func__, rc);
        goto open_i2s_shutdown;
    }

    rc = tfa9887_htc_init(amp);
    if (rc) {
        ALOGE("%s: Failed to perform HTC initialization: %d\n",
                __func__, rc);
        goto open_i2s_shutdown;
    }

open_i2s_shutdown:
    /* Shut down I2S interface */
    amp->initializing = false;
    pthread_join(amp->write_thread, NULL);
    /* Remember to power off, since we powered on in hw_init */
    tfa9887_hw_power(amp, false);

open_err:
    return rc;
}

int tfa9887_power(bool on)
{
    int rc;
    struct tfa9887_amp_t *amp = NULL;

    if (!main_amp) {
        ALOGE("%s: TFA9887 not open!\n", __func__);
    }

    amp = main_amp;
    rc = tfa9887_hw_power(amp, on);
    if (rc) {
        ALOGE("Unable to power on amp: %d\n", rc);
    }

    ALOGI("%s: Set amplifier power to %d\n", __func__, on);

    return 0;
}

int tfa9887_set_mode(audio_mode_t mode)
{
    int rc;
    uint32_t dsp_mode;
    struct tfa9887_amp_t *amp = NULL;

    if (!main_amp) {
        ALOGE("%s: TFA9887 not opened\n", __func__);
        return -ENODEV;
    }

    dsp_mode = get_mode(mode);

    amp = main_amp;
    if (dsp_mode == amp->mode) {
        ALOGV("No mode change needed, already mode %d", dsp_mode);
        return 0;
    }
    rc = tfa9887_lock(amp, true);
    if (rc) {
        /* Try next amp */
        return -ENOSYS;
    }
    rc = tfa9887_mute(amp, TFA9887_MUTE_DIGITAL);
    rc = tfa9887_set_dsp_mode(amp, dsp_mode);
    if (rc == 0) {
        /* Only count DSP mode switches that were successful */
        amp->mode = dsp_mode;
    }
    rc = tfa9887_mute(amp, TFA9887_MUTE_OFF);
    rc = tfa9887_lock(amp, false);

    ALOGV("%s: Set amplifier audio mode to %d\n", __func__, mode);

    return 0;
}

int tfa9887_set_mute(bool on)
{
    int rc;
    struct tfa9887_amp_t *amp = NULL;

    if (!main_amp) {
        ALOGE("%s: TFA9887 not open!\n", __func__);
        return -ENODEV;
    }

    amp = main_amp;
    rc = tfa9887_mute(amp, on ? TFA9887_MUTE_DIGITAL : TFA9887_MUTE_OFF);
    if (rc) {
        ALOGE("Unable to mute: %d\n", rc);
    }

    ALOGI("%s: Set mute to %d\n", __func__, on);

    return 0;
}

int tfa9887_close(void)
{
    struct tfa9887_amp_t *amp = NULL;

    if (!main_amp) {
        ALOGE("%s: TFA9887 not open!\n", __func__);
    }

    amp = main_amp;
    tfa9887_hw_power(amp, false);
    close(amp->fd);

    free(main_amp);

    return 0;
}
