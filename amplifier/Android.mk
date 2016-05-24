LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
	amp_loader.c
LOCAL_CFLAGS := -Wall -Wextra
LOCAL_MODULE := amp_loader
include $(BUILD_EXECUTABLE)
