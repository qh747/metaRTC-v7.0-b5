//
// Copyright (c) 2019-2025 yanggaofeng
//

#include <yangcapture/linux/YangVideoCaptureLinux.h>
#include <yangutil/yangavinfotype.h>
#include <fcntl.h>

#if Yang_OS_LINUX
#include <assert.h>
#include <linux/fb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

YangVideoCaptureLinux::YangVideoCaptureLinux(YangVideoInfo* context) {
	m_para = context;

	m_vhandle = new YangVideoCaptureHandle(context);
	m_camIdx = context->vIndex;

	m_width = m_para->width;
	m_height = m_para->height;

	m_vd_id = 0;

	memset(&m_buf, 0, sizeof(m_buf));
	m_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	m_buf.memory = V4L2_MEMORY_MMAP;

	m_isloop = 0;
	m_isFirstFrame = 0;
	m_buffer_count = 0;
	m_timestatmp = 0;

	m_fmt = YangI420;
}

YangVideoCaptureLinux::~YangVideoCaptureLinux() {
	if (m_isloop) {
		this->stop();

		while (m_isStart) {
			yang_usleep(1000);
		}
	}

	this->stopCapture();
	this->stopCamDev();

	yang_delete(m_vhandle);
}

void YangVideoCaptureLinux::setVideoCaptureStart() {
	m_vhandle->m_isCapture = 1;
}

void YangVideoCaptureLinux::setVideoCaptureStop() {
	m_vhandle->m_isCapture = 0;
}

void YangVideoCaptureLinux::setOutVideoBuffer(YangVideoBuffer* buf) {
	m_vhandle->setVideoBuffer(buf);
}

void YangVideoCaptureLinux::setPreVideoBuffer(YangVideoBuffer* buf) {
	m_vhandle->setPreVideoBuffer(buf);
}

void YangVideoCaptureLinux::initstamp() {
	m_vhandle->initstamp();
}

