//
// Copyright (c) 2019-2025 yanggaofeng
//
#include <yangencoder/YangEncoderFactory.h>
#include <yangencoder/YangAudioEncoderOpus.h>
#include <yangencoder/YangFfmpegEncoderMeta.h>
#include <yangencoder/YangGpuEncoderFactory.h>
#include <yangencoder/YangH265EncoderMeta.h>
#include <yangencoder/YangH265EncoderSoft.h>
#include <yangencoder/YangVideoEncoderFfmpeg.h>
#include <yangencoder/YangVideoEncoderMac.h>
#include <yangencoder/YangEncoderMediacodec.h>

#if Yang_Enable_Openh264
#include <yangencoder/YangOpenH264Encoder.h>
#else
#include <yangencoder/YangH264EncoderMeta.h>
#include <yangencoder/YangH264EncoderSoft.h>
#endif

YangAudioEncoder* YangEncoderFactory::CreateAudioEncoder(YangAudioInfo* info) {
    YangAudioCodec codec = static_cast<YangAudioCodec>(info->audioEncoderType);
    
    if (codec == Yang_AED_OPUS) {
        return new YangAudioEncoderOpus();
    }
    else {
        return NULL;
    }
}

YangVideoEncoder* YangEncoderFactory::CreateVideoEncoder(YangVideoInfo* info) {
    // 平台级硬编码优先级最高
#if Yang_OS_ANDROID
    return new YangEncoderMediacodec();
#elif Yang_OS_APPLE
    return new YangVideoEncoderMac();
#elif Yang_OS_WIN
    YangGpuEncoderFactory gf;
    return gf.createGpuEncoder();
#endif

    YangVideoCodec codec = static_cast<YangVideoCodec>(info->videoEncoderType);

    if (codec == Yang_VED_H264 && info->videoEncHwType == 0) {
#if Yang_Enable_Openh264
        return new YangOpenH264Encoder();
#else
        return new YangH264EncoderSoft();
#endif
    }
    else if (codec == Yang_VED_H265 && info->videoEncHwType == 0) {
        return new YangH265EncoderSoft();
    }
    else {
#if Yang_Enable_Ffmpeg_Codec
        return new YangVideoEncoderFfmpeg(codec, info->videoEncHwType);
#else
        return NULL;
#endif
    }
}

YangVideoEncoderMeta* YangEncoderFactory::CreateVideoEncoderMeta(YangVideoInfo* info) {
#if Yang_Enable_Ffmpeg_Codec
    if (info->videoEncHwType > 0) {
        return new YangFfmpegEncoderMeta();
    }    
#endif

    YangVideoCodec codec = static_cast<YangVideoCodec>(info->videoEncoderType);

    if (codec == Yang_VED_H264 && info->videoEncoderType == 0) {
#if !Yang_Enable_Openh264
        return new YangH264EncoderMeta();
#else
        return NULL;
#endif
    }
    else if (codec == Yang_VED_H265 && info->videoEncoderType == 1) {
        return new YangH265EncoderMeta();
    }
    else {
        return NULL;
    }
}
