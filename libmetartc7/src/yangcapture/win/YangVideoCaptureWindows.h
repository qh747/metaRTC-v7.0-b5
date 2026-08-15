//
// Copyright (c) 2019-2025 yanggaofeng
//

#ifndef YANGCAPTURE_SRC_YangLivingVideoCaptureWinImpl_H_
#define YANGCAPTURE_SRC_YangLivingVideoCaptureWinImpl_H_
#include <yangutil/yangtype.h>
#if Yang_OS_WIN
#include <yangcapture/win/YangWinVideoCaptureHandle.h>
#include <yangcapture/YangVideoCapture.h>
#include <windows.h>
#include <control.h>

#include "amstream.h"
#include "qedit.h"

#include <dmodshow.h>
#include <Dmoreg.h>
#include <initguid.h>


struct YangVideoCaptureType{
	YangYuvType yuvType;
	int32_t state;
	int32_t width;
	int32_t height;
};
class YangVideoCaptureWindows: public YangVideoCapture {
public:
    YangVideoCaptureWindows(YangVideoInfo *pcontext);
	virtual ~YangVideoCaptureWindows();
    YangWinVideoCaptureHandle *m_vhandle;
	int32_t init();
	void setVideoCaptureStart();
	void setVideoCaptureStop();

	void setOutVideoBuffer(YangVideoBuffer *pbuf);
	void setPreVideoBuffer(YangVideoBuffer *pbuf);

	void initstamp();
	void stopLoop();

protected:
	void startLoop();

	void setRevolutionPara();
	void setRevolution();
	IPin* FindPin(IBaseFilter *pFilter, PIN_DIRECTION dir);
	IPin* GetOutPin(IBaseFilter *pFilter, PIN_DIRECTION pin_dir, int32_t nPin);
	IPin* GetInPin(IBaseFilter *pFilter, PIN_DIRECTION pin_dir, int32_t nPin);

	IMediaControl *m_pm;
	IGraphBuilder *m_pg;
	ICaptureGraphBuilder2 *m_pb;
	IBaseFilter *m_videoSrc, *m_grabberF;
	ISampleGrabber *m_grabber;
	IBaseFilter *m_nullRender;

	IMediaEventEx *m_event;
private:
	void setWH(YangVideoCaptureType *pct);
	REFGUID getUID();

	YangVideoCaptureType m_yuy2, m_i420 , m_nv12, m_yv12, m_p010,m_p016;
	int32_t m_width, m_height;
	int32_t m_vd_id;
	int32_t hasVideo;
	int32_t m_preframe, m_isOpAddMinus;
	ULONG m_t_time;
	int32_t m_buffer_count;
	int32_t m_isloop;
	int32_t m_isFirstFrame;

	long m_timestatmp;

};
#endif
#endif /* YANGCAPTURE_SRC_YangLivingVideoCaptureWinImpl_H_ */
