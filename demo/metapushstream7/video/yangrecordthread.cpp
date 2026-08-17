//
// Copyright (c) 2019-2022 yanggaofeng
//

#include <QDebug>
#include <QMapIterator>
#include "yangrecordthread.h"

YangRecordThread::YangRecordThread() {
    m_isLoop = 0;
    m_isStart = 0;

    m_playwidget = nullptr;
    m_videoBuffer = nullptr;
}

YangRecordThread::~YangRecordThread() {
    m_playwidget = nullptr;
    m_videoBuffer = nullptr;

    this->stopAll();
}

void YangRecordThread::stopAll() {
    if(m_isLoop) {
        m_isLoop = 0;

        while (m_isStart) {
            QThread::msleep(1);
        }
    }
}

void YangRecordThread::render() {
    if(m_videoBuffer && m_videoBuffer->size() > 0) {
        YangFrame frame;
        uint8_t* payload = m_videoBuffer->getVideoRef(&frame);
        
        // 读取一张yuv图像数据进行显示
        if (payload && m_playwidget && m_videoBuffer->m_width > 0) {
            m_playwidget->playVideo(payload, m_videoBuffer->m_width, m_videoBuffer->m_height);
        }

        payload = NULL;
    }
}

void YangRecordThread::run() {
    m_isLoop = 1;
    m_isStart = 1;

    while (m_isLoop) {
        QThread::msleep(20); 
        this->render();
    }

    m_isStart = 0;
}
