//
// Copyright (c) 2019-2022 yanggaofeng
//
#include <yangpush/YangPushHandleImpl.h>
#include <yangutil/sys/YangLog.h>
#include <yangutil/sys/YangUrl.h>

YangPushHandleImpl::YangPushHandleImpl(
	bool hasAudio,
	bool initVideo,
	YangVideoInfo* screenVideo,
	YangVideoInfo* outVideo,
	YangContext* context,
	YangSysMessageI* message) {

	m_rtcPub = NULL;
	m_screenInfo = screenVideo;
	m_outInfo = outVideo;
	m_context = context;
	m_message = message;

	m_cap = new YangPushPublish(m_context);

	m_hasAudio = hasAudio;
	m_isInit = initVideo;

	this->init();
}

YangPushHandleImpl::~YangPushHandleImpl() {
	if (m_rtcPub) {
		m_rtcPub->disConnect();
	}

	m_cap->stopAll();

	yang_delete(m_rtcPub);
	yang_delete(m_cap);
}

void YangPushHandleImpl::disconnect() {
	if (m_cap) {
		if (m_hasAudio) {
			m_cap->stopAudioCaptureState();
		}
		
		m_cap->stopVideoCaptureState();
	}

	this->stopPublish();
}

void YangPushHandleImpl::init() {
	if(m_isInit) {
		return;
	}

	this->changeSrc(Yang_VideoSrc_Camera);
	m_isInit = true;
}

void YangPushHandleImpl::changeSrc(int videoType) {
	if (videoType == Yang_VideoSrc_Camera) {
		m_cap->startCamera();
	}
}

void YangPushHandleImpl::stopPublish() {
	if (m_rtcPub) {
		m_rtcPub->disConnect();
	}

	yang_stop(m_rtcPub);
	yang_stop_thread(m_rtcPub);
	yang_delete(m_rtcPub);
	
	m_cap->deleteVideoEncoding();
}

YangVideoBuffer* YangPushHandleImpl::getPreVideoBuffer() {
	return m_cap->getPreVideoBuffer();
}

int YangPushHandleImpl::publish(char* url, yangbool isWhip) {
	int err = Yang_Ok;
	memset(&m_url, 0, sizeof(m_url));

    if (!isWhip) {
        err = yang_url_parse(m_context->avinfo.sys.familyType, url,  &m_url);

		if (err != Yang_Ok) {
			return err;
		}
    }

	m_context->avinfo.sys.transType = m_url.netType;
	m_context->avinfo.audio.audioEncoderType = Yang_AED_OPUS;
	m_context->avinfo.audio.sample = 48000;

	this->stopPublish();

	yang_trace(
		"\nnetType==%d,server=%s,port=%d,app=%s,stream=%s\n",
        m_url.netType, 
		m_url.server, 
		m_url.port, 
		m_url.app,
        m_url.stream
	);

	if (m_rtcPub == NULL) {
		m_rtcPub = new YangRtcPublish(m_context);
	}

	if (m_hasAudio && m_cap->startAudioCapture() == Yang_Ok) {
	    m_cap->initAudioEncoding();
	}
	else {
		m_hasAudio = false;
	}

	m_cap->initVideoEncoding();
	m_cap->setRtcNetBuffer(m_rtcPub);

	if (m_hasAudio) {
        m_cap->startAudioEncoding();
	}
		
	m_cap->startVideoEncoding();

    err = m_rtcPub->init(url, isWhip);

    if (err != Yang_Ok) {
        return yang_error_wrap(err, " connect server failure!");
	}
                
	m_rtcPub->start();

	if (m_hasAudio) {
		m_cap->startAudioCaptureState();
	}
		
	m_cap->startVideoCaptureState();
	return err;
}

