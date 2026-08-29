//
// Copyright (c) 2019-2025 yanggaofeng
//

#include <yangaudiodev/YangAudioCapture.h>
#include <yangutil/sys/YangLog.h>

YangAudioCapture::YangAudioCapture() {
	m_isStart = yangfalse;
}

void YangAudioCapture::run() {
	m_isStart = 1;
	this->startLoop();
	m_isStart = 0;
}

void YangAudioCapture::stop() {
	this->stopLoop();
}


