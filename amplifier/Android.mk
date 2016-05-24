LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
	amp_loader.c
LOCAL_CFLAGS := -Wall -Wextra -std=c11
LOCAL_MODULE := amp_loader
include $(BUILD_EXECUTABLE)
