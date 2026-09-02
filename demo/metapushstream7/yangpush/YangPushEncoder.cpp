//
// Copyright (c) 2019-2022 yanggaofeng
//
#include <yangpush/YangPushEncoder.h>
#include <yangencoder/YangEncoderFactory.h>

YangPushEncoder::YangPushEncoder(YangContext* context) {
	m_context = context;

	m_audioEncoder = NULL;
	m_videoEncoder = NULL;

	m_outVideoBuffer = NULL;
	m_outAudioBuffer = NULL;

	m_videoMeta = NULL;
}

YangPushEncoder::~YangPushEncoder() {
	yang_stop(m_audioEncoder);
	yang_stop(m_videoEncoder);

	yang_stop_thread(m_audioEncoder);
	yang_stop_thread(m_videoEncoder);

	yang_delete(m_audioEncoder);
	yang_delete(m_videoEncoder);

	yang_delete(m_outVideoBuffer);
	yang_delete(m_outAudioBuffer);

	yang_free(m_videoMeta);
	m_context = NULL;
}

void YangPushEncoder::initAudioEncoder() {
	if (m_outAudioBuffer == NULL) {
        m_outAudioBuffer = new YangAudioEncoderBuffer(m_context->avinfo.audio.audioCacheNum);
	}   
	
	if (m_audioEncoder == NULL) {
		m_audioEncoder = new YangAudioEncoderHandle(&m_context->avinfo.audio);

		m_audioEncoder->setOutAudioBuffer(m_outAudioBuffer);
		m_audioEncoder->init();
	}
}

void YangPushEncoder::initVideoEncoder() {
	if (m_outVideoBuffer == NULL) {
		m_outVideoBuffer = new YangVideoEncoderBuffer(m_context->avinfo.video.evideoCacheNum);
	}

	if (m_context && m_context->avinfo.enc.createMeta) {
		if (m_videoMeta == NULL) {
  		    m_videoMeta = (YangVideoMeta*) calloc(1, sizeof(YangVideoMeta));
		}

		YangVideoEncoderMeta* meta = YangEncoderFactory::CreateVideoEncoderMeta(
			&(m_context->avinfo.video)
		);

		meta->yang_initVmd(
			m_videoMeta, 
			&(m_context->avinfo.video),
			&m_context->avinfo.enc
		);

		yang_delete(meta);
	}
	
	if (m_videoEncoder == NULL) {
		m_videoEncoder = new YangVideoEncoderHandle(
			m_context,&(m_context->avinfo.video)
		);
		
		m_videoEncoder->setOutVideoBuffer(m_outVideoBuffer);
		m_videoEncoder->init();
		m_videoEncoder->setVideoMetaData(m_videoMeta);
	}
}

void YangPushEncoder::sendMsgToEncoder(YangRequestType req){
	if (m_videoEncoder) {
		m_videoEncoder->sendMsgToEncoder(req);
	}
}

void YangPushEncoder::startAudioEncoder() {
	if (m_audioEncoder && !m_audioEncoder->m_isStart) {
		m_audioEncoder->start();
		yang_usleep(1000);
	}
}

void YangPushEncoder::startVideoEncoder() {
	if (m_videoEncoder && !m_videoEncoder->m_isStart) {
		m_videoEncoder->start();
		yang_usleep(2000);
	}
}

void YangPushEncoder::setInAudioBuffer(YangAudioBuffer* buf) {
	if (m_audioEncoder != NULL) {
	    m_audioEncoder->setInAudioBuffer(buf);
	}
}

void YangPushEncoder::setInVideoBuffer(YangVideoBuffer* buf) {
	if (m_videoEncoder != NULL) {
		m_videoEncoder->setInVideoBuffer(buf);
	}
}

