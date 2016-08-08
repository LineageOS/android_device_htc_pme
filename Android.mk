#
# Copyright 2016 The CyanogenMod Project
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
#

# This contains the module build definitions for the hardware-specific
# components for this device.
#
# As much as possible, those components should be built unconditionally,
# with device-specific names to avoid collisions, to avoid device-specific
# bitrot and build breakages. Building a component unconditionally does
# *not* include it on all devices, so it is safe even with hardware-specific
# components.

LOCAL_PATH := $(call my-dir)

ifneq ($(filter pme,$(TARGET_DEVICE)),)

include $(call all-makefiles-under,$(LOCAL_PATH))

include $(CLEAR_VARS)

ADSP_IMAGES := \
    adsp.b00 adsp.b01 adsp.b02 adsp.b03 adsp.b04 adsp.b05 adsp.b06 adsp.b07 \
    adsp.b08 adsp.b09 adsp.b10 adsp.b11 adsp.mdt adpver.cfg

ADSP_SYMLINKS := $(addprefix $(TARGET_OUT_VENDOR)/firmware/,$(notdir $(ADSP_IMAGES)))
$(ADSP_SYMLINKS): $(LOCAL_INSTALLED_MODULE)
	@echo "ADSP firmware link: $@"
	@mkdir -p $(dir $@)
	@rm -rf $@
	$(hide) ln -sf /firmware/adsp/$(notdir $@) $@

ALL_DEFAULT_INSTALLED_MODULES += $(ADSP_SYMLINKS)

MBA_IMAGES := \
    mba.b00 mba.b01 mba.b02 mba.b03 mba.b04 mba.b05 mba.mbn mba.mdt

MBA_SYMLINKS := $(addprefix $(TARGET_OUT_VENDOR)/firmware/,$(notdir $(MBA_IMAGES)))
$(MBA_SYMLINKS): $(LOCAL_INSTALLED_MODULE)
	@echo "MBA firmware link: $@"
	@mkdir -p $(dir $@)
	@rm -rf $@
	$(hide) ln -sf /firmware/radio/$(notdir $@) $@

ALL_DEFAULT_INSTALLED_MODULES += $(MBA_SYMLINKS)

MISC_IMAGES := \
    qdsp6m.qdb radiover.cfg version.cfg

MISC_SYMLINKS := $(addprefix $(TARGET_OUT_VENDOR)/firmware/,$(notdir $(MISC_IMAGES)))
$(MISC_SYMLINKS): $(LOCAL_INSTALLED_MODULE)
	@echo "Misc firmware link: $@"
	@mkdir -p $(dir $@)
	@rm -rf $@
	$(hide) ln -sf /firmware/radio/$(notdir $@) $@

ALL_DEFAULT_INSTALLED_MODULES += $(MISC_SYMLINKS)

MODEM_IMAGES := \
    modem.b00 modem.b01 modem.b02 modem.b03 modem.b04 modem.b05 \
    modem.b06 modem.b07 modem.b08 modem.b09 modem.b10 modem.b11 \
    modem.b12 modem.b13 modem.b15 modem.b16 modem.b17 modem.b18 \
    modem.b19 modem.b20 modem.mdt

MODEM_SYMLINKS := $(addprefix $(TARGET_OUT_VENDOR)/firmware/,$(notdir $(MODEM_IMAGES)))
$(MODEM_SYMLINKS): $(LOCAL_INSTALLED_MODULE)
	@echo "Modem firmware link: $@"
	@mkdir -p $(dir $@)
	@rm -rf $@
	$(hide) ln -sf /firmware/radio/$(notdir $@) $@

ALL_DEFAULT_INSTALLED_MODULES += $(MODEM_SYMLINKS)

SLPI_IMAGES := \
    slpi.b00 slpi.b01 slpi.b02 slpi.b03 slpi.b04 slpi.b05 slpi.b06 \
    slpi.b07 slpi.b08 slpi.b09 slpi.b10 slpi.b11 slpi.b12 slpi.b13 \
    slpi.b14 slpi.b15 slpi.mdt slpiver.cfg

SLPI_SYMLINKS := $(addprefix $(TARGET_OUT_VENDOR)/firmware/,$(notdir $(SLPI_IMAGES)))
$(SLPI_SYMLINKS): $(LOCAL_INSTALLED_MODULE)
	@echo "SLPI firmware link: $@"
	@mkdir -p $(dir $@)
	@rm -rf $@
	$(hide) ln -sf /firmware/slpi/$(notdir $@) $@

ALL_DEFAULT_INSTALLED_MODULES += $(SLPI_SYMLINKS)

VENUS_IMAGES := \
    venus.b00 venus.b01 venus.b02 venus.b03 venus.b04 venus.mbn venus.mdt

