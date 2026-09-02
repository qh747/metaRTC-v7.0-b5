//
// Copyright (c) 2019-2022 yanggaofeng
//

#include <yangutil/sys/YangLog.h>
#include <yangutil/sys/YangUrl.h>
#include <yangutil/sys/YangUrl.h>
#include <yangpush/YangPushHandleImpl.h>

YangPushHandleImpl::YangPushHandleImpl(
	bool hasAudio,
	YangVideoInfo* outVideo,
	YangContext* context,
	YangSysMessageI* message) {

	m_outInfo = outVideo;
	m_context = context;
	m_message = message;

	m_hasAudio = hasAudio;

	m_rtcPub = NULL;
	m_pushPub = new YangPushPublish(m_context);
}

YangPushHandleImpl::~YangPushHandleImpl() {
	if (m_rtcPub) {
		m_rtcPub->disConnect();
	}

	yang_delete(m_rtcPub);
	yang_delete(m_pushPub);
}

void YangPushHandleImpl::disconnect() {
	if (m_rtcPub) {
		m_rtcPub->disConnect();

		yang_stop(m_rtcPub);
	    yang_stop_thread(m_rtcPub);
	    yang_delete(m_rtcPub);
	}

	if (m_pushPub) {
		if (m_hasAudio) {
			m_pushPub->stopAudioCaptureState();
		}
		
		m_pushPub->stopVideoCaptureState();
	}
}

void YangPushHandleImpl::init() {
	this->changeSrc(Yang_VideoSrc_Camera);
}

void YangPushHandleImpl::changeSrc(int videoType) {
	if (videoType == Yang_VideoSrc_Camera) {
		m_pushPub->startVideoCapture();
	}
}

YangVideoBuffer* YangPushHandleImpl::getPreVideoBuffer() {
	return m_pushPub->getPreVideoBuffer();
}

int YangPushHandleImpl::publish(const char* url, yangbool isWhip) {
    if (m_rtcPub != NULL) {
		yang_warn("already published!");
        return 1;
    }

	YangUrlData urlData;
	memset(&urlData, 0, sizeof(urlData));

	int err = yang_url_parse(m_context->avinfo.sys.familyType, url, &urlData);

	if (err != Yang_Ok) {
		return err;
	}

	yang_info(
		"url: %s, type: %d, ip: %s, port: %d, path: %s, app: %s, stream: %s, param: %s",
        url,
        urlData.netType, 
		urlData.server, 
		urlData.port, 
		urlData.path,
		urlData.app,
        urlData.stream,
		urlData.param
	);

	m_rtcPub = new YangRtcPublish(m_context);

	if (m_hasAudio && m_pushPub->startAudioCapture() == Yang_Ok) {
		m_pushPub->startAudioEncoding();
	}

	m_pushPub->startVideoEncoding();

	m_pushPub->setRtcNetBuffer(m_rtcPub);

    err = m_rtcPub->init(url, isWhip);

    if (err != Yang_Ok) {
        return yang_error_wrap(err, " connect server failure!");
	}
                
	m_rtcPub->start();
	return err;
}

