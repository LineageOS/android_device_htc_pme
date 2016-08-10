/*
 * Copyright (C) 2016, The CyanogenMod Project
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

#define LOG_TAG "audio_amplifier::tfa"
//#define LOG_NDEBUG 0

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <memory.h>
#include <unistd.h>
#include <fcntl.h>

#include <cutils/log.h>
#include <tinyalsa/asoundlib.h>
#include "tfa.h"
#include "tfa9888.h"
#include "tfa9888-debug.h"

struct tfaS {
    struct mixer *mixer;
    int fd;
    int swprof;
    int swvstep;
};

#define SND_CARD        0
#define AMP_PCM_DEV     47
#define AMP_MIXER_CTL   "QUAT_MI2S_RX_DL_HL Switch"

tfa_t *tfa_new(void)
{
    struct mixer *mixer;
    int fd;
    tfa_t *t;

    if ((mixer = mixer_open(SND_CARD)) == NULL) {
        ALOGE("failed to open mixer");
        return NULL;
    }

    if ((fd = open("/dev/tfa9888", O_RDWR)) < 0) {
        ALOGE("failed to open /dev/tfa9888");
        mixer_close(mixer);
        return NULL;
    }

    if ((t = malloc(sizeof(*t))) == NULL) {
        ALOGE("out of memory");
        mixer_close(mixer);
        return NULL;
    }

    t->mixer = mixer;
    t->fd = fd;
    t->swprof = -1;
    t->swvstep = -1;

    return t;
}

void tfa_destroy(tfa_t *t)
{
    mixer_close(0);
    close(t->fd);
    free(t);
}

static struct pcm_config amp_pcm_config = {
    .channels = 1,
    .rate = 48000,
    .period_size = 0,
    .period_count = 4,
    .format = 0,
    .start_threshold = 0,
    .stop_threshold = INT_MAX,
    .avail_min = 0,
};

struct pcm *tfa_clocks_on(tfa_t *t)
{
    struct mixer_ctl *ctl;
    struct pcm *pcm;
    struct pcm_params *pcm_params;

    ctl = mixer_get_ctl_by_name(t->mixer, AMP_MIXER_CTL);
    if (ctl == NULL) {
        ALOGE("%s: Could not find %s", __func__, AMP_MIXER_CTL);
        return NULL;
    }

    pcm_params = pcm_params_get(SND_CARD, AMP_PCM_DEV, PCM_OUT);
    if (pcm_params == NULL) {
        ALOGE("Could not get the pcm_params");
        return NULL;
    }

    amp_pcm_config.period_count = pcm_params_get_max(pcm_params, PCM_PARAM_PERIODS);
    pcm_params_free(pcm_params);

    mixer_ctl_set_value(ctl, 0, 1);
    pcm = pcm_open(SND_CARD, AMP_PCM_DEV, PCM_OUT, &amp_pcm_config);
    if (!pcm) {
        ALOGE("failed to open pcm at all??");
        return NULL;
    }
    if (!pcm_is_ready(pcm)) {
        ALOGE("failed to open pcm device: %s", pcm_get_error(pcm));
        pcm_close(pcm);
        return NULL;
    }

    ALOGV("clocks: ON");
    return pcm;
}

int tfa_clocks_off(tfa_t *t, struct pcm *pcm)
{
    struct mixer_ctl *ctl;

    pcm_close(pcm);

    ctl = mixer_get_ctl_by_name(t->mixer, AMP_MIXER_CTL);
    if (ctl == NULL) {
        ALOGE("%s: Could not find %s", __func__, AMP_MIXER_CTL);
        return -ENODEV;
    } else {
        mixer_ctl_set_value(ctl, 0, 0);
    }

    ALOGV("clocks: OFF");
    return 0;
}

struct pcm *tfa_clocks_and_power_on(tfa_t *t)
{
    struct pcm *pcm;

    pcm = tfa_clocks_on(t);
    if (tfa_power_on(t) < 0) {
        pcm_close(pcm);
        tfa_clocks_off(t, pcm);
        return NULL;
    } else {
        return pcm;
    }
}

static int wait_dsp_system_stable_is(tfa_t *t, int state)
{
    int retry;

    for (retry = 0; retry < 100; retry++) {
        if (tfa_dsp_system_stable(t) == state) {
            return 1;
        }
        usleep(10000);
    }
    return 0;
}

int tfa_power_on(tfa_t *t)
{
    int retry;

    tfa_set_bitfield(t, BF_POWERDOWN, 0);
    if (!wait_dsp_system_stable_is(t, 1)) {
        return -EBUSY;
    }
    ALOGV("power: ON");
    return 0;
}

void tfa_clocks_and_power_off(tfa_t *t, struct pcm *pcm)
{
    tfa_power_off(t);
    tfa_clocks_off(t, pcm);
}

void tfa_power_off(tfa_t *t)
{
    int retry;

    tfa_set_bitfield(t, BF_POWERDOWN, 1);
    if (! wait_dsp_system_stable_is(t, 0)) {
        ALOGE("failed to disable the amp");
    }
    ALOGV("power: OFF");
}

static inline unsigned bf_mask(int bf)
{
    return (1<<((bf&0xf)+1)) - 1;
}

static inline unsigned bf_shift(int bf)
{
    return (bf & 0xf0) >> 4;
}

static inline unsigned bf_register(int bf)
{
    return bf >> 8;
}

static inline unsigned bf_update_bf(int bf, unsigned old, unsigned value)
{
    unsigned shifted_mask = bf_mask(bf) << bf_shift(bf);

    value &= bf_mask(bf);
    return (old & ~shifted_mask) | (value << bf_shift(bf));
}

int tfa_set_bitfield(tfa_t *t, int bf, unsigned value)
{
    unsigned char cmd[4];
    unsigned old_value, new_value;

    old_value = tfa_get_register(t, bf_register(bf));
    new_value = bf_update_bf(bf, old_value, value);
    if (old_value != new_value) {
        return tfa_set_register(t, bf_register(bf), new_value);
    } else {
        return 0;
    }
}

int tfa_set_bitfield_always(tfa_t *t, int bf, unsigned value)
{
    unsigned char cmd[4];
    unsigned old_value, new_value;

    old_value = tfa_get_register(t, bf_register(bf));
    new_value = bf_update_bf(bf, old_value, value);
    return tfa_set_register(t, bf_register(bf), new_value);
}

static int bf_value(int bf, unsigned v)
{
    return (v >> bf_shift(bf)) & bf_mask(bf);
}

int tfa_get_bitfield(tfa_t *t, int bf)
{
    int v = tfa_get_register(t, bf_register(bf));
    if (v < 0) return v;
    return bf_value(bf, v);
}

int tfa_set_register(tfa_t *t, unsigned reg, unsigned value)
{
    unsigned char cmd[3];
    unsigned old;

    cmd[0] = reg;
    cmd[1] = value >> 8;
    cmd[2] = value;

printf("SET %x", reg);
    tfa9888_print_bitfield(stdout, reg, value);

    return write(t->fd, cmd, 3);
}

int tfa_set_register_bitfield(tfa_t *t, unsigned bf, unsigned v)
{
    return tfa_set_register(t, bf_register(bf), bf_update_bf(bf, 0, v));
}

int tfa_get_register(tfa_t *t, unsigned reg)
{
    unsigned char buf[2];
    int res;
    int v;

    buf[0] = reg;
    if ((res = write(t->fd, buf, 1)) < 0) return res;
    if ((res = read(t->fd, buf, 2)) < 0) return res;
    v = (buf[0]<<8) | buf[1];

printf("get %x", reg);
    tfa9888_print_bitfield(stdout, reg, v);

    return v;
}

static void setup_keys(tfa_t *t)
{
    unsigned short mtpdataB;

    tfa_set_register(t, 0xf, 23147);
    mtpdataB = tfa_get_bitfield(t, BF_MTPDATAB);
    tfa_set_register(t, 0xa0, mtpdataB ^ 0x5a);
}

static void setup_basic_registers(tfa_t *t)
{
    tfa_set_register(t, 0x00, 0x164d);
    tfa_set_register(t, 0x01, 0x828b);
    tfa_set_register(t, 0x02, 0x1dc8);
    tfa_set_register(t, 0x0e, 0x80);
    tfa_set_register(t, 0x20, 0x89e);
    tfa_set_register(t, 0x22, 0x543c);
    tfa_set_register(t, 0x23, 0x06);
    tfa_set_register(t, 0x24, 0x14);
    tfa_set_register(t, 0x25, 0x0a);
    tfa_set_register(t, 0x26, 0x100);
    tfa_set_register(t, 0x28, 0x1000);
    tfa_set_register(t, 0x51, 0x00);
    tfa_set_register(t, 0x52, 0xfafe);
    tfa_set_register(t, 0x70, 0x3ee4);
    tfa_set_register(t, 0x71, 0x1074);
    tfa_set_register(t, 0x83, 0x14);
}

static void tfa9888_init(tfa_t *t)
{
    setup_keys(t);
    setup_basic_registers(t);
}

static void init(tfa_t *t)
{
    tfa_set_register_bitfield(t, BF_RESET, 1);
    tfa_set_bitfield_always(t, BF_SRC_SET_CONFIGURED, 0);
    tfa_set_bitfield_always(t, BF_EXECUTE_COLD_START, 1);
    tfa_set_bitfield_always(t, BF_CF_RST_DSP, 1);

    tfa9888_init(t);
}

static void factory_trimmer(tfa_t *t)
{
printf("CRP %s %d\n", __func__, __LINE__);
    if (tfa_get_bitfield(t, BF_CALIBR_DCDC_API_CALIBRATE)) {
        unsigned boost_cur = tfa_get_bitfield(t, BF_BOOST_CUR);
        int delta = tfa_get_bitfield(t, BF_CALIBR_DCDC_DELTA);
        int sign = tfa_get_bitfield(t, BF_CALIBR_DCDC_DELTA_SIGN);
        if (sign) boost_cur - delta;
        else boost_cur + delta;
        tfa_set_bitfield(t, BF_BOOST_CUR, boost_cur > 14 ? 15 : boost_cur);
    }
printf("CRP %s %d\n", __func__, __LINE__);
}

static int cold_boot(tfa_t *t, int new_cold_started_mode)
{
printf("CRP %s %d\n", __func__, __LINE__);
    if (tfa_get_bitfield(t, BF_FLAG_COLD_STARTED) != new_cold_started_mode) {
        return tfa_dsp_write_mem_word(t, 0x8100, new_cold_started_mode, 3);
    }
printf("CRP %s %d\n", __func__, __LINE__);
    return 0;
}

static int tfa_startup(tfa_t *t, tfa_cont_t *tc, int profile_num)
{
printf("CRP %s %d\n", __func__, __LINE__);
    init(t);

    tfa_cont_write_device_registers(tc, t);
    tfa_cont_write_profile_registers(tc, profile_num, t);
    factory_trimmer(t);
    tfa_power_on(t);
    tfa_set_bitfield_always(t, BF_SRC_SET_CONFIGURED, 1);

    wait_dsp_system_stable_is(t, 1);

printf("CRP %s %d\n", __func__, __LINE__);
    return 0;
}

static int speaker_startup(tfa_t *t, tfa_cont_t *tc, int force_startup, int profile_num)
{
printf("CRP %s %d\n", __func__, __LINE__);
    if (force_startup) {
        tfa_cont_write_device_files(tc, t);
        tfa_cont_write_profile_files(tc, profile_num, t, 0);
    }

    tfa_startup(t, tc, profile_num);

    if (tfa_get_bitfield(t, BF_ENBL_COOLFLUX)) {
        tfa_start_dsp(t, tc);
    }

printf("CRP %s %d\n", __func__, __LINE__);
    return 0;
}

static int cold_startup(tfa_t *t, tfa_cont_t *tc)
{
printf("CRP %s %d\n", __func__, __LINE__);
    tfa_startup(t, tc, 0);
    cold_boot(t, 1);
    return tfa_start_dsp(t, tc);
printf("CRP %s %d\n", __func__, __LINE__);
}

static void key2(tfa_t *t, int off)
{
    tfa_set_register(t, bf_register(BF_HIDDEN_CODE), 0x5a6b);
    tfa_set_register(t, bf_register(BF_MTPKEY2), off ? 0 : 90);
    tfa_set_register(t, bf_register(BF_HIDDEN_CODE), 0);
}

static int wait_calibration(tfa_t *t)
{
    int retry;

    if (tfa_get_bitfield(t, BF_CALIBRATION_ONETIME)) {
        for (retry = 0; retry < 50 && tfa_get_bitfield(t, BF_FLAG_MTP_BUSY); retry++) {
            usleep(10*1000);
        }
        if (retry >= 50) {
            ALOGE("CRP Timeout waiting for mtp to stop being busy");
            return -EBUSY;
        }
        for (retry = 0; retry < 25 && !tfa_get_bitfield(t, BF_CALIBR_RON_DONE); retry++) {
            usleep(50*1000);
        }
        if (retry >= 25) {
            ALOGE("CRP Timeout waiting for calibration to complete");
            return -EBUSY;
        }
    } else {
        int calibrated = 0;
        for (retry = 0; retry < 20 && !calibrated; retry++) {
            tfa_dsp_read_mem(t, 516, 1, &calibrated);
        }
        if (!calibrated) {
            ALOGE("CRP Timeout waiting for dsp to report calibrated");
        }
    }

    return 0;
}

static int speaker_calibration(tfa_t *t)
{
printf("CRP %s %d\n", __func__, __LINE__);
    if (tfa_get_bitfield(t, BF_CALIBRATION_ONETIME)) {
        key2(t, 0);
    }

    wait_calibration(t);

    if (tfa_get_bitfield(t, BF_CALIBRATION_ONETIME)) {
        key2(t, 1);
    }

printf("CRP %s %d\n", __func__, __LINE__);
    return 0;
}

static int speaker_boost(tfa_t *t, tfa_cont_t *tc, int force_startup, int profile_num)
{
printf("CRP %s %d\n", __func__, __LINE__);
    /* TODO/idea: Drop all the "int" returns, make them voids, add error checking on all
     * the read/writes which set an error_occurred flag if we have problems.
     * When we get here if we have that flag, close the fd, reopen it and
     * and then cold_boot(t, 0); cold_startup();
     */

    if (force_startup) {
        cold_startup(t, tc);
    }

    if (tfa_get_bitfield(t, BF_FLAG_COLD_STARTED)) {
        int swvstep = speaker_startup(t, tc, force_startup, profile_num);
        tfa_set_swprof(t, profile_num);
        tfa_set_swvstep(t, swvstep);
        if (tfa_get_bitfield(t, BF_ENBL_COOLFLUX)) {
            tfa_set_bitfield(t, BF_COOLFLUX_CONFIGURED, 1);
            speaker_calibration(t);
        }
    }

