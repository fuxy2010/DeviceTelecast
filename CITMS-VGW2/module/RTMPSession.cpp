// **********************************************************************
// 作者: 傅佑铭
// 版本: 1.0
// 日期: 2011-01 ~ 2011-11
// 修改历史记录: 
// 日期, 作者, 变更内容
// **********************************************************************
#include "stdafx.h"
#include "RTMPSession.h"

using namespace ScheduleServer;

int CRTMPSession::connect(char* url)
{
	_rtmp->Link.timeout = 15;

	if(!RTMP_SetupURL(_rtmp, url))
	{
		printf("\nRTMP_SetupURL() failed!");
		RTMP_Free(_rtmp);
		return -1;
	}
	
	//设置可写,即发布流, 必须在连接前使用,否则无效
	RTMP_EnableWrite(_rtmp);

	if(!RTMP_Connect(_rtmp, NULL))
	{
		printf("\nRTMP_Connect() failed!");
		RTMP_Free(_rtmp);
		return -2;
	} 
	
	if(!RTMP_ConnectStream(_rtmp, 2))
	{
		printf("\nRTMP_ConnectStream() failed!");
		RTMP_Close(_rtmp);
		RTMP_Free(_rtmp);
		return -3;
	}

	if(true)
	{
		RTMPPacket pack;
		RTMPPacket_Alloc(&pack, 4);
		pack.m_packetType = RTMP_PACKET_TYPE_CHUNK_SIZE;
		pack.m_nChannel = 0x02;
		pack.m_headerType = RTMP_PACKET_SIZE_LARGE;
		pack.m_nTimeStamp = 0;
		pack.m_nInfoField2 = 0;
		pack.m_nBodySize = 4;
		int nVal = 655360;//512000;//128000;//65536;//希望指定的大小
		pack.m_body[3] = nVal & 0xff;//大字节序
		pack.m_body[2] = nVal >> 8;
		pack.m_body[1] = nVal >> 16;
		pack.m_body[0] = nVal >> 24;
		_rtmp->m_outChunkSize = nVal;
		//we send packet 1024 bytes
		RTMP_SendPacket(_rtmp,&pack,1);
		RTMPPacket_Free(&pack);
	}

	return 0;
}

int CRTMPSession::input_video_packet(unsigned char* data, int length, unsigned long time_offset)
{
	int offset = 0;
	int ret = 0;

	NAL_TYPE type = get_video_packet_type(data, length, offset);

	if(NAL_INVALID == type)
	{
		return 404;
	}
	else if(NAL_SPS == type)
	{
		_sps_size =  length - offset;
		::memcpy(_sps, data + offset, _sps_size);

	}
	else if(NAL_PPS == type)
	{
		_pps_size = length - offset;
		::memcpy(_pps, data + offset, _pps_size);

		ret = send_video_sps_pps(_sps, _sps_size, _pps, _pps_size);
	}
	else
	{
		ret = send_video_packet(data + offset, length - offset, type, time_offset);
	}

	return ret;
}

int CRTMPSession::disconnect()
{
	if(NULL == _rtmp)
		return -1;

	RTMP_Close(_rtmp);

	return 0;
}

//sps和pps均不包含间隔符00 00 00 01
int CRTMPSession::send_video_sps_pps(unsigned char* sps, int sps_size, unsigned char* pps, int pps_size)
{
	unsigned char* body = NULL;
	int i;

	RTMPPacket* video_packet = (RTMPPacket*)&_video_packet_buf;

	memset(video_packet, 0, RTMP_HEAD_SIZE);

	video_packet->m_body = (char*)video_packet + RTMP_HEAD_SIZE;
	body = (unsigned char*)video_packet->m_body;

	i = 0;
	body[i++] = 0x17;
	body[i++] = 0x00;

	body[i++] = 0x00;
	body[i++] = 0x00;
	body[i++] = 0x00;

	/*AVCDecoderConfigurationRecord*/
	body[i++] = 0x01;
	body[i++] = sps[1];
	body[i++] = sps[2];
	body[i++] = sps[3];
	body[i++] = 0xff;

	/*sps*/
	body[i++]   = 0xe1;
	body[i++] = (sps_size >> 8) & 0xff;
	body[i++] = sps_size & 0xff;
	memcpy(body + i, sps, sps_size);
	i += sps_size;

	/*pps*/
	body[i++]   = 0x01;
	body[i++] = (pps_size >> 8) & 0xff;
	body[i++] = (pps_size) & 0xff;
	memcpy(body + i, pps, pps_size);
	i += pps_size;

	video_packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
	video_packet->m_nBodySize = i;
	video_packet->m_nChannel = 0x04;
	video_packet->m_nTimeStamp = 0;
	video_packet->m_hasAbsTimestamp = 0;
	video_packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
	video_packet->m_nInfoField2 = _rtmp->m_stream_id;

	{
		CSSLocker lock(&_rtmp_mutex);

		if(RTMP_IsConnected(_rtmp))
		{
			int ret = RTMP_SendPacket(_rtmp, video_packet, TRUE);
			return ret;
		}
	}
	
	return 404;
}

