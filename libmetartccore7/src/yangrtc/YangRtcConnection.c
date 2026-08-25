//
// Copyright (c) 2019-2025 yanggaofeng
//
#include <yangice/YangRtcSocket.h>
#include <yangice/YangRtcStun.h>

#include <yangrtc/YangRtcRtcp.h>
#include <yangrtc/YangPushH264.h>
#include <yangrtc/YangPushH265.h>
#include <yangrtc/YangBandwidth.h>
#include <yangrtc/YangPullStream.h>
#include <yangrtc/YangPushStream.h>
#include <yangrtp/YangRtpConstant.h>
#include <yangrtp/YangRtcpCompound.h>
#include <yangrtc/YangRtcConnection.h>

#include <yangutil/sys/YangLog.h>
#include <yangutil/yangavinfo.h>
#include <yangutil/sys/YangSsrc.h>
#include <yangutil/sys/YangSRtp.h>

#include <yangsdp/YangSdp.h>

static void yang_session_receive(char* buf, int32_t len, void* user) {
	YangRtcConnection* conn = (YangRtcConnection*)user;

	if (user != NULL) {
	    conn->receive(conn->session, buf, len);
    }
}

static void yang_start_stun_timer(void* user) {
	YangRtcConnection* conn = (YangRtcConnection*)user;

	if (user == NULL) {
        return;
	}
	
	if (conn->session->tm_1s && !conn->session->tm_1s->isStart) {
        yang_timer_start(conn->session->tm_1s);
	}
		
	conn->session->isSendStun = yangtrue;
}

static void yang_conn_state_change(YangRtcSession* session, YangRtcConnectionState state) {
    if (session->context.peerCallback == NULL) {
		return;
	}

	if (session->context.peerCallback->iceCallback.onConnectionStateChange == NULL) {
		return;
	}

	session->context.peerCallback->iceCallback.onConnectionStateChange(
		session->context.peerCallback->iceCallback.context,
		session->context.peerInfo->uid,
		state
	);
}

static void yang_exec_timer_task(int32_t taskId, void* user) {
	YangRtcSession* session = (YangRtcSession*)user;

	if (session == NULL) {
        return;
	}
    
	do {
        if (session->isControlled || !session->isSendStun || taskId != 1) {
			break;
		}

		if (session->context.stun.data == NULL || session->context.stun.nb <= 0) {
			break;
		}

		int32_t result = session->context.sock->write(
			&session->context.sock->session,
			session->context.stun.data, 
			session->context.stun.nb
		);

		if (result != Yang_Ok) {
            yang_error("send stun fail!");
		}

		if (session->context.state == Yang_Conn_State_New) {
			session->context.state = Yang_Conn_State_Connecting;
			yang_conn_state_change(session, session->context.state);
		}

	} while(yangfalse);

	if (session->context.state != Yang_Conn_State_Connected) {
		return;
	}

    do {
		if (!session->startRecv || session->play == NULL) {
			break;
		}

		if (taskId == 1) {
			int32_t result = session->play->send_rtcp_rr(&session->context, session->play->pullStream);

			if (result != Yang_Ok) {
                yang_error("RTCP Error:RR err ");
			}

#if Yang_Enable_RtcpXr
            if (session->context.streamConfig->streamDirection != YangRecvonly) {
				break;
			}

            result = session->play->send_rtcp_xr_rrtr(&session->context, session->play->pullStream);

			if (result != Yang_Ok) {
                yang_error("RTCP Error:XR err ");
			}
#endif
		}

		if (taskId == 100) {
#if Yang_Enable_RtcpXr
            if (session->context.streamConfig->streamDirection != YangRecvonly) {
				break;
			}

			int32_t result = session->play->send_rtcp_xr_rrtr(&session->context, session->play->pullStream);

			if (result != Yang_Ok) {
				yang_error("RTCP Error:XR err ");
			}
#endif
		}

	} while(yangfalse);

	do {
		if (session->push == NULL || taskId != 1) {
			break;
		}

		int32_t result = session->push->send_rtcp_sr(&session->context, session->push->pubStream);

		if (result != Yang_Ok) {
			yang_error("send rtcp sr Error ");
		}

		result = session->push->check_bandwidth(&session->context, session->push->pubStream);

		if (result != Yang_Ok) {
			yang_error("check bandwidth Error ");
		}

		result = session->push->check_twcc(&session->context, session->push->pubStream);

		if (result != Yang_Ok) {
			yang_error("check twcc Error ");
		}

		if (session->play && session->context.stats.recvStats.video.rtt > 0) {
			session->play->update_rtt(
				&session->context,
				session->play->pullStream,
				session->play->pullStream->videoTrack->session.track.ssrc,
				session->context.stats.recvStats.video.rtt
			);
		}

	} while(yangfalse);
}

