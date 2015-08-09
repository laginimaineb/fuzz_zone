LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := fuzz_zone
LOCAL_CFLAGS += -std=c99
LOCAL_LDLIBS := -llog
LOCAL_SRC_FILES := main.c
include $(BUILD_EXECUTABLE)
