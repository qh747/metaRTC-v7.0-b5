//
// Copyright (c) 2019-2022 yanggaofeng
//
#ifndef YANGCAPTURE_SRC_YANGAUDIOCAPTUREIMPL_H_
#define YANGCAPTURE_SRC_YANGAUDIOCAPTUREIMPL_H_

#include <yangaudiodev/YangAudioCaptureHandle.h>
#include <yangaudiodev/YangAudioCapture.h>
#include <yangavutil/audio/YangPreProcess.h>

#if Yang_OS_LINUX
#include <alsa/asoundlib.h>

class YangAudioCaptureLinux : public YangAudioCapture {
public:
	YangAudioCaptureLinux(YangAVInfo* avinfo);
	~YangAudioCaptureLinux();

public:
	virtual int32_t init();

	virtual void setCatureState(yangbool enabled);
	virtual void setOutAudioBuffer(YangAudioBuffer* buffer);

private:
	void startLoop();
	void stopLoop();

private:
    YangAudioCaptureHandle* m_ahandle;
	YangAVInfo* m_avinfo;
	
	int32_t m_loops;

	snd_pcm_uframes_t m_frames;
	snd_pcm_t* m_handle;

	yangbool m_mono;
};

#endif
#endif // YANGCAPTURE_SRC_YANGAUDIOCAPTUREIMPL_H_
