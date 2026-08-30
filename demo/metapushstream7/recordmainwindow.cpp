//
// Copyright (c) 2019-2022 yanggaofeng
//
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

    m_isStartpush = false;

    m_playWidget = NULL;

    m_layout = new QHBoxLayout();
    ui->vdMain->setLayout(m_layout);

#if Yang_OS_APPLE
    m_playWidget = new YangYuvPlayWidget(this);
#else
    m_playWidget = new YangPlayWidget(this);
#endif

    m_layout->addWidget(m_playWidget);
    m_layout->setSpacing(0);

    ui->m_url->setText("http://127.0.0.1:8080/index/api/whip?app=live&stream=test");

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

    ui->m_b_rec->setText("start");
    m_isStartpush = false;

    yang_post_message(YangM_Push_Disconnect, 0, NULL);
}

void RecordMainWindow::receiveSysMessage(YangSysMessage* message, int32_t result) {
    switch (message->messageId) {
        case YangM_Push_Connect: {
            if (result) {
                ui->m_b_rec->setText("开始");
                m_isStartpush = false;
                ui->m_l_err->setText("push error(" + QString::number(result) + ")!");
            }

            break;
        }
        case YangM_Push_Disconnect: {
            break;
        }
        case YangM_Push_StartVideoCapture: {
            m_recThread->m_videoBuffer = YangPushFactory::GetPreVideoBuffer(m_message);
            break;
        }
    }
}

void RecordMainWindow::closeEvent(QCloseEvent* event) {
    this->closeAll();
    exit(0);
}

void RecordMainWindow::initVideoThread(YangRecordThread* recThd) {
    m_recThread = recThd;
    m_recThread->m_playwidget = m_playWidget;
}

void RecordMainWindow::closeAll() {
    if (m_context == NULL) {
        return;
    }

    m_recThread->stopAll();
    m_recThread = NULL;
    
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

        m_isStartpush = true;

        yang_info("url: %s", ui->m_url->text().toLatin1().data());
        m_url = ui->m_url->text().toLatin1().data();

        yang_post_message(
            ui->m_c_whip->checkState() == Qt::CheckState::Checked ? 
                YangM_Push_Connect_Whip : 
                YangM_Push_Connect,
            0,
            this,
            (void*)m_url.c_str()
        );
    }
    else {
        ui->m_b_rec->setText("start");
        m_isStartpush = false;

        yang_post_message(YangM_Push_Disconnect, 0, NULL);
    }
}

void RecordMainWindow::on_m_c_whip_clicked() {
    if (ui->m_c_whip->checkState() == Qt::CheckState::Checked) {
        m_context->avinfo.sys.mediaServer = Yang_Server_Whip_Whep;
        ui->m_url->setText("http://127.0.0.1:8080/index/api/whip?app=live&stream=test");
    }
    else {
        m_context->avinfo.sys.mediaServer = Yang_Server_Zlm;
        ui->m_url->setText("webrtc://127.0.0.1:8080/live/test");
    }
}

void RecordMainWindow::on_m_c_janus_clicked() {
    if (ui->m_c_janus->checkState() == Qt::CheckState::Checked) {
        ui->m_url->setText("http://127.0.0.1:7080/whip/create");
        m_janus.show();
    }
    else {
        m_janus.hide();
        this->on_m_c_whip_clicked();
    }
}
