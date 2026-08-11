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
    void initPara(YangContext* context);
    void stopAll();

private:
    void render();

private:
    virtual void run();

public:
    int32_t m_isLoop;

    int32_t m_sid;
    int32_t showType;

#if Yang_OS_APPLE
    YangYuvPlayWidget* m_video;
#else
    YangPlayWidget* m_video;
#endif

    YangVideoBuffer* m_videoBuffer;

private:
    int32_t m_isStart;

    YangColor m_bgColor;
    YangColor m_textColor;

    YangContext* m_para;
    YangFrame m_frame;
};

#endif // YANGTHREAD_H
