//
// Copyright (c) 2019-2025 yanggaofeng
//
#ifndef __YangLivingVideoCaptureHandle__
#define __YangLivingVideoCaptureHandle__

#include <yangutil/yangavinfotype.h>
#include <yangavutil/video/YangYuvConvert.h>
#include <yangutil/buffer/YangVideoBuffer.h>

class YangVideoCaptureHandle {
public:
	YangVideoCaptureHandle(YangVideoInfo* context);
	~YangVideoCaptureHandle();

public:
    void putBuffer(int64_t timestamp, uint8_t* buf, int32_t len);
	void putBuffer(YangColorSpace type, uint8_t* bufY, uint8_t* bufU, uint8_t* bufV);
	
	void startLoop();
	
	void setCaptureFormat(int32_t format);

	inline void initstamp() { m_baseStamp = m_curStamp; }

    inline void setVideoBuffer(YangVideoBuffer* buf) { m_outVideoBuffer = buf; }
	inline void setPreVideoBuffer(YangVideoBuffer* buf) { m_preVideoBuffer = buf; }

public:
	bool m_isCapture;

private:
	YangVideoBuffer* m_outVideoBuffer;
	YangVideoBuffer* m_preVideoBuffer;

	YangFrame m_videoFrame;
	YangYuvConvert m_yuv;

	uint8_t* m_buf;
	uint8_t* m_androidBuf;

    int32_t m_width;
	int32_t m_height;

	int32_t m_bufLen;

	int32_t m_yLen;
	int32_t m_uLen;

	int64_t m_curStamp;
	int64_t m_baseStamp;
	
	int m_encoderVideoFormat;
	int m_captureVideoFormat;

	RotationModeEnum m_rotate;
};

#endif // __YangLivingVideoCaptureHandle__

