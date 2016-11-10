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

#define LOG_TAG "audio_amplifier"
//#define LOG_NDEBUG 0

#include <stdio.h>
#include <stdlib.h>
#include <cutils/log.h>
#include <cutils/str_parms.h>

#include <hardware/audio_amplifier.h>
#include <msm8974/platform.h>
#include <system/audio.h>

#include "tfa.h"
#include "tfa-cont.h"
#include "tfa9888.h"

#define VSTEP                   0
#define FORCED_GAIN             75
#define FORCED_GAIN_EARPIECE    100

#define UNUSED __attribute__ ((unused))

typedef struct amp_device {
    amplifier_device_t amp_dev;
    tfa_t *tfa;
    tfa_cont_t *tc;
    audio_mode_t mode;
    struct pcm *pcm;
} amp_device_t;

static amp_device_t *amp_dev = NULL;

static int amp_set_mode(struct amplifier_device *device, audio_mode_t mode)
{
    int ret = 0;
    amp_device_t *dev = (amp_device_t *) device;

    dev->mode = mode;
    return ret;
}

#define PROFILE_MUSIC           0
#define PROFILE_RINGTONE        1
#define PROFILE_FM              2
#define PROFILE_VIDEO_RECORD    3
#define PROFILE_HANDSFREE_NB    4
#define PROFILE_HANDSFREE_WB    5
#define PROFILE_HANDSFREE_SWB   6
#define PROFILE_HANDSET         7
#define PROFILE_VOIP            8
#define PROFILE_MFG             9
#define PROFILE_AUDIO_EVALUATION 10
#define PROFILE_CALIBRATION     11

enum {
    IS_EARPIECE, IS_SPEAKER, IS_VOIP, IS_OTHER
};

static int classify_snd_device(uint32_t snd_device)
{
    switch(snd_device) {
    case SND_DEVICE_OUT_HANDSET:
    case SND_DEVICE_OUT_VOICE_HANDSET:
        return IS_EARPIECE;

    case SND_DEVICE_OUT_SPEAKER:
    case SND_DEVICE_OUT_VOICE_SPEAKER:
        return IS_SPEAKER;

    case SND_DEVICE_OUT_VOICE_TX:
        return IS_VOIP;

    case SND_DEVICE_OUT_SPEAKER_EXTERNAL_1:
    case SND_DEVICE_OUT_SPEAKER_EXTERNAL_2:
    case SND_DEVICE_OUT_SPEAKER_REVERSE:
    case SND_DEVICE_OUT_SPEAKER_VBAT:
    case SND_DEVICE_OUT_LINE:
    case SND_DEVICE_OUT_HEADPHONES:
    case SND_DEVICE_OUT_HEADPHONES_44_1:
    case SND_DEVICE_OUT_SPEAKER_AND_HEADPHONES:
    case SND_DEVICE_OUT_SPEAKER_AND_LINE:
    case SND_DEVICE_OUT_SPEAKER_AND_HEADPHONES_EXTERNAL_1:
    case SND_DEVICE_OUT_SPEAKER_AND_HEADPHONES_EXTERNAL_2:
    case SND_DEVICE_OUT_VOICE_SPEAKER_VBAT:
    case SND_DEVICE_OUT_VOICE_HEADPHONES:
    case SND_DEVICE_OUT_VOICE_LINE:
    case SND_DEVICE_OUT_HDMI:
    case SND_DEVICE_OUT_SPEAKER_AND_HDMI:
    case SND_DEVICE_OUT_BT_SCO:
    case SND_DEVICE_OUT_BT_SCO_WB:
    case SND_DEVICE_OUT_VOICE_TTY_FULL_HEADPHONES:
    case SND_DEVICE_OUT_VOICE_TTY_VCO_HEADPHONES:
    case SND_DEVICE_OUT_VOICE_TTY_HCO_HANDSET:
    case SND_DEVICE_OUT_AFE_PROXY:
    case SND_DEVICE_OUT_USB_HEADSET:
    case SND_DEVICE_OUT_SPEAKER_AND_USB_HEADSET:
    case SND_DEVICE_OUT_TRANSMISSION_FM:
    case SND_DEVICE_OUT_ANC_HEADSET:
    case SND_DEVICE_OUT_ANC_FB_HEADSET:
    case SND_DEVICE_OUT_VOICE_ANC_HEADSET:
    case SND_DEVICE_OUT_VOICE_ANC_FB_HEADSET:
    case SND_DEVICE_OUT_SPEAKER_AND_ANC_HEADSET:
    case SND_DEVICE_OUT_ANC_HANDSET:
    case SND_DEVICE_OUT_SPEAKER_PROTECTED:
    case SND_DEVICE_OUT_VOICE_SPEAKER_PROTECTED:
    case SND_DEVICE_OUT_SPEAKER_PROTECTED_VBAT:
    case SND_DEVICE_OUT_VOICE_SPEAKER_PROTECTED_VBAT:
    case SND_DEVICE_OUT_SPEAKER_WSA:
    case SND_DEVICE_OUT_VOICE_SPEAKER_WSA:
    default:
        return IS_OTHER;
    }
}


