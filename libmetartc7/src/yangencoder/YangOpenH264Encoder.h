//
// Copyright (c) 2019-2025 yanggaofeng
//
#ifndef __YangOpenH264Encoder__
#define __YangOpenH264Encoder__
#include <yangencoder/YangVideoEncoder.h>
#include <yangutil/buffer/YangVideoBuffer.h>
#include <yangutil/buffer/YangVideoEncoderBuffer.h>
#include <wels/codec_api.h>

class YangOpenH264Encoder: public YangVideoEncoder {
public:
	YangOpenH264Encoder();
	virtual ~YangOpenH264Encoder();

public:
	virtual int32_t init(YangContext* context, YangVideoInfo* info);

	virtual void setVideoMetaData(YangVideoMeta* meta);
	virtual void sendMsgToEncoder(YangRtcEncoderMessage* msg);

private:
	virtual int32_t encode(YangFrame* frame, YangEncoderCallback* cb);

private: 
    int32_t m_sendKeyframe;
	ISVCEncoder* m_264Handle;
};

#endif // __YangOpenH264Encoder__
