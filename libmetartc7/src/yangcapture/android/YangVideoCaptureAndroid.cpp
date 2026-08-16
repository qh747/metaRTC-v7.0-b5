//
// Copyright (c) 2019-2025 yanggaofeng
//

#include <yangcapture/android/YangVideoCaptureAndroid.h>

#if Yang_OS_ANDROID
#include <iostream>


YangVideoCaptureAndroid::YangVideoCaptureAndroid(YangVideoInfo *pcontext,void* pwindow) {
	m_para = pcontext;
	m_vhandle = new YangVideoCaptureHandle(pcontext);

	m_camIdx = pcontext->vIndex;
	m_width = m_para->width;
	m_height = m_para->height;
	m_isloop = 0;

	m_pre_videoBuffer=NULL;

	m_camera=new YangCameraAndroid((ANativeWindow*)pwindow, (YangCameraType)m_camIdx);
	m_camera->setSize(m_width, m_height);
	m_camera->setUser(m_vhandle);
}

YangVideoCaptureAndroid::~YangVideoCaptureAndroid() {
	/*if (m_isloop) {
		stop();
		while (m_isStart) {
			yang_usleep(1000);
		}
	}*/

	yang_delete(m_vhandle);
	yang_delete(m_camera);
}
void YangVideoCaptureAndroid::setVideoCaptureStart() {
	m_vhandle->m_isCapture = 1;
}
void YangVideoCaptureAndroid::setVideoCaptureStop() {
	m_vhandle->m_isCapture = 0;
}

void YangVideoCaptureAndroid::setOutVideoBuffer(YangVideoBuffer *pbuf) {
	m_vhandle->setVideoBuffer(pbuf);
}

void YangVideoCaptureAndroid::setPreVideoBuffer(YangVideoBuffer *pbuf) {
	m_pre_videoBuffer=pbuf;
	m_vhandle->setPreVideoBuffer(pbuf);
}
void YangVideoCaptureAndroid::initstamp() {
	m_vhandle->initstamp();
}
int32_t YangVideoCaptureAndroid::setPara() {

	return Yang_Ok;
}

int32_t YangVideoCaptureAndroid::init() {
	if(m_camera) m_camera->initCamera();
	return Yang_Ok;
}

void YangVideoCaptureAndroid::stopLoop() {
	m_isloop = 0;
}

void YangVideoCaptureAndroid::startLoop() {
	m_isloop = 1;
	m_isloop=0;
}

#endif
