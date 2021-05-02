LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_C_INCLUDES += \
                $(LOCAL_PATH)/. \
                $(LOCAL_PATH)/system \
                $(LOCAL_PATH)/system/audio_effects
LOCAL_SRC_FILES  += \
                 mp3reader.cpp \
                 primitives.c \
                 tinysndfile.c
LOCAL_MODULE := libmp3tools
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0 SPDX-license-identifier-BSD
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_SYSTEM_EXT_MODULE := true
LOCAL_SHARED_LIBRARIES +=
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)
