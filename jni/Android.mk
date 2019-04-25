LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := curl
LOCAL_SRC_FILES := libcurl.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/curl
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := implant
LOCAL_SRC_FILES := implant.cpp
LOCAL_STATIC_LIBRARIES := libcurl
LOCAL_LDLIBS := -lz
LOCAL_CPP_FEATURES := exceptions
include $(BUILD_EXECUTABLE)
