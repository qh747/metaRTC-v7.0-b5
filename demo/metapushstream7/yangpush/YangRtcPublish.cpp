//
// Copyright (c) 2019-2025 yanggaofeng
//
#include <yangrtc/YangWhip.h>
#include <yangpush/YangRtcPublish.h>
#include <yangstream/YangStreamType.h>
#include <yangstream/YangStreamCapture.h>
#include <yangutil/sys/YangLog.h>
#include <yangavutil/video/YangNalu.h>
#include <yangavutil/video/YangMeta.h>
#include <yangavutil/video/YangVideoEncoderMeta.h>

YangRtcPublish::YangRtcPublish(YangContext* context) {
	m_context = context;

	m_videoBuffer = NULL;
	m_audioBuffer = NULL;
    
	m_isInit = 0;
	m_isStart = 0;
	m_isInLoop = 0;

	m_meta = NULL;
}

YangRtcPublish::~YangRtcPublish() {
	if (m_isInLoop) {
		this->stop();

		while (m_isStart) {
			yang_usleep(1000);
		}
	}

	m_context = NULL;
	m_videoBuffer = NULL;
	m_audioBuffer = NULL;
	m_meta = NULL;
}

void YangRtcPublish::sendRequest(int32_t uid, uint32_t ssrc, YangRequestType reqType) {
    if (m_context->streams) {
        m_context->streams->sendRequest(uid, ssrc, reqType);
	}        
}

void YangRtcPublish::stop() {
	m_isInLoop = 0;
}

void YangRtcPublish::run() {
	m_isStart = 1;
    m_isInLoop = 1;

	YangStreamCapture data;
	memset(&data, 0, sizeof(YangStreamCapture));

	yang_create_streamCapture(&data);

	data.initAudio(
		data.context,
		m_context->avinfo.sys.transType,
		m_context->avinfo.audio.sample, 
		m_context->avinfo.audio.channel,
		(YangAudioCodec)m_context->avinfo.audio.audioEncoderType
	);

	data.initVideo(
		data.context,
		m_context->avinfo.sys.transType
	);

	YangVideoCodec videoType = (YangVideoCodec)m_context->avinfo.video.videoEncoderType;

	YangVideoMeta* vmd = NULL;
	if(m_context->avinfo.enc.createMeta == 0) {
		vmd = (YangVideoMeta*)calloc(sizeof(YangVideoMeta), 1);
	}

	int32_t ret = Yang_Ok;

	while (m_isInLoop == 1) {
		// 等待peerConnection连接就绪
		if (NULL == m_peerConn || (NULL != m_peerConn && !m_peerConn->isConnected())) {
			yang_usleep(500);
			continue;
		}

		if (m_audioBuffer && m_audioBuffer->size() > 0) {
			YangFrame audioFrame;
	        memset(&audioFrame, 0, sizeof(YangFrame));

			audioFrame.payload = m_audioBuffer->getAudioRef(&audioFrame);

			data.setAudioData(data.context, &audioFrame);
			int32_t sendAudioResult = m_peerConn->on_audio(data.getAudioFrame(data.context));

			if (sendAudioResult != Yang_Ok) {
				m_peerConn->close();
				continue;
			}
		}

		if (m_videoBuffer && m_videoBuffer->size() > 0) {
			YangFrame videoFrame;
	        memset(&videoFrame, 0, sizeof(YangFrame));

			videoFrame.payload = m_videoBuffer->getEVideoRef(&videoFrame);

			if (videoFrame.frametype == YANG_Frametype_I) {
				if (m_meta) {
					data.setVideoMeta(
						data.context,
						m_meta->livingMeta.buffer,
						m_meta->livingMeta.bufLen, 
						videoType
					);
				} 
				else {
					if (!vmd->isInit) {
                        if (videoType == Yang_VED_H264) {
							yang_createH264Meta(vmd, &videoFrame);
							yang_getConfig_Flv_H264(
								&vmd->mp4Meta,
								vmd->livingMeta.buffer,
								&vmd->livingMeta.bufLen
							);
                        } 
                        else if (videoType == Yang_VED_H265) {
							yang_createH265Meta(vmd, &videoFrame);
							yang_getConfig_Flv_H265(
								&vmd->mp4Meta,
								vmd->livingMeta.buffer,
								&vmd->livingMeta.bufLen
							);
						}
					}

					data.setVideoMeta(
						data.context,
						vmd->livingMeta.buffer,
						vmd->livingMeta.bufLen, 
						videoType
					);
				}

				data.setVideoFrametype(data.context, YANG_Frametype_Spspps);
				data.setMetaTimestamp(data.context, videoFrame.pts);

                ret = m_peerConn->on_video(data.getVideoFrame(data.context));

				if (!m_context->avinfo.enc.createMeta) {
					YangH264NaluData nalu;
					memset(&nalu, 0, sizeof(YangH264NaluData));

                    if (videoType == Yang_VED_H264) {
                        yang_parseH264Nalu(&videoFrame, &nalu);
					}
                    else {
                        yang_parseH265Nalu(&videoFrame, &nalu);
					}

					if (nalu.keyframePos > -1) {
						videoFrame.payload += nalu.keyframePos + 4;
						videoFrame.nb -= (nalu.keyframePos + 4);
					} 
					else {
						videoFrame.payload = NULL;
						continue;
					}
				}
			}

			data.setVideoData(data.context, &videoFrame, videoType);
            int32_t sendVideoResult = m_peerConn->on_video(data.getVideoFrame(data.context));

			if (sendVideoResult != Yang_Ok) {
				m_peerConn->close();
			}
		}		
	}

	yang_destroy_streamCapture(&data);
	yang_free(vmd);

	m_isStart = 0;
}

int32_t YangRtcPublish::init(char* url, yangbool isWhip) {
    YangPeerInfo info;
    yang_avinfo_initPeerInfo(&info, &m_context->avinfo);

    info.uid = 0;
    info.direction = YangSendonly;

    m_peerConn = new YangPeerConnection7(
        &info,
		NULL,
		NULL,
		this,
		NULL
	);
    
	m_peerConn->addAudioTrack(Yang_AED_OPUS);
    m_peerConn->addVideoTrack(Yang_VED_H264);

    m_peerConn->addTransceiver(YangMediaAudio, info.direction);
    m_peerConn->addTransceiver(YangMediaVideo, info.direction);

	int32_t ret = isWhip ? 
	    yang_whip_connectWhipWhepServer(&m_peerConn->m_peer, url) : 
		yang_whip_connectSfuServer(
			&m_peerConn->m_peer, 
			url, 
			m_context->avinfo.sys.mediaServer
		);

    if (ret == Yang_Ok) {
        yang_reindex(m_audioBuffer);
        yang_reindex(m_videoBuffer);
	}

    return ret;
}
