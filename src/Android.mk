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

include $(CLEAR_VARS)
ifneq (,$(filter $(TARGET_PRODUCT),t7_an400))
LOCAL_CFLAGS+=-DTWODSP
endif
ifneq (,$(filter $(TARGET_PRODUCT),t982_ar301))
LOCAL_CFLAGS+=-DONEDSP
endif
LOCAL_SRC_FILES  += \
                 dsp_start.c
LOCAL_MODULE := startdsp
LOCAL_SHARED_LIBRARIES += liblog libcutils
LOCAL_MODULE_TAGS := optional
#LOCAL_INIT_RC := startdsp.rc
LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_EXECUTABLE)
