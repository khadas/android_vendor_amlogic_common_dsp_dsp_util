LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_C_INCLUDES += \
                $(LOCAL_PATH)/. \
                $(LOCAL_PATH)/../mp3tools \
                $(LOCAL_PATH)/../include
LOCAL_SRC_FILES  += \
                hifi4_basic_test.c \
                hifi4_flatbuf_test.c \
                hifi4_tbuf_test.c \
                hifi4rpc_awe_test.c \
                hifi4rpc_client_test.cpp \
                hifi4rpc_codec_test.cpp \
                hifi4rpc_pcm_test.c \
                hifi4rpc_rsp_test.c \
                hifi4rpc_vsp_test.c \
                hifi4rpc_gain_test.c \
                hifi4rpc_xaf_test.c \
                hifi4rpc_pcm_dspc_test.c
LOCAL_MODULE := hifi4rpc_client_test
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0 SPDX-license-identifier-FTL SPDX-license-identifier-GPL SPDX-license-identifier-LGPL-2.1 SPDX-license-identifier-MIT legacy_by_exception_only legacy_notice
LOCAL_LICENSE_CONDITIONS := by_exception_only notice restricted
LOCAL_SYSTEM_EXT_MODULE := true
LOCAL_SHARED_LIBRARIES += \
                        libhifi4rpc_client \
                        libhifi4rpc \
                        libmp3tools

LOCAL_MODULE_TAGS := optional

ifeq ($(C4A_HIFI4),true)
    LOCAL_CFLAGS+=-DC4A_HIFI4
    LOCAL_C_INCLUDES += $(TOP)/external/alsa-lib/include
    LOCAL_SHARED_LIBRARIES += libasound
else
    LOCAL_CFLAGS+=-DANDROIDPLATFORM
    LOCAL_C_INCLUDES += $(TOP)/external/tinyalsa/include/tinyalsa
    LOCAL_SHARED_LIBRARIES += libtinyalsa
endif

include $(BUILD_EXECUTABLE)
