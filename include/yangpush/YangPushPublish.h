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
	/**
	 * @brief 将编码后的数据缓冲区传递到RTC网络层
	 * @param rtcPub RTC网络层实例
	 */
	void setRtcNetBuffer(YangRtcPublish* rtcPub);
    
	/**
	 * @brief 将编码后的数据缓冲区传递给实时本地预览
	 * @return 预编码后的数据缓冲区
	 */
	YangVideoBuffer* getPreVideoBuffer();

	void startAudioEncoding();
	void startVideoEncoding();

	int32_t startAudioCapture();
	int32_t startVideoCapture();

	void stopAudioCaptureState();
	void stopVideoCaptureState();
	
private:
	YangContext* m_context;

	YangPushEncoder* m_encoder;
	YangPushCapture* m_capture;

	bool isAudioEncode;
	bool isVideoEncode;

	bool isAudioCapture;
	bool isVideoCapture;
};

#endif // YangPushPublish_H
