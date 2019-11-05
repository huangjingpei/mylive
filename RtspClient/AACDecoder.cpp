#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "AACDecoder.h"

AACDecoder::AACDecoder() :
		handle_(NULL),
		samplerate_(-1),
		channel_(-1),
		neAACDec_init_(0) {
}

AACDecoder::~AACDecoder() {
	if (handle_) {
		NeAACDecClose(handle_);
		handle_ = NULL;
	}
}


int AACDecoder::decode(char *aacBuf, unsigned long int aacBufLen, char *outBuf) {
	int res = 0;
	if (outBuf == NULL) {
		return -1;
	}
	if ((handle_) && (!neAACDec_init_)) {
		long res = NeAACDecInit(handle_, (unsigned char*) aacBuf, aacBufLen,
				&samplerate_, &channel_);
		if (res < 0) {
			printf("neAACDec cannot init.");
			return -2;
		}
		neAACDec_init_ = 1;
	}



	uint donelen = 0;
	while (donelen < aacBufLen) {
		uint framelen = getFrameLenth(aacBuf + donelen);

		if (donelen + framelen > aacBufLen) {
			break;
		}

		//decode
		NeAACDecFrameInfo info;
		void *buf = NeAACDecDecode(handle_, &info,
				(unsigned char*) aacBuf + donelen, framelen);
		if (buf && info.error == 0) {
			printf("info. bytesconsumed %d samples %d object_type %d samplerate %d channels %d\n",
					info.bytesconsumed, info.samples, info.object_type,
					info.samplerate, info.channels);
			FILE *fp = fopen("abc.pcm", "a+");
			unsigned short monoBuffer[4096];
			for(int i = 0,j = 0; i < info.samples; i+=2, j++) {
				monoBuffer[j] = (*((unsigned short*)buf+i + *(unsigned short *)buf +i +1))/2;
			}
			fwrite(monoBuffer, 1, info.samples, fp);
			fclose(fp);
		} else {
			printf("NeAACDecDecode() failed info.error %d\n", info.error);
		}

		donelen += framelen;
	}



	return 0;
}

uint AACDecoder::getFrameLenth(const char *aacHeader) const {
	uint len = *(uint*) (aacHeader + 3);
	len = ntohl(len); //Little Endian
	len = len << 6;
	len = len >> 19;
	return len;
}

int AACDecoder::initAACDecoder(unsigned char defObjectType, unsigned long defSampleRate) {
	NeAACDecConfigurationPtr conf = NULL;
	unsigned long cap = NeAACDecGetCapabilities();
	do {
		printf("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);

		handle_ = NeAACDecOpen();
		printf("%s %s %d handle_ %p\n", __FILE__, __FUNCTION__, __LINE__, handle_);

		if (handle_ == NULL) {
			goto err;
		}
		printf("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
		conf = NeAACDecGetCurrentConfiguration(handle_);
		if (conf == NULL) {
			goto err;
		}
		printf("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
		conf->defObjectType = defObjectType;
		conf->defSampleRate = defSampleRate;
		conf->outputFormat  = FAAD_FMT_16BIT;
		conf->dontUpSampleImplicitSBR = 1;

		NeAACDecSetConfiguration(handle_, conf);
		printf("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);

	} while (0);

	return 0;
	err: if (handle_) {
		NeAACDecClose(handle_);
		handle_ = NULL;
	}
	return -1;
}

void AACDecoder::destoryAACDecoder() {
	if (handle_) {
		NeAACDecClose(handle_);
		handle_ = NULL;
	}
}
