//
// Copyright (c) 2019-2022 yanggaofeng
//
#include <QDebug>
#include <QSettings>

#if (QT_VERSION < QT_VERSION_CHECK(6,0,0))
#include <QDesktopWidget>
#endif

#include <yang_config_os.h>
#include <yangpush/YangPushCommon.h>
#include <yangutil/sys/YangLog.h>
#include <yangutil/sys/YangFile.h>
#include <yangutil/sys/YangSocket.h>
#include <yangutil/sys/YangIni.h>
#include <yangutil/sys/YangString.h>
#include <yangpush/YangPushFactory.h>

#include "ui_recordmainwindow.h"
#include "recordmainwindow.h"

RecordMainWindow::RecordMainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::RecordMainWindow) {
    ui->setupUi(this);

    m_context = new YangContext();
    m_context->init("yang_config.ini");

    yang_setLogLevel(m_context->avinfo.sys.logLevel);
    yang_setLogFile(m_context->avinfo.sys.enableLogFile, NULL);

    m_context->avinfo.sys.mediaServer = Yang_Server_Zlm; 
    
    m_context->avinfo.audio.enableMono = yangfalse;
    m_context->avinfo.audio.enableAudioFec = yangfalse;
    m_context->avinfo.audio.sample = 48000;
    m_context->avinfo.audio.channel = 2;
    m_context->avinfo.audio.enableAec = yangfalse;
    m_context->avinfo.audio.audioCacheNum = 8;
    m_context->avinfo.audio.audioCacheSize = 8;
    m_context->avinfo.audio.audioPlayCacheNum = 8;
    m_context->avinfo.audio.audioEncoderType = Yang_AED_OPUS;
    
    m_context->avinfo.video.videoCacheNum = 10;
    m_context->avinfo.video.evideoCacheNum = 10;
    m_context->avinfo.video.videoPlayCacheNum = 10;
    m_context->avinfo.video.videoEncoderFormat = YangI420;
    m_context->avinfo.video.videoEncoderType = Yang_VED_H264;

#if Yang_Enable_GPU_Encoding
    m_context->avinfo.video.videoEncHwType = YangV_Hw_Nvdia; 
    m_context->avinfo.video.videoEncoderFormat = YangI420;
    m_context->avinfo.enc.createMeta = 0;
#endif
    
    m_context->avinfo.rtc.rtcLocalPort = 17000;
    m_context->avinfo.rtc.iceCandidateType = YangIceHost;
    m_context->avinfo.rtc.rtcLocalPort = 10000 + yang_random() % 15000;

    m_context->avinfo.enc.enc_threads = 4;

#if Yang_Enable_Openh264
    // OpenH264: 内联输出头,从码流提取
    m_context->avinfo.enc.createMeta = 0;  
#else
    // x264: 预生成元数据,不重复头
    m_context->avinfo.enc.createMeta = 1;
#endif

#if Yang_OS_APPLE
    m_context->avinfo.video.videoCaptureFormat = YangNv12;
    m_context->avinfo.video.videoEncoderFormat = (m_context->avinfo.video.videoEncHwType == 0) ? 
        YangI420 : 
        YangNv12;
#endif

    m_videoType = Yang_VideoSrc_Camera;
    m_hasAudio = true;

    m_isStartpush = 0;
    m_isStartRecord = false;
    m_initRecord = false;

    m_isDrawmouse = true;
    m_screenInternal = 33;

    m_win0 = NULL;
    m_isVr = 0;

    m_hb0 = new QHBoxLayout();
    ui->vdMain->setLayout(m_hb0);

#if Yang_OS_APPLE
     m_win0 = new YangYuvPlayWidget(this);
#else
     m_win0 = new YangPlayWidget(this);
#endif

    m_hb0->addWidget(m_win0);
    m_hb0->setSpacing(0);

    memset(m_localIp, 0, sizeof(m_localIp));
    yang_getLocalInfo(m_context->avinfo.sys.familyType, m_localIp);

    char s[128] = { 0 };
    sprintf(s, "http://%s:1985/rtc/v1/whip/?app=live&stream=livestream", m_localIp);
    ui->m_url->setText(s);

    memcpy(&m_screenInfo, &m_context->avinfo.video, sizeof(YangVideoInfo));

#if (QT_VERSION < QT_VERSION_CHECK(6,0,0))
    QDesktopWidget* desk = QApplication::desktop();
    m_screenInfo.width = desk->screenGeometry().width();
    m_screenInfo.height = desk->screenGeometry().height();

    m_screenInfo.outWidth = m_screenInfo.width;
    m_screenInfo.outHeight = m_screenInfo.height;
#endif
}

