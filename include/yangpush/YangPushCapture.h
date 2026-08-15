
//
// Copyright (c) 2019-2025 yanggaofeng
//
#ifndef YANGAPP_YangPushCapture_H_
#define YANGAPP_YangPushCapture_H_
#include <yangutil/yangtype.h>
#include <yangutil/sys/YangThread2.h>
#include <yangaudiodev/YangAudioCapture.h>
#include <yangcapture/YangVideoCapture.h>
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
	int32_t initAudio(YangPreProcess* preProc = NULL);

    void startCamera();
    void stopCamera();
    
	void startAudioCapture();
	void startVideoCapture();
    
	void startAudioCaptureState();
	void startVideoCaptureState();

	void stopAudioCaptureState();
	void stopVideoCaptureState();

	void setAec(YangRtcAec* aec);
	void setInAudioBuffer(vector<YangAudioPlayBuffer*> *pbuf);
    
	inline YangAudioBuffer* getOutAudioBuffer() { return m_out_audioBuffer; }
	inline YangVideoBuffer* getOutVideoBuffer() { return m_out_videoBuffer; }
	inline YangVideoBuffer* getPreVideoBuffer() { return m_pre_videoBuffer; }

	inline YangVideoBuffer* getScreenOutVideoBuffer() { return m_screen_out_videoBuffer; }
	inline YangVideoBuffer* getScreenPreVideoBuffer() { return m_screen_pre_videoBuffer; }

	void stopAll();

private:
	YangAudioCapture* m_audioCapture;
	YangVideoCapture* m_videoCapture;

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
