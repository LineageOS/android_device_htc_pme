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

#ifndef __TFA_CONT_H__
#define __TFA_CONT_H__

typedef struct tfa_contS tfa_cont_t;

#include "tfa.h"

tfa_cont_t *tfa_cont_new(const char *fname);
void tfa_cont_destroy(tfa_cont_t *);
int tfa_cont_get_cal_profile(tfa_cont_t *tc);
const char *tfa_cont_get_profile_name(tfa_cont_t *tc, int profile);

void tfa_cont_write_device_files(tfa_cont_t *, tfa_t *);
void tfa_cont_write_patch(tfa_cont_t *tc, tfa_t *t);
void tfa_cont_write_profile(tfa_cont_t *tc, int prof_num, int vstep, tfa_t *t);
void tfa_cont_write_profile_files(tfa_cont_t *tc, int profile_num, tfa_t *t, int vstep);

void tfa_cont_write_device_registers(tfa_cont_t *tc, tfa_t *t);
void tfa_cont_write_profile_registers(tfa_cont_t *tc, int prof_num, tfa_t *t);

int tfa_cont_get_current_vstep(tfa_cont_t *tc);
void tfa_cont_write_file_vstep(tfa_cont_t *tc, int profile_num, int vstep, tfa_t *t);

#endif
