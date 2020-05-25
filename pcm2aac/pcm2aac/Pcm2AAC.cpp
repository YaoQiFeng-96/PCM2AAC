#include "Pcm2AAC.h"

CPcm2AAC::CPcm2AAC() :
	m_pcmSize(0),
	m_sampleRate(0), m_channels(0), m_sampleFmt(AV_SAMPLE_FMT_NONE),
	m_codec(nullptr), m_codecCtx(nullptr), m_packet(nullptr), m_frame(nullptr),
	m_resampleCtx(nullptr), m_pts(0)
{
	memset(m_pcmBuffer, 0, 10240);
	for (auto i = 0; i < 8; i++)
	{
		m_pcmPointer[i] = new char[10240];
		memset(m_pcmPointer[i], 0, 10240);
	}
	memset(m_adtsHead, 0, 7);
}


CPcm2AAC::~CPcm2AAC()
{
	avcodec_send_frame(m_codecCtx, NULL);
	uint8_t *p = nullptr;
	int size = 0;
	while (GetData(p, &size))
	{
	}
	free(p);
	p = nullptr;

	for (auto i = 0; i < 8; i++)
	{
		delete[] m_pcmPointer[i];
		m_pcmPointer[i] = nullptr;
	}
	if (nullptr != m_packet)
	{
		av_packet_free(&m_packet);
		m_packet = nullptr;
	}
	if (nullptr != m_frame)
	{
		av_frame_free(&m_frame);
		m_frame = nullptr;
	}
	if (nullptr != m_codecCtx)
	{
		avcodec_close(m_codecCtx);
		avcodec_free_context(&m_codecCtx);
		m_codecCtx = nullptr;
	}
	if (nullptr != m_resampleCtx)
	{
		swr_free(&m_resampleCtx);
		m_resampleCtx = nullptr;
	}
}

bool CPcm2AAC::Init(int sample_rate, AVSampleFormat sample_fmt, int channels)
{
	m_sampleRate = sample_rate;
	m_sampleFmt = sample_fmt;
	m_channels = channels;

	m_codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
	if (!m_codec)
	{
		fprintf(stderr, "codec not found. \n");
		return false;
	}
	m_codecCtx = avcodec_alloc_context3(m_codec);
	if (!m_codecCtx)
	{
		fprintf(stderr, "could not allocate audio codec context. \n");
		return false;
	}
	m_codecCtx->channels = m_channels;
	m_codecCtx->channel_layout = av_get_default_channel_layout(m_channels);
	m_codecCtx->sample_rate = m_sampleRate;
	m_codecCtx->sample_fmt = AV_SAMPLE_FMT_FLTP;
	m_codecCtx->bit_rate = 64000;
	m_codecCtx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

	if (avcodec_open2(m_codecCtx, m_codec, NULL) < 0)
	{
		fprintf(stderr, "could not open codec. \n");
		return false;
	}
	m_packet = av_packet_alloc();
	if (nullptr == m_packet)
		return false;
	m_frame = av_frame_alloc();
	if (nullptr == m_frame)
		return false;

	m_resampleCtx = swr_alloc_set_opts(NULL, m_codecCtx->channel_layout, m_codecCtx->sample_fmt,
		m_codecCtx->sample_rate, m_codecCtx->channel_layout, m_sampleFmt, m_codecCtx->sample_rate, 0, NULL);
	if (nullptr == m_resampleCtx)
	{
		fprintf(stderr, "coult not allocate resample context. \n");
		return false;
	}
	if (swr_init(m_resampleCtx) < 0)
	{
		fprintf(stderr, "could not open resample context. \n");
		return false;
	}
	return true;
}

