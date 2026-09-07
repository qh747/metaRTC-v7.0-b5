//
// Copyright (c) 2019-2025 yanggaofeng
//

#include <yangrtc/YangPush.h>
#include <yangrtc/YangRtcConnection.h>
#include <yangutil/sys/YangLog.h>
#include <yangrtp/YangRtpHeader.h>
#include <yangrtp/YangRtpPacket.h>
#include <yangrtp/YangRtpConstant.h>
#include <yangrtc/YangPushAudio.h>

static int32_t yang_on_rtp(void* sess, YangPushAudioRtp* rtp, YangFrame* frame) {
	YangRtcSession* session = (YangRtcSession*)sess;
    
	// 1.设置rtp数据参数
	{
        // 重制上一次发送的rtp数据
	    yang_reset_rtpPacket(&rtp->audioRawPacket);
        
	    // 设置rtp首部
        rtp->audioRawPacket.header.payload_type = session->audioPayloadType;
	    rtp->audioRawPacket.header.ssrc = rtp->audioSsrc;
	    rtp->audioRawPacket.frame_type = YangFrameTypeAudio;
	    rtp->audioRawPacket.header.marker = yangtrue;
    
	    rtp->audioRawPacket.header.sequence = rtp->audioSeq++;
	    rtp->audioRawPacket.header.timestamp = frame->pts;
	    rtp->audioRawPacket.header.padding_length = 0;
	    rtp->audioRawPacket.payload_type = YangRtpPacketPayloadTypeRaw;
        
	    // 设置rtp负载
	    rtp->audioRawData.payload = rtp->audioBuffer;
        rtp->audioRawData.nb = frame->nb;
    
	    yang_memcpy(rtp->audioRawData.payload, frame->payload, frame->nb);
	}
	
	// 2.编码rtp数据
	{
        // 初始化rtp缓冲区
	    yang_init_buffer(
			&rtp->buf, 
			yang_get_rtpBuffer(rtp->audioRtpBuffer), 
			kRtpPacketSize
		);
        
	    // 编码rtp首部到缓冲区
	    int err = yang_encode_rtpHeader(&rtp->buf, &(rtp->audioRawPacket.header));
    
	    if (err != Yang_Ok) {
	    	return yang_error_wrap(
	    		err, 
	    		"audio rtp header(%d) encode packet fail", rtp->audioRawPacket.payload_type
	    	);
	    }
        
	    // 编码rtp负载到缓冲区
	    err = yang_encode_rtpPayload(&rtp->buf, &rtp->audioRawData);
    
	    if (err != Yang_Ok) {
	    	return yang_error_wrap(
	    		err, 
	    		"audio rtp payload(%d) encode packet fail", rtp->audioRawPacket.payload_type
	    	);
	    }
       
	    // 编码填充数据到缓冲区
	    if (rtp->audioRawPacket.header.padding_length > 0) {
	    	uint8_t padding = rtp->audioRawPacket.header.padding_length;
    
	    	if (!yang_buffer_require(&rtp->buf, padding)) {
	    		return yang_error_wrap(
					ERROR_RTC_RTP_MUXER, 
					"padding requires %d bytes", padding
				);
	    	}
    
	    	yang_memset(rtp->buf.head, padding, padding);
	    	yang_buffer_skip(&rtp->buf, padding);
	    }
	}
    
	// 3.发送rtp音频数据
	{
        int32_t err = yang_send_avpacket(session, &(rtp->audioRawPacket), &rtp->buf);
	    session->context.stats.sendStats.audioRtpPacketCount++;
        
	    if (err != Yang_Ok) {
	    	return yang_error_wrap(
	    		err, 
	    		"audio rtp send packet fail"
	    	);
	    }
        
	    // rtp音频数据统计回调
        if (session && session->context.stats.on_pub_audioRtp) {
            session->context.stats.on_pub_audioRtp(
	    		&session->context.stats.sendStats, 
	    		&(rtp->audioRawPacket), 
	    		&rtp->buf
	    	);
        }
	}

	return Yang_Ok;
}

void yang_create_pushAudio(YangPushAudio* push, YangRtpBuffer* buffer) {
	if (push == NULL) {
		return;
	}

	YangPushAudioRtp* rtp = (YangPushAudioRtp*)yang_calloc(sizeof(YangPushAudioRtp), 1);

	rtp->audioBuffer = (char*)yang_calloc(kRtpPacketSize, 1);
	rtp->audioSeq = 0;
	rtp->audioSsrc = 0;
	rtp->audioRtpBuffer = buffer;

	push->push = rtp;
	push->on_audio = yang_on_rtp;
}

void yang_destroy_pushAudio(YangPushAudio* push) {
	if (push == NULL|| push->push == NULL) {
		return;
	}

	YangPushAudioRtp* rtp = push->push;

	yang_free(rtp->audioBuffer);
	yang_destroy_rtpPacket(&rtp->audioRawPacket);

	yang_free(push->push);
}
