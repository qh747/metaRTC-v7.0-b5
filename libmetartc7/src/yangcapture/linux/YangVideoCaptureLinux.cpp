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

	m_devFd = 0;

	m_isloop = 0;
	m_isFirstFrame = false;
	m_buffer_count = 0;

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
    
	    if ((m_devFd = open(devStr, O_RDWR)) == -1) {
	    	yang_error("open video device error!");
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
         
	    if (ioctl(m_devFd, VIDIOC_QUERYCAP, &cap) != 0) {
	    	yang_error("set video driver param error!");
	    	return ERROR_SYS_Linux_NoVideoDriver;
	    }
    
	    yang_trace("driver name %s card = %s cap = %0x", cap.driver, cap.card, cap.capabilities);
        
	    // 枚举所有像素格式
        YangColorSpace matchFormat = YangI420;

        int32_t matchWeight = m_para->width;
		int32_t matchHeight = m_para->height;

		bool isMatchFormat = false;
		bool isMatchResolution = false;

	    while ((vet = ioctl(m_devFd, VIDIOC_ENUM_FMT, &fmt)) != -1) {
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
    
	    	while (!ioctl(m_devFd, VIDIOC_ENUM_FRAMESIZES, &frmsize)) {
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
			yang_error("no match video format!");
			return ERROR_SYS_Linux_NoVideoCatpureInterface;
		}

		m_fmt = matchFormat;

		if (!isMatchResolution) {
			m_para->width = matchWeight;
			m_para->height = matchHeight;
		}

		struct v4l2_format v4_format;
	    memset(&v4_format, 0, sizeof(v4_format));
	    
	    v4_format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	    v4_format.fmt.pix.width = m_para->width;
	    v4_format.fmt.pix.height = m_para->height;
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
    
	    if ((ioctl(m_devFd, VIDIOC_S_FMT, &v4_format)) != 0) {
	    	yang_error("set video format error!");
	    	return ERROR_SYS_Linux_VideoDeveceOpenFailure;
	    }

	    if(m_vhandle) {
			m_vhandle->setCaptureFormat(m_para->videoCaptureFormat);
		}
	}
    
    // 3. 设置视频流帧率
	{
        struct v4l2_streamparm Stream_Parm;
	    memset(&Stream_Parm, 0, sizeof(struct v4l2_streamparm));
        
		// 设置视频流类型为视频采集
	    Stream_Parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		// 设置帧率，每秒采集m_para->frame个帧
	    Stream_Parm.parm.capture.timeperframe.denominator = m_para->frame;
	    Stream_Parm.parm.capture.timeperframe.numerator = 1;
    
	    if (ioctl(m_devFd, VIDIOC_S_PARM, &Stream_Parm)) {
	    	yang_error("set video frame rate error!");
			return ERROR_SYS_Linux_VideoDeveceOpenFailure;
	    }
	}

	// 4. 设置视频流缓冲区
	{ 
        struct v4l2_requestbuffers tV4L2_reqbuf;
	    memset(&tV4L2_reqbuf, 0, sizeof(struct v4l2_requestbuffers));
        
		// 申请REQ_BUF_NUM个缓冲区
	    tV4L2_reqbuf.count = REQ_BUF_NUM;
		// 设置缓冲区类型为视频采集
	    tV4L2_reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		// 设置缓冲区类型为内存映射
	    tV4L2_reqbuf.memory = V4L2_MEMORY_MMAP;
    
	    if (ioctl(m_devFd, VIDIOC_REQBUFS, &tV4L2_reqbuf)) {
			yang_error("request video buffer error!");
			return ERROR_SYS_Linux_VideoDeveceOpenFailure;
	    }

	    m_buffer_count = tV4L2_reqbuf.count;

	    for (uint32_t i = 0; i < tV4L2_reqbuf.count; i++) {
	    	struct v4l2_buffer tV4L2buf;
	    	memset(&tV4L2buf, 0, sizeof(struct v4l2_buffer));

	    	tV4L2buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	    	tV4L2buf.memory = V4L2_MEMORY_MMAP;
	    	tV4L2buf.index = i;

	    	if (ioctl(m_devFd, VIDIOC_QUERYBUF, &tV4L2buf)) {
                printf("search!");
			}
    
	    	m_user_buffer[i].length = tV4L2buf.length;
	    	m_user_buffer[i].start = (uint8_t*)mmap(
                NULL, 
                tV4L2buf.length,
                PROT_READ | PROT_WRITE, 
                MAP_SHARED, 
			    m_devFd, 
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

void YangVideoCaptureLinux::stopLoop() {
	m_isloop = 0;
}

void YangVideoCaptureLinux::stopCapture() {
	enum v4l2_buf_type type;

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == ioctl(m_devFd, VIDIOC_STREAMOFF, &type)) {
		yang_error("stop video stream error!");
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

	if (-1 == close(m_devFd)) {
		yang_error("close video device error!");
		exit(EXIT_FAILURE);
	}
}

void YangVideoCaptureLinux::startLoop() {
	// 将空缓冲区递交给驱动，让驱动向缓冲区中写入视频帧数据
	for (int32_t idx = 0; idx < m_buffer_count; idx++) {
		struct v4l2_buffer tV4L2buf;
		memset(&tV4L2buf, 0, sizeof(struct v4l2_buffer));

		tV4L2buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		tV4L2buf.memory = V4L2_MEMORY_MMAP;
		tV4L2buf.index = idx;

		if (ioctl(m_devFd, VIDIOC_QBUF, &tV4L2buf)) {
			yang_error("put video buffer error!");
		}
	}
    
	// 让摄像头开始采集视频流
	enum v4l2_buf_type v4l2type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(m_devFd, VIDIOC_STREAMON, &v4l2type)) {
		yang_error("start video stream error!");
	}

	fd_set fds;
	
	FD_ZERO(&fds);
	FD_SET(m_devFd, &fds);

	m_isloop = 1;

	while (m_isloop) {
		struct timeval tv;
		tv.tv_sec = 2;
		tv.tv_usec = 0;

		int32_t r = select(m_devFd + 1, &fds, NULL, NULL, &tv);
        
		// I/O 复用异常处理
		{
            if (-1 == r) {
		    	// 被信号中断则继续等待
		    	if (EINTR == errno) {
		    		continue;
		    	}
    
		    	yang_error("video capture select error!");
		    	exit(EXIT_FAILURE);
		    }
            else if (0 == r) {
		    	yang_error("video capture select timeout!");
		    	exit(EXIT_FAILURE);
		    }
		}
		
		// 读取视频帧数据
		{
		    struct v4l2_buffer buffer;
    
	        memset(&buffer, 0, sizeof(buffer));
	        buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	        buffer.memory = V4L2_MEMORY_MMAP;
            
	        // 从驱动取出已填满的缓冲区
	        if (ioctl(m_devFd, VIDIOC_DQBUF, &buffer) != 0) {
	        	yang_error("get video buffer error!");
	        	exit(1);
	        }
            
	        // 计算时间戳
	        long timeDiff = 0;
        
	        if (!m_isFirstFrame) {
	        	m_isFirstFrame = true;
        
	        	m_startTime.tv_sec = buffer.timestamp.tv_sec;
	        	m_startTime.tv_usec = buffer.timestamp.tv_usec;
	        } 
	        else {
	        	timeDiff = (buffer.timestamp.tv_sec - m_startTime.tv_sec) * 1000000 +
	        	           (buffer.timestamp.tv_usec - m_startTime.tv_usec);
	        }
            
	        // 处理当前视频帧数据
	        if (m_vhandle) {
	        	m_vhandle->putBuffer(
	        		timeDiff, 
	        		m_user_buffer[buffer.index].start,
	        		m_user_buffer[buffer.index].length
	        	);
	        }
        
	        // 将已处理完的缓冲区重新递交给驱动
	        if (ioctl(m_devFd, VIDIOC_QBUF, &buffer) != 0) {
	        	yang_error("put video buffer error!");
	        	exit(1);
	        }
		}
	}
}

#endif
