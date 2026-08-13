//
// Copyright (c) 2019-2025 yanggaofeng
//
#ifndef YangPushPublish_H
#define YangPushPublish_H

#include <yangutil/buffer/YangAudioEncoderBuffer.h>
#include <yangutil/buffer/YangVideoEncoderBuffer.h>
#include <yangutil/buffer/YangAudioBuffer.h>
#include <yangutil/buffer/YangVideoBuffer.h>
#include <yangpush/YangPushCapture.h>
#include <yangpush/YangPushEncoder.h>
#include <yangpush/YangRtcPublish.h>

class YangPushPublish : public YangSendRequestCallback {
public:
	YangPushPublish(YangContext* context);
	virtual ~YangPushPublish();

public:
    virtual void sendRequest(int32_t uid, uint32_t ssrc, YangRequestType type);

public:
	void startCamera();
	void stopCamera();

	void stopAll();
    
	void setCaptureType(int pct);
	void setScreenInterval(int32_t interval);
	void setDrawmouse(bool isDraw);
	void setRtcNetBuffer(YangRtcPublish* prr);
	
    void setInAudioBuffer(std::vector<YangAudioPlayBuffer*>* pbuf);

	YangPushCapture* getPushCapture();
	YangVideoBuffer* getPreVideoBuffer();
	YangVideoBuffer* getOutPreVideoBuffer();
	YangVideoBuffer* getOutVideoBuffer();

	void startPubVideo();
	void startPubAudio();

	void initAudioEncoding();
	void initVideoEncoding();

	int32_t startAudioCapture();
	int32_t startVideoCapture();

	void startAudioEncoding();
	void startVideoEncoding();

	void deleteVideoEncoding();

	void startAudioCaptureState();
	void startVideoCaptureState();

	void stopAudioCaptureState();
	void stopVideoCaptureState();
	void stopScreenCaptureState();
	
	void change(int32_t st);
	void sendMsgToEncoder(YangRequestType req);

private:
    void stopAudioState();
	void stopVideoState();

	void initCapture();
	
private:
	YangVideoBuffer* m_outVideoBuffer;
	YangVideoBuffer* m_outPreVideoBuffer;

	YangContext* m_context;

	YangPushEncoder* m_encoder;
	YangPushCapture* m_capture;

	YangVideoInfo* m_videoInfo;

	int32_t isStartAudioCapture;
	int32_t isStartVideoCapture;
	int32_t isStartScreenCapture;

	int32_t isStartAudioEncoder;
	int32_t isStartVideoEncoder;
	
	int m_captureType;
};

#endif // YangPushPublish_H
