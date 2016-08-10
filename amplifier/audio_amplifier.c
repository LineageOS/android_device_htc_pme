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
#include <system/audio.h>

#include <tinyalsa/asoundlib.h>

#define UNUSED __attribute__ ((unused))

typedef struct amp_device {
    amplifier_device_t amp_dev;
    audio_mode_t mode;
    struct mixer *mixer;        /* adev_dot_common + 160 */
} amp_device_t;

static amp_device_t *amp_dev = NULL;

#define AMP_MIXER_CTL "Initial external PA"

#if 0
static int set_clocks_enabled(bool enable)
{
    enum mixer_ctl_type type;
    struct mixer_ctl *ctl;

    if (mixer == NULL) {
        ALOGE("Error opening mixer 0");
        return -1;
    }

    ctl = mixer_get_ctl_by_name(mixer, AMP_MIXER_CTL);
    if (ctl == NULL) {
        ALOGE("%s: Could not find %s\n", __func__, AMP_MIXER_CTL);
        return -ENODEV;
    }

    type = mixer_ctl_get_type(ctl);
    if (type != MIXER_CTL_TYPE_ENUM) {
        ALOGE("%s: %s is not supported\n", __func__, AMP_MIXER_CTL);
        return -ENOTTY;
    }

    mixer_ctl_set_value(ctl, 0, enable);
    return 0;
}
#endif

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

    mixer_close(dev->mixer);
    free(dev);

    return 0;
}

static int amp_module_open(const hw_module_t *module, const char *name UNUSED,
        hw_device_t **device)
{
    int ret;

    if (amp_dev) {
        ALOGE("%s:%d: Unable to open second instance of the amplifier\n", __func__, __LINE__);
        return -EBUSY;
    }

    amp_dev = calloc(1, sizeof(amp_device_t));
    if (!amp_dev) {
        ALOGE("%s:%d: Unable to allocate memory for amplifier device\n", __func__, __LINE__);
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

    amp_dev->mixer = mixer_open(0);

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