int CRTMPSession::send_aac_spec(const unsigned int type, const unsigned int sample_rate, const unsigned int channel_num)
{
	unsigned char* body;
	int len;

	//len = spec_len;//spec data长度一般是

	memset(_audio_packet, 0, RTMP_HEAD_SIZE);

	_audio_packet->m_body = (char*)_audio_packet + RTMP_HEAD_SIZE;
	body = (unsigned char*)_audio_packet->m_body;

	//首4位含义
	//0 = Linear PCM, platform endian
	//1 = ADPCM
	//2 = MP3
	//3 = Linear PCM, little endian
	//4 = Nellymoser 16 kHz mono
	//5 = Nellymoser 8 kHz mono
	//6 = Nellymoser
	//7 = G.711 A-law logarithmic PCM
	//8 = G.711 mu-law logarithmic PCM
	//9 = reserved
	//10 = AAC
	//11 = Speex
	//14 = MP3 8 kHz
	//15 = Device-specific sound
	body[0] = 0xAF;
	body[1] = 0x00;
	get_aac_spec(body + 2, type, sample_rate, channel_num);

	TRACE("\nAAC<%x %x %x %x>", body[0], body[1], body[2], body[3]);

	_audio_packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
	_audio_packet->m_nBodySize = 4;
	_audio_packet->m_nChannel = 0x04;
	_audio_packet->m_nTimeStamp = 0;
	_audio_packet->m_hasAbsTimestamp = 0;
	_audio_packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
	_audio_packet->m_nInfoField2 = _rtmp->m_stream_id;

	{
		CSSLocker lock(&_rtmp_mutex);

		if(RTMP_IsConnected(_rtmp))
		{
			return RTMP_SendPacket(_rtmp, _audio_packet, TRUE);
		}
	}

	return 404;
}

int CRTMPSession::send_video_packet(unsigned char* data, unsigned long length, NAL_TYPE type, unsigned long time_offset)
{
	long timeoffset;
	unsigned char* body = NULL;

	timeoffset = time_offset;//GetTickCount() - start_time;  /*start_time为开始直播时的时间戳*/

	RTMPPacket* video_packet = (RTMPPacket*)&_video_packet_buf;

	//TRACE("\nF<%x %x %x %x> ", data[0], data[1], data[2], data[3]);
	/*去掉帧界定符*/
	/*if(0 == data[2])//00 00 00 01
	{
		data += 4;
		length -= 4;
	}
	else if(1 == data[2])//00 00 01
	{
		data += 3;
		length -= 3;
	}
	//type = data[0] & 0x1f;*/

	//packet = (RTMPPacket *)base_malloc(RTMP_HEAD_SIZE+len+9);

	memset(video_packet, 0, RTMP_HEAD_SIZE);

	video_packet->m_body = (char*)video_packet + RTMP_HEAD_SIZE;

	/*send video packet*/
	body = (unsigned char*)video_packet->m_body;
	memset(body, 0, length + 9);

	/*key frame*/
	body[0] = (type == NAL_SLICE_IDR) ? 0x17 : 0x27;

	body[1] = 0x01;   /*nal unit*/
	body[2] = 0x00;
	body[3] = 0x00;
	body[4] = 0x00;

	body[5] = (length >> 24) & 0xff;
	body[6] = (length >> 16) & 0xff;
	body[7] = (length >>  8) & 0xff;
	body[8] = (length ) & 0xff;

	/*copy data*/
	memcpy(&body[9], data, length);

	video_packet->m_hasAbsTimestamp = 0;
	video_packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
	video_packet->m_nBodySize = length + 9;
	video_packet->m_nInfoField2 = _rtmp->m_stream_id;
	video_packet->m_nChannel = 0x04;
	video_packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
	video_packet->m_nTimeStamp = timeoffset;

	/*调用发送接口*/
	{
		CSSLocker lock(&_rtmp_mutex);

		if(RTMP_IsConnected(_rtmp))
		{
			int ret = RTMP_SendPacket(_rtmp, video_packet, TRUE);
			if(!ret)
				TRACE("\nSE %d", ret);
			//TRACE("\nS %d", ret);

			return ret;
		}
	}

	return 404;
}

int CRTMPSession::input_audio_packet(unsigned char* data, int length, unsigned long time_offset)
{
	unsigned char* body = NULL;

	if(7 > length)
		return -1;

	//对于AAC而言加上界定符(FF F1)每一帧的前7字节是帧的描述信息
	data += 7;
	length += 7;

	memset(_audio_packet, 0, RTMP_HEAD_SIZE);

	_audio_packet->m_body = (char*)_audio_packet + RTMP_HEAD_SIZE;
	body = (unsigned char*)_audio_packet->m_body;

	body[0] = 0xAF;
	body[1] = 0x01;
	memcpy(body + 2, data, length);
	
	_audio_packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
	_audio_packet->m_nBodySize = length + 2;
	_audio_packet->m_nChannel = 0x04;
	_audio_packet->m_nTimeStamp = time_offset;
	_audio_packet->m_hasAbsTimestamp = 0;
	_audio_packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
	_audio_packet->m_nInfoField2 = _rtmp->m_stream_id;

	/*调用发送接口*/
	{
		CSSLocker lock(&_rtmp_mutex);

		if(RTMP_IsConnected(_rtmp))
		{
			return RTMP_SendPacket(_rtmp, _audio_packet, TRUE);
		}
	}

	return 404;
}
