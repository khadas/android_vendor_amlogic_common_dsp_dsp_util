LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_C_INCLUDES += \
                $(TOP)/external/tinyalsa/include/tinyalsa \
                $(LOCAL_PATH)/../include
LOCAL_SRC_FILES  += \
                aml_audio_util.c \
                aml_flatbuf_api.c \
                aml_resampler.c \
                aml_wakeup_api.c \
                rpc_client_aac.c \
                rpc_client_aipc.c \
                rpc_client_mp3.c \
                rpc_client_pcm.c \
                rpc_client_shm.c \
                rpc_client_vsp.c \
                aml_pcm_gain_api.c \
                aml_tbuf_api.c \
                rpc_util.c \
                aml_pcm_api.c
LOCAL_MODULE := libhifi4rpc_client
LOCAL_SHARED_LIBRARIES += \
                libhifi4rpc \
                libtinyalsa
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