static int32_t yang_rtcconn_dispatch_rtcp(YangRtcSession* session, YangRtcpCommon* rtcp) {
	uint16_t rtcpType = rtcp->header.type;

	// For TWCC packet.
	if (YangRtcpType_rtpfb == rtcpType && 15 == rtcp->header.rc) {
#if Yang_Enable_TWCC
	    session->context.twcc.decode(&session->context.twcc.session, rtcp);
#endif
	    return Yang_Ok;
	}

	// For REMB packet.
	if (YangRtcpType_psfb == rtcpType && 15 == rtcp->header.rc) {
		return Yang_Ok;
	}

	// Ignore special packet.
	if (YangRtcpType_rr == rtcpType && rtcp->rb->ssrc == 0) {
		return Yang_Ok;
	}

	int32_t err = Yang_Ok;

	if (session->push != NULL) {
		err = session->push->on_rtcp(&session->context, session->push->pubStream, rtcp);

		if (err != Yang_Ok) {
			return yang_error_wrap(err, "handle publish rtcp");
		}
	}

	if (session->play != NULL) {
		err = session->play->on_rtcp(&session->context,	session->play->pullStream, rtcp);

		if (err != Yang_Ok) {
			return yang_error_wrap(err, "handle play rtcp");
		}
	}

	return err;
}

static void yang_rtcconn_startTimers(YangRtcSession* session) {
	if (session->tm_1s && !session->tm_1s->isStart) {
        yang_timer_start(session->tm_1s);
	}

#if Yang_Enable_TWCC
	YangStreamDirection opt = session->context.streamConfig->streamDirection;

	if (session->context.twccId > 0 && 
		(opt == YangRecvonly || opt == YangSendrecv) &&
		session->tm_100ms && 
		!session->tm_100ms->isStart) {
        
		yang_timer_start(session->tm_100ms);
	}
#endif
}

static int32_t yang_rtcconn_on_rtcp(YangRtcSession* session, char* data, int32_t nb_data) {
	int32_t err = Yang_Ok;
    
	char* unprotected_buf = data;
	int32_t nb_unprotected_buf = nb_data;

#if Yang_Enable_Dtls
    err = yang_dec_rtcp(&session->context.srtp, data, &nb_unprotected_buf);

	if (err != Yang_Ok) {
		return (err != srtp_err_status_replay_fail) ?
		    yang_error_wrap(err, "rtcp unprotect") : 
			Yang_Ok;
	}
#endif
    
    YangBuffer buffer;
	yang_init_buffer(&buffer, unprotected_buf, nb_unprotected_buf);
    
	err = yang_decode_rtcpCompound(&session->rtcp_compound, &buffer);

	if (err != Yang_Ok) {
		return yang_error_wrap(err, "decode rtcp plaintext=%u", nb_unprotected_buf);
	}

	for (int32_t i = 0; i < session->rtcp_compound.rtcpVector.vsize; i++) {
		YangRtcpCommon* rtcp = &session->rtcp_compound.rtcpVector.payload[i];
		err = yang_rtcconn_dispatch_rtcp(session, rtcp);

		if (Yang_Ok != err) {
			yang_rtcpCompound_clear(&session->rtcp_compound);

			return yang_error_wrap(
				err,
				"cipher=%u, plaintext=%u, rtcp=(%u,%u,%u,%u)", 
				nb_data,
				nb_unprotected_buf, 
				rtcp->nb_data, 
				rtcp->header.rc,
				rtcp->header.type, 
				rtcp->ssrc
			);
		}
	}

	yang_rtcpCompound_clear(&session->rtcp_compound);
	return err;
}

static yangbool yang_rtcconn_alive(YangRtcSession* session) {
	if (session == NULL || session->context.state != Yang_Conn_State_Connected) {
        return yangfalse;
	}
		
	return session->lastStunTime + session->sessionTimeout > yang_get_system_time();
}

