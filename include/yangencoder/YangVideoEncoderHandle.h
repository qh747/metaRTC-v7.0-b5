//
// Copyright (c) 2019-2025 yanggaofeng
//
#ifndef __YangVideoEncoderHandle__
#define __YangVideoEncoderHandle__

#include <yangutil/sys/YangThread2.h>
#include <yangutil/buffer/YangVideoEncoderBuffer.h>
#include <yangutil/buffer/YangVideoBuffer.h>
#include <yangencoder/YangVideoEncoder.h>

class YangVideoEncoderHandle : public YangThread, public YangEncoderCallback {
public:
	YangVideoEncoderHandle(YangContext* context, YangVideoInfo* info);
	virtual ~YangVideoEncoderHandle();

public:
	void init();
	virtual void stop();

	virtual void onVideoData(YangFrame* frame);
	virtual void onAudioData(YangFrame* frame) {}

public:
    void sendMsgToEncoder(YangRequestType type);

	inline void setOutVideoBuffer(YangVideoEncoderBuffer* buf) { m_outVideoBuffer = buf; }
	inline void setInVideoBuffer(YangVideoBuffer* buf) { m_inVideoBuffer = buf; }
	inline void setVideoMetaData(YangVideoMeta* meta) { m_meta = meta; }

protected:
    virtual void run();

public:
    int32_t m_isStart;

private:
    int32_t m_isInit;
	int32_t m_isConvert;

	YangContext* m_context;

	YangVideoBuffer* m_inVideoBuffer;
	YangVideoEncoderBuffer* m_outVideoBuffer;

	YangVideoMeta* m_meta;

	YangVideoInfo* m_info;
	
	int32_t m_hasEncMsg;
	YangRtcEncoderMessage m_encMsg;
};

#endif // __YangVideoEncoderHandle__
