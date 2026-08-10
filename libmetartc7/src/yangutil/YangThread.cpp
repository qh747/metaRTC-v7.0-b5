//
// Copyright (c) 2019-2025 yanggaofeng
//

#include <yangutil/sys/YangLog.h>
#include <yangutil/sys/YangThread2.h>

void* YangThread::RunTask(void* obj) {
    reinterpret_cast<YangThread*>(obj)->run();
    return NULL;
}

int32_t YangThread::start() {
    if (yang_thread_create(&m_thread, 0, &YangThread::RunTask, this)) {
        yang_error("YangThread::start could not start thread");
        return -1;
    }

    return 0;
}

void* YangThread::join() {
    void* ret;
    yang_thread_join(m_thread, &ret);
    return ret;
}
