//
// Copyright (c) 2019-2025 yanggaofeng
//

#include <yangice/YangRtcSocket.h>

#include <yangrtc/YangRtcRtcp.h>
#include <yangrtc/YangRtcSession.h>

#include <yangutil/sys/YangLog.h>
#include <yangutil/sys/YangSRtp.h>

#include <yangrtp/YangRtpConstant.h>
#include <yangstream/YangStreamType.h>

int32_t yang_send_rtcppacket(YangRtcContext *context, char *data, int32_t nb) {
	int32_t err = Yang_Ok;
	int32_t nn_encrypt = nb;

#if Yang_Enable_Dtls
	if ((err = yang_enc_rtcp(&context->srtp, data, &nn_encrypt)) != Yang_Ok) {
		return yang_error_wrap(err, "srtp protect");
	}
#endif

	return context->sock->write(&context->sock->session, data, nn_encrypt);
}

int32_t yang_send_rtcp_fb_pli(YangRtcContext *context, uint32_t ssrc) {

	char buf[kRtpPacketSize];
	YangBuffer stream;
	yang_init_buffer(&stream, buf, sizeof(buf));
	yang_write_1bytes(&stream, 0x81);
	yang_write_1bytes(&stream, kPsFb);
	yang_write_2bytes(&stream, 2);
	yang_write_4bytes(&stream, ssrc);
	yang_write_4bytes(&stream, ssrc);
	return yang_send_rtcppacket(context,stream.data,yang_buffer_pos(&stream));
}


int32_t yang_send_rtcp_fb_twcc(YangRtcContext *context, YangRecvTWCC* twcc,int32_t twcc_fb_count) {
	int32_t err = Yang_Ok;
	char pkt[kRtpPacketSize];
	YangBuffer stream ;
	yang_init_buffer(&stream,pkt, sizeof(pkt));
	if ((err = yang_twcc_recv_encode(twcc,&stream)) != Yang_Ok) {
		return yang_error_wrap(err, "encode, count=%u",twcc_fb_count);
	}

	return yang_send_rtcppacket(context,stream.data,yang_buffer_pos(&stream));
}

int32_t yang_send_rtcp_sr(YangRtcContext* context, yangbool isAudio, uint32_t ssrc) {
	YangSendStats* stats = &context->stats.sendStats;
    
	// 如果还未发送过rtp数据则不发送rtcp sr消息
	if (isAudio && stats->audioStartTime == 0) {
		return Yang_Ok;
	}
	else if (!isAudio && stats->videoStartTime == 0) {
		return Yang_Ok;
	}
    
	// 媒体时钟频率：音频用协商的采样率，视频固定 90000
    uint32_t clockRate = isAudio ? context->peerInfo->pushAudio.sample : 90000;

	// 1) NTP 时间戳：发送此刻的绝对墙上时间（公式与 on_recvRR 中保持一致）
    uint64_t timeMs = yang_get_milli_time();
    uint64_t ntp = (
		// 高32位：整秒（1970纪元 + 偏移 = 1900纪元）
		(timeMs / 1000 + 2208988800ULL) << 32) | 
		// 低32位：秒的小数部分（单位 1/2^32 秒）
	    (uint64_t)((timeMs % 1000) / 1000.0 * 4294967296.0
	);

	// 2) RTP 时间戳：以最后一个实际发出的媒体包为锚点外推
    uint64_t nowUs = yang_get_system_time();
    uint64_t lastSendUs = isAudio ? stats->lastAudioSendTime : stats->lastVideoSendTime;
    uint32_t lastRtpTs  = isAudio ? stats->lastAudioRtpTs    : stats->lastVideoRtpTs;

    uint32_t ts = lastRtpTs + (uint32_t)((nowUs - lastSendUs) * clockRate / YANG_UTIME_SECONDS);
    
	YangBuffer stream;
	char buf[kRtpPacketSize];

	yang_init_buffer(&stream, buf, sizeof(buf));

	yang_write_1bytes(&stream, 0x80);
	yang_write_1bytes(&stream, kSR);
	yang_write_2bytes(&stream, 6);
	yang_write_4bytes(&stream, ssrc);

	yang_write_8bytes(&stream, ntp);
	yang_write_4bytes(&stream, ts);
	yang_write_4bytes(&stream, isAudio ? stats->audioRtpPacketCount : stats->videoRtpPacketCount);
	yang_write_4bytes(&stream, isAudio ? stats->audioRtpBytes : stats->videoRtpBytes);

	return yang_send_rtcppacket(context, stream.data, yang_buffer_pos(&stream));
}