int32_t YangVideoCaptureLinux::init() {
	// 1. 打开摄像头设备
	{
        char devStr[30];
	    memset(devStr, 0, 30);
    
	    sprintf(devStr, "/dev/video%d", m_camIdx);
    
	    if ((m_vd_id = open(devStr, O_RDWR)) == -1) {
	    	yang_error("open video device Error!");
	    	return ERROR_SYS_Linux_VideoDeveceOpenFailure;
	    }
	}
	
	// 2. 设置摄像头参数
	{
		struct v4l2_fmtdesc fmt;
	    memset(&fmt, 0, sizeof(struct v4l2_fmtdesc));
    
	    fmt.index = 0;
	    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    
	    int32_t vet = 0;
    
	    // 查询设备能力
        struct v4l2_capability cap;
         
	    if (ioctl(m_vd_id, VIDIOC_QUERYCAP, &cap) != 0) {
	    	yang_error("\n VIDIOC_QUERYCAP error!");
	    	return ERROR_SYS_Linux_NoVideoDriver;
	    }
    
	    yang_trace("\ndriver name %s card = %s cap = %0x\n", cap.driver, cap.card, cap.capabilities);
        
	    // 枚举所有像素格式
        YangColorSpace matchFormat = YangI420;

        int32_t matchWeight = m_para->width;
		int32_t matchHeight = m_para->height;

		bool isMatchFormat = false;
		bool isMatchResolution = false;

	    while ((vet = ioctl(m_vd_id, VIDIOC_ENUM_FMT, &fmt)) != -1) {
	    	fmt.index++;
            
			// 设置像素格式
            if (fmt.pixelformat == V4L2_PIX_FMT_YUYV) {
				isMatchFormat = true;
				matchFormat = YangYuy2;
	        }
            else if (fmt.pixelformat == V4L2_PIX_FMT_YUV420) {
				isMatchFormat = true;
				matchFormat = YangI420;
	        }
            else if (fmt.pixelformat == V4L2_PIX_FMT_NV12) {
				isMatchFormat = true;
				matchFormat = YangNv12;
	        }
            else if (fmt.pixelformat == V4L2_PIX_FMT_YVU420) {
				isMatchFormat = true;
				matchFormat = YangYv12;
	        }
            else {
				continue;
			}
            
			// 设置分辨率
	    	struct v4l2_frmsizeenum frmsize;
	    	frmsize.pixel_format = fmt.pixelformat;
	    	frmsize.index = 0;
    
	    	while (!ioctl(m_vd_id, VIDIOC_ENUM_FRAMESIZES, &frmsize)) {
				frmsize.index++;
                  
				// 跳过非固定分辨率
                if (frmsize.type != V4L2_FRMSIZE_TYPE_DISCRETE) {
					continue;
				}

				// 设置当前分辨率
				matchWeight = (int)frmsize.discrete.width;
				matchHeight = (int)frmsize.discrete.height;

	    		// 设备只支持若干固定分辨率 && 用户设置的分辨率与设备支持的分辨率一致
                if (m_para->width != matchWeight || m_para->height != matchHeight) {
					continue;
	    		}
                
				// 像素格式匹配
				isMatchResolution = true;
				break;
	    	}

			if (isMatchResolution) {
				break;
			}
	    }

		if (!isMatchFormat) {
			yang_error("no match format!");
			return ERROR_SYS_Linux_NoVideoCatpureInterface;
		}

		m_fmt = matchFormat;

		if (!isMatchResolution) {
			m_para->width = matchWeight;
			m_para->height = matchHeight;

			m_width = matchWeight;
			m_height = matchHeight;
		}

		struct v4l2_format v4_format;
	    memset(&v4_format, 0, sizeof(v4_format));
	    
	    v4_format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	    v4_format.fmt.pix.width = m_width;
	    v4_format.fmt.pix.height = m_height;
	    v4_format.fmt.pix.field = V4L2_FIELD_NONE;
        
        if (m_fmt == YangYuy2) {
	        v4_format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	    }
        else if (m_fmt == YangI420) {
	        v4_format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
	    }
        else if (m_fmt == YangNv12) {
	        v4_format.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;
	    }
        else if (m_fmt == YangYv12) {
	        v4_format.fmt.pix.pixelformat = V4L2_PIX_FMT_YVU420;
	    }
    
	    if ((ioctl(m_vd_id, VIDIOC_S_FMT, &v4_format)) != 0) {
	    	yang_error("\n set fmt error!");
	    	return ERROR_SYS_Linux_VideoDeveceOpenFailure;
	    }

	    if(m_vhandle) {
			m_vhandle->setCaptureFormat(m_para->videoCaptureFormat);
		}
	}
    
    // 3. 初始化视频流参数
	{
        struct v4l2_streamparm Stream_Parm;
	    memset(&Stream_Parm, 0, sizeof(struct v4l2_streamparm));
        
		// 设置视频流类型为视频采集
	    Stream_Parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		// 设置帧率，每秒采集m_para->frame个帧
	    Stream_Parm.parm.capture.timeperframe.denominator = m_para->frame;
	    Stream_Parm.parm.capture.timeperframe.numerator = 1;
    
	    if (ioctl(m_vd_id, VIDIOC_S_PARM, &Stream_Parm)) {
	    	yang_error("\n..........................set video frame error!");
			return ERROR_SYS_Linux_VideoDeveceOpenFailure;
	    }
	}

	// 4. 申请视频缓冲区
	{ 
        struct v4l2_requestbuffers tV4L2_reqbuf;
	    memset(&tV4L2_reqbuf, 0, sizeof(struct v4l2_requestbuffers));
        
		// 申请REQ_BUF_NUM个缓冲区
	    tV4L2_reqbuf.count = REQ_BUF_NUM;
		// 设置缓冲区类型为视频采集
	    tV4L2_reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		// 设置缓冲区类型为内存映射
	    tV4L2_reqbuf.memory = V4L2_MEMORY_MMAP;
    
	    if (ioctl(m_vd_id, VIDIOC_REQBUFS, &tV4L2_reqbuf)) {
			yang_error("VIDIOC_REQBUFS");
			return ERROR_SYS_Linux_VideoDeveceOpenFailure;
	    }

	    m_buffer_count = tV4L2_reqbuf.count;

	    for (uint32_t i = 0; i < tV4L2_reqbuf.count; i++) {
	    	struct v4l2_buffer tV4L2buf;
	    	memset(&tV4L2buf, 0, sizeof(struct v4l2_buffer));

	    	tV4L2buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	    	tV4L2buf.memory = V4L2_MEMORY_MMAP;
	    	tV4L2buf.index = i;

	    	if (ioctl(m_vd_id, VIDIOC_QUERYBUF, &tV4L2buf)) {
                printf("search!");
			}
    
	    	m_user_buffer[i].length = tV4L2buf.length;
	    	m_user_buffer[i].start = (uint8_t*)mmap(
                NULL, 
                tV4L2buf.length,
                PROT_READ | PROT_WRITE, 
                MAP_SHARED, 
			    m_vd_id, 
			    tV4L2buf.m.offset
		    );

	    	if (MAP_FAILED == m_user_buffer[i].start) {
	    		yang_error(" error! mmap");
	    		return ERROR_SYS_Linux_VideoDeveceOpenFailure;
	    	}
	    }
	}

	return Yang_Ok;
}

