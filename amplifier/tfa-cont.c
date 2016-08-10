#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "tfa-cont.h"

#define MAX_PROFILES 16
#define MAX_LIVE_DATA 16

typedef struct {
    unsigned char n_hunks;              /* 0 */
    unsigned char unknown1;
    unsigned char slave;
    unsigned char unknown2;
    unsigned name_offset;               /* 4 */
    unsigned unknown;                   /* 8 */
    unsigned hunk_offsets[];            /* 12 */
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
printf("unknown: %x\n", p->unknown1);
    printf("slave: %u\n", p->slave);
printf("unknown: %x\n", p->unknown2);
    printf("name: %s\n", get_string(tc, p->name_offset));
    printf("hunks:");
    for (i = 0; i < p->n_hunks; i++) {
        printf(" %x[%x]", p->hunk_offsets[i]>>24, p->hunk_offsets[i] & 0xffffff);
    }
    printf("\n");
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
            if (dev_data[3] == 1) {
                tc->profiles[n_profiles] = (tfa_cont_profile_t *) &buf[offset];
                printf("Profile %d at %d\n", n_profiles, offset);
                dump_profile(tc, tc->profiles[n_profiles]);
                n_profiles++;
            } else if (dev_data[3] == 18) {
                tc->live_data[n_live_data] = offset;
                printf("Live data %d at %d\n", n_live_data, offset);
                n_live_data++;
            } else if (dev_data[3] == 5) {
                unsigned char *dev_info = &buf[offset];
printf("Got patch at %d %p\n", offset, dev_info);
                tc->dev_id = dev_info[44];
                if (dev_info[45] == 0xff && dev_info[46] == 0xff) {
                    unsigned id = dev_info[49] | (dev_info[48]<<8) | (dev_info[47]<<16);
                    if (id && id != 0xffffff) {
                        tc->dev_id = id;
printf("Dev_id = %d\n", id);
                    }
                }
                tc->patch_len = *(short *) &dev_info[14] - 36;
                tc->patch = dev_info + 44;
                printf("Device info at %d has dev_id %d\n", offset, tc->dev_id);
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
