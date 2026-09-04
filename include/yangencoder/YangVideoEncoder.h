//
// Copyright (c) 2019-2025 yanggaofeng
//

#ifndef YANGENCODER_INCLUDE_YANGVideoENCODER_H_
#define YANGENCODER_INCLUDE_YANGVideoENCODER_H_

#include <stdint.h>
#include <yangutil/yangavinfotype.h>
#include <yangencoder/YangEncoder.h>

typedef struct{
	YangRequestType request;
	int32_t requestValue;

} YangRtcEncoderMessage;

class YangVideoEncoder {
public:
	virtual ~YangVideoEncoder() {}

public:
    virtual int32_t init(YangContext* context, YangVideoInfo* info) = 0;
	virtual int32_t encode(YangFrame* frame, YangEncoderCallback* cb) = 0;
	virtual void setVideoMetaData(YangVideoMeta* meta) = 0;
	virtual void sendMsgToEncoder(YangRtcEncoderMessage* msg) = 0;

protected:
    int32_t m_isInit;
	uint8_t* m_vbuffer;

    YangVideoInfo m_videoInfo;
};

#endif // YANGENCODER_INCLUDE_YANGENCODER_H_
