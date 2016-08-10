#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "tfa-cont.h"
#include "tfa9888.h"

#define MAX_PROFILES 16
#define MAX_LIVE_DATA 16
#define MAX_REGISTERS 40

typedef struct {
    unsigned offset:24;
    unsigned type:8;
} hunk_t;

typedef struct {
    unsigned char n_hunks;              /* 0 */
    unsigned char group;
    unsigned char slave;
    unsigned char unknown2;
    unsigned name_offset;               /* 4 */
    unsigned unknown;                   /* 8 */
    hunk_t hunk[];                      /* 12 */
} tfa_cont_profile_t;

struct tfa_contS {
    unsigned char *raw;
    int         n_devices;
    /* All of this should be per device, but we only have 1 */
    int         dev_id;
    tfa_cont_profile_t *profiles[MAX_PROFILES];
    int         n_profiles;
    int         live_data[MAX_LIVE_DATA];
    int         n_live_data;
    unsigned       patch_len;
    unsigned char *patch;
    hunk_t         registers[MAX_REGISTERS];
    int            n_registers;
};

typedef struct {
    unsigned short magic;
    unsigned char  version[2];
    unsigned char  sub_version[2];
    unsigned char  unknown2[34];
    unsigned short n_devices;
} header_t;

static const char *get_string(tfa_cont_t *tc, unsigned offset)
{
    if (offset >> 24 != 3) return "Undefined string";
    return (const char *) &tc->raw[offset&0xffffff];
}

static void dump_profile(tfa_cont_t *tc, tfa_cont_profile_t *p)
{
    int i;

    printf("n-hunks: %x\n", p->n_hunks);
    printf("group: %x\n", p->group);
    printf("slave: %u\n", p->slave);
printf("unknown: %x\n", p->unknown2);
    printf("name: %s\n", get_string(tc, p->name_offset));
    printf("hunks:");
    for (i = 0; i < p->n_hunks; i++) {
        printf(" %d[%d]", p->hunk[i].type, p->hunk[i].offset);
    }
    printf("\n\n");
}

static void handle_dev_info(tfa_cont_t *tc, unsigned char *dev_info)
{
    tc->dev_id = dev_info[44];
    if (dev_info[45] == 0xff && dev_info[46] == 0xff) {
        unsigned id = dev_info[49] | (dev_info[48]<<8) | (dev_info[47]<<16);
        if (id && id != 0xffffff) {
            tc->dev_id = id;
        }
    }
    tc->patch_len = *(short *) &dev_info[14] - 36;
    tc->patch = dev_info + 44;
    printf("Device info at %ld has dev_id %d\n", tc->raw - dev_info, tc->dev_id);
}

tfa_cont_t *tfa_cont_new(const char *fname)
{
    FILE *f = NULL;
    tfa_cont_t *tc = NULL;
    header_t *hdr;
    unsigned char *buf = NULL;
    size_t n_buf;
    struct stat stat_buf;
    int j;

    if (stat(fname, &stat_buf) < 0) {
        perror(fname);
        return NULL;
    }

    n_buf = stat_buf.st_size;

    if ((f = fopen(fname, "r")) == 0) {
        perror(fname);
        goto error;
    }

    buf = calloc(n_buf, 1);
    if (buf == NULL) {
        perror("buf");
        goto error;
    }

    if (fread(buf, 1, n_buf, f) != n_buf) {
        perror("fread");
        goto error;
    }
    hdr = (header_t *) buf;

    if (hdr->magic != 0x4d50) {
        fprintf(stderr, "Invalid magic number: %s\n", fname);
        goto error;
    }

    if (hdr->n_devices != 1) {
        fprintf(stderr, "This code only supports 1 device, do more work.\n");
        goto error;
    }

    printf("Version %c%c.%c%c\n", hdr->version[0], hdr->version[1], hdr->sub_version[0], hdr->sub_version[1]);

    tc = malloc(sizeof(*tc));
    if (tc == NULL) {
        perror("tc");
        goto error;
    }

    tc->raw = buf;

    /* If more than 1 device is needed, turn this into a loop:
       tc->n_devices = hdr->n_devices;
       for (i = 0; i < tc->n_devices; i++) {
        unsigned char *dev = &buf[4*(i+10)];
    */
    {
        unsigned char *dev = &buf[4*(0+10)];
        unsigned char *dev_data_list = &buf[dev[6] | (dev[7]<<8) | (dev[8]<<16)];
        unsigned char n_dev_data_list = *dev_data_list;
        int n_profiles = 0, n_live_data = 0;

printf("Profile list at offset %ld has %u\n", dev_data_list - buf, n_dev_data_list);
        for (j = 0; j < n_dev_data_list; j++) {
            unsigned char *dev_data = &dev_data_list[4*(j+3)];
            unsigned offset = dev_data[0] | (dev_data[1]<<8) | (dev_data[2]<<16);
            switch(dev_data[3]) {
            case 1:
                tc->profiles[n_profiles] = (tfa_cont_profile_t *) &buf[offset];
                printf("Profile %d at %d\n", n_profiles, offset);
                dump_profile(tc, tc->profiles[n_profiles]);
                n_profiles++;
                break;
            case 5:
                handle_dev_info(tc, &buf[offset]);
                break;
            case 16:
                tc->registers[tc->n_registers].offset = offset;
                tc->registers[tc->n_registers].type   = dev_data[3];
                tc->n_registers++;
                break;
            case 18:
                tc->live_data[n_live_data] = offset;
                printf("Live data %d at %d\n", n_live_data, offset);
                n_live_data++;
                break;
            }
        }
        tc->n_profiles = n_profiles;
        tc->n_live_data = n_live_data;
    }

    return tc;

error:
    if (f) fclose(f);
    if (tc) free(tc);
    if (buf) free(buf);

    return NULL;
}

