//
// Copyright (c) 2019-2022 yanggaofeng
//
#include <yangpush/YangPushHandleImpl.h>
#include <yangutil/sys/YangLog.h>
#include <yangutil/sys/YangUrl.h>

YangPushHandleImpl::YangPushHandleImpl(
	bool hasAudio,
	bool initVideo,
	int videoType,
	YangVideoInfo* screenVideo,
	YangVideoInfo* outVideo,
	YangContext* context,
	YangSysMessageI* message) {

	m_rtcPub = NULL;
	m_videoType = videoType;
	m_screenInfo = screenVideo;
	m_outInfo = outVideo;
	m_context = context;
	m_message = message;

	m_cap = new YangPushPublish(m_context);
	m_cap->setCaptureType(m_videoType);

	m_hasAudio = hasAudio;
	m_isInit = initVideo;

	this->init();

	m_send = NULL;
}

YangPushHandleImpl::~YangPushHandleImpl() {
	if (m_rtcPub) {
		m_rtcPub->disConnectMediaServer();
	}

	if (m_cap) {
		m_cap->stopAll();
	}

	yang_delete(m_rtcPub);
	yang_delete(m_cap);
}

void YangPushHandleImpl::disconnect() {
	if (m_cap) {
		if(m_hasAudio) m_cap->stopAudioCaptureState();
		m_cap->stopVideoCaptureState();
		m_cap->stopScreenCaptureState();
	}
	stopPublish();
}
void YangPushHandleImpl::init() {
	if(m_isInit) return;
	changeSrc(m_videoType,true);
	m_isInit=true;
}
void YangPushHandleImpl::startCapture() {

}
YangSendVideoI* YangPushHandleImpl::getSendVideo(){
	if(m_send==NULL&&m_videoType==Yang_VideoSrc_OutInterface) {
		m_send=new YangSendVideoImpl();
		if(m_cap){
			m_send->m_outVideoBuffer=m_cap->getOutVideoBuffer();
			m_send->m_outPreVideoBuffer=m_cap->getOutPreVideoBuffer();
		}
	}
	return m_send;
}
void YangPushHandleImpl::switchToCamera(bool pisinit) {
	m_videoType = Yang_VideoSrc_Camera;
	if(m_cap) m_cap->setCaptureType(m_videoType);
	if(m_cap) m_cap->setVideoInfo(&m_context->avinfo.video);
	//if(!pisinit) stopScreen();

	startCamera();
}



void YangPushHandleImpl::switchToOutside(bool pisinit){
	if(m_cap) m_cap->setCaptureType(m_videoType);
	if(m_cap) m_cap->setVideoInfo(m_outInfo);

}
void YangPushHandleImpl::changeSrc(int videoSrcType,bool pisinit){
	m_videoType=videoSrcType;
	if(m_videoType==Yang_VideoSrc_Camera){
			switchToCamera(pisinit);
		}
}

void YangPushHandleImpl::stopPublish() {
	if (m_rtcPub) {
		m_rtcPub->disConnectMediaServer();
	}
	yang_stop(m_rtcPub);
	yang_stop_thread(m_rtcPub);
	yang_delete(m_rtcPub);
	if(m_cap) m_cap->deleteVideoEncoding();
}
YangVideoBuffer* YangPushHandleImpl::getPreVideoBuffer() {
	if (m_videoType == Yang_VideoSrc_Camera) {
		if (m_cap)	return m_cap->getPreVideoBuffer();
	}
	return NULL;
}




int YangPushHandleImpl::publish(char* url,yangbool isWhip) {

	int err = Yang_Ok;
	memset(&m_url,0,sizeof(m_url));
    if(!isWhip){
        if(yang_url_parse(m_context->avinfo.sys.familyType,url, &m_url)==Yang_Ok){
            yang_trace("\nnetType==%d,server=%s,port=%d,app=%s,stream=%s\n",
                    m_url.netType, m_url.server, m_url.port, m_url.app,
                    m_url.stream);

        }else{
            return 1;
        }
    }
	m_context->avinfo.sys.transType=m_url.netType;
	 m_context->avinfo.audio.audioEncoderType=Yang_AED_OPUS;
	 m_context->avinfo.audio.sample=48000;


	stopPublish();
	yang_trace("\nnetType==%d,server=%s,port=%d,app=%s,stream=%s\n",
			m_url.netType, m_url.server, m_url.port, m_url.app,
			m_url.stream);

	if (m_rtcPub == NULL) {
		m_rtcPub = new YangRtcPublish(m_context);

	}

	  if(m_hasAudio) {
	        m_hasAudio=bool(m_cap->startAudioCapture()==Yang_Ok);
	    }
	if (m_hasAudio) {

			m_cap->initAudioEncoding();
		}

		m_cap->initVideoEncoding();
		m_cap->setRtcNetBuffer(m_rtcPub);

		if (m_hasAudio)
			m_cap->startAudioEncoding();
		m_cap->startVideoEncoding();

        if(isWhip){
             err = m_rtcPub->init(url,yangtrue);

        }else{
            err = m_rtcPub->init(url,yangfalse) ;
        }

            if (err != Yang_Ok)
                    return yang_error_wrap(err, " connect server failure!");
	m_rtcPub->start();
	if (m_hasAudio)
		m_cap->startAudioCaptureState();
	if (m_videoType == Yang_VideoSrc_Camera)
		m_cap->startVideoCaptureState();

	return err;

}

void YangPushHandleImpl::startCamera() {
	if(m_cap) m_cap->startCamera();
}


void YangPushHandleImpl::stopCamera() {
	if(m_cap) m_cap->stopCamera();
}



