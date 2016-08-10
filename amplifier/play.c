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

void dump_reg(int reg)
{
    unsigned v = tfa_get_register(t, reg);;
//    printf("REG %x: ", reg);
//    tfa9888_print_bitfield(stdout, reg, v);
}

int main()
{
    time_t start = time(NULL);
    struct pcm *pcm;
    tfa_cont_t *tc;
    int cal_profile;

    t = tfa_new();
    if (! t) exit(1);

    tc = tfa_cont_new("/system/etc/Tfa98xx.cnt");
    if (! tc) exit(1);

    cal_profile = tfa_cont_get_cal_profile(tc);
    printf("Calibration: %d: %s\n", cal_profile, tfa_cont_get_profile_name(tc, cal_profile));
    printf("Revision: %x\n", tfa_get_bitfield(t, TFA98XX_BF_DEVICE_REV));

    tfa_startup(t);

    /* tfaRunStartDSP */
    pcm = tfa_power_on(t);
    tfa_cont_write_patch(tc, t);
    tfa_power_off(t, pcm);

    printf("nEnabling clocks\n");
    pcm = tfa_mi2s_enable(t);
    tfa_set_bitfield(t, TFA98XX_BF_POWERDOWN, 0);
    printf("\n\nWaiting for clocks\n");
    while (time(NULL) - start < 10 && tfa_get_bitfield(t, TFA98XX_BF_FLAG_LOST_CLK)) sleep(1);
    printf("Clocks enabled, powering up\n");
    tfa_set_bitfield(t, TFA98XX_BF_POWERDOWN, 0);
    dump_reg(0);
    while (time(NULL) - start < 10) sleep(1);

    printf("Disabling\n");
    tfa_mi2s_disable(t, pcm);

    return 0;
}
