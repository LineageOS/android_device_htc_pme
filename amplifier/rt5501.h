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

#ifndef _RT55XX_H_
#define _RT55XX_H_

#define RT55XX_DEVICE "/dev/rt5501"
#define RT55XX_MAX_REG_DATA 15

enum {
    RT55XX_INIT = 0,
    RT55XX_MUTE,
    RT55XX_MAX_FUNC
};

enum RT55XX_Mode {
    RT55XX_MODE_OFF = RT55XX_MAX_FUNC,
    RT55XX_MODE_PLAYBACK,
    RT55XX_MODE_PLAYBACK8OH,
    RT55XX_MODE_PLAYBACK16OH,
    RT55XX_MODE_PLAYBACK32OH,
    RT55XX_MODE_PLAYBACK64OH,
    RT55XX_MODE_PLAYBACK128OH,
    RT55XX_MODE_PLAYBACK256OH,
    RT55XX_MODE_PLAYBACK500OH,
    RT55XX_MODE_PLAYBACK1KOH,
    RT55XX_MODE_VOICE,
    RT55XX_MODE_TTY,
    RT55XX_MODE_FM,
    RT55XX_MODE_RING,
    RT55XX_MODE_MFG,
    RT55XX_MODE_BEATS_8_64,
    RT55XX_MODE_BEATS_128_500,
    RT55XX_MODE_MONO,
    RT55XX_MODE_MONO_BEATS,
    RT55XX_MAX_MODE
};

enum HEADSET_OM {
    HEADSET_8OM = 0,
    HEADSET_16OM,
    HEADSET_32OM,
    HEADSET_64OM,
    HEADSET_128OM,
    HEADSET_256OM,
    HEADSET_500OM,
    HEADSET_1KOM,
    HEADSET_MONO,
    HEADSET_OM_UNDER_DETECT,
};

struct rt55xx_reg_data {
    unsigned char addr;
    unsigned char val;
};

struct rt55xx_config {
    unsigned int reg_len;
    struct rt55xx_reg_data reg[RT55XX_MAX_REG_DATA];
};

struct rt55xx_comm_data {
    unsigned int out_mode;
    struct rt55xx_config config;
};

struct rt55xx_config_data {
    unsigned int mode_num;
    struct rt55xx_comm_data cmd_data[RT55XX_MAX_MODE];
};

#define RT55XX_IOCTL_MAGIC 'g'
#define RT55XX_SET_MODE     _IOW(RT55XX_IOCTL_MAGIC, 0x03, int)
#define RT55XX_SET_PARAM    _IOW(RT55XX_IOCTL_MAGIC, 0x04, struct rt55xx_config_data)
#define RT55XX_QUERY_OM     _IOR(RT55XX_IOCTL_MAGIC, 0x08, int)

int rt55xx_open(void);
int rt55xx_set_mode(audio_mode_t mode);

#endif
