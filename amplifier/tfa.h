#ifndef __TFA_H__
#define __TFA_H__

typedef struct tfaS tfa_t;

struct tfa_profile {
    int something[4];
};

tfa_t *tfa_new(void);
int tfa_imp_check(tfa_t *, int *ohm_ret, int right_amp);
int tfa_imp_set(tfa_t *, int right_amp);
int tfa_cnt_loadfile(tfa_t *, const char *fnmae, int dunno);
int tfa_start(tfa_t *, int something, struct tfa_profile *profile);
void tfa_destroy(tfa_t *);

#endif
