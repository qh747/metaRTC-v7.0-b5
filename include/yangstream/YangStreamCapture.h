//
// Copyright (c) 2019-2025 yanggaofeng
//

#ifndef INCLUDE_YANGSTREAM_YANGSTREAMCAPTURE_H_
#define INCLUDE_YANGSTREAM_YANGSTREAMCAPTURE_H_
#include <stdint.h>
#include <yangutil/yangavinfo.h>

typedef struct{
	void* context;

	void (*initAudio)(void* ctx, int32_t sample);
    
	void (*setAudioData)(void* ctx, YangFrame* frame);
	void (*setVideoData)(void* ctx, YangFrame* frame);

	YangFrame* (*getVideoFrame)(void* ctx);
	YangFrame* (*getAudioFrame)(void* ctx);

	void (*setVideoMeta)(void* ctx, uint8_t* buf, int32_t len);
	void (*setMetaTimestamp)(void* ctx, int64_t timestamp);
	void (*setVideoFrametype)(void* ctx, int32_t type);

} YangStreamCapture;

#ifdef __cplusplus
extern "C"{
#endif

void yang_create_streamCapture(YangStreamCapture* stream);
void yang_destroy_streamCapture(YangStreamCapture* stream);

#ifdef __cplusplus
}
#endif

#endif // INCLUDE_YANGSTREAM_YANGSTREAMCAPTURE_H_