int32_t YangVideoCaptureLinux::readBuffer() {
	if (ioctl(m_vd_id, VIDIOC_DQBUF, &m_buf) != 0) {
		yang_error("VIDIOC_DQBUF");
		exit(1);
	}

	if (m_isFirstFrame) {
        m_timestatmp = (m_buf.timestamp.tv_sec - m_startTime.tv_sec) * 1000000 +
		               (m_buf.timestamp.tv_usec - m_startTime.tv_usec);

	} 
	else {
		m_isFirstFrame = 1;
		m_startTime.tv_sec = m_buf.timestamp.tv_sec;
		m_startTime.tv_usec = m_buf.timestamp.tv_usec;
		m_timestatmp = 0;
	}

	if (m_vhandle) {
		m_vhandle->putBuffer(
			m_timestatmp, 
			m_user_buffer[m_buf.index].start,
			m_user_buffer[m_buf.index].length
		);
	}

	if (ioctl(m_vd_id, VIDIOC_QBUF, &m_buf) != 0) {
		yang_error("VIDIOC_QBUF");
		exit(1);
	}

	return Yang_Ok;
}

void YangVideoCaptureLinux::stopLoop() {
	m_isloop = 0;
}

void YangVideoCaptureLinux::stopCapture() {
	enum v4l2_buf_type type;

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == ioctl(m_vd_id, VIDIOC_STREAMOFF, &type)) {
		yang_error("Fail to ioctl 'VIDIOC_STREAMOFF'");
		exit(EXIT_FAILURE);
	}
}

void YangVideoCaptureLinux::stopCamDev() {
	int32_t i = 0;

	for (i = 0; i < m_buffer_count; i++) {
		if (-1 == munmap(m_user_buffer[i].start, m_user_buffer[i].length)) {
			exit(EXIT_FAILURE);
		}
	}

	if (-1 == close(m_vd_id)) {
		yang_error("Fail to close fd");
		exit(EXIT_FAILURE);
	}
}

void YangVideoCaptureLinux::startLoop() {
	for (int32_t i = 0; i < m_buffer_count; i++) {
		struct v4l2_buffer tV4L2buf;
		memset(&tV4L2buf, 0, sizeof(struct v4l2_buffer));

		tV4L2buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		tV4L2buf.memory = V4L2_MEMORY_MMAP;
		tV4L2buf.index = i;

		if (ioctl(m_vd_id, VIDIOC_QBUF, &tV4L2buf)) {
			yang_error("VIDIOC_QBUF");
		}
	}

	enum v4l2_buf_type v4l2type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(m_vd_id, VIDIOC_STREAMON, &v4l2type)) {
		yang_error("VIDIOC_STREAMON");
	}

	fd_set fds;
	struct timeval tv;
	int32_t r;
	FD_ZERO(&fds);
	FD_SET(m_vd_id, &fds);
	m_isloop = 1;
	m_vhandle->m_start_time = 0;

	while (m_isloop) {
		tv.tv_sec = 2;
		tv.tv_usec = 0;
		r = select(m_vd_id + 1, &fds, NULL, NULL, &tv);

		if (-1 == r) {
			if (EINTR == errno)
				continue;
			yang_error("video capture Fail to select");
			exit(EXIT_FAILURE);
		}

		if (0 == r) {
			yang_error("video capture select Timeout\n");
			exit(EXIT_FAILURE);
		}
		
		this->readBuffer();
	}
}

#endif
