/**********
 This library is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the
 Free Software Foundation; either version 3 of the License, or (at your
 option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

 This library is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 more details.

 You should have received a copy of the GNU Lesser General Public License
 along with this library; if not, write to the Free Software Foundation, Inc.,
 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 **********/
// Copyright (c) 1996-2017, Live Networks, Inc.  All rights reserved
// A demo application, showing how to create and run a RTSP client (that can potentially receive multiple streams concurrently).
//
// NOTE: This code - although it builds a running application - is intended only to illustrate how to develop your own RTSP
// client application.  For a full-featured RTSP client application - with much more functionality, and many options - see
// "openRTSP": http://www.live555.com/openRTSP/
#include "liveMedia.hh"
#include "Base64.hh"
#include "BasicUsageEnvironment.hh"
#include <sys/time.h>
#include <string.h>
#include <stdio.h>


// Forward function definitions:

// RTSP 'response handlers':
void continueAfterDESCRIBE(RTSPClient *rtspClient, int resultCode,
		char *resultString);
void continueAfterSETUP(RTSPClient *rtspClient, int resultCode,
		char *resultString);
void continueAfterPLAY(RTSPClient *rtspClient, int resultCode,
		char *resultString);

// Other event handler functions:
void subsessionAfterPlaying(void *clientData); // called when a stream's subsession (e.g., audio or video substream) ends
void subsessionByeHandler(void *clientData); // called when a RTCP "BYE" is received for a subsession
void streamTimerHandler(void *clientData);
// called at the end of a stream's expected duration (if the stream has not already signaled its end using a RTCP "BYE")

// The main streaming routine (for each "rtsp://" URL):
RTSPClient* openURL(UsageEnvironment &env, char const *progName,
		char const *rtspURL);

// Used to iterate through each stream's 'subsessions', setting up each one:
void setupNextSubsession(RTSPClient *rtspClient);

// Used to shut down and close a stream (including its "RTSPClient" object):
void shutdownStream(RTSPClient *rtspClient, int exitCode = 1);

// A function that outputs a string that identifies each stream (for debugging output).  Modify this if you wish:
UsageEnvironment& operator<<(UsageEnvironment &env,
		const RTSPClient &rtspClient) {
	return env << "[URL:\"" << rtspClient.url() << "\"]: ";
}

// A function that outputs a string that identifies each subsession (for debugging output).  Modify this if you wish:
UsageEnvironment& operator<<(UsageEnvironment &env,
		const MediaSubsession &subsession) {
	return env << subsession.mediumName() << "/" << subsession.codecName();
}

void usage(UsageEnvironment &env, char const *progName) {
	env << "Usage: " << progName << " <rtsp-url-1> ... <rtsp-url-N>\n";
	env << "\t(where each <rtsp-url-i> is a \"rtsp://\" URL)\n";
}

#if 0
volatile char eventLoopWatchVariable = 0;
int main(int argc, char** argv) {
  // Begin by setting up our usage environment:
  TaskScheduler* scheduler = BasicTaskScheduler::createNew();
  UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);

  // We need at least one "rtsp://" URL argument:
  if (argc < 2) {
    usage(*env, argv[0]);
    return 1;
  }

  // There are argc-1 URLs: argv[1] through argv[argc-1].  Open and start streaming each one:
  for (int i = 1; i <= argc-1; ++i) {
    openURL(*env, argv[0], argv[i]);
  }

  // All subsequent activity takes place within the event loop:
  env->taskScheduler().doEventLoop(&eventLoopWatchVariable);
    // This function call does not return, unless, at some point in time, "eventLoopWatchVariable" gets set to something non-zero.

  return 0;

  // If you choose to continue the application past this point (i.e., if you comment out the "return 0;" statement above),
  // and if you don't intend to do anything more with the "TaskScheduler" and "UsageEnvironment" objects,
  // then you can also reclaim the (small) memory used by these objects by uncommenting the following code:
  /*
    env->reclaim(); env = NULL;
    delete scheduler; scheduler = NULL;
  */
}
#else
#include "LiveRtspClient.h"
#include "SpecificData.h"
#include "BitStream.h"
char *fSPS = NULL;
char *fPPS = NULL;
int fSPSSize = 0;
int fPPSSize = 0;


#endif
// Define a class to hold per-stream state that we maintain throughout each stream's lifetime:

class StreamClientState {
public:
	StreamClientState();
	virtual ~StreamClientState();

public:
	MediaSubsessionIterator *iter;
	MediaSession *session;
	MediaSubsession *subsession;
	TaskToken streamTimerTask;
	double duration;
};

