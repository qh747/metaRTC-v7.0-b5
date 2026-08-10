//
// Copyright (c) 2019-2025 yanggaofeng
//

#include <atomic>
#include <yangutil/sys/YangLog.h>
#include <yangutil/sys/YangThread.h>
#include <yangutil/yangavinfotype.h>
#include <yangutil/sys/YangSysMessageHandle.h>

static std::atomic<YangSysMessageHandle*> g_instance{nullptr};

void yang_post_message(int32_t st, int32_t uid, YangSysMessageI* handle, void* user) {
	YangSysMessageHandle* inst = g_instance.load();
	if (inst) {
		inst->putMessage(handle, st, uid, 0, user);
	}
}

void yang_post_state_message(int32_t st, int32_t uid, int32_t handleState, YangSysMessageI* handle) {
	YangSysMessageHandle* inst = g_instance.load();
	if (inst) {
		inst->putMessage(handle, st, uid, handleState);
	}
}

YangSysMessageHandle::YangSysMessageHandle() {
	YangSysMessageHandle* expected = nullptr;
	g_instance.compare_exchange_strong(expected, this);

	m_loop = yangfalse;
	m_isStart = yangfalse;

	yang_thread_mutex_init(&m_mutex, NULL);
	yang_thread_mutex_init(&m_lock, NULL);
	yang_thread_cond_init(&m_cond_mess, NULL);

	m_receive = NULL;
}

YangSysMessageHandle::~YangSysMessageHandle() {
	if (m_isStart) {
		this->stop();

		while (m_isStart) {
			yang_usleep(1000);
		}
	}

	YangSysMessageHandle* expected = this;
	g_instance.compare_exchange_strong(expected, nullptr);

	yang_thread_mutex_destroy(&m_mutex);
	yang_thread_mutex_destroy(&m_lock);
	yang_thread_cond_destroy(&m_cond_mess);
}

void YangSysMessageHandle::run() {
	m_isStart = yangtrue;
    this->startLoop();
	m_isStart = yangfalse;
}

void YangSysMessageHandle::stop() {
	this->stopLoop();
}

void YangSysMessageHandle::putMessage(
	YangSysMessageI* handle, int32_t pst, int32_t uid, int32_t handleState, void* user) {
	if (!m_loop.load()) {
		return;
	}

	YangSysMessage* mes = new YangSysMessage();

	mes->uid = uid;
	mes->messageId = pst;
	mes->handleState = handleState;
	mes->handle = handle;
    mes->user=user;

    yang_thread_mutex_lock(&m_mutex);
	m_sysMessages.push_back(mes);
	yang_thread_mutex_unlock(&m_mutex);

	yang_thread_mutex_lock(&m_lock);
	yang_thread_cond_signal(&m_cond_mess);
	yang_thread_mutex_unlock(&m_lock);
}

void YangSysMessageHandle::startLoop() {
    m_loop = yangtrue;

    yang_thread_mutex_lock(&m_lock);
    while (m_loop.load()) {
        yang_thread_cond_wait(&m_cond_mess, &m_lock);

        while (true) {
            yang_thread_mutex_lock(&m_mutex);
            if (m_sysMessages.empty()) {
                yang_thread_mutex_unlock(&m_mutex);
                break;
            }

            YangSysMessage* mes = m_sysMessages.front();
            m_sysMessages.erase(m_sysMessages.begin());
            yang_thread_mutex_unlock(&m_mutex);
        
            this->handleMessage(mes);
            mes->handle = NULL;
            delete mes;
        }
    }
    yang_thread_mutex_unlock(&m_lock);
}

void YangSysMessageHandle::stopLoop() {
	yang_thread_mutex_lock(&m_lock);
	
	m_loop = yangfalse;
	yang_thread_cond_signal(&m_cond_mess);
	
	yang_thread_mutex_unlock(&m_lock);
}
