LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SHARED_LIBRARIES := liblog
LOCAL_SRC_FILES := acdbloader.c
LOCAL_MODULE := libacdbloader
OCAL_MODULE_TAGS := optional
LOCAL_CFLAGS = -Werror

include $(BUILD_SHARED_LIBRARY)
