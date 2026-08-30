//
// Copyright (c) 2019-2025 yanggaofeng
//
#include <yangutil/sys/YangCUrl.h>
#include <yangutil/sys/YangLog.h>
#include <yangutil/sys/YangSocket.h>

static int32_t yang_url_copy(char* dst, int32_t dstSize, const char* src, int32_t srcLen) {
	if (srcLen < 0 || dstSize <= 0 || srcLen >= dstSize) {
		yang_error("url field exceeds buffer: src len = %d dst size = %d", srcLen, dstSize);
		return 1;
	}

	yang_memset(dst, 0, dstSize);

	if (srcLen > 0 && src) {
		yang_memcpy(dst, src, srcLen);
	}

	dst[srcLen] = 0;
	return Yang_Ok;
}

static int32_t yang_url_query_value(const char* query, const char* key, const char** val, int32_t* vlen) {
	int32_t keylen = (int32_t) yang_strlen(key);
	const char* p = query;

	while (p && *p) {
		const char* amp = yang_strchr(p, '&');
		int32_t seglen = amp ? (int32_t) (amp - p) : (int32_t) yang_strlen(p);
		const char* eq = NULL;

		for (int32_t i = 0; i < seglen; i++) {
			if (p[i] == '=') {
				eq = p + i;
				break;
			}
		}

		if (eq && (int32_t) (eq - p) == keylen && yang_memcmp(p, key, keylen) == 0) {
			*val = eq + 1;
			*vlen = (int32_t) ((p + seglen) - (eq + 1));
			return 1;
		}

		p = amp ? amp + 1 : NULL;
	}

	return 0;
}

static yangbool yang_url_query_nonempty(const char* query, const char* key) {
	const char* val = NULL;
	int32_t vlen = 0;

	return yang_url_query_value(query, key, &val, &vlen) ? vlen > 0 : yangfalse;
}

static yangbool yang_url_is_query_form(const char* path) {
	const char* q = yang_strchr(path, '?');

	if (!q || q[1] == 0) {
		return yangfalse;
	}

	q++;

	return yang_url_query_nonempty(q, "app") && yang_url_query_nonempty(q, "stream");
}

static int32_t yang_url_slash_parts(
	const char* path, 
	const char** app, 
	int32_t* applen,
	const char** stream, 
	int32_t* streamlen) {

	const char* q = yang_strchr(path, '?');
	int32_t plen = q ? (int32_t) (q - path) : (int32_t) yang_strlen(path);

	while (plen > 0 && path[plen - 1] == '/') {
		plen--;
	}

	if (plen <= 0) {
		return 0;
	}

	int32_t slashCount = 0;
	int32_t firstSlash = -1;

	for (int32_t i = 0; i < plen; i++) {
		if (path[i] == '/') {
			slashCount++;

			if (firstSlash < 0) {
				firstSlash = i;
			}
		}
	}

	if (slashCount != 1 || firstSlash <= 0 || firstSlash >= plen - 1) {
		return slashCount == 0 ? 1 : (slashCount + 1);
	}

	if (app && applen) {
		*app = path;
		*applen = firstSlash;
	}

	if (stream && streamlen) {
		*stream = path + firstSlash + 1;
		*streamlen = plen - firstSlash - 1;
	}

	return 2;
}

static yangbool yang_url_require_slash_form(const char* url) {
	const char* p = yang_strstr(url, "://");

	if (!p) {
		return yangfalse;
	}

	int32_t len = (int32_t) (p - url);

	if (len == 4 && yang_memcmp(url, "rtmp", 4) == 0) {
		return yangtrue;
	}

	if (len == 6 && yang_memcmp(url, "webrtc", 6) == 0) {
		return yangtrue;
	}

	return yangfalse;
}