int32_t yang_send_rtcp_rr(YangRtcContext *context,yangbool isAudio, uint32_t ssrc,
		YangReceiveNackBuffer *rtp_queue, const uint64_t last_send_systime,
		YangNtp *last_send_ntp) {

	// @see https://tools.ietf.org/html/rfc3550#section-6.4.2
	uint8_t fraction_lost = 0;
	uint32_t interarrival_jitter;
	uint32_t rr_lsr,rr_dlsr,dlsr;

	uint32_t cumulative_number_of_packets_lost = 0 & 0x7FFFFF;
	uint32_t extended_highest_sequence =
			yang_nackbuffer_get_extended_highest_sequence(rtp_queue);
	YangBuffer stream;
	char buf[kRtpPacketSize];
	yang_init_buffer(&stream, buf, sizeof(buf));
	yang_write_1bytes(&stream, 0x81);
	yang_write_1bytes(&stream, kRR);
	yang_write_2bytes(&stream, 7);
	yang_write_4bytes(&stream, ssrc);


	if(!isAudio){
		fraction_lost=context->stats.getFractionLost(&context->stats.recvStats.video);
		cumulative_number_of_packets_lost=context->stats.getLostCount(&context->stats.recvStats.video) & 0x7FFFFF;
	}

	interarrival_jitter = 0;

	 rr_lsr = 0;
	 rr_dlsr = 0;

	if (last_send_systime > 0) {
		rr_lsr = (last_send_ntp->ntp_second << 16)	| (last_send_ntp->ntp_fractions >> 16);
		dlsr = (yang_update_system_time() - last_send_systime) / 1000;
		rr_dlsr = ((dlsr / 1000) << 16) | ((dlsr % 1000) * 65536 / 1000);
	}

	yang_write_4bytes(&stream, ssrc);
	yang_write_1bytes(&stream, fraction_lost);
	yang_write_3bytes(&stream, cumulative_number_of_packets_lost);
	yang_write_4bytes(&stream, extended_highest_sequence);
	yang_write_4bytes(&stream, interarrival_jitter);
	yang_write_4bytes(&stream, rr_lsr);
	yang_write_4bytes(&stream, rr_dlsr);

	return yang_send_rtcppacket(context,stream.data,yang_buffer_pos(&stream));
}

int32_t yang_send_rtcp_xr_rrtr(YangRtcContext *context, uint32_t ssrc) {


	/*
	 @see: http://www.rfc-editor.org/rfc/rfc3611.html#section-2

	 0                   1                   2                   3
	 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 |V=2|P|reserved |   PT=XR=207   |             length            |
	 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 |                              SSRC                             |
	 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 :                         report blocks                         :
	 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

	 @see: http://www.rfc-editor.org/rfc/rfc3611.html#section-4.4

	 0                   1                   2                   3
	 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 |     BT=4      |   reserved    |       block length = 2        |
	 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 |              NTP timestamp, most significant word             |
	 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 |             NTP timestamp, least significant word             |
	 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 */
	int64_t now = yang_update_system_time();
	char buf[kRtpPacketSize];
	YangBuffer stream;
	YangNtp cur_ntp;

	yang_ntp_from_time_ms(&cur_ntp, now / 1000);

	yang_init_buffer(&stream, buf, sizeof(buf));
	yang_write_1bytes(&stream, 0x80);
	yang_write_1bytes(&stream, kXR);
	yang_write_2bytes(&stream, 4);
	yang_write_4bytes(&stream, ssrc);
	yang_write_1bytes(&stream, 4);
	yang_write_1bytes(&stream, 0);
	yang_write_2bytes(&stream, 2);
	yang_write_4bytes(&stream, cur_ntp.ntp_second);
	yang_write_4bytes(&stream, cur_ntp.ntp_fractions);

	return yang_send_rtcppacket(context,stream.data,yang_buffer_pos(&stream));
}

int32_t yang_check_send_nacks(YangRtcContext *context, YangRtpRecvNack *nack,
		uint32_t ssrc, uint32_t *sent_nacks, uint32_t *timeout_nacks) {

	int32_t err=Yang_Ok;
	int32_t nb_protected_buf;
	char buf[kRtcpPacketSize];
	YangBuffer stream;

    yang_recvnack_initvec(nack,ssrc);
	yang_rtcpNack_clear(nack->rtcp.nack);

	yang_recvnack_get_nack_seqs(nack, nack->rtcp.nack, timeout_nacks);

	if (nack->rtcp.nack->vsize == 0)
		return Yang_Ok;


	yang_init_buffer(&stream, buf, sizeof(buf));

	yang_rtcpNack_init(&nack->rtcp,ssrc);

	yang_encode_rtcpNack(&nack->rtcp, &stream);
	nb_protected_buf = yang_buffer_pos(&stream);

#if Yang_Enable_Dtls
	if((err=yang_enc_rtcp(&context->srtp, stream.data, &nb_protected_buf))!=Yang_Ok){
		yang_rtcpNack_clear(nack->rtcp.nack);
		return yang_error_wrap(err, "check send nacks");
	}
#endif

	context->sock->write(&context->sock->session, stream.data, nb_protected_buf);
	yang_rtcpNack_clear(nack->rtcp.nack);

	return Yang_Ok;
}


