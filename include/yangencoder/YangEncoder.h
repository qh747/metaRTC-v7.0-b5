//
// Copyright (c) 2019-2025 yanggaofeng
//
#ifndef YANGENCODER_INCLUDE_YANGENCODER_H_
#define YANGENCODER_INCLUDE_YANGENCODER_H_

#include <yangutil/yangavinfo.h>

class YangEncoderCallback{
public:
	virtual ~YangEncoderCallback() {};

public:
	virtual void onVideoData(YangFrame* frame) = 0;
	virtual void onAudioData(YangFrame* frame) = 0;
};

#endif // YANGENCODER_INCLUDE_YANGENCODER_H_
