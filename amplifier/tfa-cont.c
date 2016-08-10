#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
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
    tfa_cont_profile_t *dev_profile;
    unsigned char *patch;
    int         patch_len;
};

typedef struct {
    unsigned short magic;
    unsigned char  version[2];
    unsigned char  sub_version[2];
    unsigned short size;
    int            crc;
    char           customer[8];
    char           application[8];
    char           type[8];
} header_t;

typedef struct {
    header_t       hdr;
    unsigned char  unknown[4];
    unsigned short n_devices;
} device_info_file_t;

typedef struct {
    header_t       hdr;
    unsigned char  data[];
} patch_t;

static const char *get_string(tfa_cont_t *tc, unsigned offset)
{
    if (offset >> 24 != 3) return "Undefined string";
    return (const char *) &tc->raw[offset&0xffffff];
}

static void dump_header(header_t *hdr)
{
    printf("magic    %x\n", hdr->magic);
    printf("version  %c%c.%c%c\n", hdr->version[0], hdr->version[1], hdr->sub_version[0], hdr->sub_version[1]);
    printf("size     %d\n", hdr->size);
    printf("crc      %x\n", hdr->crc);
    printf("customer %.8s\n", hdr->customer);
    printf("appl     %.8s\n", hdr->application);
    printf("type     %.8s\n", hdr->type);
}

static void dump_hunks(hunk_t *hunks, int n_hunks)
{
    int i;

    for (i = 0; i < n_hunks; i++) {
        printf(" %d[%d]", hunks[i].type, hunks[i].offset);
    }
}