static void yang_rtcconn_startudp(YangRtcSession* session) {
	if (session == NULL) {
		return;
	}

	session->videoCodec = (YangVideoCodec)session->context.peerInfo->pushVideo.videoEncoderType;
	session->isSendDtls = 0;

	session->sessionTimeout = session->context.peerInfo->rtc.sessionTimeout;

	YangRtcDirection direct = session->context.peerInfo->direction;

    if (direct == YangSendonly || direct == YangSendrecv) {
#if Yang_Enable_RTC_Audio
    	if(session->pushAudio == NULL) {
    		session->pushAudio = (YangPushAudio*)yang_calloc(1, sizeof(YangPushAudio));
    	}

		if(session->pushAudioRtpBuffer == NULL) {
			session->pushAudioRtpBuffer = (YangRtpBuffer*) yang_calloc(1,sizeof(YangRtpBuffer));
			
			yang_create_rtpBuffer(
				session->pushAudioRtpBuffer, 
				100, 
				kRtpPacketSize
			);
		}
		
		yang_create_pushAudio(
			session->pushAudio, 
			session->pushAudioRtpBuffer
		);

		session->pushAudio->push->audioSsrc = session->context.audioSsrc;
#endif

#if Yang_Enable_RTC_Video
        if (session->pushVideoRtpBuffer == NULL) {
			session->pushVideoRtpBuffer = (YangRtpBuffer*)yang_calloc(1, sizeof(YangRtpBuffer));

			yang_create_rtpBuffer(
				session->pushVideoRtpBuffer, 
				1400, 
				kRtpPacketSize
			);
		}

    	session->pushH264 = NULL;

        if (session->videoCodec == Yang_VED_H264) {
            session->pushH264 = (YangPushH264*)yang_calloc(1, sizeof(YangPushH264));

			yang_create_pushH264(
				session->pushH264, 
				session->pushVideoRtpBuffer
			);

			session->pushH264->push->videoSsrc = session->context.videoSsrc;
        }

#if	Yang_Enable_H265_Encoding
        session->pushH265 = NULL;

        if (session->videoCodec == Yang_VED_H265) {
            session->pushH265 = (YangPushH265*)yang_calloc(1, sizeof(YangPushH265));

			yang_create_pushH265(
				session->pushH265,  
				session->pushVideoRtpBuffer
			);

			session->pushH265->push->videoSsrc = session->context.videoSsrc;
        }
#endif
#endif
		if (session->push == NULL) {
			session->push = (YangRtcPush*)yang_calloc(1, sizeof(YangRtcPush));

			yang_create_rtcpush(
				session->push,
				session->context.audioSsrc, 
				session->context.videoSsrc
			);
		}
    }

	if (direct == YangRecvonly || direct == YangSendrecv) {
		if (session->playRtpBuffer == NULL) {
			session->playRtpBuffer = (YangRtpBuffer*)yang_calloc(1, sizeof(YangRtpBuffer));

			yang_create_rtpBuffer(
				session->playRtpBuffer, 
				1500, 
				kRtpPacketSize
			);
		}

		if (session->play == NULL) {
			session->play = (YangRtcPull*)yang_calloc(1, sizeof(YangRtcPull));

			yang_create_rtcpull(
				&session->context, 
				session->play,
				session->playRtpBuffer
			);

		}
	}

#if Yang_Enable_Dtls
	yang_create_rtcdtls(session->context.dtls, session->isControlled);

	session->context.dtls->session.sslCallback = &session->context.peerCallback->sslCallback;
	session->context.dtls->session.uid = session->context.peerInfo->uid;
#endif

	session->tm_1s = (YangCTimer*)yang_calloc(1, sizeof(YangCTimer));
	yang_create_timer(session->tm_1s, session, 1, 1000);

	session->tm_1s->doTask = yang_exec_timer_task;

	session->tm_100ms = (YangCTimer*) yang_calloc(1, sizeof(YangCTimer));
	yang_create_timer(session->tm_100ms, session, 100, 100);

	session->tm_100ms->doTask = yang_exec_timer_task;

	session->startRecv = 0;
	session->isSendStun = yangfalse;

	session->activeState = yangtrue;
}

