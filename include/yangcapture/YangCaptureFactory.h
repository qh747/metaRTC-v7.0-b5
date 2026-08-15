//
// Copyright (c) 2019-2025 yanggaofeng
//

#ifndef INCLUDE_YANGCAPTURE_YANGCAPTUREFACTORY_H_
#define INCLUDE_YANGCAPTURE_YANGCAPTUREFACTORY_H_

#include <yangcapture/YangVideoCapture.h>
#include <yangaudiodev/YangAudioCapture.h>

class YangCaptureFactory {
public:
	static YangAudioCapture* CreateAudioCapture(YangAVInfo* avinfo);
    static YangVideoCapture* CreateVideoCapture(YangVideoInfo* context);
    static YangVideoCapture* CreateAndroidCapture(YangVideoInfo* context, void* window);
};

#endif // INCLUDE_YANGCAPTURE_YANGCAPTUREFACTORY_H_
