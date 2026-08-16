//
// Copyright (c) 2019-2025 yanggaofeng
//

#include <yangutil/sys/YangLog.h>
#include <yangutil/sys/YangTime.h>
#include <yangcapture/YangVideoCaptureHandle.h>

YangVideoCaptureHandle::YangVideoCaptureHandle(YangVideoInfo* context) {
	memset(&m_videoFrame, 0, sizeof(YangFrame));

	m_curStamp = 0;
	m_baseStamp = 0;

	m_isCapture = false;

	m_outVideoBuffer = NULL;
	m_preVideoBuffer = NULL;

	m_width = context->width;
	m_height = context->height;

	m_yLen = m_width * m_height;
	m_uLen = m_yLen >> 2;

	m_buf = NULL;
	m_androidBuf = NULL;
   
	// YUV420P格式的缓冲区大小 = width * height * 3 / 2
	m_bufLen = m_width * m_height * 3 / 2;

	m_encoderVideoFormat = context->videoEncoderFormat;
	m_captureVideoFormat = context->videoCaptureFormat;

	m_rotate = static_cast<RotationModeEnum>(context->rotate);
}

YangVideoCaptureHandle::~YangVideoCaptureHandle() {
	m_outVideoBuffer = NULL;
	m_preVideoBuffer = NULL;

	yang_deleteA(m_buf);
	yang_deleteA(m_androidBuf);
}

void YangVideoCaptureHandle::setCaptureFormat(int32_t format) {
    m_captureVideoFormat = format;

    if((format == YangYuy2 || format == YangNv12) && !m_buf) {
        m_buf = new uint8_t[m_bufLen];
    }
}

void YangVideoCaptureHandle::putBuffer(int64_t timestamp, uint8_t* buf, int32_t len) {
	m_curStamp = timestamp;

	if (m_baseStamp == 0) {
		m_baseStamp = timestamp;
	}

	m_videoFrame.pts = m_curStamp - m_baseStamp;

	uint8_t* tmp = NULL;
	if (m_buf) {
		if (m_captureVideoFormat == YangYuy2) {
			if (m_encoderVideoFormat == YangI420) {
				m_yuv.yuy2toI420(buf, m_buf, m_width, m_height);
			}
			else if (m_encoderVideoFormat == YangNv12) {
				m_yuv.yuy2tonv12(buf, m_buf, m_width, m_height);
			}
			else if (m_encoderVideoFormat == YangArgb) {
				m_yuv.yuy2toargb(buf, m_buf, m_width, m_height);
			}

			tmp = m_buf;
        }
		else if (m_captureVideoFormat == YangNv12) {
            if (m_encoderVideoFormat == YangI420) {
                m_yuv.nv12toI420(buf, m_buf, m_width, m_height);
                tmp = m_buf;
            }
            else if (m_encoderVideoFormat == YangNv12) {
                tmp = buf;
			}
        }
		else if (m_captureVideoFormat == YangI420) {
            if (m_encoderVideoFormat == YangI420) {
                tmp = buf;
            }
            else if (m_encoderVideoFormat == YangNv12) {
                m_yuv.i420tonv12(buf,m_buf,m_width,m_height);
                tmp = m_buf;
            }
		}
		else if (m_captureVideoFormat == YangArgb) {
			tmp = buf;
		}

		m_videoFrame.payload = tmp;
		m_videoFrame.nb = m_bufLen;
	}
	else {
		m_videoFrame.payload = buf;
		m_videoFrame.nb = len;
	}
	
	if (m_preVideoBuffer) {
		m_preVideoBuffer->putVideo(&m_videoFrame);
	}
	
	if (m_isCapture && m_outVideoBuffer) {
		m_outVideoBuffer->putVideo(&m_videoFrame);
	}
}

void YangVideoCaptureHandle::putBuffer(YangColorSpace type, uint8_t* bufY, uint8_t* bufU, uint8_t* bufV) {
	if(m_androidBuf == NULL) {
	    m_androidBuf = new uint8_t[m_bufLen];
	}

	if (type == YangNv21) {
        m_yuv.nv21toI420(bufY, m_androidBuf, m_width, m_height);
	}
	else {
        if (bufY) {
			memcpy(m_androidBuf, bufY, m_yLen);
		}

	    if (bufU) {
			memcpy(m_androidBuf + m_yLen, bufU, m_uLen);
		}

	    if (bufV) {
			memcpy(m_androidBuf + m_yLen + m_uLen, bufV, m_uLen);
		}
	}

	m_curStamp = yang_get_system_time();

	if (m_baseStamp == 0) {
		m_baseStamp = m_curStamp;

		if (m_rotate != kRotate0 && m_buf == NULL) {
			m_buf = new uint8_t[m_bufLen];
		}
	}

	if (m_rotate != kRotate0) {
		m_yuv.rotateI420(
			m_androidBuf,
			m_buf,
			m_width,
			m_height,
			(RotationMode)m_rotate
		);

		m_videoFrame.payload = m_buf;
	}
	else {
		m_videoFrame.payload = m_androidBuf;
	}

	m_videoFrame.nb = m_bufLen;
	m_videoFrame.pts = m_curStamp - m_baseStamp;

	if (m_isCapture && m_outVideoBuffer) {
		m_outVideoBuffer->putVideo(&m_videoFrame);
	}
}