static int32_t yang_rtcconn_on_video(YangRtcSession* session, YangFrame* frame) {
    if (session == NULL || frame == NULL || session->context.state != Yang_Conn_State_Connected) {
        return Yang_Ok;
	}

#if Yang_Enable_Dtls
	if (session->context.dtls->session.state != YangDtlsStateClientDone) {
		return Yang_Ok;
	}
#endif

#if Yang_Enable_RTC_Video
    if (frame->frametype == YANG_Frametype_Spspps) {
		if (session->pushH264 != NULL) {
            return session->pushH264->on_spspps(session, session->pushH264->push, frame);
	    }

#if	Yang_Enable_H265_Encoding
	    if (session->pushH265 != NULL) {
            return session->pushH265->on_spspps(session, session->pushH265->push, frame);
		}
#endif
	}
    else {
        if (session->pushH264 != NULL) {
            return session->pushH264->on_video(session, session->pushH264->push, frame);
	    }

#if	Yang_Enable_H265_Encoding
	    if (session->pushH265 != NULL) {
            return session->pushH265->on_video(session, session->pushH265->push, frame);
	    }
#endif
	}
#endif

	return Yang_Ok;
}

static int32_t yang_rtcconn_on_audio(YangRtcSession* session, YangFrame* frame) {
    if (session == NULL || frame == NULL || session->context.state != Yang_Conn_State_Connected) {
        return Yang_Ok;
	}

#if Yang_Enable_Dtls
	if (session->context.dtls->session.state != YangDtlsStateClientDone) {
		return Yang_Ok;
	}
#endif

#if Yang_Enable_RTC_Audio
	if (session->pushAudio != NULL) {
        return session->pushAudio->on_audio(session, session->pushAudio->push, frame);
	}
#endif

	return Yang_Ok;
}

static int32_t yang_rtcconn_on_message(YangRtcSession* session, YangFrame* frame) {
    if (session == NULL || frame == NULL || session->context.state != Yang_Conn_State_Connected) {
        return Yang_Ok;
	}

#if Yang_Enable_Dtls
	if (session->context.dtls->session.isRecvAlert || 
		session->context.dtls->session.state != YangDtlsStateClientDone) {
    
		return Yang_Ok;
	}
		
#endif

#if Yang_Enable_Datachannel
	if (session->datachannel && session->datachannel->send_message) {
		session->datachannel->send_message(session->datachannel->context, frame);
	}
#endif

	return Yang_Ok;
}

static void yang_rtcconn_close(YangRtcSession* session) {
	if (session == NULL) {
        return;
	}
		
#if Yang_Enable_Dtls
	session->context.dtls->session.isSendAlert = yangtrue;

	if (session->context.dtls &&
		!session->context.dtls->session.isRecvAlert && 
		session->context.dtls->sendDtlsAlert) {

		session->context.dtls->sendDtlsAlert(&session->context.dtls->session);
	}
#else
	if (session->isControlled == 0) {
		char alerts[15];
		yang_memset(alerts, 0, 15);

		alerts[0] = 30;

		for (int i = 0; i < 5; i++) {
			yang_usleep(1000 * 20);

			if (yang_rtc_sendData(session->context.sock, alerts, 15) != Yang_Ok) {
				yang_error("send error");
			}
		}
	}
#endif

	session->context.state = Yang_Conn_State_Closed;
	yang_conn_state_change(session, session->context.state);
}


static int32_t yang_rtcconn_notify(YangRtcSession* session, YangRtcMessageType mess) {
#if Yang_Enable_RTC_Video
	if (session == NULL || session->play == NULL || session->play->pullStream->videoTrack == NULL) {
        return ERROR_RTC_CALLBACK;
	}

	if (mess == YangRTC_Decoder_Error) {
		session->play->pullStream->videoTrack->setRequestKeyframeState(
			&(session->play->pullStream->videoTrack->session), 
			yangfalse
		);

		return yang_send_rtcp_fb_pli(
			&session->context, 
			session->play->pullStream->videoTrack->session.track.ssrc
		);
	}
#endif

	return Yang_Ok;
}

