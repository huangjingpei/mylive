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
    libutils \
    libcutils \
    liblive555
LOCAL_C_INCLUDES:= \
    $(LOCAL_PATH)/../../live/liveMedia/include \
    $(LOCAL_PATH)/../../live/groupsock/include \
    $(LOCAL_PATH)/../../live/UsageEnvironment/include \
    $(LOCAL_PATH)/../../live/BasicUsageEnvironment/include
    


LOCAL_SRC_FILES = GSRtspClient.cpp \
                    LiveRtspClient.cpp



#-----------------------------------------------------------
LOCAL_MODULE := testRtspClient
LOCAL_MULTILIB := 32
LOCAL_MODULE_TAGS :=optional
#-----------------------------------------------------------
include $(BUILD_EXECUTABLE)




