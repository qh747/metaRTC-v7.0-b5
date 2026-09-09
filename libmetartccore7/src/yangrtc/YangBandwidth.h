//
// Copyright (c) 2019-2025 yanggaofeng
//

#ifndef SRC_YANGRTC_YANGBANDWIDTH_H_
#define SRC_YANGRTC_YANGBANDWIDTH_H_

#include <yangrtc/YangRtcTwcc.h>
#include <yangrtc/YangRtcStats.h>
#include <yangutil/yangavinfo.h>

#define Yang_LostRate_defaultCount 5

typedef enum {
	YangLowLostRate,
	YangHighLostRate

} YangLostRateState;

typedef struct {
	int32_t hightLostCount;
	int32_t lowLostCount;

	int32_t lastCheckSn;
	int32_t preCheckSn;

	int32_t lossPacketCount;
	double lostRate;

	YangLostRateState lostRateState;
	YangLostRateState preLostRateState;

} YangBandwidthSession;

typedef struct {
	YangBandwidthSession session;

	int32_t (*checkBandWidth)(
		YangBandwidthSession* bw,
		YangRtcStats* stats,
		YangPeerInfo* peerInfo,
		YangPeerCallback* peerCallback,
		uint32_t ssrc
	);

} YangBandwidth;

void yang_create_bandwidth(YangBandwidth* bw);
void yang_destroy_bandwidth(YangBandwidth* bw);

#endif // SRC_YANGRTC_YANGBANDWIDTH_H_