static void yang_rtcconn_receive(YangRtcSession* session, char* data, int32_t size) {
	if (session == NULL || !session->activeState) {
        return;
	}

	session->lastStunTime = yang_get_system_time();

	uint8_t bt = (uint8_t)data[0];

	//rtp or rtcp
	if (bt > 127 && bt < 192 && size > 12) {
		bt = (uint8_t)data[1];
        
		// rtcp
		if (bt >= 192 && bt <= 223) {
			yang_rtcconn_on_rtcp(session, data, size);
		}
		// rtp
		else {
            session->startRecv = 1;
            
			if (session->play) {
				session->play->on_rtp(&session->context, session->play->pullStream, data, size);
			}
		}
	}
	// stun
	else if (size > 0 && (bt==0x00 || bt==0x01)) {
		// stun binding request
		if (bt == 0x00) {
			YangStunPacket request;
			yang_memset(&request, 0, sizeof(YangStunPacket));
            
			int32_t err = session->ice.session.stun.decode(&request, data, size);

			if (err != Yang_Ok) {
				yang_error("decode stun packet failed");

				session->context.state = Yang_Conn_State_Failed;
				yang_conn_state_change(session, session->context.state);
				return;
			} 

			err = session->ice.session.stun.createResponseStunPacket(&request, session);

			if (err != Yang_Ok) {
				yang_error("create response stun packet failed");
				return;
			}

			if (session->context.state == Yang_Conn_State_New) {
				session->context.state = Yang_Conn_State_Connecting;
				yang_conn_state_change(session, session->context.state);
			}
		}
		// stun binding response
		else if (bt == 0x01 && data[1] == 0x01) {
			int32_t err = session->ice.session.stun.decode2(data, size);

			if (err != Yang_Ok) {
				yang_error("decode stun packet failed");
				return;
			}

#if Yang_Enable_Dtls
			if (!session->isSendDtls) {
				session->isSendDtls = yangtrue;

                err = session->context.dtls->startHandShake(&session->context.dtls->session);

				if (err != Yang_Ok) {
                    yang_error("dtls start handshake failed!");
				}
			}
#else
			if (session->context.state == Yang_Conn_State_Connecting) {
				session->context.state = Yang_Conn_State_Connected;
		        yang_conn_state_change(session, session->context.state);
		        yang_rtcconn_startTimers(session);
		        
				if (session->context.peerCallback && 
					session->context.peerCallback->rtcCallback.sendRequest) {
                
					session->context.peerCallback->rtcCallback.sendRequest(
						session->context.peerCallback->rtcCallback.context,
						session->context.peerInfo->uid, 
						0,
						Yang_Req_Connected
					);
				}
			}
#endif
		}
	}
	// dtls
	if (bt > 19 && bt < 64) {
#if Yang_Enable_Dtls
		if (session->context.dtls == NULL) {
            return;
		}

        int32_t err = session->context.dtls->processData(
#if Yang_Enable_Datachannel
			session->datachannel,
#else
            NULL,
#endif
			&session->context.dtls->session, 
			data,
			size
		);

		if (err == Yang_Ok && 
			session->context.state == Yang_Conn_State_Connecting &&
		    session->context.dtls->session.handshake_done) {
			
			session->context.state = Yang_Conn_State_Connected;
			yang_conn_state_change(session, session->context.state);

			yang_rtcconn_startTimers(session);
	    }

		if (!session->isControlled && 
			session->context.peerCallback && 
			session->context.peerCallback->rtcCallback.sendRequest) {

			session->context.peerCallback->rtcCallback.sendRequest(
				session->context.peerCallback->rtcCallback.context,
				session->context.peerInfo->uid, 
				0,
				Yang_Req_Connected
			);
		}
#else
		if (session->isControlled) {
			session->context.streamConfig->sslCallback.sslAlert(
				session->context.streamConfig->sslCallback.context,
				session->context.streamConfig->uid,
				"warning",
				"CN"
			);
		}
#endif
	}
}

static void yang_rtcconn_on_ice(YangRtcSession* session, char* remoteIp, int32_t port) {
	if (session == NULL || remoteIp == NULL) {
        return;
	}

	session->context.sock->updateRemoteAddress(&session->context.sock->session, remoteIp, port);
}

