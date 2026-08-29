//
// Copyright (c) 2019-2025 yanggaofeng
//
#ifndef ___YangAudioCaptureHandle__
#define ___YangAudioCaptureHandle__

#include <yangutil/buffer/YangAudioBuffer.h>
#include <yangavutil/audio/YangAudioUtil.h>
#include <yangavutil/audio/YangRtcAec.h>
#include <yangutil/yangavinfotype.h>

class YangAudioCaptureHandle {
public:
    YangAudioCaptureHandle(YangAVInfo* avinfo);
	virtual ~YangAudioCaptureHandle();

public:
	void putBuffer(uint8_t* buffer, int32_t len);

	void setOutAudioBuffer(YangAudioBuffer* buffer);
	void setCaptureState(yangbool state);

private:
	yangbool m_enabled;

	YangFrame m_frame;
	YangAudioBuffer* m_buffer;
};

#endif // ___YangAudioCaptureHandle__
