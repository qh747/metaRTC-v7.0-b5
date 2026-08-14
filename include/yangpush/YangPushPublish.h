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
    
	void setRtcNetBuffer(YangRtcPublish* prr);

	YangVideoBuffer* getPreVideoBuffer();
	YangVideoBuffer* getOutPreVideoBuffer();
	YangVideoBuffer* getOutVideoBuffer();

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
	
private:
	YangVideoBuffer* m_outVideoBuffer;
	YangVideoBuffer* m_outPreVideoBuffer;

	YangContext* m_context;

	YangPushEncoder* m_encoder;
	YangPushCapture* m_capture;

	YangVideoInfo* m_videoInfo;

	bool isStartAudioCapture;
	bool isStartVideoCapture;
	bool isStartScreenCapture;

	bool isStartAudioEncoder;
	bool isStartVideoEncoder;
};

#endif // YangPushPublish_H