static int32_t yang_rtcconn_add_audio_track(YangRtcSession* session, YangAudioCodec codec) {
	if (session == NULL) {
        return ERROR_RTC_CONNECT;
	}

	session->context.audioCodec = codec;
	session->context.enableAudioTrack = yangtrue;

	if (codec == Yang_AED_OPUS) {
        session->audioPayloadType = YangAudioPayloadType;
	}

	return Yang_Ok;
}

static int32_t yang_rtcconn_add_video_track(YangRtcSession* session, YangVideoCodec codec) {
	if (session == NULL) {
        return ERROR_RTC_CONNECT;
	}

	session->context.videoCodec = codec;
	session->context.enableVideoTrack = yangtrue;

	return Yang_Ok;
}

static int32_t yang_rtcconn_add_transceiver(
	YangRtcSession* session,
	YangMediaTrack media,
	YangRtcDirection direction) {

	if (session == NULL) {
        return ERROR_RTC_CONNECT;
	}

    if (media == YangMediaAudio) {
        session->context.audioDirection = direction;
	}
    else if (media == YangMediaVideo) {
        session->context.videoDirection = direction;
	}
		
	return Yang_Ok;
}

static int32_t yang_rtcconn_create_datachannel(YangRtcSession* session) {
	if (session == NULL) {
        return ERROR_RTC_CONNECT;
	}

	session->enableDatachannel = yangtrue;

#if Yang_Enable_Datachannel
	if (session->datachannel == NULL) {
		session->datachannel = (YangDatachannel*)yang_calloc(sizeof(YangDatachannel), 1);
		yang_create_datachannel(session->datachannel, &session->context);
	}
#endif

	return Yang_Ok;
}

static void yang_rtcconn_turn_receive(void* session, char* data, int32_t size) {
	if (session == NULL || data == NULL) {
        return;
	}

	yang_rtcconn_receive((YangRtcSession*)session, data, size);
}

static int32_t yang_rtcconn_turn_send_data(YangRtcSocketSession* session, char* data, int32_t nb) {
	if (session == NULL || data == NULL) {
        return ERROR_RTC_SOCKET;
	}

	YangRtcConnection* conn = (YangRtcConnection*)session->user;

	if (conn->session->ice.session.turnconn && conn->session->ice.session.turnconn->sendData) {
		return conn->session->ice.session.turnconn->sendData(
			&(conn->session->ice.session.turnconn->session),
			conn->session->ice.session.uid,
			data,
			nb
		);
	}

	return ERROR_RTC_TURN;
}

static int32_t yang_rtcconn_create_offer(YangRtcSession* session, char** psdp) {
	if (session == NULL) {
        return ERROR_RTC_CONNECT;
	}

	int32_t err = session->ice.initIce(&session->ice.session);

	return (err != Yang_Ok) ?
	    yang_error_wrap(
			err,
			"ice request fail(%s)",
			(session->ice.session.candidateType == YangIceStun) ? "STUN" : "TURN"
		) :
        yang_sdp_genLocalSdp(
	    	session, 
	    	session->context.peerInfo->rtc.rtcLocalPort, 
	    	psdp,
	    	session->context.peerInfo->direction
	    );
}

static int32_t yang_rtcconn_create_answer(YangRtcSession* session, char* answer) {
	if (session == NULL || answer == NULL) {
        return ERROR_RTC_CONNECT;
	}
    
	session->isControlled = yangtrue;
	session->context.peerInfo->rtc.isControlled = yangtrue;

	return yang_sdp_genLocalSdp2(
		session,
		session->context.peerInfo->rtc.rtcLocalPort, 
		answer,
		session->context.peerInfo->direction
	);
}

