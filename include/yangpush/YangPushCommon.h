//
// Copyright (c) 2019-2025 yanggaofeng
//
#ifndef INCLUDE_YANGPUSH_YANGPUSHCOMMON_H_
#define INCLUDE_YANGPUSH_YANGPUSHCOMMON_H_

// 摄像头源
#define Yang_VideoSrc_Camera 0

enum YangPushMessageType {
	YangM_Push_StartAudioCapture,
	YangM_Push_StartVideoCapture,
	YangM_Push_Connect,
    YangM_Push_Connect_Whip,
	YangM_Push_Disconnect
};

#endif // INCLUDE_YANGPUSH_YANGPUSHCOMMON_H_
