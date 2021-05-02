LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_C_INCLUDES += \
                $(LOCAL_PATH)/../include
LOCAL_SRC_FILES  += \
                 rpc_test.c
LOCAL_MODULE := hifi4_rpc_test
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0 SPDX-license-identifier-BSD SPDX-license-identifier-LGPL legacy_by_exception_only
LOCAL_LICENSE_CONDITIONS := by_exception_only notice restricted
LOCAL_SYSTEM_EXT_MODULE := true
LOCAL_SHARED_LIBRARIES += \
                        libhifi4rpc \
                        libhifi4rpc_client
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)
