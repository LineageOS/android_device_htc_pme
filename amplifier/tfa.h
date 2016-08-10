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

#ifndef __TFA_H__
#define __TFA_H__

#include <tinyalsa/asoundlib.h>

typedef struct tfaS tfa_t;

#include "tfa-cont.h"

#define TFA_DEV_FAMILY   2
#define TFA_NUM_SPEAKERS 2
#define TFA_DAI          2

struct tfa_profile {
    int something[4];
};

tfa_t *tfa_new(void);

struct pcm *tfa_clocks_on(tfa_t *);
int tfa_clocks_off(tfa_t *, struct pcm *pcm);

int tfa_power_on(tfa_t *);
void tfa_power_off(tfa_t *);

struct pcm *tfa_clocks_and_power_on(tfa_t *);
void tfa_clocks_and_power_off(tfa_t *, struct pcm *);

void tfa_set_register(tfa_t *t, unsigned reg, unsigned value);
int tfa_get_register(tfa_t *t, unsigned reg);
void tfa_set_bitfield(tfa_t *, int bf, unsigned value);
void tfa_set_bitfield_always(tfa_t *, int bf, unsigned value);
int tfa_get_bitfield(tfa_t *, int bf);

void tfa_start(tfa_t *, tfa_cont_t *tc, int profile_num, int vstep);
void tfa_stop(tfa_t *t);

void tfa_start_dsp(tfa_t *t, tfa_cont_t *tc);

int tfa_set_swprof(tfa_t *, int swprof);
int tfa_get_swprof(tfa_t *);

int tfa_set_swvstep(tfa_t *, int swvstep);
int tfa_get_swvstep(tfa_t *);

int tfa_dsp_system_stable(tfa_t *);

void tfa_dsp_read_mem(tfa_t *t, unsigned dmem_and_madd, int n_words, int *data);
void tfa_dsp_write_mem_word(tfa_t *t, unsigned madd, int value, unsigned dmem);

void tfa_dsp_patch(tfa_t *t, int n_data, unsigned char *data);
void tfa_dsp_write_tables(tfa_t *t, int audio_fs);

void tfa_dsp_msg(tfa_t *t, unsigned n_bytes, unsigned char *bytes);
void tfa_dsp_msg_id(tfa_t *t, unsigned n_bytes, unsigned char *bytes, unsigned id);
void tfa_dsp_msg_cmd_id(tfa_t *t, unsigned char cmd1, unsigned char cmd2, unsigned n_bytes, unsigned char *bytes);

int tfa_wakelock(tfa_t *t, int on);

void tfa_destroy(tfa_t *);

#endif