printf("CRP %s %d\n", __func__, __LINE__);
    return 0;
}

int tfa_start(tfa_t *t, tfa_cont_t *tc, int profile_num, int vstep)
{
printf("CRP %s %d\n", __func__, __LINE__);

    if (tfa_get_swprof(t) == -1) {
        if (profile_num <= 0) {
            profile_num = tfa_cont_get_cal_profile(tc);
            ALOGI("Calibration: %d: %s", profile_num, tfa_cont_get_profile_name(tc, profile_num));
        }
    }

printf("CRP start profile %s vstep %d\n", tfa_cont_get_profile_name(tc, profile_num), vstep);

    speaker_boost(t, tc, 0, profile_num);

    if (profile_num >= 0) {
        tfa_cont_write_profile(tc, profile_num, vstep, t);
    }

    if (strstr(tfa_cont_get_profile_name(tc, profile_num), ".standby")) {
        tfa_power_off(t);
    } else {
        tfa_power_on(t);
    }

#if TODO
    // tfa_cont_search_for_filter_keyword(tc, profile_num);
    if (tfa_cont_get_current_vstep(tc) != vstep) {
        tfa_cont_write_vstep_files(tc, profile_num, vstep, t);
    }

    tfa_set_swprof(t, profile_num);
    tfa_set_swvstep(t, tfa_cont_get_current_vstep(tc));
#else
    vstep;
#endif

printf("CRP %s %d\n", __func__, __LINE__);
    return 0;
}

