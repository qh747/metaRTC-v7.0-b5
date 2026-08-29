//
// Copyright (c) 2019-2025 yanggaofeng
//
#ifndef YANGCAPTURE_SRC_YANGAUDIOCAPTUREIMPL_H_
#define YANGCAPTURE_SRC_YANGAUDIOCAPTUREIMPL_H_
#include <yangaudiodev/YangAudioCaptureHandle.h>
#include <yangavutil/audio/YangPreProcess.h>
#include <yangaudiodev/YangAudioCapture.h>

#if Yang_OS_ANDROID
#include <yangaudiodev/android/YangAudioAndroid.h>

using namespace std;

class YangAudioCaptureAndroid: public YangAudioCapture {
public:
	YangAudioCaptureAndroid(YangAVInfo* avinfo);
	~YangAudioCaptureAndroid();

public:
	virtual int32_t init();
	virtual void setCatureState(yangbool enabled);
	virtual void setOutAudioBuffer(YangAudioBuffer* buffer);

private:
	void startLoop();
	void stopLoop();

public:
    YangAudioCaptureHandle* m_ahandle;

private:
    YangAudioAndroid* m_audioAndroid;

	int32_t m_size;
	int32_t m_loops;
	int32_t m_channel;
	uint32_t  m_sample;
};

#endif
#endif // YANGCAPTURE_SRC_YANGAUDIOCAPTUREIMPL_H_
