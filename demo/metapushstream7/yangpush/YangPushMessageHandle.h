//
// Copyright (c) 2019-2022 yanggaofeng
//

#ifndef SRC_YANGMEETING_SRC_YangPushMessageHandle_H_
#define SRC_YANGMEETING_SRC_YangPushMessageHandle_H_

#include <yangutil/sys/YangSysMessageHandle.h>
#include <yangutil/sys/YangThread2.h>
#include "YangPushHandleImpl.h"

class YangPushMessageHandle : public YangSysMessageHandle {
public:
    YangPushMessageHandle(
		bool hasAudio,
		int videoType,
		YangVideoInfo* screenVideo,
		YangVideoInfo* outVideo,
		YangContext* context,
		YangSysMessageI* message,
		YangSysMessageHandleI* messageHandle);

	virtual ~YangPushMessageHandle();

public:
	virtual void initAll() {}
	virtual void deleteAll();
	virtual void handleMessage(YangSysMessage* mss);

public:
	YangPushHandleImpl* m_push;

private:
	YangContext* m_context;
	YangSysMessageHandleI* m_receive;
};

#endif // SRC_YANGMEETING_SRC_YangPushMessageHandle_H_