static void dump_profile(tfa_cont_t *tc, tfa_cont_profile_t *p)
{
    printf("n-hunks: %x\n", p->n_hunks);
    printf("group: %x\n", p->group);
    printf("slave: %u\n", p->slave);
printf("unknown: %x\n", p->unknown2);
    printf("name: %s\n", get_string(tc, p->name_offset));
    printf("hunks:");
    dump_hunks(p->hunk, p->n_hunks);
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
    device_info_file_t *dev_info;
    unsigned char *dev;
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
    dev_info = (device_info_file_t *) buf;

    if (dev_info->hdr.magic != 0x4d50) {
        fprintf(stderr, "Invalid header magic number: %s\n", fname);
        goto error;
    }

    if (dev_info->n_devices != 1) {
        fprintf(stderr, "This code only supports 1 device, do more work.\n");
        goto error;
    }

    dump_header(&dev_info->hdr);

    tc = malloc(sizeof(*tc));
    if (tc == NULL) {
        perror("tc");
        goto error;
    }

    tc->raw = buf;

    /* If more than 1 device is needed, turn this into a loop:
       tc->n_devices = dev_info->n_devices;
       for (i = 0; i < tc->n_devices; i++) {
          dev = &buf[4*(i+10)];
    */
    dev = &buf[4*(0+10)];
    tc->dev_profile = (tfa_cont_profile_t *) &buf[dev[6] | (dev[7]<<8) | (dev[8]<<16)];

    printf("device hunks: ");
    dump_hunks(tc->dev_profile->hunk, tc->dev_profile->n_hunks);
    printf("\n");

    for (j = 0; j < tc->dev_profile->n_hunks; j++) {
        hunk_t *hunk = &tc->dev_profile->hunk[j];

        switch(hunk->type) {
        case 1:
            tc->profiles[tc->n_profiles] = (tfa_cont_profile_t *) &buf[hunk->offset];
            printf("Profile %d at %d\n", tc->n_profiles, hunk->offset);
            dump_profile(tc, tc->profiles[tc->n_profiles]);
            tc->n_profiles++;
            break;
        case 5:
            handle_dev_info(tc, &buf[hunk->offset]);
            break;
        }
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

typedef struct {
    unsigned char reg;
    unsigned short mask;
    unsigned short value;
} write_register_t;

static int write_register(write_register_t *wr, tfa_t *t)
{
    unsigned value;

    value = tfa_get_register(t, wr->reg);
    value = (value & ~wr->mask) | (wr->value & wr->mask);

    return tfa_set_register(t, wr->reg, value);
}

static int write_item(tfa_cont_t *tc, hunk_t *hunk, tfa_t *t)
{
    unsigned char *ptr = &tc->raw[hunk->offset];

    switch(hunk->type) {
    case 7: assert(0); /* select_mode */ ; break;
    case 23: assert(0); /* write filter */ ; break;
    case 26: assert(0); /* write dsp mem */; break;
    case 16: return tfa_set_bitfield(t, ptr[2] | (ptr[3]<<8), ptr[0] | (ptr[1]<<8));
    case 2: return write_register((write_register_t *) ptr, t);
    case 3: assert(0); /* log string? */; break;
    default: assert(0);
    }

    return 0;
}

static int write_file(tfa_cont_t *tc, unsigned char *ptr, int a3, int a4)
{
    header_t *h = (header_t *) &ptr[8];
    dump_header(h);
tc; a3; a4;
return 0;
}

int tfa_cont_write_device_files(tfa_cont_t *tc, tfa_t *t)
{
    int i;
    tfa_cont_profile_t *p = tc->dev_profile;

    for (i = 0; i < p->n_hunks; i++) {
        unsigned char *ptr = &tc->raw[p->hunk[i].offset];

        if (p->hunk[i].type == 4) {
            write_file(tc, ptr, 0, 100);
        } else if ((p->hunk[i].type >= 8 && p->hunk[i].type < 15) || p->hunk[i].type == 22) {
            int len;
            unsigned char msg[255];

            //create_dsp_buffer_msg(ptr, msg, &len);
len = 0;
            tfa_dsp_msg(t, len, msg);
        } else if (p->hunk[i].type == 21) {
            int len = *(short *) ptr;
            tfa_dsp_msg(t, len, ptr+2);
        } else if (p->hunk[i].type == 26) {
            /* tfaRunWriteDspMem */
            assert(0);
        }
    }
    return 0;
}

int tfa_cont_write_profile_files(tfa_cont_t *tc, int profile_num, tfa_t *t)
{
    int i;
    tfa_cont_profile_t *p = tc->profiles[profile_num];

    for (i = 0; i < p->n_hunks; i++) {
        unsigned char *ptr = &tc->raw[p->hunk[i].offset];

        if (p->hunk[i].type == 5) {
            patch_t *patch = (patch_t *) &ptr[8];
            tfa_dsp_patch(t, patch->hdr.size - sizeof(patch->hdr), patch->data);
            write_file(tc, ptr, 0, 100);
        } else if (p->hunk[i].type == 4) {
            /* tfaContWriteFile */
            assert(0);
        } else if (p->hunk[i].type == 26) {
            /* tfaRunWriteDspMem */
            assert(0);
        }
    }
    return 0;
}

int tfa_cont_write_device_registers(tfa_cont_t *tc, tfa_t *t)
{
    int i;
    tfa_cont_profile_t *p = tc->dev_profile;

    for (i = 0; i < p->n_hunks; i++) {
        switch(p->hunk[i].type) {
        case 2:
        case 16:
            write_item(tc, &p->hunk[i], t);
            break;
        case 0:
        case 1:
            return 0;
        }
    }
    return 0;
}

int tfa_cont_write_profile_registers(tfa_cont_t *tc, int profile_num, tfa_t *t)
{
    int i;
    tfa_cont_profile_t *profile = tc->profiles[profile_num];

    for (i = 0; i < profile->n_hunks && profile->hunk[i].type != 17; i++) {
        if (profile->hunk[i].type == 2 || profile->hunk[i].type == 16) {
            write_item(tc, &profile->hunk[i], t);
        }
    }
    return 0;
}

static int get_audio_fs(tfa_cont_t *tc, tfa_cont_profile_t *prof)
{
    int i;

    for (i = 0; i < prof->n_hunks; i++) {
        unsigned char *ptr = (unsigned char *) &tc->raw[prof->hunk[i].offset];

        if (prof->hunk[i].type == 17 && *(short *) &ptr[1] == 0x203) {
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

    profile = tc->profiles[prof_num];

    swprof_num = tfa_get_swprof(t);
    if (swprof_num < 0 || swprof_num >= tc->n_profiles) {
        printf("Invalid current profile: %d\n", swprof_num);
        swprof = NULL;
    } else {
        swprof = tc->profiles[swprof_num];
    }

    if (swprof && swprof->group == profile->group && profile->group) {
        old_audio_fs = 8;
    } else {
        old_audio_fs = tfa_get_bitfield(t, BF_AUDIO_FS);
        tfa_set_bitfield(t, BF_COOLFLUX_CONFIGURED, 0);
        tfa_power_off(t);
    }

    if (swprof) {
        printf("Disabling profile %d\n", swprof_num);
        int found_marker = 0;

        for (i = 0; i < swprof->n_hunks; i++) {
            if (swprof->hunk[i].type == 17) {
                found_marker = 1;
            } else if (found_marker) {
                write_item(tc, &swprof->hunk[i], t);
            }
        }
    }

    printf("Writing profile %d\n", prof_num);

    for (i = 0; i < profile->n_hunks; i++) {
        unsigned T = profile->hunk[i].type;
        if (T < 4 || T == 6 || T == 7 || T == 16 || T >= 18) {
            write_item(tc, &profile->hunk[i], t);
        }
    }

    if (!swprof || swprof->group != profile->group || !profile->group) {
        tfa_set_bitfield(t, BF_SRC_SET_CONFIGURED, 1);
        tfa_power_on(t);
        tfa_set_bitfield(t, BF_COOLFLUX_CONFIGURED, 0);
    }

    new_audio_fs = get_audio_fs(tc, profile);
    old_audio_fs = swprof ? get_audio_fs(tc, swprof) : -1;
    audio_fs = tfa_get_bitfield(t, BF_AUDIO_FS);

printf("new_audio_fs %d old_audio_fs %d audio_fs %d\n", new_audio_fs, old_audio_fs, audio_fs);

    if (new_audio_fs != old_audio_fs || new_audio_fs != audio_fs) {
        tfa_dsp_write_tables(t, new_audio_fs);
    }

    return 0;
}
