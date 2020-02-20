LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_CFLAGS+=-DANDROIDPLATFORM
LOCAL_C_INCLUDES += \
                $(LOCAL_PATH)/../include
LOCAL_SRC_FILES  += \
                 dsp_util.c
LOCAL_MODULE := dsp_util
LOCAL_SHARED_LIBRARIES += \
                        libc
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)
