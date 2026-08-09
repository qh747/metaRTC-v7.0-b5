//
// Copyright (c) 2019-2025 yanggaofeng
//
#ifndef YANGUTIL_YANGAVINFOTYPE_H_
#define YANGUTIL_YANGAVINFOTYPE_H_

#include <yangutil/yangtype.h>
#include <yangutil/yangavinfo.h>

typedef struct {
	void* context;
	void (*receiveData)(void* context, YangFrame* msgFrame);
} YangChannelDataRecvI;

typedef struct {
	void* context;
	void (*sendData)(void* context, YangFrame* msgFrame);
} YangChannelDataSendI;

#ifdef __cplusplus
#include <yangstream/YangStreamManager.h>
#include <yangstream/YangSynBufferManager.h>

extern "C"{
#include <yangutil/yangframebuffer.h>
}

class YangContext {
public:
	YangContext();
	virtual ~YangContext();

public:
	void init(const char* filename);
	void init();

public:
	YangAVInfo avinfo;
	YangRtcCallback rtcCallback;
	YangSendRtcMessage sendRtcMessage;
	YangChannelDataRecvI channeldataRecv;
	YangChannelDataSendI channeldataSend;

#if Yang_OS_ANDROID
	void* nativeWindow;
#endif

#if Yang_Enable_Vr
    char bgFilename[256];
#endif

    YangSynBufferManager synMgr;
    YangStreamManager* streams;
};

extern "C" {
void yang_init_avinfo(YangAVInfo* avinfo);
}
#else
#include <yangutil/yangframebuffer.h>
void yang_init_avinfo(YangAVInfo* avinfo);
#endif

#endif // YANGUTIL_YANGAVINFOTYPE_H_
