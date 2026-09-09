//
// Copyright (c) 2019-2025 yanggaofeng
//
#include <yangrtc/YangRtcStats.h>

#include <yangutil/sys/YangLog.h>
#include <yangutil/sys/YangTime.h>

void yang_stats_on_pub_video_rtp(YangSendStats* stats, YangRtpPacket* pkt, YangBuffer* buf) {
	// 统计已发送视频rtp包数量
	stats->videoRtpPacketCount++;
	// 统计已发送视频rtp包字节数
	stats->videoRtpBytes += yang_buffer_pos(buf);
    
	// 记录最后一个视频rtp包的发送时间
	stats->lastVideoSendTime = yang_get_system_time();
	// 记录最后一个视频rtp包的时间戳
	stats->lastVideoRtpTs = pkt->header.timestamp;
    
	// 如果视频开始时间未设置，则设置为最后一个视频rtp包的发送时间
	if (stats->videoStartTime == 0) {
		stats->videoStartTime = stats->lastVideoSendTime;
	}
}

void yang_stats_on_pub_audio_rtp(YangSendStats* stats, YangRtpPacket* pkt, YangBuffer* buf) {
	// 统计已发送音频rtp包数量
	stats->audioRtpPacketCount++;
	// 统计已发送音频rtp包字节数
	stats->audioRtpBytes += yang_buffer_pos(buf);
    
	// 记录最后一个音频rtp包的发送时间
	stats->lastAudioSendTime = yang_get_system_time();
	// 记录最后一个音频rtp包的时间戳
	stats->lastAudioRtpTs = pkt->header.timestamp;
    
	// 如果音频开始时间未设置，则设置为最后一个音频rtp包的发送时间
	if (stats->audioStartTime == 0) {
		stats->audioStartTime = stats->lastAudioSendTime;
	}
}

uint64_t yang_stats_getLostCount(YangRemoteRecvStats* stats) {
	int64_t expected = (stats->cycleCount << 16) + stats->maxSn - stats->startSn + 1;
	int64_t lost = expected - stats->rtpPacketCount;

	return lost > 0 ? (uint64_t)lost : 0;
}

uint32_t yang_stats_geLostInterval(YangRemoteRecvStats* stats) {
	uint64_t lost = yang_stats_getLostCount(stats);

	if (lost < stats->lastLost) {
		return 0;
	}

	uint32_t interval = (uint32_t)(lost - stats->lastLost);
    stats->lastLost = lost;
    
	return interval;
}

int64_t yang_stats_getExpectedPacketsInterval(YangRemoteRecvStats* stats) {

    int64_t expected = (stats->cycleCount << 16) + stats->maxSn - stats->startSn + 1;
    int32_t ret = expected - stats->lastExpected;
    stats->lastExpected = expected;
    return ret;
}

uint8_t yang_stats_getFractionLost(YangRemoteRecvStats* stats){
	 int64_t expected_interval = yang_stats_getExpectedPacketsInterval(stats);
	 uint8_t fraction_lost = 0;
	 uint32_t lostInterval=0;
	 if (expected_interval) {
		 lostInterval=yang_stats_geLostInterval(stats);
		 fraction_lost = (uint8_t)((lostInterval << 8) / expected_interval);
	 }
	 return fraction_lost;
}

void yang_stats_on_play_video_rtp(YangRecvStats* stats, YangRtpPacket* pkt) {
	uint16_t seq = pkt->header.sequence;

	if (stats->video.lastSn > 0xFF00 && 
		seq < 0xFF && 
		(!stats->video.cycleCount || stats->video.rtpPacketCount - stats->video.lastRtpPcackCount > 0x1FFF)
	) {

		stats->video.cycleCount++;
		stats->video.lastRtpPcackCount = stats->video.rtpPacketCount;
		stats->video.maxSn = seq;
	} 
	else if (seq > stats->video.maxSn) {
		stats->video.maxSn = seq;
	}

	if (!stats->video.rtpPacketCount) {
		stats->video.startSn = seq;
	} 
	else if (!stats->video.cycleCount && seq < stats->video.startSn) {
		stats->video.startSn = seq;
	}

	stats->video.lastSn = seq;
	stats->video.rtpPacketCount++;
}

void yang_stats_on_play_audio_rtp(YangRecvStats* stats,YangRtpPacket* pkt){
	stats->audio.rtpPacketCount++;
}

int32_t yang_stats_on_recvRR(
	yangbool isAudio,
	YangRecvStats* recvStats,
	YangSendStats* sendStats,   
	YangRtcpCommon* rtcp) {

	YangRemoteRecvStats* stats = isAudio ? &recvStats->audio : &recvStats->video;
    
	// 统计收到RR包数量
	++stats->reportsReceived;

	// 计算丢包率：该字段占1字节，取值范围为0-255，除以256.0得到0.0 ~ 1.0的丢包率
	stats->fractionLost = rtcp->rb->fraction_lost / 256.0;
    
	// 获取对端收到SR到回复RR的时间间隔
	uint32_t dlsr = rtcp->rb->dlsr;

	// 本端发出SR那一刻的NTP时间戳中间32位，由对端原样抄回来
	uint32_t lastSr = rtcp->rb->lsr;
    
	// lastSr大于0表示对端收到过SR消息，因此才能计算RTT
	if (lastSr > 0) {
		// 获取当前时间戳，单位为毫秒
		uint64_t timeMs = yang_get_milli_time();

		// 转换为NTP时间戳
		uint64_t ntpMs = (
			// 高32位：整秒（1970纪元 + 偏移 = 1900纪元）
			(timeMs / 1000 + 2208988800ULL) << 32 |
			// 低32位：秒的小数部分（单位 1/2^32 秒）
			(uint64_t)((timeMs % 1000) / 1000.0 * 4294967296.0)
		);
        
		// 取NTP时间戳的中间32位：秒的低16位 + 小数的高16位，单位1/65536秒，与LSR格式一致
		uint32_t currNtp = (uint32_t)((ntpMs >> 16) & 0xffffffff);

		// 计算RTT：RR到达时刻A − 本端发SR时刻(LSR) − 对端延迟(DLSR)
		uint32_t rtt = (currNtp - lastSr - dlsr) * 1000 / 65536;
        
		// 统计RTT
		stats->totalRtt += rtt;

		// RTT 最小取 1，避免为 0（0 会被当成"无数据"）
		stats->rtt = rtt > 0 ? rtt : 1;
	}

	return Yang_Ok;
}

void yang_create_rtcstats(YangRtcStats* stats) {
	if (stats == NULL) {
		return;
	}

	memset(stats, 0, sizeof(YangRtcStats));

	stats->on_pub_videoRtp = yang_stats_on_pub_video_rtp;
	stats->on_pub_audioRtp = yang_stats_on_pub_audio_rtp;

	stats->on_play_audioRtp = yang_stats_on_play_audio_rtp;
	stats->on_play_videoRtp = yang_stats_on_play_video_rtp;

	stats->on_recvRR = yang_stats_on_recvRR;
    
	stats->getLostCount = yang_stats_getLostCount;
	stats->getFractionLost = yang_stats_getFractionLost;
}

void yang_destroy_rtcstats(YangRtcStats* stats) {
	if (stats == NULL) {
		return;
	}

	memset(stats, 0, sizeof(YangRtcStats));
}