// If you're streaming just a single stream (i.e., just from a single URL, once), then you can define and use just a single
// "StreamClientState" structure, as a global variable in your application.  However, because - in this demo application - we're
// showing how to play multiple streams, concurrently, we can't do that.  Instead, we have to have a separate "StreamClientState"
// structure for each "RTSPClient".  To do this, we subclass "RTSPClient", and add a "StreamClientState" field to the subclass:

class ourRTSPClient: public RTSPClient {
public:
	static ourRTSPClient* createNew(UsageEnvironment &env, char const *rtspURL,
			int verbosityLevel = 0, char const *applicationName = NULL,
			portNumBits tunnelOverHTTPPortNum = 0);

protected:
	ourRTSPClient(UsageEnvironment &env, char const *rtspURL,
			int verbosityLevel, char const *applicationName,
			portNumBits tunnelOverHTTPPortNum);
	// called only by createNew();
	virtual ~ourRTSPClient();

public:
	StreamClientState scs;
};

// Define a data sink (a subclass of "MediaSink") to receive the data for each subsession (i.e., each audio or video 'substream').
// In practice, this might be a class (or a chain of classes) that decodes and then renders the incoming audio or video.
// Or it might be a "FileSink", for outputting the received data into a file (as is done by the "openRTSP" application).
// In this example code, however, we define a simple 'dummy' sink that receives incoming data, but does nothing with it.

#define FRAME_BUFFER_OFFSET (64)
#define SAVE_AAC
class DummySink: public MediaSink {
public:
	static DummySink* createNew(UsageEnvironment &env,
			MediaSubsession &subsession, // identifies the kind of data that's being received
			char const *streamId = NULL); // identifies the stream itself (optional)

private:
	DummySink(UsageEnvironment &env, MediaSubsession &subsession,
			char const *streamId);
	// called only by "createNew()"
	virtual ~DummySink();

	static void afterGettingFrame(void *clientData, unsigned frameSize,
			unsigned numTruncatedBytes, struct timeval presentationTime,
			unsigned durationInMicroseconds);
	void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
			struct timeval presentationTime, unsigned durationInMicroseconds);

	int getSamplingFrequencyIndex(int samplerate);

private:
	// redefined virtual functions:
	virtual Boolean continuePlaying();

private:
	unsigned char *fReceiveBuffer;
	unsigned char *fFrameBuffer;
	MediaSubsession &fSubsession;
	char *fStreamId;
	int fBufferOffset;
	struct timeval fTVFrame;
	SpecificData *fSpecificData;
#ifdef SAVE_AAC
	BitWriter *bitWriter;
#endif

};

#define RTSP_CLIENT_VERBOSITY_LEVEL 1 // by default, print verbose output from each "RTSPClient"

static unsigned rtspClientCount = 0; // Counts how many streams (i.e., "RTSPClient"s) are currently in use.
const int startCode = 0x1000000;

RTSPClient* openURL(UsageEnvironment &env, char const *progName,
		char const *rtspURL) {
	// Begin by creating a "RTSPClient" object.  Note that there is a separate "RTSPClient" object for each stream that we wish
	// to receive (even if more than stream uses the same "rtsp://" URL).
	RTSPClient *rtspClient = ourRTSPClient::createNew(env, rtspURL,
	RTSP_CLIENT_VERBOSITY_LEVEL, progName);
	if (rtspClient == NULL) {
		env << "Failed to create a RTSP client for URL \"" << rtspURL << "\": "
				<< env.getResultMsg() << "\n";
		return NULL;
	}

	++rtspClientCount;

	// Next, send a RTSP "DESCRIBE" command, to get a SDP description for the stream.
	// Note that this command - like all RTSP commands - is sent asynchronously; we do not block, waiting for a response.
	// Instead, the following function call returns immediately, and we handle the RTSP response later, from within the event loop:
	rtspClient->sendDescribeCommand(continueAfterDESCRIBE);
	return rtspClient;
}

// Implementation of the RTSP 'response handlers':

