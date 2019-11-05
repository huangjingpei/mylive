#ifndef __AACDECODER_H__
#define __AACDECODER_H__

#include "faad.h"
using namespace std;


class AACDecoder {
public:
    AACDecoder();
    ~AACDecoder();
public:

    //init AAC decoder
    int initAACDecoder(unsigned char defObjectType,unsigned long defSampleRate);

    int decode(char *aacBuf, unsigned long int aacBufLen, char *outBuf);

    //destroy aac decoder
    void destoryAACDecoder();

private:
    //parse AAC ADTS header, get frame length
    unsigned int getFrameLenth(const char *aacHeader) const;

    //AAC decoder properties
    NeAACDecHandle handle_;
    unsigned long int samplerate_;
    unsigned char channel_;


    AACDecoder(const AACDecoder &dec);
    AACDecoder& operator=(const AACDecoder &rhs);

	bool neAACDec_init_;


};


#endif /*__AACDECODER_H__*/
