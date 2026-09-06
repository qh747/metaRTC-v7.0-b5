//
// Copyright (c) 2019-2025 yanggaofeng
//

#include <yangstream/YangStreamCapture.h>
#include <yangutil/sys/YangEndian.h>
#include <yangutil/yangtype.h>

typedef struct {
	uint8_t* sps;
	int32_t spsLen;

	uint8_t* pps;
	int32_t ppsLen;

	int32_t frametype;
	int32_t videoLen;
	uint8_t* src;

	int64_t videoTimestamp;
	int64_t baseTimestamp;
	int64_t timestampDiff;
	int32_t preTimestamp;

	int64_t metaTime;

} YangVideoStreamCapture;

typedef struct {
	uint8_t* audioBuffer;
	uint8_t* src;
	int32_t srcLen;
	int32_t audioLen;

	int64_t atime;
	int64_t unitAudioTime;
	int32_t frametype;
	YangAudioCodec audioType;

} YangAudioStreamCapture;

typedef struct{
	YangFrame audioFrame;
	YangFrame videoFrame;

	YangAudioStreamCapture audio;
	YangVideoStreamCapture video;

} YangStreamCaptureContext;

void yang_streamcapture_initAudio(void* ctx, int32_t sample) {
	if (ctx == NULL) {
		return;
	}

	YangStreamCaptureContext* context = (YangStreamCaptureContext*)ctx;
	context->audio.unitAudioTime = sample / 50;
}

void yang_streamcapture_setAudioData(void* ctx, YangFrame* audioFrame) {
	if (ctx == NULL) {
		return;
	}
	
	YangStreamCaptureContext* context = (YangStreamCaptureContext*)ctx;

	context->audio.src = audioFrame->payload;
	context->audio.srcLen = audioFrame->nb;
	context->audio.audioLen = audioFrame->nb;

	context->audio.atime += context->audio.unitAudioTime;
}

void yang_streamcapture_setVideoData(void* ctx, YangFrame* frame) {
	if (ctx == NULL) {
		return;
	}

	YangStreamCaptureContext* context = (YangStreamCaptureContext*)ctx;

	context->video.src = frame->payload;
	context->video.videoLen = frame->nb;

	context->video.videoTimestamp = frame->pts;

	if (context->video.preTimestamp == 0) {
		context->video.baseTimestamp = context->video.videoTimestamp;
		context->video.timestampDiff = 0;
	} 
	else {
		if (context->video.videoTimestamp <= context->video.preTimestamp) {
			return;
		}

		context->video.timestampDiff = context->video.videoTimestamp - context->video.baseTimestamp;
	}

	context->video.preTimestamp = context->video.videoTimestamp;
	context->video.frametype = frame->frametype;
}

YangFrame* yang_streamcapture_getAudioFrame(void* ctx) {
	if (ctx == NULL) {
		return NULL;
	}

	YangStreamCaptureContext* context = (YangStreamCaptureContext*)ctx;

	context->audioFrame.payload = context->audio.src;
	context->audioFrame.nb = context->audio.audioLen;

	context->audioFrame.pts = context->audio.atime;
	
	return &context->audioFrame;
}

YangFrame* yang_streamcapture_getVideoFrame(void* ctx) {
	if (ctx == NULL) {
		return NULL;
	}

	YangStreamCaptureContext* context = (YangStreamCaptureContext*)ctx;

	context->videoFrame.payload = context->video.src;
	context->videoFrame.nb = context->video.videoLen;

	context->videoFrame.pts = (context->video.frametype == YANG_Frametype_Spspps) ?
	    context->video.metaTime * 9 / 100 :
        context->video.timestampDiff * 9 / 100;

	context->videoFrame.frametype = context->video.frametype;
	
	return &context->videoFrame;
}

void yang_streamcapture_setVideoMeta(void* ctx, uint8_t* buf, int32_t len) {
	if (ctx == NULL) {
		return;
	}

	YangStreamCaptureContext* context = (YangStreamCaptureContext*)ctx;

	context->video.src = buf;
	context->video.videoLen = len;

	if(context->video.sps == NULL || context->video.pps == NULL) {
        context->video.spsLen = *(buf + 12) + 1;
		context->video.sps = yang_malloc(context->video.spsLen);

        yang_memcpy(context->video.sps, buf + 13, context->video.spsLen);

		context->video.ppsLen = *((buf + 13) + context->video.spsLen + 1) + 1;
		context->video.pps = yang_malloc(context->video.ppsLen);

        yang_memcpy(
			context->video.pps, 
			(buf + 13) + context->video.spsLen + 2, 
			context->video.ppsLen
		);
	}
}

void yang_streamcapture_setVideoFrametype(void* ctx, int32_t type) {
	if (ctx == NULL) {
		return;
	}

	YangStreamCaptureContext* context = (YangStreamCaptureContext*)ctx;
	context->video.frametype = type;
}

void yang_streamcapture_setMetaTimestamp(void* ctx, int64_t timestamp) {
	if (ctx == NULL) {
		return;
	}

	YangStreamCaptureContext* context = (YangStreamCaptureContext*)ctx;

	if (context->video.preTimestamp == 0) {
		context->video.baseTimestamp = timestamp;
		context->video.metaTime = 0;
	} 
	else {
		context->video.metaTime = timestamp-context->video.baseTimestamp;
	}
}

void yang_create_streamCapture(YangStreamCapture* stream) {
	if (stream == NULL) {
		return;
	}

	YangStreamCaptureContext* context = yang_calloc(sizeof(YangStreamCaptureContext), 1);

	context->video.videoTimestamp = 0;
	context->video.baseTimestamp = 0;
	context->video.timestampDiff = 0;
	context->video.preTimestamp = 0;
	context->video.src = NULL;
	context->video.videoLen = 0;
	context->video.frametype = 1;
	context->video.metaTime = 0;
	context->video.sps = NULL;
	context->video.spsLen = 0;
	context->video.pps = NULL;
	context->video.ppsLen = 0;

	context->audio.atime = 0;
	context->audio.unitAudioTime = 960;
	context->audio.src = NULL;
	context->audio.srcLen = 0;
	context->audio.frametype = 1;
	context->audio.audioLen = 0;
	context->audio.audioType = Yang_AED_AAC;
    
	stream->context = context;

	stream->initAudio = yang_streamcapture_initAudio;

	stream->getAudioFrame = yang_streamcapture_getAudioFrame;
	stream->getVideoFrame = yang_streamcapture_getVideoFrame;

	stream->setAudioData = yang_streamcapture_setAudioData;
    stream->setVideoData = yang_streamcapture_setVideoData;

	stream->setMetaTimestamp = yang_streamcapture_setMetaTimestamp;
	stream->setVideoFrametype = yang_streamcapture_setVideoFrametype;
	stream->setVideoMeta = yang_streamcapture_setVideoMeta;
}

void yang_destroy_streamCapture(YangStreamCapture* stream) {
	if (stream == NULL || stream->context == NULL) {
		return;
	}

	YangStreamCaptureContext* context = (YangStreamCaptureContext*)stream->context;

	if(context->video.sps){
		yang_free(context->video.sps);
	}

	if(context->video.pps){
		yang_free(context->video.pps);
	}

	context->audio.audioBuffer = NULL;
	context->audio.src = NULL;

	yang_free(stream->context);
}



