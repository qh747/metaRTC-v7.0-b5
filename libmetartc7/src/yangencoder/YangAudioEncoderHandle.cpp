//
// Copyright (c) 2019-2025 yanggaofeng
//

#include <yangencoder/YangAudioEncoderHandle.h>
#include <yangencoder/YangEncoderFactory.h>
#include <yangavutil/audio/YangMakeWave.h>
#include <yangutil/yangavinfotype.h>

YangAudioEncoderHandle::YangAudioEncoderHandle(YangAudioInfo* info) {
	m_info = info;

	m_isStart = 0;
	m_isConvert = 1;
    
    m_enc = NULL;

	m_in_audioBuffer = NULL;
	m_out_audioBuffer = NULL;
}

YangAudioEncoderHandle::~YangAudioEncoderHandle() {
	if (m_isConvert) {
		this->stop();

		while (m_isStart) {
			yang_usleep(1000);
		}
	}

	yang_delete(m_enc);

	m_info = NULL;
	m_in_audioBuffer = NULL;
	m_out_audioBuffer = NULL;
}
void YangAudioEncoderHandle::stop() {
	m_isConvert = 0;
}

void YangAudioEncoderHandle::run() {
	m_isStart = 1;

	if (m_in_audioBuffer == NULL) {
		return;
	}

	m_isConvert = 1;

	yang_reindex(m_in_audioBuffer);
	yang_reindex(m_out_audioBuffer);

	YangFrame audioFrame;
	memset(&audioFrame, 0, sizeof(YangFrame));

	uint8_t payload[4096];
	memset(payload, 0, sizeof(payload));

	while (m_isConvert == 1) {
		if (m_in_audioBuffer->size() == 0) {
			yang_usleep(200);
			continue;
		}

		audioFrame.payload = payload;
		audioFrame.uid = 0;
		audioFrame.nb = 0;

		if (m_in_audioBuffer->getAudio(&audioFrame)) {
			yang_usleep(200);
			continue;
		} 
		else {
			m_enc->encoder(&audioFrame, this);
		}
	}	
	
	m_isStart = 0;
}

void YangAudioEncoderHandle::init() {
	if (m_enc != NULL) {
		return;
	}

	m_enc = YangEncoderFactory::CreateAudioEncoder(m_info);
   
	if (m_enc == NULL) {
		return;
	}

	m_enc->init(m_info);
}

void YangAudioEncoderHandle::setInAudioBuffer(YangAudioBuffer* buf) {
	m_in_audioBuffer = buf;
}
void YangAudioEncoderHandle::setOutAudioBuffer(YangAudioEncoderBuffer* buf) {
	m_out_audioBuffer = buf;
}

void YangAudioEncoderHandle::onVideoData(YangFrame* frame) {
    (void)frame;
}

void YangAudioEncoderHandle::onAudioData(YangFrame* frame) {
	m_out_audioBuffer->putAudio(frame);
}
