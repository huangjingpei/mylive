LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
-include vendor/grandstream/gs_avs/avs.mk

#-----------------------------------------------------------
#-include $(MTK_PATH_SOURCE)/hardware/mtkcam/mtkcam.mk

#-----------------------------------------------------------

#LOCAL_STATIC_LIBRARIES := \
#    live-media \
#    groupsock \
#    basic-usage-environment \
#    usage-environment \


LOCAL_SHARED_LIBRARIES := \
    liblive555
LOCAL_C_INCLUDES:= \
    $(LOCAL_PATH)/../liveMedia/include \
    $(LOCAL_PATH)/../groupsock/include \
    $(LOCAL_PATH)/../UsageEnvironment/include \
    $(LOCAL_PATH)/../BasicUsageEnvironment/include
    


LOCAL_SRC_FILES = testRTSPClient.cpp



#-----------------------------------------------------------
LOCAL_MODULE := testRtspClient
LOCAL_MULTILIB := 32
LOCAL_MODULE_TAGS :=optional
#-----------------------------------------------------------
include $(BUILD_EXECUTABLE)