void continueAfterDESCRIBE(RTSPClient *rtspClient, int resultCode,
		char *resultString) {
	do {
		UsageEnvironment &env = rtspClient->envir(); // alias
		StreamClientState &scs = ((ourRTSPClient*) rtspClient)->scs; // alias

		if (resultCode != 0) {
			env << *rtspClient << "Failed to get a SDP description: "
					<< resultString << "\n";
			delete[] resultString;
			break;
		}

		char *const sdpDescription = resultString;
		env << *rtspClient << "Got a SDP description:\n" << sdpDescription
				<< "\n";

		// Create a media session object from this SDP description:
		scs.session = MediaSession::createNew(env, sdpDescription);
		delete[] sdpDescription; // because we don't need it anymore
		if (scs.session == NULL) {
			env << *rtspClient
					<< "Failed to create a MediaSession object from the SDP description: "
					<< env.getResultMsg() << "\n";
			break;
		} else if (!scs.session->hasSubsessions()) {
			env << *rtspClient
					<< "This session has no media subsessions (i.e., no \"m=\" lines)\n";
			break;
		}

		// Then, create and set up our data source objects for the session.  We do this by iterating over the session's 'subsessions',
		// calling "MediaSubsession::initiate()", and then sending a RTSP "SETUP" command, on each one.
		// (Each 'subsession' will have its own data source.)
		scs.iter = new MediaSubsessionIterator(*scs.session);
		setupNextSubsession(rtspClient);
		return;
	} while (0);

	// An unrecoverable error occurred with this stream.
	shutdownStream(rtspClient);
}

// By default, we request that the server stream its data using RTP/UDP.
// If, instead, you want to request that the server stream via RTP-over-TCP, change the following to True:
#define REQUEST_STREAMING_OVER_TCP False

void setupNextSubsession(RTSPClient *rtspClient) {
	UsageEnvironment &env = rtspClient->envir(); // alias
	StreamClientState &scs = ((ourRTSPClient*) rtspClient)->scs; // alias

	scs.subsession = scs.iter->next();
	if (scs.subsession != NULL) {
		if (1) { //(strcmp(scs.subsession->codecName(), "H264") == 0) {

			if (!scs.subsession->initiate()) {
				env << *rtspClient << "Failed to initiate the \""
						<< *scs.subsession << "\" subsession: "
						<< env.getResultMsg() << "\n";
				setupNextSubsession(rtspClient); // give up on this subsession; go to the next one
			} else {
				env << *rtspClient << "Initiated the \"" << *scs.subsession
						<< "\" subsession (";
				if (scs.subsession->rtcpIsMuxed()) {
					env << "client port " << scs.subsession->clientPortNum();
				} else {
					env << "client ports " << scs.subsession->clientPortNum()
							<< "-" << scs.subsession->clientPortNum() + 1;
				}
				env << ")\n";

				// Continue setting up this subsession, by sending a RTSP "SETUP" command:
				rtspClient->sendSetupCommand(*scs.subsession,
						continueAfterSETUP, False, REQUEST_STREAMING_OVER_TCP);
			}
		}

		return;
	}

	// We've finished setting up all of the subsessions.  Now, send a RTSP "PLAY" command to start the streaming:
	if (scs.session->absStartTime() != NULL) {
		// Special case: The stream is indexed by 'absolute' time, so send an appropriate "PLAY" command:
		rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY,
				scs.session->absStartTime(), scs.session->absEndTime());
	} else {
		scs.duration = scs.session->playEndTime()
				- scs.session->playStartTime();
		rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY);
	}
}

void continueAfterSETUP(RTSPClient *rtspClient, int resultCode,
		char *resultString) {
	do {
		UsageEnvironment &env = rtspClient->envir(); // alias
		StreamClientState &scs = ((ourRTSPClient*) rtspClient)->scs; // alias

		if (resultCode != 0) {
			env << *rtspClient << "Failed to set up the \"" << *scs.subsession
					<< "\" subsession: " << resultString << "\n";
			break;
		}

		env << *rtspClient << "Set up the \"" << *scs.subsession
				<< "\" subsession (";
		if (scs.subsession->rtcpIsMuxed()) {
			env << "client port " << scs.subsession->clientPortNum();
		} else {
			env << "client ports " << scs.subsession->clientPortNum() << "-"
					<< scs.subsession->clientPortNum() + 1;
		}
		env << ")\n";

		// Having successfully setup the subsession, create a data sink for it, and call "startPlaying()" on it.
		// (This will prepare the data sink to receive data; the actual flow of data from the client won't start happening until later,
		// after we've sent a RTSP "PLAY" command.)

		scs.subsession->sink = DummySink::createNew(env, *scs.subsession,
				rtspClient->url());
		// perhaps use your own custom "MediaSink" subclass instead
		if (scs.subsession->sink == NULL) {
			env << *rtspClient << "Failed to create a data sink for the \""
					<< *scs.subsession << "\" subsession: "
					<< env.getResultMsg() << "\n";
			break;
		}
		env << *rtspClient << "Created a data sink for the \""
				<< *scs.subsession << "\" subsession\n";
		scs.subsession->miscPtr = rtspClient; // a hack to let subsession handler functions get the "RTSPClient" from the subsession
		scs.subsession->sink->startPlaying(*(scs.subsession->readSource()),
				subsessionAfterPlaying, scs.subsession);
		// Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
		if (scs.subsession->rtcpInstance() != NULL) {
			scs.subsession->rtcpInstance()->setByeHandler(subsessionByeHandler,
					scs.subsession);
		}
	} while (0);
	delete[] resultString;

	// Set up the next subsession, if any:
	setupNextSubsession(rtspClient);
}

