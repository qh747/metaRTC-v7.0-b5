//
// Copyright (c) 2019-2022 yanggaofeng
//

#include <yangaudiodev/linux/YangAudioCaptureLinux.h>
#include <yangavutil/audio/YangAudioUtil.h>

#if Yang_OS_LINUX

YangAudioCaptureLinux::YangAudioCaptureLinux(YangAVInfo* avinfo) {
	m_avinfo = avinfo;
	m_ahandle = new YangAudioCaptureHandle(avinfo);

	m_loops = 0;
	m_handle = NULL;

	m_frames = (avinfo->audio.audioEncoderType == Yang_AED_AAC) ? 1024 : avinfo->audio.sample / 50;
	m_mono = yangfalse;
}

YangAudioCaptureLinux::~YangAudioCaptureLinux() {
	if (m_loops) {
		this->stop();

		while (m_isStart) {
			yang_usleep(500);
		}
	}

	if (m_handle) {
		snd_pcm_close(m_handle);
		m_handle = NULL;
	}

	yang_delete(m_ahandle);
}

void YangAudioCaptureLinux::setCatureState(yangbool enabled) {
    m_ahandle->setCaptureState(enabled);
}

void YangAudioCaptureLinux::setOutAudioBuffer(YangAudioBuffer* buffer) {
	m_ahandle->setOutAudioBuffer(buffer);
}

int32_t YangAudioCaptureLinux::init() {
	// 1. 获取PCM采集设备名称
	char device_name[64];
	memset(device_name, 0, 64);

	if (m_avinfo->audio.aIndex > -1) {
        sprintf(device_name, "hw:%d,%d", m_avinfo->audio.aIndex, m_avinfo->audio.aSubIndex);
	}
	else {
		strcpy(device_name, "default");
	}
	
	// 2. 打开PCM采集设备
	int32_t err = snd_pcm_open(
		&m_handle,
		device_name,
		SND_PCM_STREAM_CAPTURE, 
		0
	);

	if (err < 0) {
		yang_error("unable to open pcm device: %s\n", snd_strerror(err));
		yang_exit(1);
	}
    
	// 3. 获取PCM采集设备参数
	snd_pcm_hw_params_t* hw_params = NULL;
	err = snd_pcm_hw_params_malloc(&hw_params);

	if (err < 0) {
		yang_error("cannot allocate hardware parameter structure (%s)\n", snd_strerror(err));
		yang_exit(1);
	}
    
	// 4. 初始化PCM采集设备参数
	err = snd_pcm_hw_params_any(m_handle, hw_params);

	if (err < 0) {
		yang_error("cannot initialize hardware parameter structure (%s)\n", snd_strerror(err));
		yang_exit(1);
	}
    
	// 5. 设置PCM采集数据访问格式为交错访问：L R L R ...
	err = snd_pcm_hw_params_set_access(
		m_handle, 
		hw_params, 
		SND_PCM_ACCESS_RW_INTERLEAVED
	);

	if (err < 0) {
		yang_error("cannot set access type (%s)\n", snd_strerror(err));
		yang_exit(1);
	}
    
	// 6. 设置采样格式为有符号 16 位小端（S16_LE）
	err = snd_pcm_hw_params_set_format(m_handle, hw_params, SND_PCM_FORMAT_S16_LE);

	if (err < 0) {
		yang_error("cannot set sample format (%s)\n", snd_strerror(err));
		yang_exit(1);
	}
    
	// 7. 设置采样率
	uint32_t sampleRate = m_avinfo->audio.sample;
	err = snd_pcm_hw_params_set_rate_near(m_handle, hw_params, &sampleRate, 0);

	if (err < 0) {
		yang_error("cannot set sample rate (%s)\n", snd_strerror(err));
		yang_exit(1);
	}
    
	// 8. 设置声道数量
	err = snd_pcm_hw_params_set_channels(m_handle, hw_params, m_avinfo->audio.channel);

	if (err < 0) {
		yang_error("cannot set double channel (%s)\n", snd_strerror(err));

		err = snd_pcm_hw_params_set_channels(m_handle, hw_params, 1);
		m_mono = yangtrue;
		
		if (err < 0) {
			yang_error("cannot set single channel (%s)\n", snd_strerror(err));
			yang_exit(1);
		}
	}
    
	// 9. 设置周期大小，即每次读写的帧数
	int32_t dir = 0;
	err = snd_pcm_hw_params_set_period_size_near(m_handle, hw_params, &m_frames, &dir);

	if (err < 0) {
		yang_error("cannot set period size (%s)\n", snd_strerror(err));
		yang_exit(1);
	}
    
	// 10. 应用之前设置的硬件参数
	if ((err = snd_pcm_hw_params(m_handle, hw_params)) < 0) {
		yang_error("cannot set parameters (%s)\n", snd_strerror(err));
		yang_exit(1);
	}

	snd_pcm_hw_params_free(hw_params);
	return Yang_Ok;
}

void YangAudioCaptureLinux::startLoop() {
	int32_t  audiolen = m_frames * m_avinfo->audio.channel * 2;
	uint8_t* audioBuf = new uint8_t[audiolen];
    
	uint8_t* stereoBuf = NULL;

	if (m_mono) {
		stereoBuf = new uint8_t[m_frames * 2 * 2];
	}

	unsigned long status = snd_pcm_prepare(m_handle);

	if (status < 0) {
		yang_error("cannot prepare audio interface for use (%s)\n", snd_strerror(status));
		yang_exit(1);
	}

	status = snd_pcm_start(m_handle);

	if (status < 0) {
		yang_error("cannot start audio interface for use (%s)\n", snd_strerror(status));
		yang_exit(1);
	}

	m_loops = yangtrue;

	while (m_loops) {
		yang_usleep(5000);

		if (snd_pcm_avail_update(m_handle) < m_frames) {
			continue;
		}

		int32_t readlen = snd_pcm_readi(m_handle, (short*)audioBuf, m_frames);

	    if (readlen != m_frames) {
	    	if (readlen < 0) {
	    		yang_error("read from audio interface failed (%s)", snd_strerror(readlen));
                
				readlen = snd_pcm_prepare(m_handle);

	    		if (readlen < 0) {
	    			yang_error(
						"cannot prepare audio interface for use (%s)", 
						snd_strerror(readlen)
					);

					continue;
	    		}

                readlen = snd_pcm_start(m_handle);

	    		if (readlen < 0) {
	    			yang_error(
						"cannot prepare audio interface for use (%s)", 
						snd_strerror(readlen)
					);

					continue;
	    		}
	    	} 
	    	else {
	    		yang_error(
					"Couldn't read as many samples as I wanted (%d instead of %d)", 
					readlen, 
					m_frames
				);
	    	}
    
	    	continue;
	    }
        
		uint8_t* usedBuf = audioBuf;

		if (m_mono) {
			MonoToStereo(
				(int16_t*)stereoBuf, 
				(int16_t*)audioBuf, 
				m_frames
			);

			usedBuf = stereoBuf;
		} 
        
		m_ahandle->putBuffer(usedBuf, audiolen);
	}

	snd_pcm_close(m_handle);

	yang_deleteA(audioBuf);
	yang_deleteA(stereoBuf);

	m_handle = NULL;
}

void YangAudioCaptureLinux::stopLoop() {
	m_loops = 0;
}

#endif
