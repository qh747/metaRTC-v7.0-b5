//
// Copyright (c) 2019-2022 yanggaofeng
//
#ifndef YANGPUSH_YANGPUSHHANDLEIMPL_H_
#define YANGPUSH_YANGPUSHHANDLEIMPL_H_

#include <yangpush/YangPushPublish.h>
#include <yangpush/YangPushHandle.h>
#include <yangpush/YangRtcPublish.h>
#include <yangpush/YangSendVideoImpl.h>

class YangPushHandleImpl : public YangPushHandle {
public:
	YangPushHandleImpl(
		bool hasAudio,
		YangVideoInfo* outVideo,
		YangContext* context,
		YangSysMessageI* message
	);
	
	virtual ~YangPushHandleImpl();

public:
    virtual void init();

    virtual int32_t publish(const char* url, yangbool isWihp);

	virtual void disconnect();
	
    virtual void changeSrc(int videoType);

	virtual YangVideoBuffer* getPreVideoBuffer();

private:
	bool m_hasAudio;

	YangPushPublish* m_pushPub;
	YangRtcPublish* m_rtcPub;

	YangContext* m_context;
	YangSysMessageI* m_message;

	YangVideoInfo* m_outInfo;
};

#endif // YANGPUSH_YANGPUSHHANDLEIMPL_H_