int tfa_start_dsp(tfa_t *t, tfa_cont_t *tc)
{
    int audio_fs;

printf("CRP %s %d\n", __func__, __LINE__);
    tfa_cont_write_patch(tc, t);
    tfa_dsp_write_mem_word(t, 512, 0, 1);
    tfa_set_bitfield_always(t, BF_CF_RST_DSP, 0);
    audio_fs = tfa_get_bitfield(t, BF_AUDIO_FS);
    tfa_dsp_write_tables(t, audio_fs);

printf("CRP %s %d\n", __func__, __LINE__);
    return 0;
}

int tfa_stop(tfa_t *t)
{
    tfa_power_off(t);

    return 0;
}

#if TODO
int tfa_set_mode(tfa_t *t, int mode)
{
    tfa_clocks_on(t);
    if (mode == -1) {
        t->current_mode = mode;
        tfa_stop(t);
    } else {
        for (retry = 0; retry < 50 && tfa_get_bitfield(t, BF_FLAG_LOST_CLK); retry++) {
            usleep(2000);
        }
    tfa_clocks_off(t);
}

int tfa_set_volume(tfa_t *t, int on)
{
    if (t->current_volume != on) {
        t->current_volme = on;
        if (t->current_mode == 0 || t->current_mode == 3) {
            tfa_set_mode(t, 1);
        }
    }

    return 0;
}
#endif

int tfa_dsp_system_stable(tfa_t *t)
{
    unsigned v = tfa_get_register(t, bf_register(BF_FLAG_ENBL_REF));
    return bf_value(BF_FLAG_ENBL_REF, v) && bf_value(BF_FLAG_CLOCKS_STABLE, v);
}

static void bytes2data(int n_bytes, unsigned char *bytes, int *data)
{
    int i;

    for (i = 0; i < n_bytes; i += 3) {
        data[i/3] = (bytes[i] << 16) | (bytes[i+1] << 8) | bytes[i+2];
        data[i/3] = data[i/3] << 8 >> 8;        /* sign extend */
    }
}

static void data2bytes(int n_data, int *data, unsigned char *bytes)
{
    int i;

    for (i = 0; i < n_data; i++) {
        bytes[i*3] = data[i] >> 16;
        bytes[i*3+1] = data[i] >> 8;
        bytes[i*3+2] = data[i];
    }
}

static void dump_raw(unsigned char *raw, int n)
{
    while (n--) printf(" %02x", *raw++);
}

static int tfa_read_data(tfa_t *t, char reg, int n_bytes, unsigned char *buf)
{
    int ret;

    write(t->fd, &reg, 1);
    ret = read(t->fd, buf, n_bytes);
if (ret >= 0) {
printf("RD");
dump_raw(buf, n_bytes);
}
    return ret;
}

static int tfa_write_data_raw(tfa_t *t, unsigned char *bytes, unsigned n)
{
if (n == 3) {
printf("SET %x", bytes[0]);
tfa9888_print_bitfield(stdout, bytes[0], (bytes[1]<<8) | bytes[2]);
} else {
printf("WR");
dump_raw(bytes, n);
}
printf("\n");
    return write(t->fd, bytes, n);
}

#define MAX_BYTES_TO_TRANSFER       253

static int tfa_write_data(tfa_t *t, unsigned char reg, unsigned char *bytes, unsigned n)
{
    unsigned char buf[MAX_BYTES_TO_TRANSFER+1];

    if (n > MAX_BYTES_TO_TRANSFER) {
        return -EINVAL;
    }

    buf[0] = reg;
    memcpy(&buf[1], bytes, n);

    return tfa_write_data_raw(t, buf, n+1);
}

int tfa_dsp_read_mem(tfa_t *t, unsigned dmem_and_madd, int n_words, int *data)
{
    int dmem;
    unsigned char tmp[MAX_BYTES_TO_TRANSFER];
    int n_read = 0;

    dmem = (dmem_and_madd >> 16) & 0xf;
    if (! dmem) dmem = 1;

    tfa_set_bitfield(t, BF_CF_DMEM, dmem);
    tfa_set_register(t, bf_register(BF_CF_MADD), dmem_and_madd);

    while (n_read < n_words) {
        int this_n_read = MAX_BYTES_TO_TRANSFER / 3;
        if (this_n_read > n_words - n_read) {
            this_n_read = n_words - n_read;
        }
        tfa_read_data(t, 0x92, this_n_read*3, tmp);
        bytes2data(this_n_read*3, tmp, &data[n_read]);
        n_read += this_n_read;
    }

    return n_read;
}

int tfa_dsp_write_mem_word(tfa_t *t, unsigned madd, int value, unsigned dmem)
{
    unsigned char bytes[4];

    tfa_set_bitfield(t, BF_CF_DMEM, dmem);
    tfa_set_register(t, bf_register(BF_CF_MADD), madd);
    bytes[0] = bf_register(BF_CF_MEMA);
    data2bytes(1, &value, &bytes[1]);
    return tfa_write_data_raw(t, bytes, 4);
}

static int tfa_dsp_msg_write(tfa_t *t, unsigned n_bytes, unsigned char *bytes, unsigned *msg_id)
{
    int dmem;

    dmem = tfa_get_register(t, bf_register(BF_CF_DMEM));
    dmem = bf_update_bf(BF_CF_DMEM, dmem, 1);
    dmem = bf_update_bf(BF_CF_AIF, dmem, 0);
    tfa_set_register(t, bf_register(BF_CF_DMEM), dmem);
    tfa_set_register(t, bf_register(BF_CF_MADD), 1);

    if (msg_id) {
        tfa_write_data(t, 0x92, (unsigned char *) msg_id, 3);
    }

    while (n_bytes > 0) {
        unsigned this_n_write = MAX_BYTES_TO_TRANSFER / 3 * 3;

        if (this_n_write > n_bytes) {
            this_n_write = n_bytes;
        }

        tfa_write_data(t, 0x92, bytes, this_n_write);

        n_bytes -= this_n_write;
        bytes += this_n_write;
    }

    dmem = bf_update_bf(BF_CF_INT, dmem, 1);
    dmem = bf_update_bf(0x9080, dmem, 1);

    return tfa_set_register(t, bf_register(BF_CF_INT), dmem);
}

int tfa_dsp_msg_status(tfa_t *t, int max_retries)
{
    int retry;

    for (retry = 0; retry < max_retries; retry++) {
        int ack = tfa_get_bitfield(t, BF_CF_ACK);
        if (ack & 1) return ack;
    }
    return -1;
}

int tfa_dsp_msg(tfa_t *t, unsigned n_bytes, unsigned char *bytes)
{
    int status;

    tfa_dsp_msg_write(t, n_bytes, bytes, NULL);
    status = tfa_dsp_msg_status(t, 2);
    return 0;
}

int tfa_dsp_msg_id(tfa_t *t, unsigned n_bytes, unsigned char *bytes, unsigned msg_id)
{
    int status;

    tfa_dsp_msg_write(t, n_bytes, bytes, &msg_id);
    status = tfa_dsp_msg_status(t, 2); /* ?? is this right */
    return 0;
}

int tfa_dsp_patch(tfa_t *t, int n_data, unsigned char *data)
{
    int dmem_and_madd;
    int rom_id, expected_rom_id;

#if WHAT_AM_I_DOING_WRONG_HERE
    dmem_and_madd = (data[1] << 8) | data[2];
    expected_rom_id = (data[3] << 16) | (data[4] << 8) | data[5];

    if (dmem_and_madd == 0xffff) {
        if (expected_rom_id && expected_rom_id != 0xfffff) {
           ALOGE("patch specifies rom_id to check but no address to use to check it");
           return -EINVAL;
        }
    } else {
        if (! tfa_dsp_system_stable(t)) {
            ALOGE("failed to write patch because DSP is not stable.");
            return -ENOENT;
        }

        tfa_dsp_read_mem(t, dmem_and_madd, 1, &rom_id);
        if (rom_id != expected_rom_id) {
             ALOGE("Invalid rom_id %d, expected %d", rom_id, expected_rom_id);
             return -EINVAL;
        }
    }
#endif

    data += 6;
    n_data -= 6;

    while (n_data > 0) {
        unsigned hunk_n = data[0] | (data[1] << 8);
        data += 2;
        n_data -= 2;

        tfa_write_data_raw(t, data, hunk_n);

        data += hunk_n;
        n_data -= hunk_n;
    }

    return 0;
}

int tfa_dsp_write_tables(tfa_t *t, int audio_fs)
{
    unsigned char msg[15] = { 0 };
    int frac_delay;

    switch(audio_fs) {
    case 0: frac_delay = 40; break;
    case 1: frac_delay = 38; break;
    case 2: frac_delay = 37; break;
    case 3: frac_delay = 59; break;
    case 4: frac_delay = 56; break;
    case 5: frac_delay = 56; break;
    case 6: frac_delay = 52; break;
    case 7: frac_delay = 48; break;
    default: frac_delay = 46;
    }

    /* 3 byte "words" 0x804 1 0 1 0 */
    msg[0] = 0;
    msg[1] = 0x80;
    msg[2] = 4;
    if (audio_fs) {
        msg[5] = 1;
        msg[11] = 1;
    }

    tfa_set_bitfield(t, BF_CS_FRAC_DELAY, frac_delay);
    return tfa_dsp_msg(t, 15, msg);
}

int tfa_set_swprof(tfa_t *t, int swprof)
{
    int old_swprof = t->swprof;

    t->swprof = swprof;
    tfa_set_bitfield(t, BF_SW_PROFILE, swprof+1);

    return old_swprof;
}

int tfa_get_swprof(tfa_t *t)
{
    return t->swprof;
}

int tfa_set_swvstep(tfa_t *t, int swvstep)
{
    int old_swvstep = t->swvstep;

    t->swvstep = swvstep;
    tfa_set_bitfield(t, BF_CTRL_DIGTOANA, swvstep+1);

    return old_swvstep;
}

int tfa_get_swvstep(tfa_t *t)
{
    return t->swvstep;
}
