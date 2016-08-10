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
        ALOGE("%s: Could not find %s\n", __func__, AMP_MIXER_CTL);
        return NULL;
    }

    pcm_params = pcm_params_get(SND_CARD, AMP_PCM_DEV, PCM_OUT);
    if (pcm_params == NULL) {
        ALOGE("Could not get the pcm_params\n");
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

    return pcm;
}

int tfa_clocks_off(tfa_t *t, struct pcm *pcm)
{
    struct mixer_ctl *ctl;

    pcm_close(pcm);

    ctl = mixer_get_ctl_by_name(t->mixer, AMP_MIXER_CTL);
    if (ctl == NULL) {
        ALOGE("%s: Could not find %s\n", __func__, AMP_MIXER_CTL);
        return -ENODEV;
    } else {
        mixer_ctl_set_value(ctl, 0, 0);
    }

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

printf("%s %d\n", __func__, state);
    for (retry = 0; retry < 1000; retry++) {
        if (tfa_dsp_system_stable(t) == state) {
            return 1;
        }
        usleep(1000);
    }
    return 0;
}

int tfa_power_on(tfa_t *t)
{
    int retry;

    tfa_set_bitfield(t, TFA98XX_BF_POWERDOWN, 0);
    if (! wait_dsp_system_stable_is(t, 1)) {
        return -EBUSY;
    }
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

    tfa_set_bitfield(t, TFA98XX_BF_POWERDOWN, 1);
#if 0
    if (! wait_dsp_system_stable_is(t, 0)) {
        printf("failed to disable the amp\n");
    }
#endif
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
    unsigned old;

    old = tfa_get_register(t, bf_register(bf));
    return tfa_set_register(t, bf_register(bf), bf_update_bf(bf, old, value));
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
    mtpdataB = tfa_get_bitfield(t, TFA98XX_BF_MTPDATAB);
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

static void setup_tdm(tfa_t *t)
{
    tfa_set_bitfield(t, TFA98XX_BF_TDM_SAMPLE_SIZE, 23);
    tfa_set_bitfield(t, TFA98XX_BF_TDM_NBCK, 2);
    tfa_set_bitfield(t, TFA98XX_BF_TDM_SOURCE6_IO, 0);
    tfa_set_bitfield(t, TFA98XX_BF_TDM_SOURCE5_IO, 0);
    tfa_set_bitfield(t, TFA98XX_BF_TDM_SOURCE5_SLOT, 1);
    tfa_set_bitfield(t, TFA98XX_BF_TDM_SOURCE7_SLOT, 1);
    tfa_set_bitfield(t, TFA98XX_BF_TDM_SOURCE8_SLOT, 0);
}

static void tfa9888_init(tfa_t *t)
{
    setup_keys(t);
    setup_basic_registers(t);
    setup_tdm(t);
}

static void init(tfa_t *t)
{
    tfa_set_register(t, TFA98XX_BF_RESET, bf_value(TFA98XX_BF_RESET, 1));
    tfa_set_register(t, TFA98XX_BF_SRC_SET_CONFIGURED, 0);
    tfa_set_register(t, TFA98XX_BF_EXECUTE_COLD_START, 1);
    tfa_set_bitfield(t, TFA98XX_BF_CF_RST_DSP, 1);

    tfa9888_init(t);
}

static void factory_trimmer(tfa_t *t)
{
    if (tfa_get_bitfield(t, TFA98XX_BF_CALIBR_DCDC_API_CALIBRATE)) {
        unsigned boost_cur = tfa_get_bitfield(t, TFA98XX_BF_BOOST_CUR);
        int delta = tfa_get_bitfield(t, TFA98XX_BF_CALIBR_DCDC_DELTA);
        int sign = tfa_get_bitfield(t, TFA98XX_BF_CALIBR_DCDC_DELTA_SIGN);
        if (sign) boost_cur - delta;
        else boost_cur + delta;
        tfa_set_bitfield(t, TFA98XX_BF_BOOST_CUR, boost_cur > 14 ? 15 : boost_cur);
    }
}

int tfa_startup(tfa_t *t, tfa_cont_t *tc)
{
    int cal_profile = -1;

    if (tfa_get_swprof(t) == -1) {
        cal_profile = tfa_cont_get_cal_profile(tc);
        printf("Calibration: %d: %s\n", cal_profile, tfa_cont_get_profile_name(tc, cal_profile));
    }

    init(t);

    tfa_cont_write_device_registers(tc, t);
    tfa_cont_write_profile_registers(tc, cal_profile, t);
    factory_trimmer(t);
    tfa_power_off(t);
    tfa_set_register(t, TFA98XX_BF_SRC_SET_CONFIGURED, 1);

    wait_dsp_system_stable_is(t, 1);

    //speaker_boost(t, 0, cal_profile_num);
#if 0
    tfa_set_bitfield(t, TFA98XX_BF_ENBL_POWERSWITCH, 1);
    tfa_set_bitfield(t, TFA98XX_BF_ENBL_PDM_SS, 0);

    tfa_set_bitfield(t, TFA98XX_BF_ENBL_BOOST, 1);
    tfa_set_bitfield(t, TFA98XX_BF_BOOST_VOLT, 6);
    tfa_set_bitfield(t, TFA98XX_BF_ENBL_BOD, 1);
    tfa_set_bitfield(t, TFA98XX_BF_EXT_TEMP, 25);
    tfa_set_bitfield(t, TFA98XX_BF_EXT_TEMP_SEL, 1);
    tfa_set_bitfield(t, TFA98XX_BF_ENBL_BOOST, 0);
#endif

    return 0;
}

int tfa_start_dsp(tfa_t *t, tfa_cont_t *tc)
{
    int audio_fs;
    struct pcm *pcm;

    pcm = tfa_clocks_and_power_on(t);
    tfa_set_bitfield(t, TFA98XX_BF_SRC_SET_CONFIGURED, 0);
    tfa_set_bitfield(t, TFA98XX_BF_EXECUTE_COLD_START, 1);
    tfa_cont_write_patch(tc, t);
    tfa_dsp_write_mem_word(t, 512, 0, 1);
    tfa_set_bitfield(t, TFA98XX_BF_CF_RST_DSP, 0);
    audio_fs = tfa_get_bitfield(t, TFA98XX_BF_AUDIO_FS);
    tfa_dsp_write_tables(t, audio_fs);
    tfa_clocks_and_power_off(t, pcm);

    return 0;
}

int tfa_stop(tfa_t *t)
{
t;
    // TODO
    return 0;
}

int tfa_dsp_system_stable(tfa_t *t)
{
    unsigned v = tfa_get_register(t, bf_register(TFA98XX_BF_FLAG_ENBL_REF));
    return bf_value(TFA98XX_BF_FLAG_ENBL_REF, v) && bf_value(TFA98XX_BF_FLAG_CLOCKS_STABLE, v);
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

static int tfa_write_data(tfa_t *t, unsigned char *bytes, unsigned n)
{
printf("WR");
dump_raw(bytes, n);
printf("\n");
    return write(t->fd, bytes, n);
}

#define MAX_BYTES_TO_TRANSFER       253

int tfa_dsp_read_mem(tfa_t *t, unsigned dmem_and_madd, int n_words, int *data)
{
    int dmem;
    unsigned char tmp[MAX_BYTES_TO_TRANSFER];
    int n_read = 0;

    dmem = (dmem_and_madd >> 16) & 0xf;
    if (! dmem) dmem = 1;

    tfa_set_bitfield(t, TFA98XX_BF_CF_DMEM, dmem);
    tfa_set_register(t, bf_register(TFA98XX_BF_CF_MADD), dmem_and_madd);

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

    tfa_set_bitfield(t, TFA98XX_BF_CF_DMEM, dmem);
    tfa_set_register(t, bf_register(TFA98XX_BF_CF_MADD), madd);
    bytes[0] = bf_register(TFA98XX_BF_CF_MEMA);
    data2bytes(1, &value, &bytes[1]);
    return tfa_write_data(t, bytes, 4);
}

int tfa_dsp_msg_write(tfa_t *t, unsigned n_bytes, unsigned char *bytes)
{
    int dmem;

    dmem = tfa_get_register(t, bf_register(TFA98XX_BF_CF_DMEM));
    dmem = bf_update_bf(TFA98XX_BF_CF_DMEM, dmem, 1);
    dmem = bf_update_bf(TFA98XX_BF_CF_AIF, dmem, 0);
    tfa_set_register(t, bf_register(TFA98XX_BF_CF_DMEM), dmem);
    tfa_set_register(t, bf_register(TFA98XX_BF_CF_MADD), 1);

    while (n_bytes > 0) {
        unsigned this_n_write = MAX_BYTES_TO_TRANSFER;

        if (this_n_write > n_bytes) {
            this_n_write = n_bytes;
        }

        tfa_write_data(t, bytes, this_n_write);

        n_bytes -= this_n_write;
        bytes += this_n_write;
    }

    dmem = bf_update_bf(TFA98XX_BF_CF_INT, dmem, 1);
    dmem = bf_update_bf(0x9080, dmem, 1);


    return tfa_set_register(t, bf_register(TFA98XX_BF_CF_INT), dmem);
}

int tfa_dsp_msg_status(tfa_t *t, int max_retries)
{
    int retry;

    for (retry = 0; retry < max_retries; retry++) {
        int ack = tfa_get_bitfield(t, TFA98XX_BF_CF_ACK);
        if (ack & 1) return ack;
    }
    return -1;
}

int tfa_dsp_msg(tfa_t *t, unsigned n_bytes, unsigned char *bytes)
{
    int status;

    tfa_dsp_msg_write(t, n_bytes, bytes);
    status = tfa_dsp_msg_status(t, 2);
    return 0;
}

int tfa_dsp_patch(tfa_t *t, int n_data, unsigned char *data)
{
    int dmem_and_madd;
    int rom_id, expected_rom_id;

    dmem_and_madd = (data[1] << 8) | data[2];
    expected_rom_id = (data[3] << 16) | (data[4] << 8) | data[5];

#if TODO
    if (dmem_and_madd == 0xffff) {
        if (expected_rom_id && expected_rom_id != 0xfffff) {
           ALOGE("patch specifies rom_id to check but no address to use to check it");
printf("invalid 1\n");
           return -EINVAL;
        }
    } else {
        if (! tfa_dsp_system_stable(t)) {
            ALOGE("failed to write patch because DSP is not stable.");
printf("invalid 2\n");
            return -ENOENT;
        }

        tfa_dsp_read_mem(t, dmem_and_madd, 1, &rom_id);
        if (rom_id != expected_rom_id) {
             ALOGE("Invalid rom_id %d, expected %d\n", rom_id, expected_rom_id);
printf("invalid 3\n");
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

        tfa_write_data(t, data, hunk_n);

        data += hunk_n;
        n_data -= hunk_n;
    }

    return 0;
}

int tfa_dsp_write_tables(tfa_t *t, int audio_fs)
{
    unsigned char buf[16] = { 0x92, 0, };
    unsigned char *msg = &buf[1];
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

    tfa_set_bitfield(t, TFA98XX_BF_CS_FRAC_DELAY, frac_delay);
#if 0
    /* I can't see in the code where the 0x92 gets added, it should just be: */
    return tfa_dsp_msg(t, 15, msg);
#else
    return tfa_dsp_msg(t, 16, buf);
#endif
}

int tfa_set_swprof(tfa_t *t, int swprof)
{
    int old_swprof = t->swprof;

    t->swprof = swprof;
    tfa_set_bitfield(t, TFA98XX_BF_SW_PROFILE, swprof+1);

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
    tfa_set_bitfield(t, TFA98XX_BF_SW_PROFILE, swvstep+1);

    return old_swvstep;
}

int tfa_get_swvstep(tfa_t *t)
{
    return t->swvstep;
}
