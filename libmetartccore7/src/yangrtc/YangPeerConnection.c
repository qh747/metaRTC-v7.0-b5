//
// Copyright (c) 2019-2025 yanggaofeng
//

#include <yangrtc/YangRtcConnection.h>

#include <yangrtc/YangPeerConnection.h>
#include <yangrtc/YangMetaConnection.h>
#include <yangutil/sys/YangLog.h>

#include <yangavutil/video/YangMeta.h>

static yangbool yang_pc_is_connect(YangPeer* peer) {
	if (peer == NULL || peer->conn == NULL) {
        return yangfalse;
	}

	YangRtcConnection* conn = (YangRtcConnection*)peer->conn;
	return conn->isConnected(conn->session);
}

static int32_t yang_pc_close(YangPeer* peer) {
	if (peer == NULL || peer->conn == NULL) {
        return ERROR_RTC_PEERCONNECTION;
	}

	YangRtcConnection* conn = (YangRtcConnection*) peer->conn;

	if (conn->session->context.state == Yang_Conn_State_Disconnected || 
		conn->session->context.state==Yang_Conn_State_Closed) {
    
		return Yang_Ok;
	}
		
	conn->session->context.state = Yang_Conn_State_Disconnected;

	if (conn->onConnectionStateChange) {
		conn->onConnectionStateChange(conn->session, conn->session->context.state);
	}

	yang_trace("\nwebrtc disconnected\n");

	conn->close(conn->session);
	yang_destroy_rtcConnection(conn);

	yang_free(peer->conn);
	return Yang_Ok;
}

static int32_t yang_pc_set_local_description(YangPeer* peer, char* sdp) {
	if (peer == NULL || peer->conn == NULL) {
        return ERROR_RTC_PEERCONNECTION;
	}

	YangRtcConnection* conn = (YangRtcConnection*)peer->conn;
	return conn->setLocalDescription(conn->session, sdp);
}

static int32_t yang_pc_set_remote_description(YangPeer* peer, char* sdp) {
	if (peer == NULL || peer->conn == NULL) {
        return ERROR_RTC_PEERCONNECTION;
	}

	YangRtcConnection* conn = (YangRtcConnection*)peer->conn;
	return conn->setRemoteDescription(conn->session, sdp);
}

static int32_t yang_add_ice_candidate(YangPeer* peer, char* candidateStr) {
	if (peer == NULL || peer->conn == NULL || candidateStr == NULL) {
        return ERROR_RTC_PEERCONNECTION;
	}

	YangRtcConnection* conn = (YangRtcConnection*)peer->conn;
	return conn->addIceCandidate(conn->session, candidateStr);
}

static int32_t yang_pc_on_video(YangPeer* peer, YangFrame* frame) {
	if (peer == NULL || peer->conn == NULL || frame == NULL) {
        return ERROR_RTC_PEERCONNECTION;
	}

	YangRtcConnection* conn = (YangRtcConnection*)peer->conn;
	return conn->on_video(conn->session, frame);
}

static int32_t yang_pc_on_audio(YangPeer* peer, YangFrame* frame) {
	if (peer == NULL || peer->conn == NULL || frame == NULL) {
        return ERROR_RTC_PEERCONNECTION;
	}

	YangRtcConnection* conn = (YangRtcConnection*)peer->conn;
	return conn->on_audio(conn->session, frame);
}

static int32_t yang_pc_on_message(YangPeer* peer, YangFrame* frame) {
	if (peer == NULL || peer->conn == NULL || frame == NULL) {
        return ERROR_RTC_PEERCONNECTION;
	}

	YangRtcConnection* conn = (YangRtcConnection*)peer->conn;
	return  conn->on_message(conn->session, frame);
}

static int32_t yang_pc_send_rtc_message(YangPeer* peer, YangRtcMessageType type) {
	if (peer == NULL || peer->conn == NULL) {
        return ERROR_RTC_PEERCONNECTION;
	}

	YangRtcConnection* conn = (YangRtcConnection*)peer->conn;
	return conn->notify(conn->session, type);
}

static yangbool yang_pc_is_alive(YangPeer* peer) {
	if (peer == NULL || peer->conn == NULL) {
        return yangfalse;
	}

	YangRtcConnection* conn = (YangRtcConnection*)peer->conn;
	return conn->isAlive(conn->session);
}

static YangRtcConnectionState yang_pc_get_connection_state(YangPeer* peer) {
	if (peer == NULL || peer->conn == NULL) {
		return Yang_Conn_State_New;
    }

	YangRtcConnection* conn = (YangRtcConnection*) peer->conn;
	return conn->session->context.state;
}

static int32_t yang_pc_create_answer(YangPeer* peer, char* answer) {
	if (peer == NULL || peer->conn == NULL) {
		return ERROR_RTC_PEERCONNECTION;
	}

	YangRtcConnection* conn = (YangRtcConnection*)peer->conn;
	return conn->createAnswer(conn->session, answer);
}

static int32_t yang_pc_create_offer(YangPeer* peer, char** psdp) {
	if (peer == NULL || peer->conn == NULL) {
		return ERROR_RTC_PEERCONNECTION;
	}

	YangRtcConnection* conn = (YangRtcConnection*)peer->conn;
	return conn->createOffer(conn->session, psdp);
}

