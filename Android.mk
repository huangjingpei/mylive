MY_LOCAL_PATH:= $(call my-dir)
LOCAL_PATH:= $(MY_LOCAL_PATH)


include $(CLEAR_VARS)


include $(CLEAR_VARS)
LOCAL_MODULE := basic-usage-environment
LOCAL_CPP_FEATURES := exceptions
LOCAL_SRC_FILES := \
    BasicUsageEnvironment/BasicHashTable.cpp \
    BasicUsageEnvironment/BasicTaskScheduler0.cpp \
    BasicUsageEnvironment/BasicTaskScheduler.cpp \
    BasicUsageEnvironment/BasicUsageEnvironment0.cpp \
    BasicUsageEnvironment/BasicUsageEnvironment.cpp \
    BasicUsageEnvironment/DelayQueue.cpp


LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/BasicUsageEnvironment/include \
    $(LOCAL_PATH)/groupsock/include \
    $(LOCAL_PATH)/UsageEnvironment/include

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/BasicUsageEnvironment/include
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := groupsock
LOCAL_CPP_FEATURES := exceptions
LOCAL_SRC_FILES := \
    groupsock/GroupsockHelper.cpp \
    groupsock/Groupsock.cpp \
    groupsock/NetInterface.cpp \
    groupsock/NetAddress.cpp \
    groupsock/GroupEId.cpp \
    groupsock/inet.c \
    groupsock/IOHandlers.cpp 
    #proxyServer/live555ProxyServer.cpp
    
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/groupsock/include \
    $(LOCAL_PATH)/UsageEnvironment/include
    LOCAL_CFLAGS += -D__ANDROID__
    
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/groupsock/include
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := usage-environment
LOCAL_CPP_FEATURES := exceptions
LOCAL_SRC_FILES := \
    UsageEnvironment/HashTable.cpp \
    UsageEnvironment/UsageEnvironment.cpp \
    UsageEnvironment/strDup.cpp
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/UsageEnvironment/include \
    $(LOCAL_PATH)/groupsock/include
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/UsageEnvironment/include
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := live-media
LOCAL_CPP_FEATURES := exceptions
LOCAL_SRC_FILES := \
    liveMedia/RTSPClient.cpp \
    liveMedia/MediaSession.cpp \
    liveMedia/RTCP.cpp \
    liveMedia/Media.cpp \
    liveMedia/FramedSource.cpp \
    liveMedia/MediaSink.cpp \
    liveMedia/DigestAuthentication.cpp \
    liveMedia/RTPSource.cpp \
    liveMedia/Locale.cpp \
    liveMedia/RTPSink.cpp \
    liveMedia/Base64.cpp \
    liveMedia/RTSPServer.cpp \

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/liveMedia/include \
    $(LOCAL_PATH)/groupsock/include \
    $(LOCAL_PATH)/UsageEnvironment/include
    LOCAL_CFLAGS += -Wno-int-to-pointer-cast -frtti -fexceptions

LOCAL_STATIC_LIBRARIES +=
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/liveMedia/include
include $(BUILD_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE := liblive555
LOCAL_WHOLE_STATIC_LIBRARIES := usage-environment live-media groupsock basic-usage-environment
include $(BUILD_SHARED_LIBRARY)


include $(LOCAL_PATH)/testProgs/Android.mk

