//
// Copyright (c) 2019-2022 yanggaofeng
//
#ifndef YANGPUSH_YANGPUSHHANDLEIMPL_H_
#define YANGPUSH_YANGPUSHHANDLEIMPL_H_
#include <yangpush/YangPushPublish.h>
#include <yangpush/YangPushHandle.h>
#include <yangpush/YangRtcPublish.h>
#include <yangpush/YangSendVideoImpl.h>
#include <yangutil/sys/YangUrl.h>

class YangPushHandleImpl :public YangPushHandle {
public:
	YangPushHandleImpl(
		bool hasAudio,
		bool initVideo,
		YangVideoInfo* screenVideo,
		YangVideoInfo* outVideo,
		YangContext* context,
		YangSysMessageI* message
	);
	
	virtual ~YangPushHandleImpl();

public:
    virtual int32_t publish(char* url, yangbool isWihp);
	virtual void disconnect();

	virtual void init();
    virtual void changeSrc(int videoType);

	virtual YangVideoBuffer* getPreVideoBuffer();

private:
	void stopPublish();

private:
	bool m_hasAudio;
	bool m_isInit;

	YangPushPublish* m_cap;
	YangRtcPublish* m_rtcPub;

	YangContext* m_context;
	YangUrlData m_url;
	YangSysMessageI* m_message;
	YangVideoInfo* m_screenInfo;

	YangVideoInfo* m_outInfo;
};

#endif // YANGPUSH_YANGPUSHHANDLEIMPL_H_
