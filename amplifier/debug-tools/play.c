#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include "tfa.h"
#include "tfa-cont.h"
#include "tfa9888.h"
#include "tfa9888-debug.h"

tfa_t *t;

int tfa_imp_check(tfa_t *t, int *ohm_ret, int is_right)
{
    struct pcm *pcm;

    pcm = tfa_clocks_on(t);
    *ohm_ret = tfa_get_bitfield(t, is_right ? BF_CALIBR_R25C_R : BF_CALIBR_R25C_L);
    tfa_clocks_off(t, pcm);

    if (is_right) {
        return 5000 <= *ohm_ret && *ohm_ret <= 8000;
    } else {
        return 6500 <= *ohm_ret && *ohm_ret <= 9500;
    }
    return 0;
}

int main()
{
    time_t start = time(NULL);
    struct pcm *pcm;
    tfa_cont_t *tc;
    int cal_profile;
    int ohm_left, ohm_right;

    /* tfa_new is equivalent to tfa_probe_all */
    t = tfa_new();
    if (! t) exit(1);

    tc = tfa_cont_new("/system/etc/Tfa98xx.cnt");
    if (! tc) exit(1);

    cal_profile = tfa_cont_get_cal_profile(tc);
    printf("Calibration: %d: %s\n", cal_profile, tfa_cont_get_profile_name(tc, cal_profile));
    printf("Revision: %x\n", tfa_get_bitfield(t, BF_DEVICE_REV));

    pcm = tfa_clocks_on(t);
    tfa_start(t, tc, 0, 0);
    tfa_clocks_off(t, pcm);

    tfa_imp_check(t, &ohm_left, 0);
    tfa_imp_check(t, &ohm_right, 1);

    tfa_stop(t);

#if 0
    printf("Enabling\n");
    pcm = tfa_clocks_and_power_on(t);
    // tfa_start(t, tc, 0, 1);
#if 0
tfa_set_bitfield(t, 0x5a07, 0xff);
tfa_set_bitfield(t, 0x5187, 0xff);
#endif
    sleep(10);
    printf("Disabling\n");
    tfa_clocks_and_power_off(t, pcm);
    tfa_stop(t);
#endif

    return 0;
}
