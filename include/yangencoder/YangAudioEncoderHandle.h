//
// Copyright (c) 2019-2025 yanggaofeng
//
#ifndef __YangAudioEncoderrHandle__
#define __YangAudioEncoderrHandle__

#include <yangutil/sys/YangThread2.h>
#include <yangutil/buffer/YangAudioEncoderBuffer.h>
#include <yangutil/buffer/YangAudioPlayBuffer.h>
#include <yangencoder/YangAudioEncoder.h>

class YangAudioEncoderHandle : public YangThread, public YangEncoderCallback {
public:
	YangAudioEncoderHandle(YangAudioInfo* info);
	virtual ~YangAudioEncoderHandle();

public:
	void init();

	void setInAudioBuffer(YangAudioBuffer* buf);
	void setOutAudioBuffer(YangAudioEncoderBuffer* buf);

	virtual void stop();

	virtual void onVideoData(YangFrame* frame);
	virtual void onAudioData(YangFrame* frame);
	
private:
	virtual void run();

public:
    int32_t m_isStart;

private:
	int32_t m_isConvert;

	YangAudioInfo* m_info;
	YangAudioEncoder* m_enc;
	
	YangAudioBuffer *m_in_audioBuffer;
	YangAudioEncoderBuffer *m_out_audioBuffer;
};

#endif // __YangAudioEncoderrHandle__
