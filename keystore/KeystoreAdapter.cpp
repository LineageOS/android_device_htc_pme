/*
 * Copyright (C) 2015, The CyanogenMod Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// #define LOG_NDEBUG 0

#define LOG_TAG "KeystoreAdapter"
#include <cutils/log.h>

#include <utils/threads.h>
#include <utils/String8.h>
#include <hardware/keymaster1.h>

FILE *f;

typedef struct {
    struct keymaster1_device base;
    union {
        struct keymaster1_device *device;
        hw_device_t *hw_device;
    } vendor;
} device_t;

static android::Mutex vendor_mutex;

static const hw_module_t *vendor;

#define PRE_CALL do { \
    device->vendor.device->client_version = device->base.client_version; \
    device->vendor.device->flags = device->base.flags; \
    device->vendor.device->context = device->base.context; \
} while(0)

#define POST_CALL do { \
    device->base.client_version = device->vendor.device->client_version; \
    device->base.flags = device->vendor.device->flags; \
    device->base.context = device->vendor.device->context; \
    fprintf(f, "\nPOST_CALL: %x, %x, %p", device->base.client_version, device->base.flags, device->base.context); fflush(f); \
} while(0)

#define CALL(x) do { PRE_CALL; auto __rv__ = x; POST_CALL; return __rv__; } while(0)

static bool try_hal(const char *class_name, const char *device_name)
{
    const hw_module_t *module;

    int rv = hw_get_module_by_class("keystore", class_name, &module);
    if (rv) {
        ALOGE("Failed to open keystore module: class %s, error %d", class_name, rv);
        vendor = NULL;
    } else {
        hw_device_t *device;

        fprintf(f, "\nloaded keystore module, class %s: %s version %x", class_name, module->name, module->module_api_version); fflush(f); fflush(f);
        if (module->methods->open(module, device_name, &device) == 0) {
             device->close(device);
             fprintf(f, "\nSuccessfully loaded the device for keystore module, class %s", class_name); fflush(f);
             vendor = module;
        } else {
             ALOGE("Failed to open a device using keystore HAL, class %s", class_name);
        }
    }
    return vendor != NULL;
}

static bool ensure_vendor_module_is_loaded(const char *device_name)
{
    android::Mutex::Autolock lock(vendor_mutex);

    if (!vendor) {
        if (!try_hal("vendor", device_name)) {
            try_hal("vendor6", device_name);
        }
    }

    return vendor != NULL;
}

static int generate_keypair(const struct keymaster1_device* dev, const keymaster_keypair_t key_type,
                        const void* key_params, uint8_t** key_blob, size_t* key_blob_length)
{
    device_t *device = (device_t *) dev;

    fprintf(f, "\n%s", __func__); fflush(f);

    CALL(device->vendor.device->generate_keypair(device->vendor.device, key_type, key_params,  key_blob, key_blob_length));
}

static int import_keypair(const struct keymaster1_device* dev, const uint8_t* key,
                      const size_t key_length, uint8_t** key_blob, size_t* key_blob_length)
{
    device_t *device = (device_t *) dev;

    fprintf(f, "\n%s", __func__); fflush(f);

    CALL(device->vendor.device->import_keypair(device->vendor.device, key, key_length, key_blob, key_blob_length));
}

static int get_keypair_public(const struct keymaster1_device* dev, const uint8_t* key_blob,
                          const size_t key_blob_length, uint8_t** x509_data,
                          size_t* x509_data_length)
{
    device_t *device = (device_t *) dev;

    fprintf(f, "\n%s", __func__); fflush(f);

    CALL(device->vendor.device->get_keypair_public(device->vendor.device, key_blob, key_blob_length, x509_data, x509_data_length));
}

static int delete_keypair(const struct keymaster1_device* dev, const uint8_t* key_blob,
                      const size_t key_blob_length)
{
    device_t *device = (device_t *) dev;

    fprintf(f, "\n%s", __func__); fflush(f);

    CALL(device->vendor.device->delete_keypair(device->vendor.device, key_blob, key_blob_length));
}

static int delete_all(const struct keymaster1_device* dev)
{
    device_t *device = (device_t *) dev;

    fprintf(f, "\n%s", __func__); fflush(f);

    CALL(device->vendor.device->delete_all(device->vendor.device));
}

static int sign_data(const struct keymaster1_device* dev, const void* signing_params,
                 const uint8_t* key_blob, const size_t key_blob_length, const uint8_t* data,
                 const size_t data_length, uint8_t** signed_data, size_t* signed_data_length)
{
    device_t *device = (device_t *) dev;

    fprintf(f, "\n%s", __func__); fflush(f);

    CALL(device->vendor.device->sign_data(device->vendor.device, signing_params, key_blob, key_blob_length, data, data_length, signed_data, signed_data_length));
}

static int verify_data(const struct keymaster1_device* dev, const void* signing_params,
                   const uint8_t* key_blob, const size_t key_blob_length,
                   const uint8_t* signed_data, const size_t signed_data_length,
                   const uint8_t* signature, const size_t signature_length)
{
    device_t *device = (device_t *) dev;

    fprintf(f, "\n%s", __func__); fflush(f);

    CALL(device->vendor.device->verify_data(device->vendor.device, signing_params, key_blob, key_blob_length, signed_data, signed_data_length, signature, signature_length));
}

static keymaster_error_t get_supported_algorithms(const struct keymaster1_device* dev,
                                              keymaster_algorithm_t** algorithms,
                                              size_t* algorithms_length)
{
    device_t *device = (device_t *) dev;

    fprintf(f, "\n%s", __func__); fflush(f);

    CALL(device->vendor.device->get_supported_algorithms(device->vendor.device, algorithms, algorithms_length));
}

static keymaster_error_t get_supported_block_modes(const struct keymaster1_device* dev,
                                               keymaster_algorithm_t algorithm,
                                               keymaster_purpose_t purpose,
                                               keymaster_block_mode_t** modes,
                                               size_t* modes_length)
{
    device_t *device = (device_t *) dev;

    fprintf(f, "\n%s", __func__); fflush(f);

    CALL(device->vendor.device->get_supported_block_modes(device->vendor.device, algorithm, purpose, modes, modes_length));
}

static keymaster_error_t get_supported_padding_modes(const struct keymaster1_device* dev,
                                                 keymaster_algorithm_t algorithm,
                                                 keymaster_purpose_t purpose,
                                                 keymaster_padding_t** modes,
                                                 size_t* modes_length)
{
    device_t *device = (device_t *) dev;

    fprintf(f, "\n%s", __func__); fflush(f);

    CALL(device->vendor.device->get_supported_padding_modes(device->vendor.device, algorithm, purpose, modes, modes_length));
}

static keymaster_error_t get_supported_digests(const struct keymaster1_device* dev,
                                           keymaster_algorithm_t algorithm,
                                           keymaster_purpose_t purpose,
                                           keymaster_digest_t** digests,
                                           size_t* digests_length)
{
    device_t *device = (device_t *) dev;

    fprintf(f, "\n%s", __func__); fflush(f);

    CALL(device->vendor.device->get_supported_digests(device->vendor.device, algorithm, purpose, digests, digests_length));
}

static keymaster_error_t get_supported_import_formats(const struct keymaster1_device* dev,
                                                  keymaster_algorithm_t algorithm,
                                                  keymaster_key_format_t** formats,
                                                  size_t* formats_length)
{
    device_t *device = (device_t *) dev;

    fprintf(f, "\n%s", __func__); fflush(f);

    CALL(device->vendor.device->get_supported_import_formats(device->vendor.device, algorithm, formats, formats_length));
}

static keymaster_error_t get_supported_export_formats(const struct keymaster1_device* dev,
                                                  keymaster_algorithm_t algorithm,
                                                  keymaster_key_format_t** formats,
                                                  size_t* formats_length)
{
    device_t *device = (device_t *) dev;

    fprintf(f, "\n%s", __func__); fflush(f);

    CALL(device->vendor.device->get_supported_export_formats(device->vendor.device, algorithm, formats, formats_length));
}

static keymaster_error_t add_rng_entropy(const struct keymaster1_device* dev, const uint8_t* data,
                                     size_t data_length)
{
    device_t *device = (device_t *) dev;

    fprintf(f, "\n%s", __func__); fflush(f);

    CALL(device->vendor.device->add_rng_entropy(device->vendor.device, data, data_length));
}

static keymaster_error_t generate_key(const struct keymaster1_device* dev,
                                  const keymaster_key_param_set_t* params,
                                  keymaster_key_blob_t* key_blob,
                                  keymaster_key_characteristics_t** characteristics)
{
    device_t *device = (device_t *) dev;

    fprintf(f, "\n%s", __func__); fflush(f);

    CALL(device->vendor.device->generate_key(device->vendor.device, params, key_blob, characteristics));
}

static keymaster_error_t get_key_characteristics(const struct keymaster1_device* dev,
                                             const keymaster_key_blob_t* key_blob,
                                             const keymaster_blob_t* client_id,
                                             const keymaster_blob_t* app_data,
                                             keymaster_key_characteristics_t** characteristics)
{
    device_t *device = (device_t *) dev;

    CALL(device->vendor.device->get_key_characteristics(device->vendor.device, key_blob, client_id, app_data, characteristics));
}

static keymaster_error_t import_key(const struct keymaster1_device* dev,
                                const keymaster_key_param_set_t* params,
                                keymaster_key_format_t key_format,
                                const keymaster_blob_t* key_data,
                                keymaster_key_blob_t* key_blob,
                                keymaster_key_characteristics_t** characteristics)
{
    device_t *device = (device_t *) dev;

    fprintf(f, "\n%s", __func__); fflush(f);

    CALL(device->vendor.device->import_key(device->vendor.device, params, key_format, key_data, key_blob, characteristics));
}

static keymaster_error_t export_key(const struct keymaster1_device* dev,
                                keymaster_key_format_t export_format,
                                const keymaster_key_blob_t* key_to_export,
                                const keymaster_blob_t* client_id,
                                const keymaster_blob_t* app_data,
                                keymaster_blob_t* export_data)
{
    device_t *device = (device_t *) dev;

    fprintf(f, "\n%s", __func__); fflush(f);

    CALL(device->vendor.device->export_key(device->vendor.device, export_format, key_to_export, client_id, app_data, export_data));
}

static keymaster_error_t delete_key(const struct keymaster1_device* dev,
                                const keymaster_key_blob_t* key)
{
    device_t *device = (device_t *) dev;

    fprintf(f, "\n%s", __func__); fflush(f);

    CALL(device->vendor.device->delete_key(device->vendor.device, key));
}

static keymaster_error_t delete_all_keys(const struct keymaster1_device* dev)
{
    device_t *device = (device_t *) dev;

    fprintf(f, "\n%s", __func__); fflush(f);

    CALL(device->vendor.device->delete_all_keys(device->vendor.device));
}

static keymaster_error_t begin(const struct keymaster1_device* dev, keymaster_purpose_t purpose,
                           const keymaster_key_blob_t* key,
                           const keymaster_key_param_set_t* in_params,
                           keymaster_key_param_set_t* out_params,
                           keymaster_operation_handle_t* operation_handle)
{
    device_t *device = (device_t *) dev;

    fprintf(f, "\n%s purpose %d key ", __func__, purpose);
    for (int i = 0; i < key->key_material_size; i++) {
        fprintf(f, " %02x", key->key_material[i]);
    }
    fprintf(f, " params {");
    for (int i = 0; i < in_params->length; i++) {
        fprintf(f, " %d,%d,%d", in_params->params[i].tag>>28, in_params->params[i].tag & ((1<<28)-1), in_params->params[i].enumerated);
    }
    fprintf(f, "}"); fflush(f);

    PRE_CALL;
    auto rv = device->vendor.device->begin(device->vendor.device, purpose, key, in_params, out_params, operation_handle);
    POST_CALL;
    fprintf(f, " ==> %d", rv);
    return rv;
}

static keymaster_error_t update(const struct keymaster1_device* dev,
                            keymaster_operation_handle_t operation_handle,
                            const keymaster_key_param_set_t* in_params,
                            const keymaster_blob_t* input, size_t* input_consumed,
                            keymaster_key_param_set_t* out_params, keymaster_blob_t* output)
{
    device_t *device = (device_t *) dev;

    fprintf(f, "\n%s", __func__); fflush(f);

    CALL(device->vendor.device->update(device->vendor.device, operation_handle, in_params, input, input_consumed, out_params, output));
}

static keymaster_error_t finish(const struct keymaster1_device* dev,
                            keymaster_operation_handle_t operation_handle,
                            const keymaster_key_param_set_t* in_params,
                            const keymaster_blob_t* signature,
                            keymaster_key_param_set_t* out_params, keymaster_blob_t* output)
{
    device_t *device = (device_t *) dev;

    fprintf(f, "\n%s", __func__); fflush(f);

    CALL(device->vendor.device->finish(device->vendor.device, operation_handle, in_params, signature, out_params, output));
}

static keymaster_error_t abort(const struct keymaster1_device* dev,
                           keymaster_operation_handle_t operation_handle)
{
    device_t *device = (device_t *) dev;

    fprintf(f, "\n%s", __func__); fflush(f);

    CALL(device->vendor.device->abort(device->vendor.device, operation_handle));
}

static int device_close(hw_device_t *hw_device)
{
    device_t *device = (device_t *) hw_device;
fprintf(f, "\n%s", __func__); fflush(f);
    int rv = device->vendor.device->common.close(device->vendor.hw_device);
fprintf(f, "\nclosed"); fflush(f);
    free(device);
fprintf(f, "\nfreed"); fflush(f);
    return rv;
}

static int device_open(const hw_module_t *module, const char *name, hw_device_t **device_out)
{
    int rv;
    device_t *device;

if (f == NULL) f = fopen("/keystore.txt", "w");
if (f == NULL) f = stderr;
fprintf(f, "\n%s %s\n", __func__, name); fflush(f);

    if (!ensure_vendor_module_is_loaded(name)) {
        return -EINVAL;
    }

    device = (device_t *) calloc(sizeof(*device), 1);
    if (!device) {
        ALOGE("%s: Failed to allocate memory", __func__);
        return -ENOMEM;
    }

    rv = vendor->methods->open(vendor, name, &device->vendor.hw_device);
    if (rv) {
        ALOGE("%s: failed to open, error %d\n", __func__, rv);
        free(device);
        return rv;
    }

    device->base.common = device->vendor.device->common;
    device->base.common.close = device_close;

    device->base.generate_keypair = generate_keypair;
    device->base.import_keypair = import_keypair;
    device->base.get_keypair_public = get_keypair_public;
    device->base.delete_keypair = delete_keypair;
    device->base.delete_all = delete_all;
    device->base.sign_data = sign_data;
    device->base.verify_data = verify_data;
    device->base.get_supported_algorithms = get_supported_algorithms;
    device->base.get_supported_block_modes = get_supported_block_modes;
    device->base.get_supported_padding_modes = get_supported_padding_modes;
    device->base.get_supported_digests = get_supported_digests;
    device->base.get_supported_import_formats = get_supported_import_formats;
    device->base.get_supported_export_formats = get_supported_export_formats;
    device->base.add_rng_entropy = add_rng_entropy;
    device->base.generate_key = generate_key;
    device->base.get_key_characteristics = get_key_characteristics;
    device->base.import_key = import_key;
    device->base.export_key = export_key;
    device->base.delete_key = delete_key;
    device->base.delete_all_keys = delete_all_keys;
    device->base.begin = begin;
    device->base.update = update;
    device->base.finish = finish;
    device->base.abort = abort;

    POST_CALL;

    *device_out = (hw_device_t *) device;
    return 0;
}

static struct hw_module_methods_t module_methods = {
    .open = device_open
};

static struct hw_module_methods_t keymaster_module_methods = {
    .open = device_open
};

struct keystore_module HAL_MODULE_INFO_SYM = {
    .common = {
         .tag = HARDWARE_MODULE_TAG,
         .module_api_version = KEYMASTER_DEVICE_API_VERSION_1_0,
         .hal_api_version = HARDWARE_HAL_API_VERSION,
         .id = KEYSTORE_HARDWARE_MODULE_ID,
         .name = "Moorefield Keystore Adapter",
         .author = "The CyanogenMod Project",
         .methods = &keymaster_module_methods,
         .dso = NULL, /* remove compilation warnings */
         .reserved = {0}, /* remove compilation warnings */
    }
};