void tfa_cont_destroy(tfa_cont_t *tc)
{
    free(tc);
}

/* Note: All these should have a device number but we only have 1 device so I'm lazy */

int tfa_cont_get_cal_profile(tfa_cont_t *tc)
{
     int i;

     for (i = 0; i < tc->n_profiles; i++) {
        const char *name = get_string(tc, tc->profiles[i]->name_offset);
        if (strstr(name, ".cal")) {
            return i;
        }
     }
     return -1;
}

const char *tfa_cont_get_profile_name(tfa_cont_t *tc, int profile)
{
    if (profile < 0 || profile > tc->n_profiles) return NULL;
    return get_string(tc, tc->profiles[profile]->name_offset);
}

int tfa_cont_write_patch(tfa_cont_t *tc, tfa_t *t)
{
    if (tc->patch) {
        tfa_dsp_patch(t, tc->patch_len, tc->patch);
    }

    return 0;
}

static int write_item(tfa_cont_t *tc, hunk_t *hunk, tfa_t *t)
{
    unsigned char *ptr = &tc->raw[hunk->offset];

    switch(hunk->type) {
    case 7: /* select_mode */ ; break;
    case 23: /* write filter */ ; break;
    case 26: /* write dsp mem */; break;
    case 16: tfa_set_bitfield(t, ptr[2] | (ptr[3]<<8), ptr[0] | (ptr[1]<<8)); break;
    case 2: /* write register */ ; break;
    case 3: /* log string? */; break;
    }

    return 0;
}

int tfa_cont_write_device_registers(tfa_cont_t *tc, tfa_t *t)
{
    int i;

    for (i = 0; i < tc->n_registers; i++) {
        write_item(tc, &tc->registers[i], t);
    }
    return 0;
}

int tfa_cont_write_profile_registers(tfa_cont_t *tc, int profile_num, tfa_t *t)
{
    int i;
    tfa_cont_profile_t *profile = tc->profiles[profile_num];

    for (i = 0; i < profile->n_hunks && profile->hunk[i].type != 17; i++) {
        if (profile->hunk[i].type == 16) {
            write_item(tc, &profile->hunk[i], t);
        }
    }
    return 0;
}

static int get_audio_fs(tfa_cont_t *tc, tfa_cont_profile_t *prof)
{
    int i;

    for (i = 0; i < prof->n_hunks; i++) {
        int *ptr = (int *) &tc->raw[prof->hunk[i].offset];

        if (prof->hunk[i].type == 17 && ptr[1] == 0x203) {
            return ptr[0];
        }
    }
    return 8;
}

int tfa_cont_write_profile(tfa_cont_t *tc, int prof_num, tfa_t *t)
{
    int swprof_num;
    tfa_cont_profile_t *profile, *swprof;
    int i;
    int new_audio_fs, old_audio_fs, audio_fs;

    swprof_num = tfa_get_swprof(t);
    if (swprof_num < 0 || swprof_num >= tc->n_profiles) {
        printf("Invalid current profile: %d\n", swprof_num);
        return -EINVAL;
    }

    profile = tc->profiles[prof_num];
    swprof = tc->profiles[swprof_num];

    if (swprof->group != profile->group || !profile->group) {
        tfa_set_bitfield(t, TFA98XX_BF_COOLFLUX_CONFIGURED, 0);
        tfa_power_on(t);
    }

    printf("Disabling profile %d\n", swprof_num);

    for (i = 0; i < swprof->n_hunks; i++) {
        if (swprof->hunk[i].type == 17) {
            write_item(tc, &swprof->hunk[i], t);
        }
    }

    printf("Writing profile %d\n", prof_num);

    for (i = 0; i < profile->n_hunks; i++) {
        unsigned t_sub_4 = profile->hunk[i].type - 4;
        if (t_sub_4 > 19 || !((1 << t_sub_4) & 0xe2ff3)) {
            write_item(tc, &profile->hunk[i], t);
        }
    }

    if (swprof->group != profile->group || !profile->group) {
        tfa_set_bitfield(t, TFA98XX_BF_SRC_SET_CONFIGURED, 1);
        tfa_power_on(t);
        tfa_set_bitfield(t, TFA98XX_BF_COOLFLUX_CONFIGURED, 0);
    }

    new_audio_fs = get_audio_fs(tc, profile);
    old_audio_fs = get_audio_fs(tc, swprof);
    audio_fs = tfa_get_bitfield(t, TFA98XX_BF_AUDIO_FS);

printf("new_audio_fs %d old_audio_fs %d audio_fs %d\n", new_audio_fs, old_audio_fs, audio_fs);

    if (new_audio_fs != old_audio_fs || new_audio_fs != audio_fs) {
        tfa_dsp_write_tables(t, new_audio_fs);
    }

    return 0;
}
