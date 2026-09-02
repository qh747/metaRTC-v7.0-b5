
//
// Copyright (c) 2019-2025 yanggaofeng
//
#ifndef SRC_YANGMEETING_INCLUDE_YangRtcPublish_H_
#define SRC_YANGMEETING_INCLUDE_YangRtcPublish_H_

#include <yangrtc/YangPeerConnection7.h>
#include <yangutil/yangavinfotype.h>
#include <yangutil/sys/YangThread2.h>
#include <yangutil/sys/YangSysMessageI.h>
#include <yangutil/buffer/YangAudioEncoderBuffer.h>
#include <yangutil/buffer/YangVideoEncoderBuffer.h>

class YangRtcPublish : public YangThread, public YangCallbackRtc {
public:
	YangRtcPublish(YangContext* context);
	virtual ~YangRtcPublish();

public:
    virtual void setMediaConfig(int32_t uid, YangAudioParam* audio, YangVideoParam* video) {}
    virtual void sendRequest(int32_t uid, uint32_t ssrc, YangRequestType reqType);

public:
    virtual void stop();

private:
    virtual void run();

public:
    int32_t init(const char* url, yangbool isWhip);

	inline void setInVideoMetaData(YangVideoMeta* meta) { m_meta = meta; }
	inline void setInAudioList(YangAudioEncoderBuffer* buf) { m_audioBuffer = buf; }
	inline void setInVideoList(YangVideoEncoderBuffer* buf) { m_videoBuffer = buf; }

	inline void disConnect() { yang_delete(m_peerConn); }

public:
	int32_t m_isStart;

private:
    YangContext* m_context;
	YangVideoMeta* m_meta;

	YangAudioEncoderBuffer* m_audioBuffer;
	YangVideoEncoderBuffer* m_videoBuffer;

	int32_t m_isInLoop;
	int32_t m_isInit;

    YangPeerConnection7* m_peerConn;
};

#endif // SRC_YANGMEETING_INCLUDE_YangRtcPublish_H_
