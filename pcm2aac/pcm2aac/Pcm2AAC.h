#pragma once

#include "headers.h"

class CPcm2AAC
{
public:
	CPcm2AAC();
	virtual ~CPcm2AAC();

	bool Init(int sample_rate, AVSampleFormat sample_fmt, int channels);
	void AddData(uint8_t *pData, int iSize);
	bool GetData(uint8_t *&pData, int *iSize);

private:
	void AddADTS(int packetLen);

private:
	char				m_pcmBuffer[102410];
	int					m_pcmSize;
	char				*m_pcmPointer[8];

	int					m_sampleRate;
	int					m_channels;
	AVSampleFormat		m_sampleFmt;

	AVCodec				*m_codec;
	AVCodecContext		*m_codecCtx;
	AVPacket			*m_packet;
	AVFrame				*m_frame;
	SwrContext			*m_resampleCtx;
	int64_t				m_pts;
	uint8_t				m_adtsHead[7];
};