static int select_profile(audio_mode_t mode, uint32_t snd_device)
{
    uint32_t device_class = classify_snd_device(snd_device);

    ALOGV("%s: mode %d devices 0x%x class %d", __func__, mode, snd_device, device_class);
    switch(mode) {
    case AUDIO_MODE_RINGTONE:
        return PROFILE_RINGTONE;
    case AUDIO_MODE_IN_COMMUNICATION:
        return PROFILE_VOIP;
    case AUDIO_MODE_NORMAL:
    case AUDIO_MODE_IN_CALL:
        if (device_class == IS_EARPIECE) {
            return PROFILE_HANDSET;
        } else {
            return PROFILE_MUSIC;
        }
    default:
        switch(device_class) {
        case IS_EARPIECE:
            return PROFILE_HANDSET;
        case IS_SPEAKER:
            return PROFILE_MUSIC;
        case IS_VOIP:
            return PROFILE_VOIP;
        default:
            return -1;
        }
    }
}

static void do_set_gain(amp_device_t *amp_dev, uint32_t gain)
{
    ALOGV("setting gain to %u", gain);
    tfa_set_bitfield(amp_dev->tfa, BF_GAIN, gain);
}

static void set_gain(amp_device_t *dev, uint32_t snd_device)
{
    switch(classify_snd_device(snd_device)) {
    case IS_EARPIECE:
        do_set_gain(dev, FORCED_GAIN_EARPIECE);
        break;
    default:
        do_set_gain(dev, FORCED_GAIN);
        break;
    }
}

static int amp_enable_output_devices(struct amplifier_device *device, uint32_t snd_device, bool enable)
{
    amp_device_t *dev = (amp_device_t *) device;
    int profile = select_profile(dev->mode, snd_device);
    if (profile < 0 || !enable) {
        if (dev->pcm) {
            tfa_clocks_off(dev->tfa, dev->pcm);
            tfa_stop(dev->tfa);
            dev->pcm = NULL;
        }
    } else {
        if (!dev->pcm) {
            dev->pcm = tfa_clocks_on(dev->tfa);
        }
        ALOGV("%s: starting profile %d vstep <hardcoded to %d>", __func__, profile, VSTEP);
        tfa_start(dev->tfa, dev->tc, profile, VSTEP);
        set_gain(dev, snd_device);
    }

    return 0;
}

static int amp_dev_close(hw_device_t *device)
{
    amp_device_t *dev = (amp_device_t *) device;

    tfa_destroy(dev->tfa);
    tfa_cont_destroy(dev->tc);

    free(dev);

    return 0;
}

static void init(void)
{
    struct pcm *pcm;

    pcm = tfa_clocks_on(amp_dev->tfa);
    tfa_start(amp_dev->tfa, amp_dev->tc, 0, 0);
    tfa_clocks_off(amp_dev->tfa, pcm);

    tfa_stop(amp_dev->tfa);
}

static int amp_module_open(const hw_module_t *module, const char *name UNUSED,
        hw_device_t **device)
{
    int ret;
    tfa_t *tfa;
    tfa_cont_t *tc;

    if (amp_dev) {
        ALOGE("%s:%d: Unable to open second instance of the amplifier\n", __func__, __LINE__);
        return -EBUSY;
    }

    tfa = tfa_new();
    if (!tfa) {
        ALOGE("%s:%d: Unable to tfa lib\n", __func__, __LINE__);
        return -ENOENT;
    }
    tc = tfa_cont_new("/system/etc/Tfa98xx.cnt");
    if (!tc) {
        ALOGE("%s:%d: Unable to open tfa container\n", __func__, __LINE__);
        tfa_destroy(tfa);
        return -ENOENT;
    }

    amp_dev = calloc(1, sizeof(amp_device_t));
    if (!amp_dev) {
        ALOGE("%s:%d: Unable to allocate memory for amplifier device\n", __func__, __LINE__);
        tfa_destroy(tfa);
        return -ENOMEM;
    }

    amp_dev->amp_dev.common.tag = HARDWARE_DEVICE_TAG;
    amp_dev->amp_dev.common.module = (hw_module_t *) module;
    amp_dev->amp_dev.common.version = HARDWARE_DEVICE_API_VERSION(1, 0);
    amp_dev->amp_dev.common.close = amp_dev_close;

    amp_dev->amp_dev.set_input_devices = NULL;
    amp_dev->amp_dev.set_output_devices = NULL;
    amp_dev->amp_dev.enable_input_devices = NULL;
    amp_dev->amp_dev.enable_output_devices = amp_enable_output_devices;
    amp_dev->amp_dev.set_mode = amp_set_mode;
    amp_dev->amp_dev.output_stream_start = NULL;
    amp_dev->amp_dev.input_stream_start = NULL;
    amp_dev->amp_dev.output_stream_standby = NULL;
    amp_dev->amp_dev.input_stream_standby = NULL;

    amp_dev->tfa = tfa;
    amp_dev->tc  = tc;

    init();

    *device = (hw_device_t *) amp_dev;

    return 0;
}

static struct hw_module_methods_t hal_module_methods = {
    .open = amp_module_open,
};

amplifier_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = AMPLIFIER_MODULE_API_VERSION_0_1,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = AMPLIFIER_HARDWARE_MODULE_ID,
        .name = "Kiwi audio amplifier HAL",
        .author = "The CyanogenMod Open Source Project",
        .methods = &hal_module_methods,
    },
};
