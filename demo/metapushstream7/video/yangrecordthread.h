//
// Copyright (c) 2019-2022 yanggaofeng
//
#ifndef YANGTHREAD_H
#define YANGTHREAD_H
#include <QThread>
#include <QVector>
#include <yangutil/yangavinfotype.h>
#include <yangutil/buffer/YangVideoBuffer.h>
#include "YangPlayWidget.h"

class YangRecordThread : public QThread {
public:
    YangRecordThread();
    virtual ~YangRecordThread();

public:
    void stopAll();

private:
    void render();

private:
    virtual void run();

public:
    int32_t m_isLoop;
    
    // 从m_videoBuffer读取数据进行渲染
#if Yang_OS_APPLE
    YangYuvPlayWidget* m_playwidget;
#else
    YangPlayWidget* m_playwidget;
#endif
    
    // 存放摄像头采集数据的缓冲区
    YangVideoBuffer* m_videoBuffer;

private:
    int32_t m_isStart;
};

#endif // YANGTHREAD_H
