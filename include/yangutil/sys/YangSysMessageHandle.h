
//
// Copyright (c) 2019-2025 yanggaofeng
//
#ifndef INCLUDE_YANGUTIL_SYS_YANGSYSMESSAGEHANDLE_H_
#define INCLUDE_YANGUTIL_SYS_YANGSYSMESSAGEHANDLE_H_

#include <atomic>
#include <vector>
#include <yangutil/sys/YangThread2.h>
#include <yangutil/sys/YangSysMessageI.h>

class YangSysMessageHandle : public YangThread {
public:
	/**
	 * 构造函数
	 */
	YangSysMessageHandle();

	/**
	 * 析构函数
	 */
	virtual ~YangSysMessageHandle();

public:
    /**
	 * 停止线程
	 */
    virtual void stop();

public:
	/**
	 * 启动线程
	 * @return 0 成功 -1 失败
	 */
	virtual int32_t start();

protected:
    /**
	 * 运行线程任务
	 */
	virtual void run();

public:
    /**
	 * 处理系统消息
	 * @param mss 系统消息
	 */
	virtual void handleMessage(YangSysMessage* mss) = 0;
	
	/**
	 * 初始化所有系统消息
	 */
	virtual void initAll() = 0;

	/**
	 * 删除所有系统消息
	 */
	virtual void deleteAll() = 0;

public:
	/**
	 * 放入系统消息
	 * @param handle 系统消息处理接口
	 * @param pst 系统消息类型
	 * @param uid 系统消息用户ID
	 * @param handleState 系统消息处理状态
	 * @param user 系统消息用户数据
	 */
	void putMessage(
		YangSysMessageI* handle,
		int32_t pst, 
		int32_t uid, 
		int32_t handleState,
		void* user = NULL);

protected:
    /**
	 * 启动系统消息处理循环
	 */
	void startLoop();
	
	/**
	 * 停止系统消息处理循环
	 */
	void stopLoop();

public:
	std::atomic<yangbool> m_isStart;
	std::atomic<int32_t> m_loop;

private:
	std::vector<YangSysMessage*> m_sysMessages;
	yang_thread_mutex_t m_lock;
	yang_thread_cond_t m_cond_mess;

	YangSysMessageHandleI* m_receive;
};

#endif // INCLUDE_YANGUTIL_SYS_YANGSYSMESSAGEHANDLE_H_