RecordMainWindow::~RecordMainWindow() {
    this->closeAll();
}

void RecordMainWindow::failure(int32_t errcode) {
    ui->m_l_err->setText("push error(" + QString::number(errcode) + ")!");
}

void RecordMainWindow::receiveSysMessage(YangSysMessage* mss, int32_t err) {
    switch (mss->messageId) {
        case YangM_Push_Connect: {
            if (err) {
                ui->m_b_rec->setText("开始");
                m_isStartpush = !m_isStartpush;
                ui->m_l_err->setText("push error(" + QString::number(err) + ")!");
            }
            break;
        }
        case YangM_Push_Disconnect: {
            break;
        }
        case YangM_Push_StartVideoCapture: {
            m_rt->m_videoBuffer = YangPushFactory::GetPreVideoBuffer(m_message);
            
            qDebug() << "message===" << m_message 
                     << "..prevideobuffer===" << m_rt->m_videoBuffer 
                     << "....ret====" << err;
            break;
        }
    }
}

void RecordMainWindow::closeEvent(QCloseEvent* event) {
    this->closeAll();
    exit(0);
}

void RecordMainWindow::initVideoThread(YangRecordThread* recThd) {
    m_rt = recThd;
    m_rt->m_playwidget = m_win0;
}

void RecordMainWindow::closeAll() {
    if (m_context == NULL) {
        return;
    }

    m_rt->stopAll();
    m_rt = NULL;
    
    yang_delete(m_message);
    yang_delete(m_context);
    
    delete ui;
}

void RecordMainWindow::startCapture() {
    yang_post_message(YangM_Push_StartVideoCapture, 0, NULL);
}

void RecordMainWindow::on_m_b_rec_clicked() {
    if (!m_isStartpush) {
        ui->m_l_err->setText("");
        ui->m_b_rec->setText("stop");

        m_isStartpush = !m_isStartpush;

        qDebug() << "url=========" << ui->m_url->text().toLatin1().data();
        m_url = ui->m_url->text().toLatin1().data();

        yang_post_message(
            ui->m_c_whip->checkState() == Qt::CheckState::Checked ? 
                YangM_Push_Connect_Whip : 
                YangM_Push_Connect,
            0,
            NULL,
            (void*)m_url.c_str()
        );
    }
    else {
        ui->m_b_rec->setText("start");
        m_isStartpush = !m_isStartpush;
        yang_post_message(YangM_Push_Disconnect, 0, NULL);
    }
}

void RecordMainWindow::on_m_c_whip_clicked() {
    char s[128] = { 0 };

    if (ui->m_c_whip->checkState() == Qt::CheckState::Checked) {
        m_context->avinfo.sys.mediaServer = Yang_Server_Whip_Whep;
        sprintf(s, "http://%s:1985/rtc/v1/whip/?app=live&stream=livestream", m_localIp);

    }
    else {
        m_context->avinfo.sys.mediaServer = Yang_Server_Zlm;
        sprintf(s, "webrtc://%s:1985/live/livestream", m_localIp);
    }

    ui->m_url->setText(s);
}

void RecordMainWindow::on_m_c_janus_clicked() {
    char s[128] = { 0 };

    if (ui->m_c_janus->checkState() == Qt::CheckState::Checked) {
        sprintf(s, "http://%s:7080/whip/endpoint/metaRTC", m_localIp);
        ui->m_url->setText(s);

        m_janus.show();
    }
}
