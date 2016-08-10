LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SHARED_LIBRARIES := \
	liblog libutils libcutils libtinyalsa

LOCAL_C_INCLUDES := \
        external/tinyalsa/include \
	hardware/libhardware/include

LOCAL_SRC_FILES := \
	audio_amplifier.c tfa.c tfa-cont.c tfa9888-debug.c

LOCAL_ADDITIONAL_DEPENDENCIES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr
LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include

LOCAL_MODULE := audio_amplifier.msm8996
LOCAL_MODULE_RELATIVE_PATH := hw
OCAL_MODULE_TAGS := optional
LOCAL_CFLAGS = -Werror

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_SHARED_LIBRARIES := \
	liblog libutils libcutils libtinyalsa

LOCAL_C_INCLUDES := \
        external/tinyalsa/include \
	hardware/libhardware/include

LOCAL_CFLAGS := -g -O0

LOCAL_SRC_FILES := \
	play.c tfa9888-debug.c tfa.c tfa-cont.c

LOCAL_ADDITIONAL_DEPENDENCIES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr
LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include

LOCAL_MODULE := play
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS = -Werror

include $(BUILD_EXECUTABLE)

