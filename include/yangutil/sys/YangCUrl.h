//
// Copyright (c) 2019-2025 yanggaofeng
//
#ifndef INCLUDE_YANGUTIL_SYS_YANGCURL_H_
#define INCLUDE_YANGUTIL_SYS_YANGCURL_H_

#include <yangutil/yangtype.h>

typedef struct {
	int32_t netType;

	int32_t port;
	char server[64];

	char path[512];

	char app[128];
	char stream[128];

	char param[128];

} YangUrlData;

// rtmp://host:port/app/stream
// webrtc://host:port/app/stream
// http(s)://host:port/path[?app=&stream=&...]
int32_t yang_url_parse(YangIpFamilyType familyType, const char* purl, YangUrlData* data);

#endif // INCLUDE_YANGUTIL_SYS_YANGCURL_H_
