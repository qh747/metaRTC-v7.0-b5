//
// Copyright (c) 2019-2025 yanggaofeng
//

#include <yangutil/sys/YangLog.h>
#include <yangencoder/YangAudioEncoderOpus.h>

#define MAX_PACKET_SIZE (3 * 1276)

#if Yang_Opus_So
void YangAudioEncoderOpus::loadLib() {
	yang_opus_encoder_create = (OpusEncoder* (*)(
		opus_int32 Fs, 
		int32_t channels,
		int32_t application, 
		int32_t* error)
	) m_lib.loadFunction("opus_encoder_create");

	yang_opus_encoder_init = (int32_t (*)(
		OpusEncoder* st,
		opus_int32 Fs,
		int32_t channels, 
		int32_t application)
	) m_lib.loadFunction("opus_encoder_init");

	yang_opus_encode = (opus_int32 (*)(
		OpusEncoder* st, 
		const opus_int16* pcm,
		int32_t frame_size, 
		uint8_t* data,
		opus_int32 max_data_bytes)
	) m_lib.loadFunction("opus_encode");

	yang_opus_encoder_ctl = (int32_t (*)(
		OpusEncoder* st, 
		int32_t request, 
		...)
	) m_lib.loadFunction("opus_encoder_ctl");

	yang_opus_encoder_destroy = (void (*)(
		OpusEncoder* st)
	) m_lib.loadFunction("opus_encoder_destroy");

	yang_opus_strerror = (const char* (*)(
		int32_t error)
	) m_lib.loadFunction("opus_strerror");
}

void YangAudioEncoderOpus::unloadLib() {
	yang_opus_encoder_create = NULL;
	yang_opus_encoder_init = NULL;
	yang_opus_encode = NULL;
	yang_opus_encoder_ctl = NULL;
	yang_opus_encoder_destroy = NULL;
	yang_opus_strerror = NULL;
}
#endif

YangAudioEncoderOpus::YangAudioEncoderOpus() {
	m_pcmBuf = NULL;
	m_pcmFrameSize = 0;
	
	m_encInBuf = NULL;
	m_encInFrameSize = 0;

	m_encOutBuf = NULL;

	m_encoder = NULL;

#if Yang_Opus_So
	unloadLib();
#endif
}

YangAudioEncoderOpus::~YangAudioEncoderOpus() {
	if (m_encoder) {
		yang_opus_encoder_destroy(m_encoder);
		m_encoder = NULL;
	}
    
	yang_deleteA(m_encInBuf);
	yang_deleteA(m_encOutBuf);
	
	yang_deleteA(m_pcmBuf);

#if Yang_Opus_So
	unloadLib();
	m_lib.unloadObject();
#endif
}

void YangAudioEncoderOpus::init(YangAudioInfo* info) {
	if (m_encoder != NULL) {
		return;
	}

#if Yang_Opus_So
	m_lib.loadObject("libopus");
	loadLib();
#endif
    
    // 1. 创建opus编码器
	int32_t err = 0;

	m_encoder = yang_opus_encoder_create(
		// 采样率
		info->sample, 
		// 声道数
		info->channel,
		// 应用场景：语音通话，更低延迟、更强抗丢包
		OPUS_APPLICATION_VOIP, 
		// 错误码
		&err
	);

	if (err < 0) {
		yang_error("failed to create an opus encoder: %s", yang_opus_strerror(err));

#ifdef _MSC_VER
        ExitProcess(1);
#else
        _exit(0);
#endif
	}
    
	// 2. 设置编码器使用固定码率，0表示关闭 VBR 可变码率
	yang_opus_encoder_ctl(m_encoder, OPUS_SET_VBR(0));
    
	// 3. 如果是用单声道则把频谱上限锁到 WB（约 8 kHz 音频带宽），省码率
	if (info->enableMono) {
		yang_opus_encoder_ctl(m_encoder, OPUS_SET_BANDWIDTH(OPUS_BANDWIDTH_WIDEBAND));
	}
    
	// 4. 如果需要纠错，则开启 FEC 前向纠错
	if(info->enableAudioFec) {
		yang_opus_encoder_ctl(m_encoder, OPUS_SET_INBAND_FEC(1));
		yang_opus_encoder_ctl(m_encoder, OPUS_SET_PACKET_LOSS_PERC(20));
	}
    
	// 5. 编码器内部预读采样数（算法延迟）。读出来放进 skip 变量
	int32_t skip = 0;
	yang_opus_encoder_ctl(m_encoder, OPUS_GET_LOOKAHEAD(&skip));

	// 6. 设置采样深度为 16-bit
	yang_opus_encoder_ctl(m_encoder, OPUS_SET_LSB_DEPTH(16));
    
	// 7. 设置待编码的帧大小：Opus 常用 20ms 一帧，采样率 / 50。
	m_pcmFrameSize = info->sample / 50;

	// 8. 计算一帧采样数：帧大小 * 声道数
	m_encInFrameSize = m_pcmFrameSize * info->channel;
    
	// 9. 分配 PCM 字节流缓冲区
	m_pcmBuf = new uint8_t[m_encInFrameSize * 2];
    
	// 10. 分配 Opus 输入和输出缓冲区
	m_encInBuf = new short[m_encInFrameSize];
	m_encOutBuf = new uint8_t[MAX_PACKET_SIZE];
}

int32_t YangAudioEncoderOpus::encoder(YangFrame* frame, YangEncoderCallback* cb) {
	if (m_encoder == NULL) {
		return 1;
	}
    
	// 拷贝 PCM 字节流到缓冲区
	memcpy(m_pcmBuf, frame->payload, frame->nb);
    
	// 把采集来的小端 16-bit PCM 字节流拆成 Opus 要的 int16 采样数组
	// 采集侧交给编码器的是 uint8_t*：每个采样 2 个字节，低字节在前、高字节在后，所以每 2 个字节构成一个 int16 采样
	for (int32_t i = 0; i < m_encInFrameSize; i++) {
		m_encInBuf[i] = m_pcmBuf[2 * i + 1] << 8 | m_pcmBuf[2 * i];
	}    

	// opus 编码
	int32_t outBufSize = yang_opus_encode(
		m_encoder, 
		m_encInBuf, 
		m_pcmFrameSize, 
		m_encOutBuf,	
		MAX_PACKET_SIZE
	);
	
	// 回调编码结果
	if (outBufSize > 0 && cb != NULL) {
		frame->payload = m_encOutBuf;
		frame->nb = outBufSize;

		cb->onAudioData(frame);
	}

	return Yang_Ok;
}
