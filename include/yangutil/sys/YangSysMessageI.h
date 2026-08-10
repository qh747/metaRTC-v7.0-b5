//
// Copyright (c) 2019-2025 yanggaofeng
//
#ifndef INCLUDE_YANGUTIL_SYS_YANGMESSAGEI_H_
#define INCLUDE_YANGUTIL_SYS_YANGMESSAGEI_H_

#include <stdio.h>
#include <stdint.h>

class YangSysMessageI {
public:
	YangSysMessageI() {};
	virtual ~YangSysMessageI() {};

public:
	virtual void success() = 0;
	virtual void failure(int32_t errcode) = 0;
};

struct YangSysMessage {
	int32_t uid;
	int32_t messageId;
	int32_t handleState;
	YangSysMessageI* handle;
    void* user;
};

class YangSysMessageHandleI {
public:
	YangSysMessageHandleI() {};
	virtual ~YangSysMessageHandleI() {};

public:
	virtual void receiveSysMessage(YangSysMessage* psm, int32_t phandleRet) = 0;
};

/**
 * 发送系统消息
 * @param st 消息ID
 * @param uid 用户ID
 * @param mhandle 消息处理接口
 * @param user 用户数据
 */
void yang_post_message(int32_t st, int32_t uid, YangSysMessageI* mhandle, void* user = NULL);

/**
 * 发送有状态的系统消息
 * @param st 消息ID
 * @param uid 用户ID
 * @param handleState 处理状态
 * @param mhandle 消息处理接口
 */
void yang_post_state_message(int32_t st, int32_t uid, int32_t handleState, YangSysMessageI* mhandle);

#endif // INCLUDE_YANGUTIL_SYS_YANGMESSAGEI_H_
