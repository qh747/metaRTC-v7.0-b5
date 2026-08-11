//
// Copyright (c) 2019-2025 yanggaofeng
//
#include <yangutil/buffer/YangVideoBuffer.h>

YangVideoBuffer::YangVideoBuffer(int32_t pBitDepthLen) {
	this->resetIndex();
    
	m_length = 0;
	m_bitDepthLen = pBitDepthLen;

    m_cache_num = 30;

	m_width = 0;
	m_height = 0;
}

YangVideoBuffer::YangVideoBuffer(int32_t pwid, int32_t phei, YangYuvType ptype, int32_t pBitDepthLen) {
	m_bitDepthLen = pBitDepthLen;
	this->init(pwid, phei, ptype);
}

void YangVideoBuffer::init(int32_t pwid, int32_t phei, YangYuvType ptype) {
	this->resetIndex();

	m_width = pwid;
	m_height = phei;

	if (ptype == YangYuy2) {
		// YUY2：打包格式 YUV422，每个像素占 2 字节，总字节数 = 宽 × 高 × 2 × 位深系数
		m_length = m_width * m_height * 2 * m_bitDepthLen;
	}
	else if (ptype == YangRgb) {
		// RGB24：每个像素占 3 字节，总字节数 = 宽 × 高 × 3 × 位深系数
		m_length = m_width * m_height * 3 * m_bitDepthLen;
	}
	else if (ptype == YangArgb || ptype == YangBgra) {
		// ARGB/BGRA：每个像素占 4 字节，总字节数 = 宽 × 高 × 4 × 位深系数
		m_length = m_width * m_height * 4 * m_bitDepthLen;
	}
	else {
		// 默认按 4:2:0 平面格式（I420/YV12/NV12/NV21/P010/P016）计算：
		// Y 平面占 宽×高 字节，U、V 平面各占 宽×高/4 字节，合计 宽×高×3/2 字节，再乘位深系数
		m_length = m_width * m_height * 3 * m_bitDepthLen / 2;
	}

    m_cache_num = 30;
	this->initFrames(m_cache_num, m_length);
}

void YangVideoBuffer::putVideo(YangFrame* pframe) {
	this->putFrame(pframe);
}

void YangVideoBuffer::getVideo(YangFrame* pframe){
	this->getFrame(pframe);
}
uint8_t* YangVideoBuffer::getVideoRef(YangFrame* pframe) {
	return this->getFrameRef(pframe);
}

int64_t YangVideoBuffer::getTimestamp(int64_t* ptimestamp) {
	YangFrame* f = this->getCurFrameRef();
	if(f) {
		*ptimestamp = f->pts;
	}

	return 0;
}

int64_t YangVideoBuffer::getNextTimestamp() {
	return this->getNextFrameTimestamp();
}

YangFrame* YangVideoBuffer::getCurVideoFrame() {
	return this->getCurFrameRef();
}
