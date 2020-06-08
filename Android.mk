LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := vinput-manager

LOCAL_SRC_FILES := vInputManager.cpp vInputDevice.cpp

LOCAL_CFLAGS := -lpthread -Wall -Wpedantic -Wextra

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_MODULE := send-key

LOCAL_SRC_FILES := sendKey.cpp

LOCAL_CFLAGS := -lpthread -Wall -Wpedantic -Wextra

include $(BUILD_EXECUTABLE)