void continueAfterPLAY(RTSPClient *rtspClient, int resultCode,
		char *resultString) {
	Boolean success = False;

	do {
		UsageEnvironment &env = rtspClient->envir(); // alias
		StreamClientState &scs = ((ourRTSPClient*) rtspClient)->scs; // alias

		if (resultCode != 0) {
			env << *rtspClient << "Failed to start playing session: "
					<< resultString << "\n";
			break;
		}

		// Set a timer to be handled at the end of the stream's expected duration (if the stream does not already signal its end
		// using a RTCP "BYE").  This is optional.  If, instead, you want to keep the stream active - e.g., so you can later
		// 'seek' back within it and do another RTSP "PLAY" - then you can omit this code.
		// (Alternatively, if you don't want to receive the entire stream, you could set this timer for some shorter value.)
		if (scs.duration > 0) {
			unsigned const delaySlop = 2; // number of seconds extra to delay, after the stream's expected duration.  (This is optional.)
			scs.duration += delaySlop;
			unsigned uSecsToDelay = (unsigned) (scs.duration * 1000000);
			scs.streamTimerTask = env.taskScheduler().scheduleDelayedTask(
					uSecsToDelay, (TaskFunc*) streamTimerHandler, rtspClient);
		}

		env << *rtspClient << "Started playing session";
		if (scs.duration > 0) {
			scs.duration = 0;
			env << " (for up to " << scs.duration << " seconds)";
		}
		env << "...\n";

		success = True;
	} while (0);
	delete[] resultString;

	if (!success) {
		// An unrecoverable error occurred with this stream.
		shutdownStream(rtspClient);
	}
}

// Implementation of the other event handlers:

void subsessionAfterPlaying(void *clientData) {
	MediaSubsession *subsession = (MediaSubsession*) clientData;
	RTSPClient *rtspClient = (RTSPClient*) (subsession->miscPtr);

	// Begin by closing this subsession's stream:
	Medium::close(subsession->sink);
	subsession->sink = NULL;

	// Next, check whether *all* subsessions' streams have now been closed:
	MediaSession &session = subsession->parentSession();
	MediaSubsessionIterator iter(session);
	while ((subsession = iter.next()) != NULL) {
		if (subsession->sink != NULL)
			return; // this subsession is still active
	}

	// All subsessions' streams have now been closed, so shutdown the client:
	shutdownStream(rtspClient);
}

void subsessionByeHandler(void *clientData) {
	MediaSubsession *subsession = (MediaSubsession*) clientData;
	RTSPClient *rtspClient = (RTSPClient*) subsession->miscPtr;
	UsageEnvironment &env = rtspClient->envir(); // alias

	env << *rtspClient << "Received RTCP \"BYE\" on \"" << *subsession
			<< "\" subsession\n";

	// Now act as if the subsession had closed:
	subsessionAfterPlaying(subsession);
}

void streamTimerHandler(void *clientData) {
	ourRTSPClient *rtspClient = (ourRTSPClient*) clientData;
	StreamClientState &scs = rtspClient->scs; // alias

	scs.streamTimerTask = NULL;

	// Shut down the stream:
	shutdownStream(rtspClient);
}

