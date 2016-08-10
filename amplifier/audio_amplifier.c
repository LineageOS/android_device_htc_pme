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

/* Implementation notes:
 *
 * adev_dot_common + 160 == mixer
 * tfa_amp_mode_MAYBE = 4 (it would be 5 is wideband was enabled)
 * tfa_imp_write() looks like it always fails (missing /data/audio/TFA_RE[12])
 */

#define LOG_TAG "audio_amplifier"
//#define LOG_NDEBUG 0

#include <stdio.h>
#include <stdlib.h>
#include <cutils/log.h>
#include <cutils/str_parms.h>

#include <hardware/audio_amplifier.h>
#include <system/audio.h>

#include "tfa.h"

#define UNUSED __attribute__ ((unused))

typedef struct amp_device {
    amplifier_device_t amp_dev;
    audio_mode_t mode;
    tfa_t *tfa;
} amp_device_t;

static amp_device_t *amp_dev = NULL;

static int amp_set_mode(struct amplifier_device *device, audio_mode_t mode)
{
    int ret = 0;
    amp_device_t *dev = (amp_device_t *) device;

    dev->mode = mode;
    return ret;
}

#define TFA_DEVICE_MASK (AUDIO_DEVICE_OUT_EARPIECE | AUDIO_DEVICE_OUT_SPEAKER)

static int amp_enable_output_devices(struct amplifier_device *device, uint32_t devices, bool enable)
{
    amp_device_t *dev = (amp_device_t *) device;
    int ret;

    if ((devices & TFA_DEVICE_MASK) != 0) {
        if (enable) {
        } else {
        }
    }

    return 0;
}

static int amp_dev_close(hw_device_t *device)
{
    amp_device_t *dev = (amp_device_t *) device;

    tfa_destroy(dev->tfa);
    free(dev);

    return 0;
}

static void set_imp(amp_device_t *amp_dev, int is_right)
{
    int ohm = -1;
    const char *which = is_right ? "right" : "left";

    if (tfa_imp_check(amp_dev->tfa, &ohm, is_right)) {
        ALOGI("%s impedence is %d\n", which, ohm);
    } else {
        ALOGI("%s impedence data unacceptable %d, set it\n", which, ohm);
        if (tfa_imp_set(amp_dev->tfa, is_right)) {
            ALOGI("failed to set %s impedence", which);
        }
        if (tfa_imp_check(amp_dev->tfa, &ohm, is_right)) {
            ALOGI("%s impedence is %d\n", which, ohm);
        }
    }
}

#define DEFAULT_CNT_FILE "/system/etc/Tfa98xx.cnt"

static void tfa_init(amp_device_t *amp_dev)
{
    int retry;

    for (retry = 0; retry < 2; retry++ ) {
        if (tfa_cnt_loadfile(amp_dev->tfa, DEFAULT_CNT_FILE, 0)) {
            ALOGI("loaded: %s", DEFAULT_CNT_FILE);
            break;
        }
        ALOGI("failed to load default cnt file: %s", DEFAULT_CNT_FILE);
        usleep(10000);
    }

    if (retry >= 2) {
        ALOGI("failed to initialize amp, set default imp and MTPEX");
        set_imp(amp_dev, 0);
        set_imp(amp_dev, 1);
        tfa_imp_set(amp_dev->tfa, -1);
    }
}

static int amp_module_open(const hw_module_t *module, const char *name UNUSED,
        hw_device_t **device)
{
    int ret;
    tfa_t *tfa;

    if (amp_dev) {
        ALOGE("%s:%d: Unable to open second instance of the amplifier\n", __func__, __LINE__);
        return -EBUSY;
    }

    tfa = tfa_new();
    if (!tfa) {
        ALOGE("%s:%d: Unable to tfa lib\n", __func__, __LINE__);
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

    tfa_init(amp_dev);

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
