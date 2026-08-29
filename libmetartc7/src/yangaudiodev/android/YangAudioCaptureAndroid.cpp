
//
// Copyright (c) 2019-2025 yanggaofeng
//
#include <yangavutil/audio/YangAudioUtil.h>

#if Yang_OS_ANDROID
#include <yangaudiodev/android/YangAudioCaptureAndroid.h>

void g_yang_androidaudio_bqRecorderCallback(uint8_t* data, int32_t nb, void* user) {
	if (user == NULL) {
		return;
	}

	YangAudioCaptureHandle* handle = (YangAudioCaptureHandle*)user;
	handle->putBuffer(data, nb);
}

YangAudioCaptureAndroid::YangAudioCaptureAndroid(YangAVInfo* avinfo) {
    m_ahandle = new YangAudioCaptureHandle(avinfo);

	m_size = 0;
	m_loops = 0;
	
	m_channel = avinfo->audio.channel;
	m_sample = avinfo->audio.sample;

	m_audioAndroid = (YangAudioAndroid*)yang_calloc(sizeof(YangAudioAndroid), 1);
}

YangAudioCaptureAndroid::~YangAudioCaptureAndroid() {
    delete m_ahandle;

	yang_destroy_audioAndroid(m_audioAndroid);
	yang_free(m_audioAndroid);
}

virtual void YangAudioCaptureAndroid::setCatureState(yangbool state) {
    m_ahandle->setCaptureState(state);
}

void YangAudioCaptureAndroid::setOutAudioBuffer(YangAudioBuffer* buffer) {
	m_ahandle->setOutAudioBuffer(buffer);
}

int32_t YangAudioCaptureAndroid::init() {
	if (yang_create_audioAndroid_record(
		m_audioAndroid,
		g_yang_androidaudio_bqRecorderCallback,
		m_ahandle, 
		m_sample,m_channel) != Yang_Ok) {

		return yang_error_wrap(ERROR_SYS_AudioRender, "init android record fail");
	}

	return Yang_Ok;
}

void YangAudioCaptureAndroid::startLoop() {
	m_loops = 1;
	m_loops = 0;
}

void YangAudioCaptureAndroid::stopLoop() {
	m_loops = 0;
}

#endif