static int32_t yang_url_parse_schema(const char** pp, const char* url, YangUrlData* data) {
	const char* p = yang_strstr(url, "://");

	if (p == NULL) {
		yang_error("parse url schema error! url: %s", url);
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

	*pp = p + 3;
	return Yang_Ok;
}

static int32_t yang_url_parse_ip(
	YangIpFamilyType familyType, 
	const char** pp, 
	const char* url, 
	YangUrlData* data) {

	const char* p = *pp;

	if (!p || *p == 0) {
		yang_error("no hostname in url! url: %s", url);
		return 1;
	}

	const char* host = NULL;

	int32_t hostlen = 0;
	char hostname[sizeof(data->server)] = { 0 };

	if (*p == '[') {
		const char* rb;

		p++;
		rb = yang_strchr(p, ']');

		if (!rb || rb == p) {
			yang_error("ipv6 hostname invalid! url: %s", url);
			return 1;
		}

		host = p;
		hostlen = (int32_t) (rb - p);
		p = rb + 1;
	} 
	else {
		const char* slash = yang_strchr(p, '/');
		const char* hostend = slash ? slash : p + yang_strlen(p);

		const char* col = NULL;
		const char* q = NULL;

		int32_t colonCount = 0;

		for (q = p; q < hostend; q++) {
			if (*q == ':') {
				colonCount++;
				if (!col) {
					col = q;
				}
			}
		}

		if (colonCount > 1) {
			yang_error("unbracketed ipv6 hostname! url: %s", url);
			return 1;
		}

		host = p;
		hostlen = col ? (int32_t) (col - p) : (int32_t) (hostend - p);
		p += hostlen;
	}

	if (hostlen <= 0) {
		yang_error("no hostname in url! url: %s", url);
		return 1;
	}

	if (yang_url_copy(hostname, sizeof(hostname), host, hostlen)) {
		return 1;
	}
    
	char ip[sizeof(data->server)];
	yang_memset(ip, 0, sizeof(ip));

	yang_getIp(familyType, hostname, ip);

	if (yang_url_copy(
		data->server, 
		sizeof(data->server), 
		ip, 
		(int32_t) yang_strlen(ip))) {

		return 1;
	}

	*pp = p;
	return Yang_Ok;
}

static int32_t yang_url_parse_port(const char** pp, const char* url, YangUrlData* data) {
	const char* p = *pp;

	if (!p || *p != ':') {
		yang_error("no port in url! url: %s", url);
		return 1;
	}

	p++;

	if (*p < '0' || *p > '9') {
		yang_error("no port in url! url: %s", url);
		return 1;
	}

	data->port = yang_atoi(p);

	if (data->port < 1 || data->port > 65535) {
		yang_error("port number invalid! port: %d url: %s", data->port, url);
		return 1;
	}

	while (*p >= '0' && *p <= '9') {
		p++;
	}

	*pp = p;
	return Yang_Ok;
}

static int32_t yang_url_parse_path(const char** pp, const char* url, YangUrlData* data) {
	const char* p = *pp;

	if (!p || *p != '/') {
		yang_error("no path in url! url: %s", url);
		return 1;
	}

	p++;

	return yang_url_copy(
		data->path, 
		sizeof(data->path), 
		p, 
		(int32_t) yang_strlen(p)
	);
}

static int32_t yang_url_parse_app(YangUrlData* data, const char* url) {
	if (yang_url_is_query_form(data->path)) {
		const char* q = yang_strchr(data->path, '?') + 1;
		const char* val = NULL;
		int32_t vlen = 0;

		yang_url_query_value(q, "app", &val, &vlen);
		return yang_url_copy(data->app, sizeof(data->app), val, vlen);
	}

	const char* app = NULL;
	int32_t applen = 0;

	int32_t parts = yang_url_slash_parts(
		data->path, 
		&app, 
		&applen, 
		NULL, 
		NULL
	);

	if (parts == 2) {
		return yang_url_copy(data->app, sizeof(data->app), app, applen);
	}

	if (yang_url_require_slash_form(url)) {
		yang_error("url app/stream format invalid! url: %s", url);
		return 1;
	}

	return Yang_Ok;
}

static int32_t yang_url_parse_stream(YangUrlData* data, const char* url) {
	if (yang_url_is_query_form(data->path)) {
		const char* q = yang_strchr(data->path, '?') + 1;
		const char* val = NULL;
		int32_t vlen = 0;

		yang_url_query_value(q, "stream", &val, &vlen);
		return yang_url_copy(data->stream, sizeof(data->stream), val, vlen);
	}

	const char* app = NULL;
	int32_t applen = 0;

	const char* stream = NULL;
	int32_t streamlen = 0;

	int32_t parts = yang_url_slash_parts(
		data->path, 
		&app, 
		&applen, 
		&stream, 
		&streamlen
	);

	if (parts == 2) {
		return yang_url_copy(data->stream, sizeof(data->stream), stream, streamlen);
	}

	if (yang_url_require_slash_form(url)) {
		yang_error("url app/stream format invalid! url: %s", url);
		return 1;
	}

	return Yang_Ok;
}

static int32_t yang_url_parse_param(YangUrlData* data, const char* url) {
	const char* q = yang_strchr(data->path, '?');

	if (!q || q[1] == 0) {
		return Yang_Ok;
	}

	const char* p = q + 1;

    char buf[sizeof(data->param)] = { 0 };
	yang_memset(buf, 0, sizeof(buf));

	int32_t used = 0;

	while (p && *p) {
		const char* amp = yang_strchr(p, '&');
		int32_t seglen = amp ? (int32_t) (amp - p) : (int32_t) yang_strlen(p);

		const char* eq = NULL;

		for (int32_t i = 0; i < seglen; i++) {
			if (p[i] == '=') {
				eq = p + i;
				break;
			}
		}

		int32_t keylen = eq ? (int32_t) (eq - p) : seglen;
		yangbool skip = yangfalse;

		if (keylen == 3 && yang_memcmp(p, "app", 3) == 0) {
			skip = yangtrue;
		} 
		else if (keylen == 6 && yang_memcmp(p, "stream", 6) == 0) {
			skip = yangtrue;
		}

		if (!skip && seglen > 0) {
			if (used > 0) {
				if (used + 1 >= (int32_t) sizeof(buf)) {
					yang_error(
						"url field exceeds buffer: src len = %d dst size = %d url: %s",
						used + 1 + seglen, 
						(int32_t) sizeof(data->param),
						url
					);

					return 1;
				}

				buf[used++] = '&';
			}

			if (used + seglen >= (int32_t) sizeof(buf)) {
				yang_error(
					"url field exceeds buffer: src len = %d dst size = %d url: %s",
					used + seglen, 
					(int32_t) sizeof(data->param),
					url
				);

				return 1;
			}

			yang_memcpy(buf + used, p, seglen);
			used += seglen;
		}

		p = amp ? amp + 1 : NULL;
	}

	if (used == 0) {
		return Yang_Ok;
	}

	return yang_url_copy(data->param, sizeof(data->param), buf, used);
}

int32_t yang_url_parse(YangIpFamilyType familyType, const char* url, YangUrlData* data) {
	if (!url || !data) {
		yang_error("url parse invalid argument");
		return 1;
	}

	yang_memset(data, 0, sizeof(YangUrlData));
    
	const char* p = NULL;

	if (yang_url_parse_schema(&p, url, data)) {
		return 1;
	}

	if (yang_url_parse_ip(familyType, &p, url, data)) {
		return 1;
	}

	if (yang_url_parse_port(&p, url, data)) {
		return 1;
	}

	if (yang_url_parse_path(&p, url, data)) {
		return 1;
	}

	if (yang_url_parse_app(data, url)) {
		return 1;
	}

	if (yang_url_parse_stream(data, url)) {
		return 1;
	}

	if (yang_url_parse_param(data, url)) {
		return 1;
	}

	return Yang_Ok;
}
