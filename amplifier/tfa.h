#ifndef __TFA_H__
#define __TFA_H__

#include <tinyalsa/asoundlib.h>

typedef struct tfaS tfa_t;

#include "tfa-cont.h"

struct tfa_profile {
    int something[4];
};

tfa_t *tfa_new(void);

struct pcm *tfa_mi2s_enable(tfa_t *);
int tfa_mi2s_disable(tfa_t *, struct pcm *pcm);
struct pcm *tfa_power_on(tfa_t *);
void tfa_power_off(tfa_t *, struct pcm *);

int tfa_set_register(tfa_t *t, unsigned reg, unsigned value);
int tfa_get_register(tfa_t *t, unsigned reg);
int tfa_set_bitfield(tfa_t *, int bf, unsigned value);
int tfa_get_bitfield(tfa_t *, int bf);

int tfa_startup(tfa_t *);
int tfa_start_dsp(tfa_t *t, tfa_cont_t *tc);

int tfa_dsp_system_stable(tfa_t *);
int tfa_dsp_read_mem(tfa_t *t, unsigned dmem_and_madd, int n_words, int *data);
int tfa_dsp_write_mem_word(tfa_t *t, unsigned madd, int value, unsigned dmem);
int tfa_dsp_patch(tfa_t *t, int n_data, unsigned char *data);
int tfa_dsp_write_tables(tfa_t *t, int audio_fs);

void tfa_destroy(tfa_t *);

#endif
