//
// Copyright (c) 2019-2025 yanggaofeng
//
#include <yangencoder/YangOpenH264Encoder.h>
#include <yangutil/yangavinfotype.h>
#include <yangutil/sys/YangLog.h>
#include <yangutil/sys/YangEndian.h>
#include <yangavutil/video/YangMeta.h>

YangOpenH264Encoder::YangOpenH264Encoder() {
	m_isInit = 0;
	m_vbuffer = new uint8_t[YANG_VIDEO_ENCODE_BUFFER_LEN];

	m_sendKeyframe = 0;
	m_264Handle = NULL;
}

YangOpenH264Encoder::~YangOpenH264Encoder(void) {
	if (m_264Handle) {
		m_264Handle->Uninitialize();
		WelsDestroySVCEncoder (m_264Handle);

		m_264Handle = NULL;
	}

	if (m_vbuffer) {
		delete[] m_vbuffer;
		m_vbuffer = NULL;
	}
}

void YangOpenH264Encoder::sendMsgToEncoder(YangRtcEncoderMessage* msg) {
	if (msg->request == Yang_Req_Sendkeyframe) {
		m_sendKeyframe = 1;
	}
}

void YangOpenH264Encoder::setVideoMetaData(YangVideoMeta* meta) {

}

int32_t YangOpenH264Encoder::init(YangContext* context, YangVideoInfo* info) {
	if (m_isInit == 1) {
		return Yang_Ok;
	};
    
	memcpy(&m_videoInfo, info, sizeof(YangVideoInfo));

	int ret = WelsCreateSVCEncoder(&m_264Handle);

	SEncParamExt eparam;
	m_264Handle->GetDefaultParams(&eparam);
    
	// 使用场景为摄像头视频实时编码
	eparam.iUsageType = CAMERA_VIDEO_REAL_TIME;
	// 最大帧率
	eparam.fMaxFrameRate = (float)info->frame;
	// 设置编码器宽度
	eparam.iPicWidth = info->outWidth;
	// 设置编码器高度
	eparam.iPicHeight = info->outHeight;
	// 设置编码器目标比特率
	eparam.iTargetBitrate = info->rate * 1024;
	// 设置编码器模式
	eparam.iRCMode = RC_BITRATE_MODE;
	// 设置编码器时间层数
	eparam.iTemporalLayerNum = 1;
	// 设置编码器空间层数
	eparam.iSpatialLayerNum = 1;
	// 设置编码器去噪
	eparam.bEnableDenoise = false;
	// 设置编码器背景检测
	eparam.bEnableBackgroundDetection = true;
	// 设置编码器自适应量化
	eparam.bEnableAdaptiveQuant = false;
	// 设置编码器帧跳过
	eparam.bEnableFrameSkip = false;
	// 设置编码器长时参考
	eparam.bEnableLongTermReference = false;
    // 设置编码器内帧间隔
	eparam.uiIntraPeriod = 15u;
	// 设置编码器SPS/PPS ID策略
	eparam.eSpsPpsIdStrategy = CONSTANT_ID;
	// 设置编码器NAL添加控制
	eparam.bPrefixNalAddingCtrl = false;

	// 设置编码器第0空间层宽度
	eparam.sSpatialLayers[0].iVideoWidth = info->outWidth;
	// 设置编码器第0空间层高度
	eparam.sSpatialLayers[0].iVideoHeight = info->outHeight;
	// 设置编码器第0空间层帧率
	eparam.sSpatialLayers[0].fFrameRate = (float)info->frame;
	// 设置编码器第0空间层比特率
	eparam.sSpatialLayers[0].iSpatialBitrate = info->rate * 1024;
	// 设置编码器第0空间层最大比特率
	eparam.sSpatialLayers[0].iMaxSpatialBitrate = eparam.iMaxBitrate;
    
	// 初始化编码器
	m_264Handle->InitializeExt(&eparam);

	// 设置编码器输入像素格式
	int videoFormat = videoFormatI420;
	m_264Handle->SetOption(ENCODER_OPTION_DATAFORMAT, &videoFormat);

	yang_trace("openh264 encoder is ready.");

	m_isInit = 1;
	return Yang_Ok;
}

