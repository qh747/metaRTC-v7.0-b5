//
// Copyright (c) 2019-2025 yanggaofeng
//
#include <yangutil/sys/YangCUrl.h>
#include <yangutil/sys/YangLog.h>
#include <yangutil/sys/YangSocket.h>

int32_t yang_url_parse(YangIpFamilyType familyType, char* url, YangUrlData* data) {
	const char* p = yang_strstr(url, "://");

	if (!p) {
		yang_error("url format error! url: %s", url);
		return 1;
	}

	int32_t len = (int32_t) (p - url);

    if (len == 4 && yang_memcmp(url, "rtmp", 4) == 0) {
		data->netType = Yang_Rtmp;
    } 
    else if (len == 6 && yang_memcmp(url, "webrtc", 6) == 0) {
        data->netType = Yang_Webrtc;
	}
    else if (len == 4 && yang_memcmp(url, "http", 4) == 0) {
		data->netType = Yang_Webrtc;
    } 
    else if (len == 5 && yang_memcmp(url, "https", 5) == 0) {
	    data->netType = Yang_Webrtc;
    }
    else {
		yang_error("url format invalid! url: %s", url);
		return 1;
	}

	p += 3;

	if (*p == 0) {
		yang_error("no hostname in url! url: %s", url);
		return 1;
	}

	const char* end = p + yang_strlen(p);
	const char* col = yang_strchr(p, ':');
	const char* slash = yang_strchr(p, '/');

	int32_t hostlen = (slash != NULL) ? slash - p : end - p;

	if (col && col - p < hostlen) {
		hostlen = col - p;
	}

	if (hostlen > 255) {
		yang_error("hostname exceeds 255 characters! url: %s", url);
		return 1;
	}

	char s1[256] = { 0 };
	yang_memcpy(s1, p, hostlen);

	yang_memset(data->server, 0, sizeof(data->server));
	yang_getIp(familyType, s1, data->server);

	p += hostlen;

	if (*p != ':') {
		yang_error("no port in url! url: %s", url);
		return 1;
	}

	p++;

	data->port = yang_atoi(p);

	if (data->port > 65535) {
		yang_error("port number invalid! port: %d url: %s", data->port, url);
		return 1;
	}

	if (!slash) {
		yang_error("No application or playpath in URL!");
		return 0;
	}

	p = slash + 1;

	char *slash2 = yang_strchr(p, '/');
	char *slash3 = NULL;
	char *slash4 = NULL;

	if (slash2)
		slash3 = yang_strchr(slash2 + 1, '/');
	if (slash3)
		slash4 = yang_strchr(slash3 + 1, '/');

	int32_t applen = end - p; 
	int32_t appnamelen = applen;
	if (slash4)
		appnamelen = slash4 - p;
	else if (slash3)
		appnamelen = slash3 - p;
	else if (slash2)
		appnamelen = slash2 - p;

	applen = appnamelen;
	 yang_memset(data->app,0,sizeof(data->app));
	 yang_memcpy(data->app,p,applen);

	p += appnamelen;

	if (*p == '/')
		p++;

	if (end - p) {
		 yang_memset(data->stream,0,sizeof(data->stream));
		 yang_memcpy(data->stream,p,end - p);

	}

	return Yang_Ok;
}

int32_t yang_http_url_parse(YangIpFamilyType familyType, char* url, YangUrlData* data) {
	int32_t len;
	int32_t hostlen;
	uint32_t  p2;

	char* end;
	char* col;
	char* slash;

	char *p = yang_strstr(url, "://");
	char s1[256]={0};

	if (!p) {
		yang_error("Srs Webrt URL: No :// in url!");
		return 1;
	}

	len = (int32_t) (p - url);

    if (len == 4 && yang_memcmp(url, "http", 4) == 0) {
    	 data->netType = Yang_Webrtc;
    	 data->port=1985;
    } else if (len == 5 && yang_memcmp(url, "https", 5) == 0) {
        data->netType = Yang_Webrtc;
        data->port=1985;
	} else if (len == 6 && yang_memcmp(url, "webrtc", 6) == 0) {
        data->netType = Yang_Webrtc;
        data->port=1985;
	} else {
		return 1;
	}

	p += 3;

	if (*p == 0) {
		yang_warn("No hostname in URL!");
		return 1;
	}

	end = p + yang_strlen(p);
	col = yang_strchr(p, ':');
	//schar *ques = yang_strchr(p, '?');
	slash = yang_strchr(p, '/');

	if (slash)
		hostlen = slash - p;
	else
		hostlen = end - p;
	if (col && col - p < hostlen)
		hostlen = col - p;

	if (hostlen < 256) {
		 yang_memcpy(s1,p,hostlen);
		 yang_memset(data->server,0,sizeof(data->server));
		 yang_getIp(familyType,s1,data->server);

	} else {
		yang_warn("Hostname exceeds 255 characters!");
	}

	p += hostlen;

	if (*p == ':') {
		p++;
		p2 = yang_atoi(p);
		if (p2 > 65535) {
			yang_warn("Invalid port number!");
		} else {
			data->port = p2;
		}
	}

	if (!slash) {
		yang_warn("No application or playpath in URL!");
		return 0;
	}
	p = slash + 1;


	 yang_memset(data->stream,0,sizeof(data->stream));
	 yang_strcpy(data->stream,p);


	return Yang_Ok;
}




