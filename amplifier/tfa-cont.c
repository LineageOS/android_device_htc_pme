#define LOG_TAG "audio_amplifier::tfa_cont"
//#define LOG_NDEBUG 0

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "tfa-cont.h"
#include "tfa9888.h"

#include <cutils/log.h>

#define MAX_PROFILES 16
#define MAX_LIVE_DATA 16
#define MAX_REGISTERS 40

#define PACKED __attribute__((packed))

typedef struct {
    unsigned offset:24;
    unsigned type:8;
} PACKED hunk_t;

typedef struct {
    unsigned char n_hunks;              /* 0 */
    unsigned char group;
    unsigned char slave;
    unsigned char unknown2;
    unsigned name_offset;               /* 4 */
    unsigned unknown;                   /* 8 */
    hunk_t hunk[];                      /* 12 */
} PACKED tfa_cont_profile_t;

typedef struct {
    unsigned short magic;
    unsigned char  version[2];
    unsigned char  sub_version[2];
    unsigned short size;
    int            crc;
    char           customer[8];
    char           application[8];
    char           type[8];
} PACKED header_t;

typedef struct {
    header_t       hdr;
    unsigned char  unknown[4];
    unsigned short n_devices;
} PACKED device_info_file_t;

typedef struct {
    header_t       hdr;
    unsigned char  data[];
} PACKED patch_t;

struct tfa_contS {
    unsigned char *raw;
    int         n_devices;
    /* All of this should be per device, but we only have 1 */
    int         dev_id;
    tfa_cont_profile_t *profiles[MAX_PROFILES];
    int         n_profiles;
    tfa_cont_profile_t *dev_profile;
    int         cur_vstep;
};

static const char *get_string(tfa_cont_t *tc, unsigned offset)
{
    if (offset >> 24 != 3) return "Undefined string";
    return (const char *) &tc->raw[offset&0xffffff];
}

static void dump_header(header_t *hdr)
{
    ALOGV("magic    %x", hdr->magic);
    ALOGV("version  %c%c.%c%c", hdr->version[0], hdr->version[1], hdr->sub_version[0], hdr->sub_version[1]);
    ALOGV("size     %d", hdr->size);
    ALOGV("crc      %x", hdr->crc);
    ALOGV("customer %.8s", hdr->customer);
    ALOGV("appl     %.8s", hdr->application);
    ALOGV("type     %.8s", hdr->type);
}

static void dump_profile(tfa_cont_t *tc, tfa_cont_profile_t *p)
{
    ALOGV("n-hunks: %x", p->n_hunks);
    ALOGV("group: %x", p->group);
    ALOGV("slave: %u", p->slave);
    ALOGV("unknown: %x", p->unknown2);
    ALOGV("name: %s", get_string(tc, p->name_offset));
    ALOGV("hunks:");
}

