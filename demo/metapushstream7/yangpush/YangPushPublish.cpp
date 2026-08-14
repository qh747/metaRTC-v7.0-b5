//
// Copyright (c) 2019-2022 yanggaofeng
//
#include <yangpush/YangPushCommon.h>
#include <yangpush/YangPushPublish.h>

YangPushPublish::YangPushPublish(YangContext* context) {
	m_context = context;
	m_context->streams->setSendRequestCallback(this);

	m_videoInfo = &context->avinfo.video;

	m_encoder = NULL;
	m_capture = NULL;

	m_outPreVideoBuffer = NULL;
	m_outVideoBuffer = NULL;

	isStartAudioCapture  = false;
	isStartVideoCapture  = false;
	isStartScreenCapture = false;
	
	isStartAudioEncoder = false;
	isStartVideoEncoder = false;
}

YangPushPublish::~YangPushPublish() {
	this->stopAll();

	m_context = NULL;

	yang_delete(m_encoder);
	yang_delete(m_capture);

	yang_delete(m_outPreVideoBuffer);
	yang_delete(m_outVideoBuffer);
}

void YangPushPublish::sendRequest(int32_t uid, uint32_t ssrc, YangRequestType type) {
	if (type < Yang_Req_Connected && m_encoder) {
		m_encoder->sendMsgToEncoder(type);
	}
}

void YangPushPublish::stopAll() {
	if (m_capture) {
		m_capture->stopAll();
	}
	
	if (m_encoder) {
		m_encoder->stopAll();
	}
}

int32_t YangPushPublish::startAudioCapture() {
    if (isStartAudioCapture) {
		return Yang_Ok;
	}

	if (m_capture == NULL) {
		m_capture = new YangPushCapture(m_context);
	}

	int32_t err = m_capture->initAudio();

	if(err != Yang_Ok) {
		return yang_error_wrap(err, "init audio capture fail");
	}

	m_capture->startAudioCapture();

	isStartAudioCapture = true;
	return err;
}

int32_t YangPushPublish::startVideoCapture() {
    if (isStartVideoCapture) {
		return Yang_Ok;
	}

	if (m_capture == NULL) {
		m_capture = new YangPushCapture(m_context);
	}

	int32_t err = m_capture->initVideo();

	if(err != Yang_Ok) {
		return yang_error_wrap(err, "init video capture fail");
	}
	
	m_capture->startVideoCapture();

	isStartVideoCapture = true;
	return err;
}

void YangPushPublish::setRtcNetBuffer(YangRtcPublish* prr) {
	yang_reindex(m_encoder->getOutAudioBuffer());
	yang_reindex(m_encoder->getOutVideoBuffer());

	m_encoder->getOutVideoBuffer()->resetIndex();

	prr->setInAudioList(m_encoder->getOutAudioBuffer());
	prr->setInVideoList(m_encoder->getOutVideoBuffer());
	prr->setInVideoMetaData(m_encoder->getOutVideoMetaData());
}

void YangPushPublish::initVideoEncoding() {
	if (isStartVideoEncoder) {
		return;
	}

	isStartVideoEncoder = true;

	if (m_encoder == NULL) {
        m_encoder = new YangPushEncoder(m_context);
	}
		
	m_encoder->setVideoInfo(m_videoInfo);
	m_encoder->initVideoEncoder();
	m_encoder->setInVideoBuffer(m_capture->getOutVideoBuffer());
}

void YangPushPublish::initAudioEncoding() {
    if (isStartAudioEncoder) {
		return;
	}

	isStartAudioEncoder = true;

	if (m_encoder == NULL) {
		m_encoder = new YangPushEncoder(m_context);
	}

	m_encoder->initAudioEncoder();
	m_encoder->setInAudioBuffer(m_capture->getOutAudioBuffer());
}

void YangPushPublish::startAudioEncoding() {
	if (m_encoder) {
		m_encoder->startAudioEncoder();
	}
}

void YangPushPublish::startVideoEncoding() {
	if (m_encoder) {
		m_encoder->startVideoEncoder();
	}
}

void YangPushPublish::deleteVideoEncoding() {
	if (m_encoder) {
		m_encoder->deleteVideoEncoder();
	}

	isStartVideoEncoder = false;
}

void YangPushPublish::startAudioCaptureState() {
	if (m_capture) {
		m_capture->startAudioCaptureState();
	}
}
YangVideoBuffer* YangPushPublish::getPreVideoBuffer(){
	return m_capture ? m_capture->getPreVideoBuffer() : NULL;
}

void YangPushPublish::startVideoCaptureState() {
	if (m_capture) {
		m_capture->startVideoCaptureState();
	}
}

void YangPushPublish::stopAudioCaptureState() {
	if (m_capture) {
		m_capture->stopAudioCaptureState();
	}
}
void YangPushPublish::stopVideoCaptureState() {
	if (m_capture) {
		m_capture->stopVideoCaptureState();
	}
}

YangVideoBuffer* YangPushPublish::getOutPreVideoBuffer() {
	return m_outPreVideoBuffer;
}

YangVideoBuffer* YangPushPublish::getOutVideoBuffer() {
	 return	m_outVideoBuffer;
}

void YangPushPublish::startCamera() {
	this->startVideoCapture();
}

void YangPushPublish::stopCamera() {
	if (m_capture) {
		m_capture->stopCamera();
	}
}
