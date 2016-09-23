#include <dlfcn.h>

#define LOG_TAG "acdb_wrapper"
#include "cutils/log.h"

/* Audio calibration related functions */
typedef void (*acdb_deallocate_t)();
typedef int  (*acdb_init_t)(const char *, char *, int);
typedef void (*acdb_send_audio_cal_t)(int, int, int , int);
typedef void (*acdb_send_voice_cal_t)(int, int);
typedef int (*acdb_reload_vocvoltable_t)(int);
typedef int  (*acdb_get_default_app_type_t)(void);
typedef int (*acdb_loader_get_calibration_t)(char *attr, int size, void *data);
typedef int (*acdb_set_audio_cal_t) (void *, void *, uint32_t);
typedef int (*acdb_get_audio_cal_t) (void *, void *, uint32_t*);
typedef int (*acdb_send_common_top_t) (void);
typedef int (*acdb_set_codec_data_t) (void *, char *);

static void *acdb_handle;
static acdb_init_t                f_acdb_init;
static acdb_deallocate_t          f_acdb_deallocate;
static acdb_send_audio_cal_t      f_acdb_send_audio_cal;
static acdb_set_audio_cal_t       f_acdb_set_audio_cal;
static acdb_get_audio_cal_t       f_acdb_get_audio_cal;
static acdb_send_voice_cal_t      f_acdb_send_voice_cal;
static acdb_reload_vocvoltable_t  f_acdb_reload_vocvoltable;
static acdb_get_default_app_type_t f_acdb_get_default_app_type;
static acdb_send_common_top_t     f_acdb_send_common_top;
static acdb_set_codec_data_t      f_acdb_set_codec_data;
static acdb_loader_get_calibration_t f_acdb_loader_get_calibration;

#define LIB_ACDB_LOADER "libacdbloader_vendor.so"

int acdb_loader_init_v2(const char *a, char *b, int c)
{
    acdb_handle = dlopen(LIB_ACDB_LOADER, RTLD_NOW);
    if (acdb_handle == NULL) {
        ALOGE("%s: DLOPEN failed for %s", __func__, LIB_ACDB_LOADER);
        return -1;
    } else {
        f_acdb_deallocate = (acdb_deallocate_t)dlsym(acdb_handle,
                                                    "acdb_loader_deallocate_ACDB");
        f_acdb_send_audio_cal = (acdb_send_audio_cal_t)dlsym(acdb_handle,
                                                    "acdb_loader_send_audio_cal_v2");
        f_acdb_set_audio_cal = (acdb_set_audio_cal_t)dlsym(acdb_handle,
                                                    "acdb_loader_set_audio_cal_v2");
        f_acdb_get_audio_cal = (acdb_get_audio_cal_t)dlsym(acdb_handle,
                                                    "acdb_loader_get_audio_cal_v2");
        f_acdb_send_voice_cal = (acdb_send_voice_cal_t)dlsym(acdb_handle,
                                                    "acdb_loader_send_voice_cal");
        f_acdb_reload_vocvoltable = (acdb_reload_vocvoltable_t)dlsym(acdb_handle,
                                                    "acdb_loader_reload_vocvoltable");
        f_acdb_get_default_app_type = (acdb_get_default_app_type_t)dlsym(
                                                    acdb_handle,
                                                    "acdb_loader_get_default_app_type");
        f_acdb_send_common_top = (acdb_send_common_top_t)dlsym(
                                                    acdb_handle,
                                                    "acdb_loader_send_common_custom_topology");
        f_acdb_set_codec_data = (acdb_set_codec_data_t)dlsym(
                                                    acdb_handle,
                                                    "acdb_loader_set_codec_data");
        f_acdb_init = (acdb_init_t)dlsym(acdb_handle,
                                                    "acdb_loader_init_v2");
        f_acdb_loader_get_calibration = (acdb_loader_get_calibration_t)
                                                dlsym(acdb_handle, "acdb_loader_get_calibration");
        ALOGI("%s: %s %s %d", __func__, a, b, c);
        return f_acdb_init(a, b, c);
    }
}

void acdb_loader_deallocate_ACDB(void) {
    ALOGI("%s", __func__);
    f_acdb_deallocate();
}

void acdb_loader_send_audio_cal_v2(int a, int b, int c, int d) {
    ALOGI("%s: %d %d %d %d", __func__, a, b, c, d);
    f_acdb_send_audio_cal(a, b, c, d);
}

void acdb_loader_send_voice_cal(int a, int b) {
    ALOGI("%s: %d %d", __func__, a, b);
    f_acdb_send_voice_cal(a, b);
}

int acdb_loader_reload_vocvoltable(int a) {
    int ret = f_acdb_reload_vocvoltable(a);
    ALOGI("%s: %d => %d", __func__, a, ret);
    return ret;
}

int acdb_loader_get_default_app_type(void) {
    int ret = f_acdb_get_default_app_type();
    ALOGI("%s: => %d", __func__, ret);
    return ret;
}

int acdb_loader_get_calibration(char *a, int b, void *c) {
    int ret = f_acdb_loader_get_calibration(a, b, c);
    ALOGI("%s: %s %d %p => %d", __func__, a, b, c, ret);
    return ret;
}

int acdb_loader_set_audio_cal_v2(void *a, void *b, uint32_t c) {
    int ret = f_acdb_set_audio_cal(a, b, c);
    ALOGI("%s: %p %p %u => %d", __func__, a, b, c, ret);
    return ret;
}

int acdb_loader_get_audio_cal_v2(void *a, void *b, uint32_t *c) {
    int ret = f_acdb_get_audio_cal(a, b, c);
    ALOGI("%s: %p %p %p => %d", __func__, a, b, c, ret);
    return ret;
}

int acdb_send_common_top(void) {
    int ret = f_acdb_send_common_top();
    ALOGI("%s: => %d", __func__, ret);
    return ret;
}

int acdb_set_codec_data(void *a, char *b) {
    int ret = f_acdb_set_codec_data(a, b);
    ALOGI("%s: %p %s => %d\n", __func__, a, b, ret);
    return ret;
}