void shutdownStream(RTSPClient *rtspClient, int exitCode) {
	if (rtspClient == NULL) {
		return;
	}

	UsageEnvironment &env = rtspClient->envir(); // alias
	StreamClientState &scs = ((ourRTSPClient*) rtspClient)->scs; // alias

	// First, check whether any subsessions have still to be closed:
	if (scs.session != NULL) {
		Boolean someSubsessionsWereActive = False;
		MediaSubsessionIterator iter(*scs.session);
		MediaSubsession *subsession;

		while ((subsession = iter.next()) != NULL) {
			if (subsession->sink != NULL) {
				Medium::close(subsession->sink);
				subsession->sink = NULL;

				if (subsession->rtcpInstance() != NULL) {
					subsession->rtcpInstance()->setByeHandler(NULL, NULL); // in case the server sends a RTCP "BYE" while handling "TEARDOWN"
				}

				someSubsessionsWereActive = True;
			}
		}

		if (someSubsessionsWereActive) {
			// Send a RTSP "TEARDOWN" command, to tell the server to shutdown the stream.
			// Don't bother handling the response to the "TEARDOWN".
			rtspClient->sendTeardownCommand(*scs.session, NULL);
		}
	}

	env << *rtspClient << "0x" << rtspClient << "Closing the stream.\n";
	Medium::close(rtspClient);
	// Note that this will also cause this stream's "StreamClientState" structure to get reclaimed.
//	if (--rtspClientCount == 0) {
//		// The final stream has ended, so exit the application now.
//		// (Of course, if you're embedding this code into your own application, you might want to comment this out,
//		// and replace it with "eventLoopWatchVariable = 1;", so that we leave the LIVE555 event loop, and continue running "main()".)
//		exit(exitCode);
//	}

}



void destoryStream(GeDuRtspHandle *handle);
GeDuRtspHandle* GeDuRtspCreate(char const *userAgent, char const *rtspURL,
		ExitNotify *func, AccessUnitNotify *func2, void *obj) {
	GeDuRtspHandle *handle = NULL;
	do {
		handle = new GeDuRtspHandle();
		if (handle == NULL) {
			goto err;
		}

		handle->scheduler = BasicTaskScheduler::createNew();
		if (handle->scheduler == NULL) {
			goto err;
		}

		handle->env = BasicUsageEnvironment::createNew2(
				*(TaskScheduler*) handle->scheduler, handle);
		if (handle->env == NULL) {
			goto err;
		}

		handle->rtsp = openURL(*(UsageEnvironment*) handle->env, userAgent,
				rtspURL);
		if (handle->rtsp == NULL) {
			goto err;
		}
		handle->object = obj;
		handle->exit_func = func;
		handle->access_unit_notify = func2;
	} while (0);
	return handle;
	err: if (handle != NULL) {
		if (handle->rtsp != NULL) {
			destoryStream(handle);
		}
		if (handle->scheduler != NULL) {
			delete (TaskScheduler*) handle->scheduler;
			handle->scheduler = NULL;
		}
		if (handle->env != NULL) {
			((UsageEnvironment*) handle->env)->reclaim();
			handle->env = NULL;
		}

		delete handle;
	}
	return NULL;
}

void destoryStream(GeDuRtspHandle *handle) {
	RTSPClient *rtspClient = (RTSPClient*) handle->rtsp;
	ExitNotify *func = (ExitNotify*) handle->exit_func;
	shutdownStream(rtspClient);
		if (--rtspClientCount == 0) {
		// The final stream has ended, so exit the application now.
		// (Of course, if you're embedding this code into your own application, you might want to comment this out,
		// and replace it with "eventLoopWatchVariable = 1;", so that we leave the LIVE555 event loop, and continue running "main()".)
		if (func != NULL) {
			func(handle->object, 1);
		}
	}
}

void GeDuRtspEventLoop(GeDuRtspHandle *handle, volatile char *watchVariable) {
	((UsageEnvironment*) handle->env)->taskScheduler().doEventLoop(
			watchVariable);

}

void GeDuRtspDestory(GeDuRtspHandle *handle) {
	char *willBeDestoryed = "yes";
	((UsageEnvironment*) handle->env)->taskScheduler().doEventLoop(
			willBeDestoryed);

	if (handle->rtsp != NULL) {
		destoryStream(handle);
	}
	if (handle->scheduler != NULL) {
		delete (TaskScheduler*) handle->scheduler;
		handle->scheduler = NULL;
	}
	if (handle->env != NULL) {
		((UsageEnvironment*) handle->env)->reclaim();
		handle->env = NULL;
	}
	delete handle;
}

// Implementation of "ourRTSPClient":

ourRTSPClient* ourRTSPClient::createNew(UsageEnvironment &env,
		char const *rtspURL, int verbosityLevel, char const *applicationName,
		portNumBits tunnelOverHTTPPortNum) {
	return new ourRTSPClient(env, rtspURL, verbosityLevel, applicationName,
			tunnelOverHTTPPortNum);
}

ourRTSPClient::ourRTSPClient(UsageEnvironment &env, char const *rtspURL,
		int verbosityLevel, char const *applicationName,
		portNumBits tunnelOverHTTPPortNum) :
		RTSPClient(env, rtspURL, verbosityLevel, applicationName,
				tunnelOverHTTPPortNum, -1) {
}

ourRTSPClient::~ourRTSPClient() {
}

