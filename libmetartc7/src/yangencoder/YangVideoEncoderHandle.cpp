//
// Copyright (c) 2019-2025 yanggaofeng
//
#include <yangencoder/YangVideoEncoderHandle.h>
#include <yangencoder/YangEncoderFactory.h>
#include <yangavutil/video/YangYuvConvert.h>
#include <yangutil/yangavinfotype.h>
#include <yangutil/sys/YangLog.h>

YangVideoEncoderHandle::YangVideoEncoderHandle(YangContext* context, YangVideoInfo* info) {
	m_context = context;
	m_info = info;

	m_isInit = 0;
	m_isStart = 0;
	m_isConvert = 1;

	m_inVideoBuffer = NULL;
	m_outVideoBuffer = NULL;
	
	m_meta = NULL;
	m_hasEncMsg = 0;
}

YangVideoEncoderHandle::~YangVideoEncoderHandle(void) {
	if (m_isConvert) {
		this->stop();

		while (m_isStart) {
			yang_usleep(1000);
		}
	}

	m_info = NULL;

	m_inVideoBuffer = NULL;
	m_outVideoBuffer = NULL;
	
	m_meta = NULL;
}

void YangVideoEncoderHandle::stop() {
	m_isConvert = 0;
}

void YangVideoEncoderHandle::run() {
	m_isStart = 1;
    m_isConvert = 1;

	int32_t inBufSize = (m_info->bitDepth == 8 ? 1 : 2) * m_info->width * m_info->height * 3 / 2;

	int32_t outBufSize = (m_info->bitDepth == 8 ? 1 : 2) * m_info->outWidth * m_info->outHeight * 3 / 2;
	uint8_t* outBuf = (m_info->width != m_info->outWidth) ? new uint8_t[outBufSize] : NULL;

	if (m_inVideoBuffer != NULL) {
		m_inVideoBuffer->resetIndex();
	}
	if (m_outVideoBuffer != NULL) {
		m_outVideoBuffer->resetIndex();
	}

	YangVideoEncoder* enc = YangEncoderFactory::CreateVideoEncoder(m_info);

	enc->init(m_context,m_info);
	enc->setVideoMetaData(m_meta);

	uint8_t* nv12Src = (m_info->videoEncHwType) ? new uint8_t[inBufSize] : NULL;

	int64_t timestamp = 0;

	YangFrame videoFrame;
	memset(&videoFrame, 0, sizeof(YangFrame));

	YangYuvConvert yuv;

	while (m_isConvert == 1) {
		if (m_inVideoBuffer->size() == 0) {
			yang_usleep(1000);
			continue;
		}
        
		// 读取原始视频帧数据
		uint8_t* inBuf = m_inVideoBuffer->getVideoRef(&videoFrame);

		if (inBuf == NULL || (timestamp && videoFrame.pts <= timestamp)) {
			continue;
		}
        
		// 更新时间戳
		timestamp = videoFrame.pts;
        
		// 视频帧格式转换
		uint8_t* usedBuf = inBuf;

		if(m_info->videoEncHwType) {
#if Yang_OS_ANDROID
			if (m_info->videoCaptureFormat == YangI420) {
				if (m_info->videoEncoderFormat == YangI420) {
					usedBuf = inBuf;
				}
				else if (m_info->videoEncoderFormat == YangNv12) {
					yuv.i420tonv12(inBuf, nv12Src, m_info->width, m_info->height);
					usedBuf = nv12Src;
				}
			}
#elif Yang_OS_APPLE
            usedBuf = inBuf;
#else
			if (m_info->videoEncoderFormat == YangI420) {
				yuv.i420tonv12(inBuf, nv12Src, m_info->width, m_info->height);
				usedBuf = nv12Src;
			}
			else if(m_info->videoEncoderFormat == YangArgb) {
				usedBuf = inBuf;
			}
#endif
		}

		// 处理给编码器的消息
		if (m_hasEncMsg == 1) {
			enc->sendMsgToEncoder(&m_encMsg);
			m_hasEncMsg = 0;
		}

		videoFrame.uid = 0;
       
		// 分辨率转换
		if (m_info->width != m_info->outWidth) {
			yuv.scaleI420(
				usedBuf,
				outBuf, 
				m_info->width, 
				m_info->height, 
				m_info->outWidth,
				m_info->outHeight
			);
		} 
		
		// 设置视频帧数据
		videoFrame.payload = (m_info->width != m_info->outWidth) ? outBuf : usedBuf;
		videoFrame.nb = (m_info->width != m_info->outWidth) ? outBufSize : inBufSize;
       
		// 编码视频帧
		enc->encode(&videoFrame, this);
		usedBuf = NULL;
	}

	yang_deleteA(outBuf);
	yang_deleteA(nv12Src);
	yang_delete(enc);

	m_isStart = 0;
}

void YangVideoEncoderHandle::sendMsgToEncoder(YangRequestType type) {
	m_hasEncMsg = 1;

	m_encMsg.request = type;
	m_encMsg.requestValue = 0;
}

void YangVideoEncoderHandle::init() {
	if (m_isInit) {
		return;
	}

	m_isInit = 1;
}

void YangVideoEncoderHandle::onVideoData(YangFrame* frame) {
	if (frame->nb > 4 && m_outVideoBuffer != NULL) {
		m_outVideoBuffer->putEVideo(frame);
	}
}
