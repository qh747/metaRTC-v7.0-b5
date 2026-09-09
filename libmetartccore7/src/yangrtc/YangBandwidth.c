//
// Copyright (c) 2019-2025 yanggaofeng
//

#include <yangrtc/YangBandwidth.h>
#include <yangrtc/YangRtcStats.h>

static int32_t yang_bandwidth_check(
	YangBandwidthSession* bw,
	YangRtcStats* stats,
	YangPeerInfo* peerInfo,
	YangPeerCallback* peerCallback,
	uint32_t ssrc) {
    
	// 重置丢包率
	bw->lostRate = 0;
   
	// 如果当前的丢包率小于高丢包率阈值，并且上一轮的丢包率状态为低丢包率，则直接返回
	if (stats->recvStats.video.fractionLost < kDefaultHighLossThreshold && 
		bw->preLostRateState == YangLowLostRate) {
		return Yang_Ok;
	}

	// 如果当前的丢包率大于高丢包率阈值，则增加高丢包率计数
	if (stats->recvStats.video.fractionLost > kDefaultHighLossThreshold) {
		// 增加高丢包率计数
		bw->hightLostCount++;
		// 重置低丢包率计数
		bw->lowLostCount = 0;

		// 如果高丢包率计数大于等于默认丢包率计数，则设置丢包率为当前丢包率，并设置丢包率为高丢包率
		if (bw->hightLostCount >= Yang_LostRate_defaultCount) {
			// 设置丢包率为当前丢包率
			bw->lostRate = stats->recvStats.video.fractionLost;
			// 设置丢包率为高丢包率状态
			bw->lostRateState = YangHighLostRate;
		}
	}
	else {
		// 增加低丢包率计数
		bw->lowLostCount++;
		// 重置高丢包率计数
		bw->hightLostCount = 0;

		// 如果高丢包率计数大于等于默认丢包率计数，则设置丢包率为当前丢包率，并设置丢包率为低丢包率
		if(bw->hightLostCount >= Yang_LostRate_defaultCount) {
			// 设置丢包率为当前丢包率
			bw->lostRate = stats->recvStats.video.fractionLost;
			// 设置丢包率为低丢包率状态
			bw->lostRateState = YangLowLostRate;
		}
	}
    
	// 如果上一轮的丢包率状态与当前的丢包率状态相同，则直接返回
	if (bw->preLostRateState == bw->lostRateState) {
		return Yang_Ok;
	}

	// 更新上一轮的丢包率状态
	bw->preLostRateState = bw->lostRateState;

	if (bw->lostRateState == YangHighLostRate) {
		// 如果丢包率为高丢包率状态，则发送高丢包率请求
		if(peerCallback->rtcCallback.sendRequest) {
			peerCallback->rtcCallback.sendRequest(
				peerCallback->rtcCallback.context,
				peerInfo->uid,
				ssrc,
				Yang_Req_HighLostPacketRate
			);
		}
	}
	else if (bw->lostRateState == YangLowLostRate) {
		// 如果丢包率为低丢包率状态，则发送低丢包率请求
		if(peerCallback->rtcCallback.sendRequest) {
			peerCallback->rtcCallback.sendRequest(
				peerCallback->rtcCallback.context,
				peerInfo->uid,
				ssrc,
				Yang_Req_LowLostPacketRate
			);
        }
	}
    
	return Yang_Ok;
}

void yang_create_bandwidth(YangBandwidth* bw) {
	bw->checkBandWidth=yang_bandwidth_check;
}

void yang_destroy_bandwidth(YangBandwidth* bw) {

}

