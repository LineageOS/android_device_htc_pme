LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := external/tinyxml

LOCAL_SRC_FILES := operator-properties.cpp

LOCAL_MODULE := operator-properties
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := libcutils liblog libtinyxml

include $(BUILD_EXECUTABLE)
