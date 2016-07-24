# Copyright (C) 2016 The CyanogenMod Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

$(call inherit-product, device/htc/pme/full_pme.mk)
$(call inherit-product, vendor/cm/config/common_full_phone.mk)

# Enhanced NFC
$(call inherit-product, vendor/cm/config/nfc_enhanced.mk)

## Device identifier. This must come after all inclusions
PRODUCT_DEVICE := pme
PRODUCT_NAME := cm_pme
PRODUCT_BRAND := HTC
PRODUCT_MODEL := HTC 10
PRODUCT_MANUFACTURER := HTC
PRODUCT_RELEASE_NAME := pme

PRODUCT_BUILD_PROP_OVERRIDES += \
    PRODUCT_DEVICE="htc_pmewl" \
    PRODUCT_NAME="pmewl_00531" \
    BUILD_FINGERPRINT="htc/pmewl_00531/htc_pmewl:6.0.1/MMB29M/770927.1:user/release-keys" \
    PRIVATE_BUILD_DESC="1.81.531.1 8.0_g CL770927 release-keys"
