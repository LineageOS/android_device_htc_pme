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
#include <dlfcn.h>
#include <errno.h>
#include <cutils/log.h>
#include <tinyalsa/asoundlib.h>
#include "tfa.h"

struct tfaS {
    struct mixer *mixer;
    void *dl;
    int (*imp_check)(int *, int);
    int (*htc_set_imp)(int, int);
    int (*cnt_loadfile)(const char *, int);
};

#define DL_LIB "libtfa_vendor_helper.so"

#define SND_CARD        0
#define AMP_PCM_DEV     47
#define AMP_MIXER_CTL   "QUAT_MI2S_RX_DL_HL Switch"


tfa_t *tfa_new(void)
{
    struct mixer *mixer;
    void *dl;
    tfa_t *t;


    if ((mixer = mixer_open(SND_CARD)) == NULL) {
        ALOGE("failed to open mixer");
        return NULL;
    }

    if ((dl = dlopen(DL_LIB, RTLD_LOCAL | RTLD_LAZY)) == NULL) {
        ALOGE("failed to dlopen %s: %s", DL_LIB, dlerror());
        mixer_close(mixer);
        return NULL;
    }

    if ((t = malloc(sizeof(*t))) == NULL) {
        ALOGE("out of memory");
        dlclose(dl);
        mixer_close(mixer);
        return NULL;
    }

    t->mixer = mixer;
    t->dl = dl;
    t->imp_check = dlsym(dl, "tfa_imp_check");
    t->htc_set_imp = dlsym(dl, "tfa_htc_set_imp");
    t->cnt_loadfile = dlsym(dl, "tfa98xx_cnt_loadfile");

    return t;
}

void tfa_destroy(tfa_t *t)
{
    mixer_close(0);
    dlclose(t->dl);
    free(t);
}

int tfa_imp_check(tfa_t *t, int *ohm_ret, int is_right)
{
    return t->imp_check(ohm_ret, is_right);
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

static int mi2s_en(tfa_t *t, int enable)
{
    enum mixer_ctl_type type;
    struct mixer_ctl *ctl;
    struct pcm *pcm;

    ctl = mixer_get_ctl_by_name(t->mixer, AMP_MIXER_CTL);
    if (ctl == NULL) {
        ALOGE("%s: Could not find %s\n", __func__, AMP_MIXER_CTL);
        return -ENODEV;
    }

    pcm = pcm_open(SND_CARD, AMP_PCM_DEV, PCM_OUT, &amp_pcm_config);
    if (! pcm) {
        ALOGE("failed to open pcm at all??");
        return -ENODEV;
    }
    if (!pcm_is_ready(pcm)) {
        ALOGE("failed to open pcm device: %s", pcm_get_error(pcm));
        pcm_close(pcm);
        return -ENODEV;
    }

    mixer_ctl_set_value(ctl, 0, enable);

    pcm_close(pcm);

    return 0;
}

int tfa_imp_set(tfa_t *t, int is_right)
{
    int ret;

    mi2s_en(t, 1);
    ret = t->htc_set_imp(is_right ? 6500 : 7700, is_right);
    mi2s_en(t, 0);

    return ret;
}

int
tfa_cnt_loadfile(tfa_t *t, const char *fname, int dunno)
{
    return t->cnt_loadfile(fname, dunno);
}

