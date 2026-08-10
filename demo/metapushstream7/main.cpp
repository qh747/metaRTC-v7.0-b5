//
// Copyright (c) 2019-2022 yanggaofeng
//


#if defined(_MSC_VER) && (_MSC_VER >= 1600)
#pragma execution_character_set("utf-8")
#endif

#include <QApplication>
#if (QT_VERSION < QT_VERSION_CHECK(6,0,0))
#include <QTextCodec>
#endif

#include <yangpush/YangPushFactory.h>
#include <video/yangrecordthread.h>
#include <yangutil/sys/YangSysMessageHandle.h>

#include "recordmainwindow.h"

int main(int argc, char *argv[]) {
#if defined (__APPLE__)
    QSurfaceFormat format;
    format.setVersion(4,1);
    format.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(format);
#endif

    QApplication a(argc, argv);

#if (QT_VERSION <= QT_VERSION_CHECK(5,0,0))
    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    QTextCodec::setCodecForLocale(codec);
    QTextCodec::setCodecForCStrings(codec);
    QTextCodec::setCodecForTr(codec);
#elif (QT_VERSION < QT_VERSION_CHECK(6,0,0))
    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    QTextCodec::setCodecForLocale(codec);
#endif

    RecordMainWindow win;
    YangSysMessageHandle* sys = YangPushFactory::CreatePushMessageHandle(
        win.m_hasAudio,
        win.m_videoType,
        &win.m_screenInfo,
        &win.m_outInfo,
        win.m_context,
        &win,
        &win);

    win.m_message = sys;
    sys->start();

    YangRecordThread videoThread;
    win.initVideoThread(&videoThread);

    videoThread.start();
    win.show();
    
    QThread::msleep(200);
    win.initPreview();

    return a.exec();
}
