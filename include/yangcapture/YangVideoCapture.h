//
// Copyright (c) 2019-2025 yanggaofeng
//
#ifndef INCLUDE_YANGCAPTURE_YANGVIDEOCAPTURE_H_
#define INCLUDE_YANGCAPTURE_YANGVIDEOCAPTURE_H_

#include <string>
#include <vector>
#include <yangutil/buffer/YangVideoBuffer.h>
#include <yangutil/sys/YangLog.h>
#include <yangutil/sys/YangThread2.h>
#include <yangutil/yangavinfotype.h>

void yang_get_camera_indexs(std::vector<int>* pvs, std::string camIdx);

class YangVideoCapture : public YangThread {
public:
	YangVideoCapture();
	virtual ~YangVideoCapture();

public:
    virtual void stop();

protected:
    virtual void run();

public:
    virtual int32_t init() = 0;
    virtual void initstamp() = 0; 

    virtual void setVideoCaptureStart() = 0;
    virtual void setVideoCaptureStop() = 0;

    virtual void setOutVideoBuffer(YangVideoBuffer *pbuf) = 0;
    virtual void setPreVideoBuffer(YangVideoBuffer *pbuf) = 0;
     
protected:
    virtual void startLoop() = 0;
    virtual void stopLoop() = 0;

public:
    int32_t m_camIdx;
    int32_t m_isStart;

protected:
    YangVideoInfo* m_para;
};

#endif // INCLUDE_YANGCAPTURE_YANGVIDEOCAPTURE_H_