int32_t YangOpenH264Encoder::encode(YangFrame* frame, YangEncoderCallback* cb) {
	// 强制下一帧为关键帧
	if (m_sendKeyframe == 1) {
		m_sendKeyframe = 0;
		m_264Handle->ForceIntraFrame(true);
	}
    
	// 设置编码器输入图片信息
    SSourcePicture picInfo;
	memset(&picInfo, 0, sizeof(SSourcePicture));
    
	// 设置编码器输入图片宽度和高度
	picInfo.iPicWidth = m_videoInfo.outWidth;
	picInfo.iPicHeight = m_videoInfo.outHeight;
    
	// 设置编码器输入像素格式
	picInfo.iColorFormat = videoFormatI420;
    
	// 设置编码器输入图片步长
	picInfo.iStride[0] = picInfo.iPicWidth;
	picInfo.iStride[1] = picInfo.iPicWidth / 2;
	picInfo.iStride[2] = picInfo.iPicWidth / 2;
    
	// 设置编码器输入图片数据
	picInfo.pData[0] = frame->payload;
	picInfo.pData[1] = frame->payload + (m_videoInfo.outWidth * m_videoInfo.outHeight);
	picInfo.pData[2] = frame->payload + (m_videoInfo.outWidth * m_videoInfo.outHeight * 5 / 4);
    
	// 设置编码器输出编码信息
    SFrameBSInfo encInfo;
	memset(&encInfo, 0, sizeof(SFrameBSInfo));

	// 编码图片
	int err = m_264Handle->EncodeFrame(&picInfo, &encInfo);

	if (err) {
		yang_error("openh264 encode err. error code: %d", err);
		return ERROR_CODEC_Encode_Video;
	}
    
	// 将多个层级的编码数据拼接到m_vbuffer缓冲区中。例如：层0：SPS + PPS，层1：I帧
	int32_t encLen = 0;

	for (int i = 0; i < encInfo.iLayerNum; ++i) {
		SLayerBSInfo* layerInfo = &encInfo.sLayerInfo[i];

		if (layerInfo != NULL) {
			int layerLen = 0;
			int naluIdx = layerInfo->iNalCount - 1;

			do {
				layerLen += layerInfo->pNalLengthInByte[naluIdx];
				--naluIdx;

			} while (naluIdx >= 0);

			memcpy(m_vbuffer + encLen, (char*)(layerInfo->pBsBuf), layerLen);
			encLen += layerLen;
		}
	}

	frame->frametype = (videoFrameTypeIDR == encInfo.eFrameType) ? YANG_Frametype_I : YANG_Frametype_P;
	
	frame->payload = (frame->frametype == YANG_Frametype_I) ? m_vbuffer : m_vbuffer + 4;
	frame->nb = (frame->frametype == YANG_Frametype_I) ? encLen : encLen - 4;

	// 将SPS和PPS前的00 00 00 01修改为SPS和PPS的长度
	if (frame->frametype == YANG_Frametype_I) {
		int32_t spsPos = yang_find_pre_start_code(
			m_vbuffer, 
			encLen
		);

		if (spsPos < 0) {
			return 1;
		}

		int32_t ppsPos = yang_find_pre_start_code(
			m_vbuffer + 4 + spsPos, 
			encLen - 4 - spsPos
		);

		if (ppsPos < 0) {
			return 1;
		}
		
		ppsPos += 4 + spsPos;

		int32_t ipos = yang_find_pre_start_code(
			m_vbuffer + 4 + ppsPos, 
			encLen - 4 - ppsPos
		);

		if (ipos < 0) {
			return 1;
		}

		ipos += 4 + ppsPos;

		int32_t spsLen = ppsPos - spsPos - 4;
		int32_t ppsLen = ipos - ppsPos - 4;

		yang_put_be32((char*)m_vbuffer, (uint32_t)spsLen);
		yang_put_be32((char*)(m_vbuffer + 4 + spsLen), (uint32_t)ppsLen);
	}

	if (cb) {
	    cb->onVideoData(frame);
    }

	return Yang_Ok;
}


