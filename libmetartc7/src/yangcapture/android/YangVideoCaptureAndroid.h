//
// Copyright (c) 2019-2025 yanggaofeng
//

#ifndef YANGCAPTURE_YangVideoCaptureAndroid_H_
#define YANGCAPTURE_YangVideoCaptureAndroid_H_

#include <yangcapture/YangVideoCaptureHandle.h>
#include <yangcapture/YangVideoCapture.h>

#if Yang_OS_ANDROID
#include <yangcapture/android/YangCameraAndroid.h>

class YangVideoCaptureAndroid: public YangVideoCapture {
public:
	YangVideoCaptureAndroid(YangVideoInfo *pcontext,void* pwindows);
	~YangVideoCaptureAndroid();
	YangVideoCaptureHandle *m_vhandle;
	YangCameraAndroid* m_camera;
	int32_t init();
	void setVideoCaptureStart();
	void setVideoCaptureStop();
	void setOutVideoBuffer(YangVideoBuffer *pbuf);
	void setPreVideoBuffer(YangVideoBuffer *pbuf);
	void initstamp();
	void stopLoop();

protected:
	void startLoop();
	int32_t setPara();
	YangVideoBuffer* m_pre_videoBuffer;

private:
	int32_t m_width, m_height;
	int32_t m_isloop;
};
#endif
#endif /* YANGCAPTURE_SRC_YANGVIDEOCAPTUREIMPL_H_ */