// Implementation of "StreamClientState":

StreamClientState::StreamClientState() :
		iter(NULL), session(NULL), subsession(NULL), streamTimerTask(NULL), duration(
				0.0) {
}

StreamClientState::~StreamClientState() {
	delete iter;
	if (session != NULL) {
		// We also need to delete "session", and unschedule "streamTimerTask" (if set)
		UsageEnvironment &env = session->envir(); // alias

		env.taskScheduler().unscheduleDelayedTask(streamTimerTask);
		Medium::close(session);
	}
}

// Implementation of "DummySink":

// Even though we're not going to be doing anything with the incoming data, we still need to receive it.
// Define the size of the buffer that we'll use:
#define DUMMY_SINK_RECEIVE_BUFFER_SIZE 100000
#define DUMMY_SINK_FRAME_BUFFER_SIZE 2000000

DummySink* DummySink::createNew(UsageEnvironment &env,
		MediaSubsession &subsession, char const *streamId) {
	return new DummySink(env, subsession, streamId);
}

DummySink::DummySink(UsageEnvironment &env, MediaSubsession &subsession,
		char const *streamId) :
		MediaSink(env), fSubsession(subsession) {
	fStreamId = strDup(streamId);
	fSpecificData = new SpecificData("av-spec");

	fReceiveBuffer = new unsigned char[DUMMY_SINK_RECEIVE_BUFFER_SIZE];
	fFrameBuffer = new unsigned char[DUMMY_SINK_FRAME_BUFFER_SIZE];
	fReceiveBuffer += FRAME_BUFFER_OFFSET;
#ifdef SAVE_AAC
	bitWriter = new BitWriter(fReceiveBuffer-7, 7);
#endif
	fTVFrame.tv_sec = 0;
	fTVFrame.tv_usec = 0;
	fBufferOffset = 0;


	if (strcmp(fSubsession.mediumName(), "video") == 0) {
		//视频时候获取sps/pps
		// Begin by Base-64 decoding the "sprop" parameter sets strings:
		char *psets = strDup(fSubsession.fmtp_spropparametersets());
		if (psets != NULL) {
			char value[128];
			size_t comma_pos = strcspn(psets, ",");
			psets[comma_pos] = '\0';
			char const *sps_b64 = psets;
			char const *pps_b64 = &psets[comma_pos + 1];
			unsigned sps_count;
			unsigned pps_count;
			unsigned char* sps = base64Decode(sps_b64, sps_count, false);
			snprintf(value, sps_count, "%s", sps);
			delete sps;
			printf("sps %s\n", value);
			fSpecificData->setSpecificItem("sps", value);
			unsigned char* pps = base64Decode(pps_b64, pps_count, false);
			snprintf(value, pps_count, "%s", pps);
			delete pps;
			printf("pps %s\n", value);
			fSpecificData->setSpecificItem("pps", value);
			delete psets;
		}
	} else if (strcmp(fSubsession.mediumName(), "audio") == 0) {
		//音频时候获取采样率采样通道
		  char *sdpString = (char *)(fSubsession.savedSDPLines());
		  char *rtpmap = strstr(sdpString, "a=rtpmap:");
		  if (rtpmap != NULL) {
			  char *start = NULL;
			  char *current = (char *)(rtpmap + 9);
			  while (*current != '\n') {
				  if (isalpha(*current) && (start == NULL)) {
					  start = current;
				  }
				  current++;
			  }

			  char audioAttr[32] = {0};
			  memcpy(audioAttr, start, current - start);
			  current = audioAttr;
			  start = current;
			  char mpeg[16] = {0};
			  int sample = 0;
			  int channel = 1;

			  while (*current != 0) {
				  if (*current == '/') {
					  if (mpeg[0] == 0) {
						  memcpy(mpeg, start, current-start);
						  start = current+1;
					  } else if (sample == 0) {
						  sample = atoi(start);
						  start = current+1;
						  channel = atoi(start);
					  }
				  }
				  current++;
			  }

			  char value[16];
			  sprintf(value, "%d", sample);
			  printf("sample %s\n", value);
			  fSpecificData->setSpecificItem("sampleRate", value);
			  sprintf(value, "%d", channel);
			  printf("channel %s\n", value);
			  fSpecificData->setSpecificItem("channels", value);

		  }

	}

}

DummySink::~DummySink() {
	delete[] (fReceiveBuffer - FRAME_BUFFER_OFFSET);
	delete[] fFrameBuffer;
	delete fStreamId;
	delete fSpecificData;
}

