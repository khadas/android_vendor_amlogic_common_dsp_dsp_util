LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_C_INCLUDES += \
                $(LOCAL_PATH)/. \
                $(LOCAL_PATH)/../mp3tools \
                $(LOCAL_PATH)/../include
LOCAL_SRC_FILES  += \
                hifi4_basic_test.c \
                hifi4_flatbuf_test.c \
                hifi4rpc_awe_test.c \
                hifi4rpc_client_test.cpp \
                hifi4rpc_codec_test.cpp \
                hifi4rpc_pcm_test.c \
                hifi4rpc_rsp_test.c \
                hifi4rpc_vsp_test.c
LOCAL_MODULE := hifi4rpc_client_test
LOCAL_SHARED_LIBRARIES += \
                        libc \
                        libhifi4rpc_client \
                        libhifi4rpc \
                        libmp3tools
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)