VENUS_SYMLINKS := $(addprefix $(TARGET_OUT_VENDOR)/firmware/,$(notdir $(VENUS_IMAGES)))
$(VENUS_SYMLINKS): $(LOCAL_INSTALLED_MODULE)
	@echo "VENUS firmware link: $@"
	@mkdir -p $(dir $@)
	@rm -rf $@
	$(hide) ln -sf /firmware/venus/$(notdir $@) $@

ALL_DEFAULT_INSTALLED_MODULES += $(VENUS_SYMLINKS)

.PHONY: RFS_LINK_PROCESSING
RFS_LINK_PROCESSING: $(LOCAL_INSTALLED_MODULE)
	mkdir -p $(TARGET_OUT)/rfs/apq/gnss/readonly
	mkdir -p $(TARGET_OUT)/rfs/msm/adsp/readonly
	mkdir -p $(TARGET_OUT)/rfs/msm/mpss/readonly
	mkdir -p $(TARGET_OUT)/rfs/mdm/adsp/readonly
	mkdir -p $(TARGET_OUT)/rfs/mdm/mpss/readonly
	mkdir -p $(TARGET_OUT)/rfs/mdm/sparrow/readonly
	ln -sf /persist/hlos_rfs/shared $(TARGET_OUT)/rfs/apq/gnss/hlos
	ln -sf /data/tombstones/modem $(TARGET_OUT)/rfs/apq/gnss/ramdumps
	ln -sf /firmware $(TARGET_OUT)/rfs/apq/gnss/readonly/firmware
	ln -sf /persist/rfs/apq/gnss $(TARGET_OUT)/rfs/apq/gnss/readwrite
	ln -sf /persist/rfs/shared $(TARGET_OUT)/rfs/apq/gnss/shared
	ln -sf /persist/hlos_rfs/shared $(TARGET_OUT)/rfs/mdm/adsp/hlos
	ln -sf /data/tombstones/lpass $(TARGET_OUT)/rfs/mdm/adsp/ramdumps
	ln -sf /firmware $(TARGET_OUT)/rfs/mdm/adsp/readonly/firmware
	ln -sf /persist/rfs/mdm/adsp $(TARGET_OUT)/rfs/mdm/adsp/readwrite
	ln -sf /persist/rfs/shared $(TARGET_OUT)/rfs/mdm/adsp/shared
	ln -sf /persist/hlos_rfs/shared $(TARGET_OUT)/rfs/mdm/mpss/hlos
	ln -sf /data/tombstones/modem $(TARGET_OUT)/rfs/mdm/mpss/ramdumps
	ln -sf /firmware $(TARGET_OUT)/rfs/mdm/mpss/readonly/firmware
	ln -sf /persist/rfs/mdm/mpss $(TARGET_OUT)/rfs/mdm/mpss/readwrite
	ln -sf /persist/rfs/shared $(TARGET_OUT)/rfs/mdm/mpss/shared
	ln -sf /persist/hlos_rfs/shared $(TARGET_OUT)/rfs/mdm/sparrow/hlos
	ln -sf /data/tombstones/sparrow $(TARGET_OUT)/rfs/mdm/sparrow/ramdumps
	ln -sf /firmware $(TARGET_OUT)/rfs/mdm/sparrow/readonly/firmware
	ln -sf /persist/rfs/mdm/sparrow $(TARGET_OUT)/rfs/mdm/sparrow/readwrite
	ln -sf /persist/rfs/shared $(TARGET_OUT)/rfs/mdm/sparrow/shared
	ln -sf /persist/hlos_rfs/shared $(TARGET_OUT)/rfs/msm/adsp/hlos
	ln -sf /data/tombstones/lpass $(TARGET_OUT)/rfs/msm/adsp/ramdumps
	ln -sf /firmware $(TARGET_OUT)/rfs/msm/adsp/readonly/firmware
	ln -sf /persist/rfs/msm/adsp $(TARGET_OUT)/rfs/msm/adsp/readwrite
	ln -sf /persist/rfs/shared $(TARGET_OUT)/rfs/msm/adsp/shared
	ln -sf /persist/hlos_rfs/shared $(TARGET_OUT)/rfs/msm/mpss/hlos
	ln -sf /data/tombstones/modem $(TARGET_OUT)/rfs/msm/mpss/ramdumps
	ln -sf /firmware $(TARGET_OUT)/rfs/msm/mpss/readonly/firmware
	ln -sf /firmware/wsd $(TARGET_OUT)/rfs/msm/mpss/readonly/wsd
	ln -sf /persist/rfs/msm/mpss $(TARGET_OUT)/rfs/msm/mpss/readwrite
	ln -sf /persist/rfs/shared $(TARGET_OUT)/rfs/msm/mpss/shared

ALL_DEFAULT_INSTALLED_MODULES += RFS_LINK_PROCESSING

endif
