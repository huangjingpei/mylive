#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <net/if.h>
#include <linux/tcp.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <unistd.h>
#include "LiveRtspClient.h"
#include "GSRtspClient.h"


#define LOGE printf
struct GSRtspClient::NetworkThread : public Thread {
    NetworkThread(GSRtspClient *session);

protected:
    virtual ~NetworkThread();

private:
    GSRtspClient *mClient;

    virtual bool threadLoop();
private:
    NetworkThread(const NetworkThread &);
    NetworkThread &operator=(const NetworkThread &);
};

GSRtspClient::NetworkThread::NetworkThread(GSRtspClient *client)
    : mClient(client) {

}

GSRtspClient::NetworkThread::~NetworkThread() {
}

void GSRtspClient::ExitNotify(void *data, bool exit)
{
    GSRtspClient * thiz = (GSRtspClient*)data;
    if (exit == 1) {
        thiz->mEventLoopExit = 1;
    }
}

void GSRtspClient::postAccessUnit(int32_t size, long long ts, char *buffer, void *data)
{
    GSRtspClient * thiz = (GSRtspClient*)data;
    char *p = buffer;
    printf("%#x %#x %#x %#x %#x\n", p[0], p[1], p[2], p[3], p[4]);
//    FILE *fp = fopen("/data/abc.h264", "a+");
//    int32_t startCode = 0x1000000;
//    fwrite(&startCode, 1, 4,fp);
//    fwrite(buffer, 1, size,fp);
//    fclose(fp);
//    if(thiz->mDecoder != NULL) {
//        sp<MetaBufer> accessUnit = BufferOp::create(size);
//        memcpy((void *)BufferOp::data(accessUnit), buffer, size);
//        thiz->mDecoder->queueAccessUnit(accessUnit);
//    }
}

bool GSRtspClient::NetworkThread::threadLoop() {
    liveRtspCreate("GsLive", mClient->szUrl, ExitNotify, postAccessUnit, this);
    while (!exitPending()) {
        liveEventLoop((&mClient->mEventLoopExit));
    }
    liveDestory();
    return true;
}

GSRtspClient::GSRtspClient()
{
    mEventLoopExit = 0;
    mThread = NULL;
}

GSRtspClient::~GSRtspClient()
{

}

sp<GSRtspClient> GSRtspClient::create()
{
    return new GSRtspClient();
}

status_t GSRtspClient::createRTSPClient(const char *url)
{
    status_t err = UNKNOWN_ERROR;
    do {
        if (mThread != NULL) {
            LOGE("rtsp client instance is running .");
            break;
        }

        if (( url != NULL) && ((strlen(url)+1) > 256)){
            LOGE("url is not valid. ");
            break;
        }

        sprintf(szUrl, "%s", url);
        mEventLoopExit = 0;
        mThread = new NetworkThread(this);
        status_t err = mThread->run("rtspclient");
        if (err != NO_ERROR) {
            break;
        }
        err = NO_ERROR;
    } while (0);
    return err;

}

status_t GSRtspClient::destoryRtspClient()
{
    status_t err = UNKNOWN_ERROR;
    do {
        if (mThread->getTid() < 0) {
            return NO_ERROR;
        }
        mEventLoopExit = 1;
        mThread->requestExitAndWait();
        mThread = NULL;
        err = NO_ERROR;
    } while (0);
    return err;
}
#if 1
int main()
{

    sp<GSRtspClient> clinet = GSRtspClient::create();

    clinet->createRTSPClient("rtsp://192.168.127.181:8554/camera.264");
    sleep(100);
    clinet->destoryRtspClient();

    return 0;
}
#endif