void DummySink::afterGettingFrame(void *clientData, unsigned frameSize,
		unsigned numTruncatedBytes, struct timeval presentationTime,
		unsigned durationInMicroseconds) {
	DummySink *sink = (DummySink*) clientData;
	sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime,
			durationInMicroseconds);
}

// If you don't want to see debugging output for each received frame, then comment out the following line:
//#define DEBUG_PRINT_EACH_RECEIVED_FRAME 0
int DummySink::getSamplingFrequencyIndex(int samplerate) {
	int idx = 3;
	switch(samplerate) {
	case 96000: idx = 0; break;
	case 88200: idx = 0x1; break;
	case 64000: idx = 0x2; break;
	case 48000: idx = 0x3; break;
	case 44100: idx = 0x4; break;
	case 32000: idx = 0x5; break;
	case 24000: idx = 0x6; break;
	case 22050: idx = 0x7; break;
	case 16000: idx = 0x8; break;
	case 12000: idx = 0x9; break;
	case 11024: idx = 0xa; break;
	case 8000: idx = 0xb; break;
	case 7350: idx = 0xc; break;
	default:
		printf("no invalid sampling frequency idx. use 48000hz\n");
		break;
	}
	return idx;
}
void DummySink::afterGettingFrame(unsigned frameSize,
		unsigned numTruncatedBytes, struct timeval presentationTime,
		unsigned /*durationInMicroseconds*/) {

	// We've just received a frame of data.  (Optionally) print out information about it:
#ifdef DEBUG_PRINT_EACH_RECEIVED_FRAME
  if (fStreamId != NULL) envir() << "Stream \"" << fStreamId << "\"; ";
  envir() << fSubsession.mediumName() << "/" << fSubsession.codecName() << ":\tReceived " << frameSize << " bytes";
  if (numTruncatedBytes > 0) envir() << " (with " << numTruncatedBytes << " bytes truncated)";
  char uSecsStr[6+1]; // used to output the 'microseconds' part of the presentation time
  sprintf(uSecsStr, "%06u", (unsigned)presentationTime.tv_usec);
  envir() << ".\tPresentation time: " << (int)presentationTime.tv_sec << "." << uSecsStr;
  if (fSubsession.rtpSource() != NULL && !fSubsession.rtpSource()->hasBeenSynchronizedUsingRTCP()) {
    envir() << "!"; // mark the debugging output to indicate that this presentation time is not RTCP-synchronized
  }
#ifdef DEBUG_PRINT_NPT
  envir() << "\tNPT: " << fSubsession.getNormalPlayTime(presentationTime);
#endif
  envir() << "\n";
#endif

	long long ts = presentationTime.tv_sec * 1000000
			+ presentationTime.tv_usec;
	AccessUnitNotify * accessUnitNotify = ((GeDuRtspHandle*)envir().userObject())->access_unit_notify;
	void *obect = ((GeDuRtspHandle*)envir().userObject())->object;
	if (strcmp(fSubsession.codecName(), "H264") == 0) {
		if ((fTVFrame.tv_sec == 0) && (fTVFrame.tv_usec == 0)) {
			fTVFrame.tv_sec = presentationTime.tv_sec;
			fTVFrame.tv_usec = presentationTime.tv_usec;
			if ((fSPS != NULL) && (NULL != fSPS)) {
				int offset = 0;
				fSPS = fSpecificData->getSpcificItem("sps", NULL);
				fPPS = fSpecificData->getSpcificItem("pps", NULL);
				memcpy(fFrameBuffer + offset, &startCode, 4);
				offset += 4;
				memcpy(fFrameBuffer + offset, fSPS, fSPSSize);
				offset += fSPSSize;
				memcpy(fFrameBuffer + offset, &startCode, 4);
				offset += 4;
				memcpy(fFrameBuffer + offset, fPPS, fPPSSize);
				offset += fPPSSize;
				fBufferOffset += offset;
			}
		}
		if ((fTVFrame.tv_sec == presentationTime.tv_sec)
				&& (presentationTime.tv_usec == fTVFrame.tv_usec)) {
			memcpy(fFrameBuffer + fBufferOffset, &startCode, 4);
			memcpy(fFrameBuffer + fBufferOffset + 4, fReceiveBuffer, frameSize);
			fBufferOffset += (frameSize + 4);
		} else {
			accessUnitNotify(fBufferOffset, ts, (char*) fFrameBuffer, obect, 1); //latency one frame.
			fBufferOffset = 0;
			fTVFrame.tv_sec = presentationTime.tv_sec;
			fTVFrame.tv_usec = presentationTime.tv_usec;
			memcpy(fFrameBuffer + fBufferOffset, &startCode, 4);
			memcpy(fFrameBuffer + fBufferOffset + 4, fReceiveBuffer, frameSize);
			fBufferOffset += (frameSize + 4);



		}
	} else {
#ifdef SAVE_AAC
		printf("fBufferOffset %d\n", frameSize);
/*
		int DummySink::getSamplingFrequencyIndex(int samplerate) {
			int idx = 3;
			switch(samplerate) {
			case 96000: idx = 0; break;
			case 88200: idx = 0x1; break;
			case 64000: idx = 0x2; break;
			case 48000: idx = 0x3; break;
			case 44100: idx = 0x4; break;
			case 32000: idx = 0x5; break;
			case 24000: idx = 0x6; break;
			case 22050: idx = 0x7; break;
			case 16000: idx = 0x8; break;
			case 12000: idx = 0x9; break;
			case 11024: idx = 0xa; break;
			case 8000: idx = 0xb; break;
			case 7350: idx = 0xc; break;
			default:
				printf("no invalid sampling frequency idx. use 48000hz\n");
				break;
			}
			return idx;
		}
*/
		//save to a aac file.
		bitWriter->Reset();
		/* adts_fixed_header */
		bitWriter->Put(12, 0xfff);	/* syncword */
		bitWriter->Put(1, 0);		/* ID */
		bitWriter->Put(2, 0);		/* layer */
		bitWriter->Put(1, 1);		/* protection_absent */
		bitWriter->Put(2, 1);	/* profile_objecttype */
		bitWriter->Put(4, getSamplingFrequencyIndex(24000));
		bitWriter->Put(1, 0);		/* private_bit */
		bitWriter->Put(3, 2); 	/* channel_configuration */
		bitWriter->Put(1, 0);		/* original_copy */
		bitWriter->Put(1, 0);		/* home */

		/* adts_variable_header */
		bitWriter->Put(1, 0);		/* copyright_identification_bit */
		bitWriter->Put(1, 0);/* copyright_identification_start */
		bitWriter->Put(13, 7 + frameSize);	/* aac_frame_length */
		bitWriter->Put(11, 0x7ff);	/* adts_buffer_fullness */
		bitWriter->Put(2, 0);		/* number_of_raw_data_blocks_in_frame */
		int len = bitWriter->Flush();

//		FILE *fp = fopen("./abc.aac", "a+");
//		fwrite((char *)fReceiveBuffer -7, 1, frameSize + 7, fp);
//		fclose(fp);
#endif
		accessUnitNotify(frameSize+7, ts, (char*) fReceiveBuffer-7, obect, 0); //latency one frame.
	}
	// Then continue, to request the next frame of data:
	continuePlaying();
}

