#ifndef __LIVE_RTSP_CLIENT_H__
#define __LIVE_RTSP_CLIENT_H__

typedef void ExitNotify(void* clientData, bool exit);
typedef void AccessUnitNotify(int size, long long ts, char *buffer, void *data, int type);

typedef struct tag_gedu_rtsp_handle {
	/*RTSP agent name*/
	char *name;
	/*RTSP exit function*/
	ExitNotify *exit_func;
	/*RTSP media unit retrieve function */
	AccessUnitNotify *access_unit_notify;
	/*Pass in by User.*/
	void *object;
	/*used inner*/
	void *rtsp;
	/*used inner*/
	void *env;
	/*used inner*/
	void *scheduler;
}GeDuRtspHandle;

GeDuRtspHandle* GeDuRtspCreate(char const* userAgent,
		char const* rtspURL,
		ExitNotify *func,
		AccessUnitNotify *func2,
		void *obj);
void GeDuRtspEventLoop(GeDuRtspHandle * handle , volatile char* watchVariable);
void GeDuRtspDestory(GeDuRtspHandle * handle);


#endif//__LIVE_RTSP_CLIENT_H__
