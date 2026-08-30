//
// Copyright (c) 2019-2025 yanggaofeng
//


#include <yangrtc/YangWhip.h>

#include "YangSrsConnection.h"
#include "YangZlmConnection.h"
#include <yangutil/sys/YangLog.h>
#include <yangutil/sys/YangCUrl.h>
#include <yangutil/sys/YangCString.h>
#include <yangutil/sys/YangHttp.h>

static int32_t yang_whip_getSignal(
	YangMetaConnection* conn,
	YangPeer* peer,
	char* url,
	char** premoteSdp,
	char* localSdp) {
    
    // 解析向媒体发送请求的url
	YangUrlData urlData;
	memset(&urlData, 0, sizeof(YangUrlData));

	int32_t err = yang_url_parse(peer->peerInfo.familyType, url, &urlData);

	if (err != Yang_Ok) {
		return err;
	}

	char* respBuf = (char*)yang_calloc(1, 1024 * 12);

    err = yang_http_post(
		yangtrue,
		peer->peerInfo.familyType,
		respBuf,
		urlData.server,
		urlData.port, 
		urlData.path, 
		(uint8_t*)localSdp, 
		yang_strlen(localSdp)
	);

	if (err != Yang_Ok) {
		yang_error("send http post failed. err: %d\n", err);
		yang_free(respBuf);

	    return err;
	}
    
	// WHIP标准规定创建资源成功返回201 Created
	if (!yang_strstr(respBuf,"201") || 
	    !yang_strstr(respBuf,"Created")) {
        
		yang_error("send http post failed. err: %s\n", respBuf);
		yang_free(respBuf);

	    return ERROR_RTC_Whip;
	}

    char* remoteSdpStart = yang_strstr(respBuf,"\r\n\r\n");

    if (remoteSdpStart == NULL) {
		yang_free(respBuf);
	    return ERROR_RTC_Whip;
	}

    remoteSdpStart = yang_strstr(remoteSdpStart, "v=0");

	if (remoteSdpStart == NULL) {
		yang_free(respBuf);
	    return ERROR_RTC_Whip;
	}

	int32_t remoteSdpLen = yang_strlen(remoteSdpStart);
    char* remoteSdpContent = (char*)yang_calloc(remoteSdpLen + 1, 1);

	yang_cstr_replace(remoteSdpStart,remoteSdpContent, "\r\n", "\n");
	
	remoteSdpStart=yang_strstr(remoteSdpContent, "\n\n");

	if (remoteSdpStart != NULL) {
        *remoteSdpStart = 0;
	}
		
    *premoteSdp = remoteSdpContent;
    yang_free(respBuf);

	return err;
}

int32_t yang_whip_connectWhipWhepServer(YangPeer* peer, char* url) {
	if (peer == NULL) {
		return ERROR_RTC_PEERCONNECTION;
    }
    
	YangMetaConnection conn;
    yang_create_metaConnection(&conn);

	if (conn.isConnected(peer)) {
		return Yang_Ok;
    }
    
	int32_t err = Yang_Ok;
	const char* errBuf = NULL;

	char* localSdp = NULL;
	char* remoteSdp = NULL;

	do {
		// 创建本端sdp
		err = conn.createOffer(peer, &localSdp);

		if (err != Yang_Ok || localSdp == NULL) {
			errBuf = "create local sdp fail!";
	    	break;
	    }
        
		// 设置本端sdp
        conn.setLocalDescription(peer, localSdp);
        
		// 向服务端发送http请求，获取对端sdp
		err = yang_whip_getSignal(&conn, peer, url, &remoteSdp, localSdp);

		if (err != Yang_Ok || remoteSdp == NULL) {
			errBuf = "get remote sdp fail!";
	    	break;
	    }
        
		// 设置对端sdp
        conn.setRemoteDescription(peer, remoteSdp);

	} while(yangfalse);

	yang_free(localSdp);
	yang_free(remoteSdp);

	return (err != Yang_Ok) ? yang_error_wrap(err,errBuf) : err;
}

int32_t yang_whip_connectSfuServer(YangPeer* peer, char* url, int32_t mediaServer) {
	if (peer == NULL) {
		return ERROR_RTC_PEERCONNECTION;
    }

	YangMetaConnection conn;
    yang_create_metaConnection(&conn);

	if (conn.isConnected(peer)) {
		return Yang_Ok;
    }

	if (mediaServer == Yang_Server_Zlm) {
		return yang_zlm_connectRtcServer(&conn, peer, url);
	}
	else if (mediaServer == Yang_Server_Srs) {
		return yang_srs_connectRtcServer(&conn, peer, url);
	}

	return ERROR_RTC_PEERCONNECTION;
}

