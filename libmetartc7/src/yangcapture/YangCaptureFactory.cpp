//
// Copyright (c) 2019-2025 yanggaofeng
//

#include <yangcapture/YangCaptureFactory.h>

#if Yang_OS_WIN
#include <yangaudiodev/win/YangWinAudioCapture.h>
#include <yangaudiodev/win/YangWinAudioApiDevice.h>
#include <yangaudiodev/win/YangAudioCaptureWindows.h>
#include "win/YangVideoCaptureWindows.h"

#elif Yang_OS_LINUX
#include <yangaudiodev/linux/YangAudioAecLinux.h>
#include <yangaudiodev/linux/YangAudioCaptureLinux.h>
#include <yangcapture/linux/YangVideoCaptureLinux.h>

#elif Yang_OS_ANDROID
#include <yangaudiodev/android/YangAudioCaptureAndroid.h>
#include <yangcapture/android/YangVideoCaptureAndroid.h>

#elif Yang_OS_APPLE
#include <yangaudiodev/mac/YangAudioCaptureMac.h>
#include <yangcapture/mac/YangVideoCaptureMac.h>
#endif

YangAudioCapture* YangCaptureFactory::CreateAudioCapture(YangAVInfo* avinfo) {
#if Yang_OS_WIN
	return new YangAudioCaptureWindows(avinfo);

#elif Yang_OS_LINUX
    return new YangAudioCaptureLinux(avinfo);

#elif Yang_OS_ANDROID
	return new YangAudioCaptureAndroid(avinfo);

#elif Yang_OS_APPLE
    return new YangAudioCaptureMac(avinfo);

#else 
    return NULL;
#endif
}

YangVideoCapture* YangCaptureFactory::CreateVideoCapture(YangVideoInfo* context) {
#if Yang_OS_WIN
	return new YangVideoCaptureWindows(context);

#elif Yang_OS_LINUX
    return new YangVideoCaptureLinux(context);

#elif Yang_OS_APPLE
    return new YangVideoCaptureMac(context);

#else
    return NULL;
#endif
}

YangVideoCapture* YangCaptureFactory::CreateAndroidCapture(YangVideoInfo* context, void* window) {
#if Yang_OS_ANDROID
	return new YangVideoCaptureAndroid(context, window);
#else
	return NULL;
#endif
}

