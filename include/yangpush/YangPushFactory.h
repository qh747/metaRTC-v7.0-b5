//
// Copyright (c) 2019-2025 yanggaofeng
//
#ifndef INCLUDE_YANGPUSH_YANGPUSHFACTORY_H_
#define INCLUDE_YANGPUSH_YANGPUSHFACTORY_H_

#include <yangpush/YangPushHandle.h>
#include <yangpush/YangSendVideoI.h>
#include <yangutil/sys/YangSysMessageHandle.h>

class YangPushFactory {
public:
    /**
	 * 创建推流消息句柄
	 * @param hasAudio 是否有音频
	 * @param videoType 视频类型
	 * @param screen 视频信息
	 * @param outVideo 视频信息
	 * @param context 上下文
	 * @param message 消息
	 * @param messageHandle 消息句柄
	 * @return 推流消息句柄
	 */
	static YangSysMessageHandle* CreatePushMessageHandle(
		bool hasAudio,
		int videoType,
		YangVideoInfo* screen, 
		YangVideoInfo* outVideo,
		YangContext* context,
		YangSysMessageI* message,
		YangSysMessageHandleI* messageHandle);
	
	/**
	 * 获取预视频缓冲区
	 * @param messageHandle 消息句柄
	 * @return 预视频缓冲区
	 */
	static YangVideoBuffer* GetPreVideoBuffer(YangSysMessageHandle* messageHandle);

	/**
	 * 获取发送视频接口
	 * @param messageHandle 消息句柄
	 * @return 发送视频接口
	 */
	static YangSendVideoI* GetSendVideo(YangSysMessageHandle* messageHandle);
};

#endif // INCLUDE_YANGPUSH_YANGPUSHFACTORY_H_
