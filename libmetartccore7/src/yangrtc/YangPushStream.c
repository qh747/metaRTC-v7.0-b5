//
// Copyright (c) 2019-2025 yanggaofeng
//
#include <yangrtc/YangPushStream.h>
#include <yangrtc/YangPush.h>
#include <yangrtc/YangRtcRtcp.h>
#include <yangrtc/YangBandwidth.h>
#include <yangrtp/YangRtpPacket.h>
#include <yangrtp/YangRtpConstant.h>
#include <yangrtp/YangRtpConstant.h>
#include <yangrtp/YangRtcpPsfbCommon.h>
#include <yangutil/sys/YangLog.h>

static int32_t yang_rtcpush_on_cache_nack(
	YangRtcPushStream* pub, 
	YangRtpPacket* pkt, 
	char* buf, 
	int len) {

	if (pub == NULL || pkt == NULL || buf == NULL) {
		return ERROR_RTC_PUBLISH;
	}

	if (pkt->frame_type == YangFrameTypeAudio) {
#if Yang_Enable_RTC_Audio
		yang_pubnackbuffer_set(
			pub->audio_queue, 
			pkt->header.sequence, 
			buf, 
			len
		);
#endif
	}
	else if (pkt->frame_type == YangFrameTypeVideo) {
#if Yang_Enable_RTC_Video
		yang_pubnackbuffer_set(
			pub->video_queue, 
			pkt->header.sequence, 
			buf, 
			len
		);
#endif
	}

	return Yang_Ok;
}

static int32_t yang_rtcpush_on_rtcp_rr(
	YangRtcContext* context, 
	YangRtcPushStream* pub, 
	YangRtcpCommon* rtcp) {

	if (rtcp->ssrc == pub->audioSsrc) {
		return context->stats.on_recvRR(
			yangtrue,
			&context->stats.recvStats,
			&context->stats.sendStats,
			rtcp
		);
    }
	else if (rtcp->ssrc == pub->videoSsrc) {
		return context->stats.on_recvRR(
			yangfalse,
			&context->stats.recvStats,
			&context->stats.sendStats,
			rtcp
		);
    }

	return Yang_Ok;
}

static int32_t yang_rtcpush_on_check_bindwidth(
	YangRtcContext* context,
	YangRtcPushStream* pub) {

	return context->bandwidth.checkBandWidth(
		&context->bandwidth.session,
		&context->stats,
		context->peerInfo,
		context->peerCallback,
		pub->videoSsrc
	);
}

static int32_t yang_rtcpush_on_rtcp_nack(
	YangRtcContext *context,
	YangRtcPushStream *pub, 
	YangRtcpCommon *rtcp) {

	if (context == NULL || pub == NULL|| rtcp == NULL) {
 		return ERROR_RTC_PUBLISH;
	}
    
	int32_t err = Yang_Ok;
    uint32_t ssrc = rtcp->nack->mediaSsrc;

	if (!ssrc) {
		return yang_error_wrap(err, "track response nack ssrc: %u", ssrc);
	}

	YangPublishNackBuffer* nackBuf = NULL;

    if (ssrc == pub->audioSsrc) {
#if Yang_Enable_RTC_Audio
		nackBuf = pub->audio_queue;
#endif
	}

    else if (ssrc == pub->videoSsrc) {
#if Yang_Enable_RTC_Video
	    nackBuf = pub->video_queue;
#endif
    }
    
	if (nackBuf == NULL) {
		return yang_error_wrap(err, "nack buffer is null. ssrc: %u", ssrc);
	}

	for (int32_t idx = 0; idx < rtcp->nack->vsize; ++idx) {
		uint16_t seq = rtcp->nack->nacks[idx];
		YangSendNackBuffer* pkt = yang_pubnackbuffer_at(nackBuf, seq);

		if (pkt == NULL || (pkt != NULL && pkt->seq != seq)) {
			continue;
		}
		
        err = yang_send_nackpacket(context, pkt->payload, pkt->nb);

		if (err != Yang_Ok) {
			return yang_error_wrap(err, "raw send");
		}
         
		yang_trace("send lost packet. seq: %hu,", seq);
		context->stats.sendStats.nackCount++;
	}

	return err;
}

