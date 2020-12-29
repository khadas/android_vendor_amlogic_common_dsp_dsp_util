LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_C_INCLUDES += \
                 $(LOCAL_PATH)/../include
LOCAL_SRC_FILES  += \
                 vad_main.c
LOCAL_MODULE := vad_trigger
LOCAL_MODULE_TAGS := optional
LOCAL_SYSTEM_EXT_MODULE := true

ifeq ($(C4A_HIFI4),true)
LOCAL_CFLAGS+=-DC4A_HIFI4
else
LOCAL_C_INCLUDES += $(TOP)/external/tinyalsa/include/tinyalsa
LOCAL_SHARED_LIBRARIES += libtinyalsa
endif
include $(BUILD_EXECUTABLE)
