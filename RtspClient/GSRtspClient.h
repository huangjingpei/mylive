#ifndef __RTSP_CLIENT_H__
#define __RTSP_CLIENT_H__
#include <utils/RefBase.h>
#include <utils/Thread.h>
#include <utils/Mutex.h>
using namespace android;
using namespace std;
class GeduRtspClient : public RefBase {
public:
    //rtsp implement
    static sp<GSRtspClient> create();
    status_t createRTSPClient(const char *url);
    status_t destoryRtspClient();
    virtual ~GSRtspClient();
    static void ExitNotify(void *data, bool exit);
    static void postAccessUnit(int32_t size, long long ts, char *buffer, void *data);
public:
    //decode implement
//    sp<VideoDecoder> mDecoder;
public:
    //render implement
//    sp<VideoRender> mRenderer;
private:
    struct NetworkThread;
    volatile char mEventLoopExit;
    sp<Thread> mThread;
    char szUrl[256];


private:
    GSRtspClient();
};
#endif//__RTSP_CLIENT_H__
