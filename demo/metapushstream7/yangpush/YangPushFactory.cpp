//
// Copyright (c) 2019-2022 yanggaofeng
//
#include <yangpush/YangPushFactory.h>
#include "YangPushMessageHandle.h"

YangSysMessageHandle* YangPushFactory::CreatePushMessageHandle(
	bool hasAudio,
	int videoType, 
	YangVideoInfo* screen, 
	YangVideoInfo* outVideo, 
	YangContext* context,
    YangSysMessageI* message,
	YangSysMessageHandleI* messageHandle) {
    return new YangPushMessageHandle(
		hasAudio,
		videoType,
		screen,
		outVideo,
		context,
		message,
		messageHandle);
}

YangVideoBuffer* YangPushFactory::GetPreVideoBuffer(YangSysMessageHandle* messageHandle) {
	if(messageHandle == NULL) {
		return NULL;
	}

	YangPushMessageHandle* pushHandle = dynamic_cast<YangPushMessageHandle*>(messageHandle);
	return (pushHandle && pushHandle->m_push) ? pushHandle->m_push->getPreVideoBuffer() : NULL;
}

YangSendVideoI* YangPushFactory::GetSendVideo(YangSysMessageHandle* messageHandle) {
	if(messageHandle == NULL) {
		return NULL;
	}

	YangPushMessageHandle* pushHandle = dynamic_cast<YangPushMessageHandle*>(messageHandle);
	return (pushHandle && pushHandle->m_push) ? pushHandle->m_push->getSendVideo() : NULL;
}
