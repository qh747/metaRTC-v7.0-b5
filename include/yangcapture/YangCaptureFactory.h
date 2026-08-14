//
// Copyright (c) 2019-2025 yanggaofeng
//

#ifndef INCLUDE_YANGCAPTURE_YANGCAPTUREFACTORY_H_
#define INCLUDE_YANGCAPTURE_YANGCAPTUREFACTORY_H_
#include <yangcapture/YangMultiVideoCapture.h>
#include <yangaudiodev/YangAudioCapture.h>

class YangCaptureFactory {
public:
	YangCaptureFactory();
	virtual ~YangCaptureFactory();

public:
	YangAudioCapture* createAudioCapture(YangContext* context);
	YangAudioCapture* createRecordAudioCapture(YangAVInfo* avinfo);
	
	YangMultiVideoCapture* createVideoCapture(YangVideoInfo* context);
    YangMultiVideoCapture* createRecordVideoCapture(YangVideoInfo* context);
    YangMultiVideoCapture* createRecordVideoCaptureAndroid(YangVideoInfo* context, void* window);
};

#endif // INCLUDE_YANGCAPTURE_YANGCAPTUREFACTORY_H_ */
