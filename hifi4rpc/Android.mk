LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_C_INCLUDES +=                      \
                $(LOCAL_PATH)/../include
LOCAL_SRC_FILES  +=               \
               rpc_dev.c
LOCAL_MODULE := libhifi4rpc
LOCAL_SYSTEM_EXT_MODULE := true
LOCAL_SHARED_LIBRARIES +=
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)
