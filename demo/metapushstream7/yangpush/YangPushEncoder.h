//
// Copyright (c) 2019-2022 yanggaofeng
//
#ifndef YANGAPP_YangPushEncoder_H_
#define YANGAPP_YangPushEncoder_H_

#include <yangutil/buffer/YangAudioBuffer.h>
#include <yangutil/buffer/YangAudioEncoderBuffer.h>
#include <yangutil/buffer/YangVideoEncoderBuffer.h>
#include <yangutil/buffer/YangVideoBuffer.h>
#include <yangencoder/YangAudioEncoderHandle.h>
#include <yangencoder/YangVideoEncoderHandle.h>

class YangPushEncoder {
public:
	YangPushEncoder(YangContext* context);
	virtual ~YangPushEncoder();

public:
	void initVideoEncoder();
	void initAudioEncoder();

	void startAudioEncoder();
	void startVideoEncoder();

	void setInAudioBuffer(YangAudioBuffer* audioBuffer);
	void setInVideoBuffer(YangVideoBuffer* videoBuffer);

	void sendMsgToEncoder(YangRequestType type);

public:
	inline YangAudioEncoderBuffer* getOutAudioBuffer() { return m_outAudioBuffer; }
	inline YangVideoEncoderBuffer* getOutVideoBuffer() { return m_outVideoBuffer; }
	inline YangVideoMeta* getOutVideoMetaData() { return m_videoMeta; }

private:
    YangContext* m_context;

	YangVideoEncoderHandle* m_videoEncoder;
	YangAudioEncoderHandle* m_audioEncoder;

	YangAudioEncoderBuffer* m_outAudioBuffer;
	YangVideoEncoderBuffer* m_outVideoBuffer;
	
	YangVideoMeta* m_videoMeta;
};

#endif // YANGAPP_YANGENCODERAPP_H_
