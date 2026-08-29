//
// Copyright (c) 2019-2025 yanggaofeng
//
#include <yangaudiodev/YangAudioCaptureHandle.h>

#include <yangutil/sys/YangLog.h>

YangAudioCaptureHandle::YangAudioCaptureHandle(YangAVInfo* avinfo) {
	if (avinfo == NULL) {
		return;
	}

    m_enabled = yangfalse;
    m_buffer = NULL;

    memset(&m_frame, 0, sizeof(YangFrame));
}

YangAudioCaptureHandle::~YangAudioCaptureHandle() {
	m_buffer = NULL;
}

void YangAudioCaptureHandle::setOutAudioBuffer(YangAudioBuffer* buffer) {
    m_buffer = buffer;
}

void YangAudioCaptureHandle::setCaptureState(yangbool state) {
    m_enabled = state;
}

void YangAudioCaptureHandle::putBuffer(uint8_t* buffer, int32_t len) {
	if (!m_enabled) {
		return;
	}

	if (m_buffer == NULL) {
		return;
	}

	m_frame.payload = buffer;
	m_frame.nb = len;

	m_buffer->putAudio(&m_frame);
}

