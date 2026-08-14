
//
// Copyright (c) 2019-2025 yanggaofeng
//
#ifndef YANGAPP_YangPushCapture_H_
#define YANGAPP_YangPushCapture_H_
#include <yangutil/yangtype.h>
#include <yangaudiodev/YangAudioCapture.h>
#include <yangutil/sys/YangThread2.h>
#include <yangcapture/YangMultiVideoCapture.h>
#include <yangutil/buffer/YangAudioBuffer.h>
#include <yangutil/buffer/YangVideoBuffer.h>
#include <yangavutil/audio/YangRtcAec.h>
#include <yangutil/buffer/YangAudioPlayBuffer.h>
class YangPushCapture : public YangThread {
public:
	YangPushCapture(YangContext* context);
	virtual ~YangPushCapture();

public:
    virtual void stop();

protected:
	virtual void run() {}

public:
    int32_t initVideo();
	int32_t initAudio(YangPreProcess* proc = NULL);

    void startCamera();
    void stopCamera();
    
	void startAudioCapture();
	void startVideoCapture();
    
	void startAudioCaptureState();
	void startVideoCaptureState();

	void stopAudioCaptureState();
	void stopVideoCaptureState();

	void setAec(YangRtcAec *paec);
	void setInAudioBuffer(vector<YangAudioPlayBuffer*> *pbuf);
    
	YangAudioBuffer* getOutAudioBuffer();
	YangVideoBuffer* getOutVideoBuffer();
	YangVideoBuffer* getPreVideoBuffer();

	YangVideoBuffer* getScreenOutVideoBuffer();
	YangVideoBuffer* getScreenPreVideoBuffer();

	void stopAll();

private:
	YangAudioCapture* m_audioCapture;
	YangMultiVideoCapture* m_videoCapture;

	YangVideoBuffer* m_out_videoBuffer;
	YangVideoBuffer* m_pre_videoBuffer;

	YangVideoBuffer* m_screen_pre_videoBuffer;
	YangVideoBuffer* m_screen_out_videoBuffer;

	YangContext* m_context;
	YangAudioBuffer* m_out_audioBuffer;

public:
    int32_t m_isStart;
	int32_t m_isConvert;
};

#endif // YANGAPP_YANGCAPTUREAPP_H_
