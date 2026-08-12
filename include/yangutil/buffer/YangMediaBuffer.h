//
// Copyright (c) 2019-2025 yanggaofeng
//

#ifndef YANGUTIL_BUFFER_YANGBUFFER_H_
#define YANGUTIL_BUFFER_YANGBUFFER_H_
#include <yangutil/sys/YangThread.h>
#include <yangutil/yangtype.h>
#include <yangutil/yangavinfo.h>
#include <yangutil/buffer/YangBufferManager.h>

#define yang_reindex(p)  if (p != NULL) { p->resetIndex(); }

class YangMediaBuffer {
public:
	YangMediaBuffer();
	virtual ~YangMediaBuffer();

public:
	void resetIndex();
    uint32_t size();

protected:
	void initFrames(int pnum,int unitsize);

	void putFrame(YangFrame* pframe);
	void getFrame(YangFrame* pframe);

	YangFrame* getCurFrameRef();
	int64_t getNextFrameTimestamp();
	uint8_t* getFrameRef(YangFrame* pframe);

public:
	int32_t m_uid;

protected:
    // 写入位置
	uint32_t m_putIndex;
    // 读取位置
	uint32_t m_getIndex;
	// 缓冲区总容量（帧数）
	uint32_t m_cache_num;
    // 当前已缓冲帧数
	uint32_t m_size;
	// 当前预览/查看位置
	uint32_t m_nextIndex;

	// 帧对象数组
	YangFrame** m_frames;
	// 实际数据内存管理
	YangBufferManager* m_bufferManager;

private:
	yang_thread_mutex_t m_lock;
};

#endif // YANGUTIL_BUFFER_YANGBUFFER_H_
