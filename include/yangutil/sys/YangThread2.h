//
// Copyright (c) 2019-2025 yanggaofeng
//
#ifndef YangThread2_H__
#define YangThread2_H__

#include <yangutil/sys/YangThread.h>
#include <yangutil/yangtype.h>

#define yang_stop_thread(x) if (x) { while(x->m_isStart) yang_usleep(1000); }
#define yang_stop(x) if (x && x->m_isStart) { x->stop(); }

class YangThread {
public:
	YangThread() { m_thread = 0; }
	virtual ~YangThread() { m_thread = 0; };

public:
    /**
     * 执行线程任务
     * @param obj 线程对象
     * @return 线程返回值
     */
    static void* RunTask(void* obj);

public:
    /**
	 * 停止线程
	 */
	virtual void stop() = 0;

protected:
    /**
	 * 运行线程任务
	 */
	virtual void run() = 0;

public:
    /**
	 * 启动线程
	 * @return 0 成功 -1 失败
	 */
	virtual int32_t start();

	/**
	 * 等待线程结束
	 * @return 线程返回值
	 */
	void* join();

public:
	/**
	 * 获取线程
	 * @return 线程
	 */
	inline yang_thread_t getThread() { return m_thread; }

private:
	yang_thread_t m_thread;
};

#endif // YangThread2_H__
