LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(C4A_HIFI4),true)
	# for GVA/C4A environment, it share the firmware path of buildroot
$(warning c4a share the firmware path of buildroot)
else
LOCAL_CFLAGS+=-DANDROIDPLATFORM
endif

LOCAL_C_INCLUDES += \
                $(LOCAL_PATH)/../include
LOCAL_SRC_FILES  += \
                 dsp_util.c
LOCAL_MODULE := dsp_util
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0 SPDX-license-identifier-BSD SPDX-license-identifier-LGPL legacy_by_exception_only
LOCAL_LICENSE_CONDITIONS := by_exception_only notice restricted
LOCAL_SYSTEM_EXT_MODULE := true
LOCAL_SHARED_LIBRARIES +=
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)