Boolean DummySink::continuePlaying() {
	if (fSource == NULL)
		return False; // sanity check (should not happen)

	// Request the next frame of data from our input source.  "afterGettingFrame()" will get called later, when it arrives:
	fSource->getNextFrame(fReceiveBuffer, DUMMY_SINK_RECEIVE_BUFFER_SIZE,
			afterGettingFrame, this, onSourceClosure, this);
	return True;
}
//#include "AACDecoder.h"
//#include "H264Decoder.h"
//AACDecoder *aacDecoder = NULL;
//H264Decoder *h264Decoder = NULL;
//void ExitNotify_(void* clientData, bool exit) {
//
//}
//void AccessUnitNotify_(int size, long long ts, char *buffer, void *object, int type) {
//	if(type == 0) {
//		if (aacDecoder == NULL) {
//			aacDecoder = new AACDecoder();
//			aacDecoder->initAACDecoder(2, 48000);
//		}
//		char outBuf[9600];
//		//aacDecoder->decode(buffer, (unsigned long int)size, outBuf);
//	} else {
//		if (h264Decoder == NULL) {
//			h264Decoder = new H264Decoder();
//		}
//		printf("h264Decoder %p\n", h264Decoder);
//		h264Decoder->Decode((unsigned char *)buffer, size);
//		printf("w %d h %d\n", h264Decoder->GetHeight(), h264Decoder->GetHeight());
//	}
//}
//int main() {
//	GeDuRtspHandle *handle = GeDuRtspCreate("testRtspClient", "rtsp://192.168.1.100:8554/a.mkv",
//			ExitNotify_, AccessUnitNotify_, NULL);
//	char running = false;
//	GeDuRtspEventLoop(handle, &running);
//
//	while(1) {
//		sleep(1);
//	}
//
//	GeDuRtspDestory(handle);
//
//}
