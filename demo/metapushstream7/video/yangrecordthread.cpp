//
// Copyright (c) 2019-2022 yanggaofeng
//

#include <QDebug>
#include <QMapIterator>
#include "yangrecordthread.h"

YangRecordThread::YangRecordThread() {
    m_isLoop = 0;
    m_isStart = 0;

    m_video = nullptr;
    m_videoBuffer = nullptr;

    m_bgColor = { 0, 0, 0 };
    m_textColor = { 0, 0, 255 };

    m_sid = 1;
    showType = 1;
}

YangRecordThread::~YangRecordThread() {
    m_video = nullptr;
    m_videoBuffer = nullptr;

    this->stopAll();
}

void YangRecordThread::initPara(YangContext* context) {
    m_para = context;
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
        uint8_t* t_vb = m_videoBuffer->getVideoRef(&m_frame);

        if(t_vb && m_video && m_videoBuffer->m_width > 0) {
            m_video->playVideo(t_vb, m_videoBuffer->m_width, m_videoBuffer->m_height);
        }

        t_vb = NULL;
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
