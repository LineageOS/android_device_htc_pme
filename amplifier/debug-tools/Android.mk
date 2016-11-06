LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SHARED_LIBRARIES := \
	liblog libutils libcutils libtinyalsa

LOCAL_C_INCLUDES := \
        external/tinyalsa/include \
	hardware/libhardware/include

LOCAL_SRC_FILES := \
	play.c tfa9888-debug.c ../tfa.c ../tfa-cont.c

LOCAL_ADDITIONAL_DEPENDENCIES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr
LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include

LOCAL_MODULE := play
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS = -I$(LOCAL_PATH)/.. -Werror

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_SHARED_LIBRARIES := \
	liblog libutils libcutils libtinyalsa

LOCAL_C_INCLUDES := \
        external/tinyalsa/include \
	hardware/libhardware/include

LOCAL_SRC_FILES := \
	tfa-mix.c ../tfa.c

LOCAL_ADDITIONAL_DEPENDENCIES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr
LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include

LOCAL_MODULE := tfa-mix
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS = -I$(LOCAL_PATH)/.. -Werror

include $(BUILD_EXECUTABLE)

