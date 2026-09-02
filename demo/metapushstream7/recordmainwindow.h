//
// Copyright (c) 2019-2022 yanggaofeng
//
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QHBoxLayout>
#include <video/yangrecordthread.h>
#include <video/YangPlayWidget.h>
#include <video/YangYuvPlayWidget.h>
#include <yangutil/yangavinfotype.h>
#include <yangutil/sys/YangSysMessageI.h>
#include <yangutil/sys/YangSysMessageHandle.h>
#include <yangpush/YangPushFactory.h>
#include "yangjanus.h"

QT_BEGIN_NAMESPACE
namespace Ui { class RecordMainWindow; }
QT_END_NAMESPACE

#define Yang_SendVideo_ 0

class RecordMainWindow : public QMainWindow, public YangSysMessageI, public YangSysMessageHandleI {
    Q_OBJECT

public:
    RecordMainWindow(QWidget* parent = nullptr);
    virtual ~RecordMainWindow();

public:
    virtual void success() {}
    virtual void failure(int32_t errcode);
    virtual void receiveSysMessage(YangSysMessage* message, int32_t result);

public:
    void startCapture();
    void initVideoThread(YangRecordThread* recThd);
    void closeEvent(QCloseEvent* event);

private slots:
    void on_m_b_rec_clicked();
    void on_m_c_whip_clicked();
    void on_m_c_janus_clicked();

private:
    void closeAll();

public:
    YangVideoInfo m_screenInfo;
    YangVideoInfo m_outInfo;
    
    YangContext* m_context;
    YangSysMessageHandle* m_message;

private:
    Ui::RecordMainWindow* ui;

    bool m_isStartpush;

    std::string m_url;

    YangJanus m_janus;

    YangRecordThread* m_recThread;

#if Yang_OS_APPLE
    YangYuvPlayWidget* m_playWidget;
#else
    YangPlayWidget* m_playWidget;
#endif

    QHBoxLayout* m_layout;   
};

#endif // MAINWINDOW_H
