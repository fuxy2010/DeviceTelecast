#ifndef _RTMP_SESSION_H_
#define _RTMP_SESSION_H_

/*#ifdef WIN32
#include <windows.h>
#pragma comment(lib,"WS2_32.lib")
#pragma comment(lib,"winmm.lib")
#endif*/

#include "stdafx.h"

//#pragma comment(lib, "../rtmp/lib/librtmp.lib")

//#include "librtmp\rtmp.h"
//#include "librtmp\rtmp_sys.h"
//#include "librtmp\amf.h"

#include "rtmp.h"
#include "rtmp_sys.h"
#include "amf.h"

#include "Locker.h"

#define RTMP_HEAD_SIZE   (sizeof(RTMPPacket) + RTMP_MAX_HEADER_SIZE)

namespace ScheduleServer
{
	typedef enum
	{
		NAL_SLICE = 1,
		NAL_SLICE_DPA = 2,
		NAL_SLICE_DPB = 3,
		NAL_SLICE_DPC = 4,
		NAL_SLICE_IDR = 5,
		NAL_SEI = 6,
		NAL_SPS = 7,
		NAL_PPS = 8,
		NAL_AUD = 9,
		NAL_FILLER = 12,
		NAL_INVALID = -1
	}
	NAL_TYPE;

	typedef struct tagAudioSpecificConfig
	{
		//little endian
		unsigned char type:5;//编码结构类型，AAC main编码为1，LOW低复杂度编码为2，SSR为3
		unsigned char sample_rate:4;//采样率,通常aac固定选中44100，即应该对应为4，但试验表明，当音频采样率小于等于44100时，应该选择3，而当音频采样率为48000时，应该选择2
		unsigned char channel_num:4;//声道数
		unsigned char tail:3;//最后3位固定为0
	}
	AudioSpecificConfig, *AudioSpecificConfigPtr;//fym

	class CRTMPSession
	{
	public:
		CRTMPSession() :
		_rtmp(NULL),
		//_video_packet(NULL),
		_audio_packet(NULL)
		{
			::memset(_sps, 0, sizeof(_sps));
			_sps_size = 0;
			::memset(_pps, 0, sizeof(_pps));
			_pps_size = 0;
			
			init();

			//_video_packet = (RTMPPacket*)malloc(RTMP_HEAD_SIZE + 51200);
			//_audio_packet = (RTMPPacket*)malloc(RTMP_HEAD_SIZE + 2048);

			//_video_packet = (RTMPPacket*)&_video_packet_buf;
			_audio_packet = (RTMPPacket*)&_audio_packet_buf;
		}

		virtual ~CRTMPSession()
		{
			uninit();

			//free(_video_packet);
			//free(_audio_packet);
		}

	public:
		static NAL_TYPE find_frame_header4(unsigned char* data, int length, int& offset)
		{
			if(5 > length)
				return NAL_INVALID;

			for(int i = 0; i < length - 3; ++i)
			{
				if(0 == data[i] && 0 == data[i + 1] && 0 == data[i + 2] && 1 == data[i + 3])
				{
					offset = i;
					return (NAL_TYPE)(data[i + 4] & 0x1f);
				}
			}

			return NAL_INVALID;
		}

		static NAL_TYPE find_frame_header3(unsigned char* data, int length, int& offset)
		{
			if(4 > length)
				return NAL_INVALID;

			for(int i = 0; i < length - 3; ++i)
			{
				if(0 == data[i] && 0 == data[i + 1] && 1 == data[i + 2])
				{
					offset = i;
					return (NAL_TYPE)(data[i + 3] & 0x1f);
				}
			}

			return NAL_INVALID;
		}

	public:
		static NAL_TYPE get_video_packet_type(unsigned char* data, int length, int& offset)
		{
			offset = 0;

			if(3 > length)
				return NAL_INVALID;

			if(0 != data[0] || 0 != data[1])
				return NAL_INVALID;

			if(1 == data[2])
			{
				offset = 3;
			}
			else if(0 == data[2] && 1 == data[3])
			{
				offset = 4;
			}
			else
			{
				return NAL_INVALID;
			}

			return (NAL_TYPE)(data[offset] & 0x1f);
		}

		static int get_aac_spec(unsigned char* data, const unsigned int type, const unsigned int sample_rate, const unsigned int channel_num)
		{
			unsigned long dw_byte = type;
			dw_byte <<= 11;

			unsigned long dw_sample_rate = 0;
			if(44100 > sample_rate)
				dw_sample_rate = 3;
			else if(44100 == sample_rate)
				dw_sample_rate = 4;
			else if(48000 == sample_rate)
				dw_sample_rate = 2;
			dw_sample_rate <<= 7;

			unsigned long dw_channel_num = channel_num;
			dw_channel_num <<= 3;

			unsigned long dw_spec = dw_byte | dw_sample_rate | dw_channel_num;

			data[0] = (unsigned char)(dw_spec >> 8);
			data[1] = 0xff & dw_spec;

			return 0;
		}

	public:
		int connect(char* url);
		int disconnect();

	public:
		int input_video_packet(unsigned char* data, int length, unsigned long time_offset);
		int input_audio_packet(unsigned char* data, int length, unsigned long time_offset);

	private://public://private:
		int send_video_sps_pps(unsigned char* sps, int sps_size, unsigned char* pps, int pps_size);
	public:
		int send_aac_spec(const unsigned int type, const unsigned int sample_rate, const unsigned int channel_num);

	private:
		void init()
		{
			uninit();

			_rtmp = RTMP_Alloc();
			RTMP_Init(_rtmp);
		}

		void uninit()
		{
			if(NULL != _rtmp)
			{
				RTMP_Close(_rtmp);
				RTMP_Free(_rtmp);
				_rtmp = NULL;
			}
		}

	private:
		int send_video_packet(unsigned char* data, unsigned long size, NAL_TYPE type, unsigned long time_offset);

	private:
		RTMP* _rtmp;

		unsigned char _video_packet_buf[65535 * 10];
		//RTMPPacket* _video_packet;
		unsigned char _sps[128];
		int _sps_size;
		unsigned char _pps[128];
		int _pps_size;

		unsigned char _audio_packet_buf[2048];
		RTMPPacket* _audio_packet;

	private:
		CSSMutex _rtmp_mutex;//编码互斥量
	};
}

#endif