static void handle_patch(tfa_cont_t *tc, unsigned char *patch_raw)
{
    tc->dev_id = patch_raw[44];
    if (patch_raw[45] == 0xff && patch_raw[46] == 0xff) {
        unsigned id = patch_raw[49] | (patch_raw[48]<<8) | (patch_raw[47]<<16);
        if (id && id != 0xffffff) {
            tc->dev_id = id;
        }
    }
    ALOGD("dev_id = %d / 0x%x", tc->dev_id, tc->dev_id);
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
        ALOGE("Invalid header magic number: %s", fname);
        goto error;
    }

    if (dev_info->n_devices != 1) {
        ALOGE("This code only supports 1 device, more development work is needed now!");
        goto error;
    }

    dump_header(&dev_info->hdr);

    tc = malloc(sizeof(*tc));
    if (tc == NULL) {
        perror("tc");
        goto error;
    }

    tc->raw = buf;
    tc->cur_vstep = -1;

    /* If more than 1 device is needed, turn this into a loop:
       tc->n_devices = dev_info->n_devices;
       for (i = 0; i < tc->n_devices; i++) {
          dev = &buf[4*(i+10)];
    */
    dev = &buf[4*(0+10)];
    tc->dev_profile = (tfa_cont_profile_t *) &buf[dev[6] | (dev[7]<<8) | (dev[8]<<16)];

    for (j = 0; j < tc->dev_profile->n_hunks; j++) {
        hunk_t *hunk = &tc->dev_profile->hunk[j];

        switch(hunk->type) {
        case 1:
            tc->profiles[tc->n_profiles] = (tfa_cont_profile_t *) &buf[hunk->offset];
            dump_profile(tc, tc->profiles[tc->n_profiles]);
            tc->n_profiles++;
            break;
        case 4:
            ALOGI("vstep file at %d", hunk->offset);
            break;
        case 5:
            handle_patch(tc, &buf[hunk->offset]);
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

typedef struct {
    unsigned char reg;
    unsigned short mask;
    unsigned short value;
} PACKED write_register_t;

static int write_register(write_register_t *wr, tfa_t *t)
{
    unsigned value;

    value = tfa_get_register(t, wr->reg);
    value = (value & ~wr->mask) | (wr->value & wr->mask);

    return tfa_set_register(t, wr->reg, value);
}

/* volume file layout is:
 * 1 byte: total # of vsteps in file
 * 3 bytes: padding
 *
 * following by that many vstep chunks
 *
 * 1 byte: # regs
 * 4 bytes per register
 * 1 byte: number of blobs
 *
 * followed by that many blob chunks
 */

typedef union {
    unsigned u:24;
    unsigned char b[3];
} cmd_union_t;

typedef struct {
    unsigned char blob_num;
    unsigned char n_payload[3]; /* in MSB byte order */
    cmd_union_t cmd;
    unsigned char data[];
} PACKED volume_file_t;

static int write_volume_blob(tfa_t *t, volume_file_t *blob, unsigned n_payload)
{
printf("CRP %s n_payload = %d blob_num %d cmd %x %x %x\n", __func__, n_payload, blob->blob_num, blob->cmd.b[0], blob->cmd.b[1], blob->cmd.b[2]);

    if (!tfa_get_bitfield(t, BF_COOLFLUX_CONFIGURED)) {
        if (!blob->blob_num) {
            if (blob->cmd.b[2]) {
                return tfa_dsp_msg_id(t, 3*(n_payload-1), blob->data, blob->cmd.u);
            } else {
                return tfa_dsp_msg(t, 3*n_payload, (unsigned char *) &blob->cmd);
            }
        } else if (blob->blob_num == 2) {
            if (blob->cmd.b[2] != 7) {
                cmd_union_t cmd = blob->cmd;
                cmd.b[2] = 7;
                return tfa_dsp_msg_id(t, 3*(n_payload-1), blob->data, cmd.u);
            } else {
                return tfa_dsp_msg(t, 3*n_payload, (unsigned char *) &blob->cmd);
            }
        }
    }

    if (blob->blob_num != 3) {
        return tfa_dsp_msg(t, 3*n_payload, (unsigned char *) &blob->cmd);
    }

    return 0;
}

static int write_volume_file(tfa_cont_t *tc, unsigned char *ptr, int vstep, int a4, tfa_t *t)
{
    int cur_step;
    int n_regs, reg;
    unsigned char *regs_ptr = 0;

    if (vstep >= *ptr) {
        ALOGE("Invalid vstep %d of %d", vstep, *ptr);
        return -EINVAL;
    }

    ALOGI("write vstep %d of a maximum of %d\n", vstep, *ptr);

    ptr += 4;
    for (cur_step = 0; cur_step <= vstep; cur_step++) {
        int blob;
        int n_regs;
        unsigned char n_blobs;

        regs_ptr = ptr;
        n_regs = *ptr++;
        ptr += 4*n_regs;
        n_blobs = *ptr++;

ALOGI("n_blobs = %d at %d", n_blobs, (int) (ptr - tc->raw));

        for (blob = 0; blob < n_blobs; blob++) {
            volume_file_t *b = (volume_file_t *) ptr;
            unsigned n_payload = (b->n_payload[0]<<16) | (b->n_payload[1] << 8) | b->n_payload[2];
            if (cur_step == vstep && (a4 >= 100 || a4 == blob)) {
                write_volume_blob(t, b, n_payload);
            }
            if (b->blob_num != 3) {
               n_payload *= 3;
            }
            ptr += n_payload + 4;
        }
    }

    n_regs = *regs_ptr++;
    for (reg = 0; reg < n_regs; reg++, regs_ptr += 4) {
        unsigned bf = (regs_ptr[0]<<8) | regs_ptr[1];
        unsigned value = regs_ptr[3];
        tfa_set_bitfield(t, bf, value);
    }

    tfa_set_swvstep(t, vstep);
    tc->cur_vstep = vstep;

    return 0;
}

static int write_file(tfa_cont_t *tc, unsigned char *ptr, int vstep, int a4, tfa_t *t)
{
    header_t *h = (header_t *) &ptr[8];
    dump_header(h);
    switch(h->magic) {
    case 0x4150: {
        patch_t *patch = (patch_t *) &ptr[8];
        return tfa_dsp_patch(t, patch->hdr.size - sizeof(patch->hdr), patch->data);
    }
    case 0x5056:
        return write_volume_file(tc, &ptr[44], vstep, a4, t);
    default:
        ALOGE("Unknown magic %x", h->magic);
        assert(0);
    }
    return 0;
}

static int write_item(tfa_cont_t *tc, hunk_t *hunk, int vstep, tfa_t *t)
{
    unsigned char *ptr = &tc->raw[hunk->offset];

    switch(hunk->type) {
    case 3: /* log string? */
    case 7: /* select mode */
    case 23: /* write filter */
    case 26: /* write dsp mem */
    default:
        assert(0);
        break;
    case 4:
    case 5:
        return write_file(tc, &tc->raw[hunk->offset], vstep, 100, t);
    case 16:
        return tfa_set_bitfield(t, ptr[2] | (ptr[3]<<8), ptr[0] | (ptr[1]<<8));
    case 2:
        return write_register((write_register_t *) ptr, t);
    }

    return 0;
}

typedef enum {
    ALL, ON, OFF
} item_mode_t;

static int write_items(tfa_cont_t *tc, tfa_cont_profile_t *profile, item_mode_t mode,
                       unsigned valid, int vstep, tfa_t *t)
{
    int found_marker = 0;
    int i;

    for (i = 0; i < profile->n_hunks; i++) {
        if (profile->hunk[i].type == 17) {
            found_marker = 1;
        } else if (mode == ALL || (mode == OFF && found_marker) || (mode == ON && !found_marker)) {
            if ((valid & (1<<profile->hunk[i].type)) != 0) {
                write_item(tc, &profile->hunk[i], vstep, t);
            }
        }
    }

    return 0;
}

int tfa_cont_write_patch(tfa_cont_t *tc, tfa_t *t)
{
printf("CRP %s %d\n", __func__, __LINE__);
    write_items(tc, tc->dev_profile, ALL, 1<<5, 0, t);
printf("CRP %s %d\n", __func__, __LINE__);
    return 0;
}

static void create_dsp_buffer_msg(unsigned char *raw, unsigned char *msg, int *len)
{
    int i;

    msg[0] = raw[3];
    msg[1] = raw[2];
    msg[2] = raw[1];

    *len = 3*(raw[0] + 1);

    for (i = 1; i < raw[0]; i++) {
        msg[i*3+0] = raw[i*4+2];
        msg[i*3+1] = raw[i*4+1];
        msg[i*3+2] = raw[i*4+0];
    }
}

int tfa_cont_write_device_files(tfa_cont_t *tc, tfa_t *t)
{
    int i;
    tfa_cont_profile_t *p = tc->dev_profile;

printf("CRP %s %d\n", __func__, __LINE__);

    for (i = 0; i < p->n_hunks; i++) {
        unsigned char *ptr = &tc->raw[p->hunk[i].offset];

        if (p->hunk[i].type == 4) {
            write_file(tc, ptr, 0, 100, t);
        } else if ((p->hunk[i].type >= 8 && p->hunk[i].type < 15) || p->hunk[i].type == 22) {
            int len;
            unsigned char msg[255];

            create_dsp_buffer_msg(ptr, msg, &len);
            tfa_dsp_msg(t, len, msg);
        } else if (p->hunk[i].type == 21) {
            int len = *(short *) ptr;
            tfa_dsp_msg(t, len, ptr+2);
        } else if (p->hunk[i].type == 26) {
            /* tfaRunWriteDspMem */
            assert(0);
        }
    }
printf("CRP %s %d\n", __func__, __LINE__);
    return 0;
}

int tfa_cont_write_profile_files(tfa_cont_t *tc, int profile_num, tfa_t *t, int vstep)
{
    int i;
    tfa_cont_profile_t *p = tc->profiles[profile_num];

printf("CRP %s %d\n", __func__, __LINE__);
    for (i = 0; i < p->n_hunks; i++) {
        unsigned char *ptr = &tc->raw[p->hunk[i].offset];

        if (p->hunk[i].type == 5) {
            patch_t *patch = (patch_t *) &ptr[8];
            tfa_dsp_patch(t, patch->hdr.size - sizeof(patch->hdr), patch->data);
        } else if (p->hunk[i].type == 4) {
            write_file(tc, ptr, vstep, 100, t);
            assert(0);
        } else if (p->hunk[i].type == 26) {
            /* tfaRunWriteDspMem */
            assert(0);
        }
    }
printf("CRP %s %d\n", __func__, __LINE__);
    return 0;
}

int tfa_cont_write_device_registers(tfa_cont_t *tc, tfa_t *t)
{
printf("CRP %s %d\n", __func__, __LINE__);
    /* TODO: Should this be stopping the registers when it hits T == 0 or 1 ? */
    write_items(tc, tc->dev_profile, ALL, (1<<2) | (1<<16), 0, t);
printf("CRP %s %d\n", __func__, __LINE__);
    return 0;
}

int tfa_cont_write_profile_registers(tfa_cont_t *tc, int profile_num, tfa_t *t)
{
printf("CRP %s %d\n", __func__, __LINE__);
    write_items(tc, tc->profiles[profile_num], ON, (1<<2) | (1<<16), 0, t);
printf("CRP %s %d\n", __func__, __LINE__);
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

int tfa_cont_write_profile(tfa_cont_t *tc, int prof_num, int vstep, tfa_t *t)
{
    int swprof_num;
    tfa_cont_profile_t *profile, *swprof;
    int new_audio_fs, old_audio_fs, audio_fs;

printf("CRP %s %d\n", __func__, __LINE__);
    profile = tc->profiles[prof_num];

    swprof_num = tfa_get_swprof(t);
    if (swprof_num < 0 || swprof_num >= tc->n_profiles) {
        ALOGE("Invalid current profile: %d", swprof_num);
        return -EINVAL;
    }

    swprof = tc->profiles[swprof_num];

    if (swprof->group == profile->group && profile->group) {
        old_audio_fs = 8;
    } else {
        old_audio_fs = tfa_get_bitfield(t, BF_AUDIO_FS);
        tfa_set_bitfield(t, BF_COOLFLUX_CONFIGURED, 0);
        tfa_power_off(t);
    }

    ALOGI("disabling profile %d", swprof_num);
    write_items(tc, swprof, OFF, UINT_MAX, vstep, t);

    ALOGI("writing profile %d", prof_num);
    // (T < 4 || T == 6 || T == 7 || T == 16 || T >= 18) {
#   define VALID_ON ((0x7) | (1<<6) | (1<<7) | (1<<16) | (UINT_MAX>>18<<18))
    write_items(tc, profile, ALL, VALID_ON, vstep, t);

    if (!swprof || swprof->group != profile->group || !profile->group) {
        tfa_set_bitfield(t, BF_SRC_SET_CONFIGURED, 1);
        tfa_power_on(t);
        tfa_set_bitfield(t, BF_COOLFLUX_CONFIGURED, 0);
    }

    write_items(tc, swprof, OFF, (1<<4) | (1<<5), vstep, t);
    write_items(tc, profile, ALL, (1<<4) | (1<<5), vstep, t);

    new_audio_fs = get_audio_fs(tc, profile);
    old_audio_fs = swprof ? get_audio_fs(tc, swprof) : -1;
    audio_fs = tfa_get_bitfield(t, BF_AUDIO_FS);

    ALOGV("new_audio_fs %d old_audio_fs %d audio_fs %d", new_audio_fs, old_audio_fs, audio_fs);

    if (new_audio_fs != old_audio_fs || new_audio_fs != audio_fs) {
        tfa_dsp_write_tables(t, new_audio_fs);
    }

printf("CRP %s %d\n", __func__, __LINE__);
    return 0;
}

int tfa_cont_get_current_vstep(tfa_cont_t *t)
{
    return t->cur_vstep;
}

int tfa_cont_write_file_vstep(tfa_cont_t *tc, int profile_num, int vstep, tfa_t *t)
{
    tfa_cont_profile_t *profile;
    int i;

printf("CRP %s:%d profile %d vstep %d\n", __func__, __LINE__, profile_num, vstep);

    profile = tc->profiles[profile_num];
    for (i = 0; i < profile->n_hunks; i++) {
        if (profile->hunk[i].type == 4) {
            unsigned char *ptr = &tc->raw[profile->hunk[i].offset];
            header_t *hdr = (header_t *) &ptr[8];
            if (hdr->magic == 0x5056) {
                write_volume_file(tc, &ptr[44], vstep, 100, t);
            }
        }
    }

printf("CRP: %s:%d\n", __func__, __LINE__);
    return 0;
}

