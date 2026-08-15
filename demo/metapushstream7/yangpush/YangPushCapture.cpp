//
// Copyright (c) 2019-2022 yanggaofeng
//

#include <yang_config_os.h>
#include <yangpush/YangPushCapture.h>
#include <yangavutil/video/YangYuvConvert.h>
#include <yangavutil/video/YangYuvUtil.h>
#include <yangcapture/YangCaptureFactory.h>

YangPushCapture::YangPushCapture(YangContext* context) {
	m_context = context;

	m_out_audioBuffer = NULL;
	m_out_videoBuffer = NULL;

	m_videoCapture = NULL;
	m_audioCapture = NULL;
	
	m_screen_pre_videoBuffer = NULL;
	m_screen_out_videoBuffer = NULL;

	m_pre_videoBuffer = new YangVideoBuffer(context->avinfo.video.bitDepth == 8 ? 1 : 2);
	m_pre_videoBuffer->isPreview = 1;
    
	m_isStart = 0;
	m_isConvert = 0;
}

YangPushCapture::~YangPushCapture() {
	m_context = NULL;

	this->stopAll();
	yang_stop_thread(this);

	yang_stop_thread(m_audioCapture);
	yang_stop_thread(m_videoCapture);

	yang_delete(m_audioCapture);
	yang_delete(m_videoCapture);

	yang_delete(m_out_audioBuffer);
	yang_delete(m_pre_videoBuffer);
	yang_delete(m_out_videoBuffer);

	m_screen_pre_videoBuffer = NULL;
	m_screen_out_videoBuffer = NULL;
}

void YangPushCapture::startAudioCaptureState() {
    if (m_audioCapture) {
		m_audioCapture->setCatureStart();
	}
}

void YangPushCapture::stopAudioCaptureState() {
    if (m_audioCapture) {
		m_audioCapture->setCatureStop();
	}
}

void YangPushCapture::setAec(YangRtcAec* aec) {
	if (m_audioCapture) {
		m_audioCapture->setAec(aec);
	}
}

void YangPushCapture::setInAudioBuffer(std::vector<YangAudioPlayBuffer*>* pbuf) {
	if (m_audioCapture) {
		m_audioCapture->setInAudioBuffer(pbuf);
	}
}

void YangPushCapture::startAudioCapture() {
	if (m_audioCapture && !m_audioCapture->m_isStart) {
		m_audioCapture->start();
	}
}

void YangPushCapture::startVideoCapture() {
	if (m_videoCapture && !m_videoCapture->m_isStart) {
		m_videoCapture->start();
	}
}

int32_t YangPushCapture::initAudio(YangPreProcess* preProc) {
	if (m_out_audioBuffer == NULL) {
		m_out_audioBuffer = new YangAudioBuffer(m_context->avinfo.audio.audioCacheNum);
	}

	if (m_audioCapture == NULL) {
		m_audioCapture = YangCaptureFactory::CreateAudioCapture(&m_context->avinfo);
        
		int32_t ret = m_audioCapture->init();

		if (ret == ERROR_SYS_NoAudioDevice || ret == ERROR_SYS_NoAudioCaptureDevice) {
			return yang_error_wrap(ret, "audio capture init fail!");
		}

		m_audioCapture->setPreProcess(preProc);
		m_audioCapture->setOutAudioBuffer(m_out_audioBuffer);

		m_audioCapture->setCatureStop();
	}

	this->stopAudioCaptureState();
	return Yang_Ok;
}

int32_t YangPushCapture::initVideo() {
	if (m_out_videoBuffer == NULL) {
		m_out_videoBuffer = new YangVideoBuffer(
			m_context->avinfo.video.bitDepth == 8 ? 1 : 2
		);
	}

	if (m_videoCapture == NULL) {
#if Yang_OS_ANDROID
		m_videoCapture = YangCaptureFactory::CreateAndroidCapture(
			&m_context->avinfo.video,
			m_context->nativeWindow
		);
#else
		m_videoCapture = YangCaptureFactory::CreateVideoCapture(&m_context->avinfo.video);
#endif
        
        int32_t err = m_videoCapture->init();

		if(err != Yang_Ok){
			return yang_error_wrap(err, "video capture init fail!");
		}

		m_out_videoBuffer->init(
			m_context->avinfo.video.width,
			m_context->avinfo.video.height,
			m_context->avinfo.video.videoEncoderFormat
		);

		m_pre_videoBuffer->init(
			m_context->avinfo.video.width,
			m_context->avinfo.video.height,
			m_context->avinfo.video.videoEncoderFormat
		);
		
		m_videoCapture->setOutVideoBuffer(m_out_videoBuffer);
		m_videoCapture->setPreVideoBuffer(m_pre_videoBuffer);
	}

	this->stopVideoCaptureState();
	return Yang_Ok;
}

void YangPushCapture::stopAll() {
	this->stop();

	yang_stop(m_audioCapture);
	yang_stop(m_videoCapture);
}

void YangPushCapture::startVideoCaptureState() {
	if (m_videoCapture) {
		m_videoCapture->initstamp();
	    m_videoCapture->setVideoCaptureStart();
	}
}

void YangPushCapture::stopVideoCaptureState() {
	if (m_videoCapture) {
		m_videoCapture->setVideoCaptureStop();
	}
}

void YangPushCapture::stop() {
	m_isConvert = 0;
}

void YangPushCapture::startCamera() {
	this->initVideo();
	this->startVideoCapture();
}

void YangPushCapture::stopCamera() {
	yang_stop(m_videoCapture);
	yang_stop_thread(m_videoCapture);
	yang_delete(m_videoCapture);
}
