//
// Copyright (c) 2019-2022 yanggaofeng
//
#include <yangpush/YangPushCommon.h>
#include <yangpush/YangPushPublish.h>

YangPushPublish::YangPushPublish(YangContext* context) {
	m_context = context;
	m_context->streams->setSendRequestCallback(this);

	m_encoder = new YangPushEncoder(m_context);
	m_capture = new YangPushCapture(m_context);

	isAudioEncode = false;
	isVideoEncode = false;

	isAudioCapture = false;
	isVideoCapture = false;
}

YangPushPublish::~YangPushPublish() {
	m_context = NULL;

	yang_delete(m_encoder);
	yang_delete(m_capture);
}

void YangPushPublish::sendRequest(int32_t uid, uint32_t ssrc, YangRequestType type) {
	if (type < Yang_Req_Connected && m_encoder) {
		m_encoder->sendMsgToEncoder(type);
	}
}

int32_t YangPushPublish::startAudioCapture() {
	if (isAudioCapture) {
		return Yang_Ok;
	}

	int32_t err = m_capture->initAudio();

	if(err != Yang_Ok) {
		return yang_error_wrap(err, "init audio capture fail");
	}

	m_capture->startAudioCapture();

	isAudioCapture = true;
	return err;
}

int32_t YangPushPublish::startVideoCapture() {
	if (isVideoCapture) {
		return Yang_Ok;
	}

	int32_t err = m_capture->initVideo();

	if(err != Yang_Ok) {
		return yang_error_wrap(err, "init video capture fail");
	}
	
	m_capture->startVideoCapture();

	isVideoCapture = true;
	return err;
}

void YangPushPublish::startAudioEncoding() {
	if (isAudioEncode) {
		return;
	}

	m_encoder->initAudioEncoder();
	m_encoder->setInAudioBuffer(m_capture->getOutAudioBuffer());
	m_encoder->startAudioEncoder();

	m_capture->startAudioCaptureState();
	isAudioEncode = true;
}

void YangPushPublish::startVideoEncoding() {
	if (isVideoEncode) {
		return;
	}

	m_encoder->initVideoEncoder();
	m_encoder->setInVideoBuffer(m_capture->getOutVideoBuffer());
	m_encoder->startVideoEncoder();

	m_capture->startVideoCaptureState();
	isVideoEncode = true;
}

void YangPushPublish::setRtcNetBuffer(YangRtcPublish* rtcPub) {
	yang_reindex(m_encoder->getOutAudioBuffer());
	yang_reindex(m_encoder->getOutVideoBuffer());

	rtcPub->setInAudioList(m_encoder->getOutAudioBuffer());
	rtcPub->setInVideoList(m_encoder->getOutVideoBuffer());
	
	rtcPub->setInVideoMetaData(m_encoder->getOutVideoMetaData());
}

YangVideoBuffer* YangPushPublish::getPreVideoBuffer(){
	return m_capture ? m_capture->getPreVideoBuffer() : NULL;
}

void YangPushPublish::stopAudioCaptureState() {
	m_capture->stopAudioCaptureState();
}
void YangPushPublish::stopVideoCaptureState() {
	m_capture->stopVideoCaptureState();
}