void CPcm2AAC::AddData(uint8_t * pData, int iSize)
{
	if (iSize > 10240)
	{
		fprintf(stderr, "add buff too bit. \n");
		return;
	}
	if (iSize + m_pcmSize > 10240)
	{
		memset(m_pcmBuffer, 0, 10240);
		m_pcmSize = 0;
	}
	memcpy(m_pcmBuffer + m_pcmSize, pData, iSize);
	m_pcmSize += iSize;

	int data_size = av_get_bytes_per_sample(m_sampleFmt);
	if (m_pcmSize < data_size * 1024 * m_channels)
	{
		return;
	}
	memcpy(m_pcmPointer[0], m_pcmBuffer, data_size * 1024 * m_channels);
	m_pcmSize -= data_size * 1024 * m_channels;
	memcpy(m_pcmBuffer, m_pcmBuffer + data_size * 1024 * m_channels, m_pcmSize);

	m_frame->pts = m_pts;
	m_pts += 1024;
	m_frame->nb_samples = 1024;
	m_frame->format = m_codecCtx->sample_fmt;
	m_frame->channel_layout = m_codecCtx->channel_layout;
	m_frame->sample_rate = m_codecCtx->sample_rate;
	if (av_frame_get_buffer(m_frame, 0) < 0)
	{
		fprintf(stderr, "could not allocate audio data buffer. \n");
		return;
	}
	
	if (swr_convert(m_resampleCtx, m_frame->extended_data, m_frame->nb_samples, (const uint8_t **)m_pcmPointer, 1024) < 0)
	{
		fprintf(stderr, "could not convert input samples. \n");
		if (nullptr != m_frame)
		{
			av_frame_unref(m_frame);
		}
		return;
	}
	int ret = avcodec_send_frame(m_codecCtx, m_frame);
	if (ret < 0)
	{
		fprintf(stderr, "avcodec_send_frame() error. \n");
		if (nullptr != m_frame)
		{
			av_frame_unref(m_frame);
		}
		return;
	}
	av_frame_unref(m_frame);
}

bool CPcm2AAC::GetData(uint8_t *& pData, int * iSize)
{
	int ret = avcodec_receive_packet(m_codecCtx, m_packet);
	if (ret < 0)
	{
		//fprintf(stderr, "avcodec_receive_packet() error. \n");
		return false;
	}
	AddADTS(m_packet->size + 7);
	pData = (uint8_t *)malloc(sizeof(uint8_t)*(m_packet->size + 7));
	memcpy(pData, m_adtsHead, 7);
	memcpy(pData + 7, m_packet->data, m_packet->size);
	*iSize = m_packet->size + 7;
	av_packet_unref(m_packet);
	return true;
}

void CPcm2AAC::AddADTS(int packetLen)
{
	memset(m_adtsHead, 0, 7);
	int profile = 1; // AAC LC  
	int freqIdx = 0xb; // 44.1KHz  
	int chanCfg = m_channels; // CPE  

	if (96000 == m_sampleRate)
	{
		freqIdx = 0x00;
	}
	else if (88200 == m_sampleRate)
	{
		freqIdx = 0x01;
	}
	else if (64000 == m_sampleRate)
	{
		freqIdx = 0x02;
	}
	else if (48000 == m_sampleRate)
	{
		freqIdx = 0x03;
	}
	else if (44100 == m_sampleRate)
	{
		freqIdx = 0x04;
	}
	else if (32000 == m_sampleRate)
	{
		freqIdx = 0x05;
	}
	else if (24000 == m_sampleRate)
	{
		freqIdx = 0x06;
	}
	else if (22050 == m_sampleRate)
	{
		freqIdx = 0x07;
	}
	else if (16000 == m_sampleRate)
	{
		freqIdx = 0x08;
	}
	else if (12000 == m_sampleRate)
	{
		freqIdx = 0x09;
	}
	else if (11025 == m_sampleRate)
	{
		freqIdx = 0x0a;
	}
	else if (8000 == m_sampleRate)
	{
		freqIdx = 0x0b;
	}
	else if (7350 == m_sampleRate)
	{
		freqIdx = 0xc;
	}
	// fill in ADTS data  
	m_adtsHead[0] = 0xFF;
	m_adtsHead[1] = 0xF1;
	m_adtsHead[2] = ((profile) << 6) + (freqIdx << 2) + (chanCfg >> 2);
	m_adtsHead[3] = (((chanCfg & 3) << 6) + (packetLen >> 11));
	m_adtsHead[4] = ((packetLen & 0x7FF) >> 3);
	m_adtsHead[5] = (((packetLen & 7) << 5) + 0x1F);
	m_adtsHead[6] = 0xFC;
}
