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
    tfa_start(t, tc);
    tfa_clocks_off(t, pcm);

    tfa_imp_check(t, &ohm_left, 0);
    tfa_imp_check(t, &ohm_right, 1);

    tfa_stop(t);

#if 0
    tfa_start_dsp(t, tc);
    tra_cont_speaker_setup(tc, cal_profile, t);
    tfa_set_swprof(t, cal_profile);
    tfa_set_swvstep(t, 0);
    if (tfa_get_bitfield(t, BF_ENBL_COOLFLUX)) {
        tfa_set_bitfield(t, BF_COOLFLUX_CONFIGURED, 1);
        // tfaRunSpeakerCalibration
    }

    if (tfa_get_bitfield(t, BF_CALIBRATION_ONETIME)) {
        printf("onetime");
    } else {
        printf("not onetime");
    }

    tfa_cont_write_profile(tc, cal_profile, t);
#endif

#if 0
    printf("nEnabling clocks\n");
    pcm = tfa_clocks_on(t);
    tfa_set_bitfield(t, BF_POWERDOWN, 0);
    printf("\n\nWaiting for clocks\n");
    while (time(NULL) - start < 10 && tfa_get_bitfield(t, BF_FLAG_LOST_CLK)) sleep(1);
    printf("Clocks enabled, powering up\n");
    tfa_set_bitfield(t, BF_POWERDOWN, 0);
    while (time(NULL) - start < 10) sleep(1);

    printf("Disabling\n");
    tfa_clocks_off(t, pcm);
#endif

    return 0;
}