static int32_t yang_rtcconn_set_local_description(YangRtcSession* session, char* sdp) {
	if (session == NULL || sdp == NULL) {
		return ERROR_RTC_PEERCONNECTION;
	}

	yang_trace("\nstartRtc,port=%d", session->context.peerInfo->rtc. rtcLocalPort);

	if (session->context.peerInfo->mediaServer == Yang_Server_P2p) {
		if (session->ice.session.candidateType > YangIceHost) {
            yang_trace(
				"\nstart ice %s", 
				session->ice.session.candidateType == YangIceTurn ? "TRUN" : "STUN"
			);
		}

		if (session->ice.session.candidateType == YangIceTurn && 
			session->ice.session.iceState == YangIceSuccess) {

			return Yang_Ok;
		}
	}

	session->context.sock->session.isControlled = session->isControlled;

	int32_t err = yang_create_rtcsocket(
		session->context.sock,
		session->context.peerInfo->familyType,
		(YangSocketProtocol)session->context.peerInfo->rtc.rtcSocketProtocol,
		session->context.peerInfo->rtc.rtcLocalPort
	);

	if (err != Yang_Ok) {
        return yang_error_wrap(err, "set local description error!");
	}

#if Yang_Enable_Tcp_Srs
	if (session->context.avinfo->sys.mediaServer == Yang_Server_Srs) {
		yang_create_rtcsocket_srs(
			session->context.sock,
			(YangSocketProtocol)session->context.avinfo->rtc.rtcSocketProtocol
		);
	}
#endif

	yang_rtcconn_startudp(session);

	session->context.sock->start(&session->context.sock->session);
	return err;
}

static int32_t yang_rtcconn_set_remote_description(YangRtcSession* session, char* sdpstr) {
	if (session == NULL || sdpstr == NULL) {
		return ERROR_RTC_PEERCONNECTION;
    }

	if (session->context.peerInfo->mediaServer == Yang_Server_P2p && 
		session->ice.session.iceState == YangIceFail) {

		yang_error("p2p ice error!");
		return ERROR_RTC_PEERCONNECTION;
	}

	session->isControlled = session->context.peerInfo->rtc.isControlled;

    YangSdp sdp;
	yang_memset(&sdp, 0, sizeof(YangSdp));

	yang_create_rtcsdp(&sdp);

#if Yang_Enable_RTC_Audio
	if (session->remote_audio == NULL) {
		session->remote_audio = (YangAudioParam*)yang_calloc(sizeof(YangAudioParam), 1);
	}
#endif

#if Yang_Enable_RTC_Video
	if (session->remote_video == NULL) {
		session->remote_video = (YangVideoParam*)yang_calloc(sizeof(YangVideoParam), 1);
	}
#endif

    int32_t err = Yang_Ok;
    
	do {
		err = yang_rtcsdp_parse(&sdp, sdpstr);

		if (err != Yang_Ok) {
	    	yang_error("sdp parse error!");
			break;
	    }

        err = yang_sdp_parseRemoteSdp(session, &sdp);

		if (err != Yang_Ok) {
		    yang_error("parse remote sdp error!");
	    }

		yang_destroy_rtcsdp(&sdp);

	} while(yangfalse);

	session->ice.session.stun.createRequestStunPacket(session, session->remoteIcePwd);

	if (session->context.peerInfo->mediaServer == Yang_Server_P2p) {
        err = session->ice.iceHandle(
			&session->ice.session,
			session,
			yang_rtcconn_turn_receive,
			session->context.peerInfo->remoteIp,
			session->context.peerInfo->remotePort
		);

		if (err != Yang_Ok){
			return yang_error_wrap(err, "set remote description error!");
		}

		if (session->ice.session.candidateType == YangIceTurn && 
			session->ice.session.iceState == YangIceSuccess) {

			err = yang_create_rtcsocket(
				session->context.sock,
				session->context.peerInfo->familyType,
				Yang_Socket_Protocol_Udp,
				session->context.peerInfo->rtc.rtcLocalPort
			);

			if (err != Yang_Ok) {
				return yang_error_wrap(err, "set remote description error!");
			}

			session->context.sock->write = yang_rtcconn_turn_send_data;

			yang_rtcconn_startudp(session);
			yang_start_stun_timer(session->context.sock->session.user);
			
			return Yang_Ok;
		}
	}

	if (!session->isControlled) {
		session->context.sock->updateRemoteAddress(
			&session->context.sock->session,
			session->context.peerInfo->remoteIp,
			session->isControlled ? 0 : session->context.peerInfo->remotePort
		);
    }

	return err;
}

static yangbool yang_rtcconn_is_connected(YangRtcSession* session) {
    return (session != NULL) ? (session->context.state == Yang_Conn_State_Connected) : yangfalse;
}

