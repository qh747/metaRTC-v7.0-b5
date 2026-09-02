//
// Copyright (c) 2019-2025 yanggaofeng
//

#ifndef YANGENCODER_INCLUDE_YANGVIDEOENCODERFACTORY_H_
#define YANGENCODER_INCLUDE_YANGVIDEOENCODERFACTORY_H_

#include <yangutil/yangavinfotype.h>
#include "YangVideoEncoder.h"
#include "yangencoder/YangAudioEncoder.h"
#include <yangavutil/video/YangVideoEncoderMeta.h>

class YangEncoderFactory {
public:
    static YangAudioEncoder* CreateAudioEncoder(YangAudioInfo* info);
    static YangVideoEncoder* CreateVideoEncoder(YangVideoInfo* info);

    static YangVideoEncoderMeta* CreateVideoEncoderMeta(YangVideoInfo* info);
};

#endif // YANGENCODER_INCLUDE_YANGVIDEOENCODERFACTORY_H_ */
