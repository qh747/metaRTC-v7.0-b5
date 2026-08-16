//
// Copyright (c) 2019-2025 yanggaofeng
//

#ifndef YANGCAPTURE_SRC_YANGVIDEOCAPTUREIMPL_H_
#define YANGCAPTURE_SRC_YANGVIDEOCAPTUREIMPL_H_

#include <yangutil/yangavinfotype.h>
#include <yangcapture/YangVideoCaptureHandle.h>
#include <yangcapture/YangVideoCapture.h>

#if Yang_OS_LINUX
#include <linux/videodev2.h>

#define REQ_BUF_NUM 4

typedef struct buffer_type_r {
	uint8_t* start;
	int32_t  length;
} buffer_type;

class YangVideoCaptureLinux : public YangVideoCapture {
public:
	YangVideoCaptureLinux(YangVideoInfo* context);
	virtual ~YangVideoCaptureLinux();

public:
	virtual int32_t init();
	virtual void initstamp();

	virtual void setVideoCaptureStart();
	virtual void setVideoCaptureStop();

	virtual void setOutVideoBuffer(YangVideoBuffer* buf);
	virtual void setPreVideoBuffer(YangVideoBuffer* buf);
	
	virtual void stopLoop();

protected:
	virtual void startLoop();

private:
	void stopCapture();
	void stopCamDev();

public:
    YangVideoCaptureHandle* m_vhandle;

private:
	YangColorSpace m_fmt;
	int32_t m_devFd;

	buffer_type m_user_buffer[REQ_BUF_NUM];
	int32_t m_buffer_count;

	int32_t m_isloop;
	bool m_isFirstFrame;
	
	struct timeval m_startTime;
};

#endif // Yang_OS_LINUX
#endif // YANGCAPTURE_SRC_YANGVIDEOCAPTUREIMPL_H_
