#ifndef __TFA_CONT_H__
#define __TFA_CONT_H__

typedef struct tfa_contS tfa_cont_t;

#include "tfa.h"

tfa_cont_t *tfa_cont_new(const char *fname);
void tfa_cont_destroy(tfa_cont_t *);
int tfa_cont_get_cal_profile(tfa_cont_t *tc);
const char *tfa_cont_get_profile_name(tfa_cont_t *tc, int profile);

int tfa_cont_write_device_files(tfa_cont_t *, tfa_t *);
int tfa_cont_write_patch(tfa_cont_t *tc, tfa_t *t);
int tfa_cont_write_profile(tfa_cont_t *tc, int prof_num, int vstep, tfa_t *t);
int tfa_cont_write_profile_files(tfa_cont_t *tc, int profile_num, tfa_t *t, int vstep);

int tfa_cont_write_device_registers(tfa_cont_t *tc, tfa_t *t);
int tfa_cont_write_profile_registers(tfa_cont_t *tc, int prof_num, tfa_t *t);

#endif
