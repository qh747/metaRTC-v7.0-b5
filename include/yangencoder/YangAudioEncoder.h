//
// Copyright (c) 2019-2025 yanggaofeng
//
#ifndef ___YangAudioEncoderPipe__
#define ___YangAudioEncoderPipe__

#include <yangutil/sys/YangThread2.h>
#include <yangutil/yangavinfotype.h>
#include <yangutil/buffer/YangAudioEncoderBuffer.h>
#include <yangutil/buffer/YangAudioBuffer.h>
#include <yangencoder/YangEncoder.h>

class YangAudioEncoder {
public:
	virtual ~YangAudioEncoder() {}

public:
    virtual void init(YangAudioInfo* info) = 0;
    virtual int32_t encoder(YangFrame* frame, YangEncoderCallback* cb) = 0;
};

#endif //___YangAudioEncoderPipe__
