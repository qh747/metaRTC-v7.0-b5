//
// Copyright (c) 2019-2022 yanggaofeng
//

#include <yangutil/yangavinfotype.h>
#include <yangutil/sys/YangLog.h>
#include <yangpush/YangPushCommon.h>
#include "YangPushMessageHandle.h"

YangPushMessageHandle::YangPushMessageHandle(
	bool hasAudio,
	int videoType,
	YangVideoInfo* screenVideo,
	YangVideoInfo* outVideo,
	YangContext* context,
	YangSysMessageI* message,
	YangSysMessageHandleI* messageHandle) {

	m_context = context;
	m_receive = messageHandle;

    m_push = new YangPushHandleImpl(
		hasAudio,
		false,
		videoType,
		screenVideo,
		outVideo,
		context,
		message
	);
}

YangPushMessageHandle::~YangPushMessageHandle() {
	this->deleteAll();
}

void YangPushMessageHandle::deleteAll() {
	m_context = NULL;
	yang_delete(m_push);
}

void YangPushMessageHandle::handleMessage(YangSysMessage* mss) {
	int32_t ret = Yang_Ok;

	switch (mss->messageId) {
	    case YangM_Push_StartVideoCapture: {
            if (m_push) {
                m_push->changeSrc(Yang_VideoSrc_Camera, false);
			}
	    	break;
	    }
	    case YangM_Push_StartScreenCapture: {
            if (m_push) {
				m_push->changeSrc(Yang_VideoSrc_Screen, false);
			}
			break;
	    }
        case YangM_Push_StartOutCapture: {
            if (m_push) {
				m_push->changeSrc(Yang_VideoSrc_OutInterface, false);
			}
            break;
        }
        case YangM_Push_Connect: {
            if (mss->user && m_push) {
				ret = m_push->publish((char*)mss->user, yangfalse);
			}
            break;
        }
        case YangM_Push_Connect_Whip: {
            if (mss->user && m_push) {
				ret = m_push->publish((char*)mss->user,yangtrue);
			}
            break;
        }
        case YangM_Push_Disconnect: {
            if (m_push) {
				m_push->disconnect();
			}
			break;
        }
		case YangM_Push_SwitchToCamera: {
            if (m_push) {
				m_push->changeSrc(Yang_VideoSrc_Camera,false);
			}
			break;
        }
		case YangM_Push_SwitchToScreen: {
            if (m_push) {
				m_push->changeSrc(Yang_VideoSrc_Screen,false);
            }
			break;
		}
		break;
		case YangM_Sys_Setvr: {
		    ret = Yang_Ok;

#if Yang_Enable_Vr
		    if (m_push) {
				m_push->addVr();
			}
#endif
		    break;
		}
	    case YangM_Sys_UnSetvr: {
		    ret = Yang_Ok;
#if Yang_Enable_Vr
		    if (m_push) {
				m_push->delVr();
			}
#endif
		    break;
		}
	}

	if (mss->handle) {
		if (ret) {
			mss->handle->failure(ret);
		}
		else {
			mss->handle->success();
		}
	}

	if (m_receive) {
			m_receive->receiveSysMessage(mss,ret);
	}
}