int32_t yang_create_rtcConnection(
	YangRtcConnection* conn, 
	YangPeerInfo* peerInfo, 
	YangPeerCallback* peerCallback) {

	if (conn == NULL || peerInfo == NULL || peerCallback == NULL) {
        return ERROR_RTC_CONNECT;
	}

    conn->session = (YangRtcSession*)yang_calloc(sizeof(YangRtcSession), 1);
	
	conn->session->context.peerInfo = peerInfo;
	conn->session->context.peerCallback = peerCallback;

	yang_create_rtcContext(&conn->session->context);
	yang_create_ice(&conn->session->ice, peerInfo,peerCallback);

	yang_memset(&conn->session->rtcp_compound, 0, sizeof(YangRtcpCompound));
	yang_create_rtcpCompound(&conn->session->rtcp_compound);

	conn->session->context.sock->session.user = conn;
	conn->session->context.sock->session.receive = yang_session_receive;
	conn->session->context.sock->session.startStunTimer = yang_start_stun_timer;

	conn->session->isControlled = yangfalse;

	conn->session->enableDatachannel = yangfalse;

	conn->session->h264PayloadType = YangH264PayloadType;
	conn->session->h265PayloadType = YangH265PayloadType;

	conn->session->audioPayloadType = YangAudioPayloadType;

	conn->close = yang_rtcconn_close;

	conn->on_video = yang_rtcconn_on_video;
	conn->on_audio = yang_rtcconn_on_audio;
	conn->on_message = yang_rtcconn_on_message;

	conn->notify = yang_rtcconn_notify;
	conn->isAlive = yang_rtcconn_alive;

	conn->receive = yang_rtcconn_receive;

	conn->updateCandidateAddress = yang_rtcconn_on_ice;
	conn->onConnectionStateChange = yang_conn_state_change;

	conn->setLocalDescription = yang_rtcconn_set_local_description;
	conn->setRemoteDescription = yang_rtcconn_set_remote_description;

	conn->createOffer = yang_rtcconn_create_offer;
	conn->createAnswer = yang_rtcconn_create_answer;

	conn->createDataChannel = yang_rtcconn_create_datachannel;
	
	conn->isConnected = yang_rtcconn_is_connected;

	conn->addAudioTrack = yang_rtcconn_add_audio_track;
	conn->addVideoTrack = yang_rtcconn_add_video_track;
	conn->addTransceiver = yang_rtcconn_add_transceiver;

	return Yang_Ok;
}

void yang_destroy_rtcConnection(YangRtcConnection* conn) {
	if (conn == NULL) {
		return;
	}

	YangRtcSession* session = (YangRtcSession*)conn->session;
	session->activeState = yangfalse;

	yang_timer_stop(session->tm_1s);
	yang_timer_stop(session->tm_100ms);

	yang_destroy_timer(session->tm_1s);
	yang_destroy_timer(session->tm_100ms);
	
	yang_free(session->tm_1s);
	yang_free(session->tm_100ms);

#if Yang_Enable_RTC_Audio
	yang_destroy_pushAudio(session->pushAudio);
	yang_free(session->remote_audio);
#endif

#if Yang_Enable_RTC_Video
	yang_destroy_pushH264(session->pushH264);
	yang_free(session->pushH264);

#if	Yang_Enable_H265_Encoding
	yang_destroy_pushH265(session->pushH265);
	yang_free(session->pushH265);
#endif

	yang_free(session->remote_video);
#endif

	yang_destroy_rtcpush(session->push);
	yang_free(session->push);

	yang_destroy_rtcpull(session->play);
	yang_free(session->play);

#if Yang_Enable_Datachannel
	yang_destroy_datachannel(session->datachannel);
	yang_free(session->datachannel);
#endif

	yang_destroy_ice(&session->ice);
	yang_destroy_rtcContext(&session->context);

	if (session->playRtpBuffer) {
		yang_destroy_rtpBuffer(session->playRtpBuffer);
		yang_free(session->playRtpBuffer);
	}

	yang_destroy_rtcpCompound(&session->rtcp_compound);

#if Yang_Enable_RTC_Audio
	if (session->pushAudioRtpBuffer) {
		yang_destroy_rtpBuffer(session->pushAudioRtpBuffer);
		yang_free(session->pushAudioRtpBuffer);
	}
#endif

#if Yang_Enable_RTC_Video
	if (session->pushVideoRtpBuffer) {
		yang_destroy_rtpBuffer(session->pushVideoRtpBuffer);
		yang_free(session->pushVideoRtpBuffer);
	}
#endif

	yang_free(conn->session);
}