static int32_t yang_pc_add_audio_track(YangPeer* peer, YangAudioCodec codec) {
	if (peer == NULL || peer->conn == NULL) {
		return ERROR_RTC_PEERCONNECTION;
	}

	YangRtcConnection* conn = (YangRtcConnection*)peer->conn;
	return conn->addAudioTrack(conn->session, codec);
}

static int32_t yang_pc_add_video_track(YangPeer* peer, YangVideoCodec codec) {
	if (peer == NULL || peer->conn == NULL) {
		return ERROR_RTC_PEERCONNECTION;
	}

	YangRtcConnection* conn = (YangRtcConnection*)peer->conn;
	return conn->addVideoTrack(conn->session, codec);
}

static int32_t yang_pc_add_transceiver(YangPeer* peer, YangMediaTrack media, YangRtcDirection direction) {
	if (peer == NULL || peer->conn == NULL) {
		return ERROR_RTC_PEERCONNECTION;
	}

	YangRtcConnection* conn = (YangRtcConnection*)peer->conn;
	return conn->addTransceiver(conn->session, media, direction);
}

static YangIceCandidateType yang_pc_get_ice_candidate_type(YangPeer* peer) {
	if (peer == NULL || peer->conn == NULL) {
		return YangIceHost;
	}

	YangRtcConnection* conn = (YangRtcConnection*) peer->conn;
	return conn->session->ice.session.candidateType;
}

static int32_t yang_pc_create_datachannel(YangPeer* peer) {
	if (peer == NULL || peer->conn == NULL) {
		return ERROR_RTC_PEERCONNECTION;
	}

	YangRtcConnection* conn = (YangRtcConnection*)peer->conn;
	return conn->createDataChannel(conn->session);
}

static int32_t yang_pc_send_request_pli(YangPeer* peer) {
	if (peer == NULL || peer->conn == NULL) {
		return ERROR_RTC_PEERCONNECTION;
	}

	YangRtcConnection* conn = (YangRtcConnection*)peer->conn;
	return conn->sendRequestPli(conn->session);
}

void yang_create_metaConnection(YangMetaConnection* peerconn) {
	if (peerconn == NULL) {
        return;
	}

	memset(peerconn, 0, sizeof(YangMetaConnection));

	peerconn->addAudioTrack = yang_pc_add_audio_track;
	peerconn->addVideoTrack = yang_pc_add_video_track;
	peerconn->addTransceiver = yang_pc_add_transceiver;

	peerconn->getIceCandidateType = yang_pc_get_ice_candidate_type;

	peerconn->createOffer = yang_pc_create_offer;
	peerconn->createAnswer = yang_pc_create_answer;

	peerconn->setLocalDescription = yang_pc_set_local_description;
	peerconn->setRemoteDescription = yang_pc_set_remote_description;

	peerconn->close = yang_pc_close;

	peerconn->on_audio = yang_pc_on_audio;
	peerconn->on_video = yang_pc_on_video;
	peerconn->on_message = yang_pc_on_message;

	peerconn->isConnected = yang_pc_is_connect;
	peerconn->isAlive = yang_pc_is_alive;

	peerconn->getConnectionState = yang_pc_get_connection_state;
	peerconn->addIceCandidate = yang_add_ice_candidate;
	
	peerconn->sendRtcMessage = yang_pc_send_rtc_message;
	peerconn->createDataChannel = yang_pc_create_datachannel;

	peerconn->sendRequestPli=yang_pc_send_request_pli;
}

void yang_create_peer(YangPeer* peer) {
	if (peer == NULL) {
		return;
	}

	if (peer->conn == NULL) {
		peer->conn = yang_calloc(1, sizeof(YangRtcConnection));

		yang_create_rtcConnection(
			(YangRtcConnection*)peer->conn,
			&peer->peerInfo, 
			&peer->peerCallback
		);
	}
}

void yang_create_peerConnection(YangPeerConnection* peerconn) {
	if (peerconn == NULL) {
		return;
    }

	peerconn->peer.conn = NULL;
	yang_create_peer(&peerconn->peer);

    YangMetaConnection conn;
	yang_create_metaConnection(&conn);

	peerconn->addAudioTrack = conn.addAudioTrack;
	peerconn->addVideoTrack = conn.addVideoTrack;
	peerconn->addTransceiver = conn.addTransceiver;

	peerconn->getIceCandidateType = conn.getIceCandidateType;

	peerconn->createOffer = conn.createOffer;
	peerconn->createAnswer = conn.createAnswer;

	peerconn->setLocalDescription =  conn.setLocalDescription;
	peerconn->setRemoteDescription = conn.setRemoteDescription;

	peerconn->close = conn.close;

	peerconn->on_audio = conn.on_audio;
	peerconn->on_video = conn.on_video;
	peerconn->on_message = conn.on_message;
	
	peerconn->isConnected = conn.isConnected;
	peerconn->isAlive = conn.isAlive;
	
	peerconn->getConnectionState = conn.getConnectionState;
	
	peerconn->sendRtcMessage = conn.sendRtcMessage;
	peerconn->addIceCandidate = conn.addIceCandidate;
	peerconn->createDataChannel = conn.createDataChannel;

	peerconn->sendRequestPli = conn.sendRequestPli;
}

void yang_destroy_peerConnection(YangPeerConnection* peerconn) {
	if (peerconn == NULL || peerconn->peer.conn == NULL) {
		return;
	}

	yang_pc_close(&peerconn->peer);
}

