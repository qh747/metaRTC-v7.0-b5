//
// Copyright (c) 2019-2025 yanggaofeng
//
#ifndef YANGAUDIOCAPTURE_H
#define YANGAUDIOCAPTURE_H

#include <yangavutil/audio/YangRtcAec.h>
#include <yangavutil/audio/YangPreProcess.h>
#include <yangutil/sys/YangThread2.h>
#include <yangutil/yangavinfotype.h>
#include "yangutil/buffer/YangAudioBuffer.h"

class YangAudioCapture : public YangThread {
public:
    YangAudioCapture();
    virtual ~YangAudioCapture() {}

public:
    virtual void stop();

protected:
    virtual void run();
    
public:
    virtual int32_t init() = 0;
    virtual void setCatureState(yangbool state) = 0;
    virtual void setOutAudioBuffer(YangAudioBuffer* buffer) = 0;
        
protected:        
    virtual void startLoop() = 0;
    virtual void stopLoop() = 0;

public:
    yangbool m_isStart;

protected:
    YangContext* m_context;
};

#endif // YANGAUDIOCAPTURE_H