static int32_t yang_rtcpush_do_request_keyframe(YangRtcContext* context, uint32_t ssrc) {
	if (context == NULL) {
		return ERROR_RTC_PUBLISH;
    }

	if (context->peerCallback && context->peerCallback->rtcCallback.sendRequest) {
		context->peerCallback->rtcCallback.sendRequest(
			context->peerCallback->rtcCallback.context,
			context->peerInfo->uid, 
			ssrc,
			Yang_Req_Sendkeyframe
		);
	}

	return Yang_Ok;
}

static int32_t yang_rtcpush_on_rtcp_ps_feedback(
	YangRtcContext* context,
	YangRtcPushStream* pub, 
	YangRtcpCommon* rtcp) {

	if (context == NULL || pub == NULL) {
		return ERROR_RTC_PUBLISH;
	}

	if (rtcp->header.rc == kPLI || rtcp->header.rc == kFIR) {
		if (pub->videoSsrc) {
			yang_rtcpush_do_request_keyframe(context, pub->videoSsrc);
		}
	}
	else {
		return yang_error_wrap(
			ERROR_RTC_RTCP, 
			"unknown payload specific feedback: %u", rtcp->header.rc
		);
	}

	return Yang_Ok;
}

static int32_t yang_rtcpush_on_rtcp(
	YangRtcContext* context,
	YangRtcPushStream* pub, 
	YangRtcpCommon* rtcp) {

	if (context == NULL || pub == NULL) {
		return ERROR_RTC_PUBLISH;
	}

	if (YangRtcpType_rr == rtcp->header.type) {
		return yang_rtcpush_on_rtcp_rr(context, pub, rtcp);
	} 
	else if (YangRtcpType_rtpfb == rtcp->header.type) {
		return yang_rtcpush_on_rtcp_nack(context, pub, rtcp);
	} 
	else if (YangRtcpType_psfb == rtcp->header.type) {
		return yang_rtcpush_on_rtcp_ps_feedback(context, pub, rtcp);
	} 

	return Yang_Ok;
}

static int32_t yang_rtcpush_send_rtcp_sr(YangRtcContext* context, YangRtcPushStream* pub) {
	if (yang_send_rtcp_sr(context, yangtrue, pub->audioSsrc) != Yang_Ok) {
		return yang_error_wrap(1, "send audio sr fail");
	}

	if (yang_send_rtcp_sr(context, yangfalse, pub->videoSsrc) != Yang_Ok) {
		return yang_error_wrap(1, "send video sr fail");
	}

	return Yang_Ok;
}

void yang_create_rtcpush(YangRtcPush* push, uint32_t audioSsrc, uint32_t videoSsrc) {
	if (push == NULL) {
		return;
    }

	YangRtcPushStream* pushStream = (YangRtcPushStream*)yang_calloc(
		1, 
		sizeof(YangRtcPushStream)
	);
	
	pushStream->mw_msgs = 0;

#if Yang_Enable_RTC_Audio
	pushStream->audio_queue = (YangPublishNackBuffer*)yang_calloc(
		1, 
		sizeof(YangPublishNackBuffer)
	);

	yang_create_pubNackbuffer(
		pushStream->audio_queue, 
		Yang_AUDIO_Publish_NackBuffer_Count
	);
#endif

#if Yang_Enable_RTC_Video
	pushStream->video_queue = (YangPublishNackBuffer*)yang_calloc(
		1,
		sizeof(YangPublishNackBuffer)
	);

	yang_create_pubNackbuffer(
		pushStream->video_queue, 
		Yang_Video_Publish_NackBuffer_Count
	);
#endif

	pushStream->audioSsrc = audioSsrc;
	pushStream->videoSsrc = videoSsrc;

	push->pubStream = pushStream;

	push->cache_nack = yang_rtcpush_on_cache_nack;

	push->send_rtcp_sr = yang_rtcpush_send_rtcp_sr;

	push->on_rtcp = yang_rtcpush_on_rtcp;
	push->on_rtcp_ps_feedback = yang_rtcpush_on_rtcp_ps_feedback;

	push->check_bandwidth = yang_rtcpush_on_check_bindwidth;
}

void yang_destroy_rtcpush(YangRtcPush* push) {
	if (push == NULL) {
		return;
    }

#if Yang_Enable_RTC_Audio
	yang_destroy_pubNackbuffer(push->pubStream->audio_queue);
	yang_free(push->pubStream->audio_queue);
#endif

#if Yang_Enable_RTC_Video
	yang_destroy_pubNackbuffer(push->pubStream->video_queue);
	yang_free(push->pubStream->video_queue);
#endif

	yang_free(push->pubStream);
}

