// **********************************************************************
// 作者: 傅佑铭
// 版本: 1.0
// 日期: 2011-01 ~ 2011-11
// 修改历史记录: 
// 日期, 作者, 变更内容
// **********************************************************************
#ifndef _AUDIO_CODEC_H_
#define _AUDIO_CODEC_H_

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "GeneralDef.h"
#include "MediaData.h"
#include "Locker.h"

#define PKT_SAMPLE_NUM	(AUDIO_FRAME_LENGTH * AUDIO_FRAME_PER_PACKET)
#define PKT_FRAME_NUM	AUDIO_FRAME_PER_PACKET

namespace ScheduleServer
{
	//任务基类
	class CAudioCodec
	{

	public:
		CAudioCodec() : _encoder(NULL), _decoder(NULL) {};
		virtual ~CAudioCodec() {};

		virtual int encode(short* frame, unsigned char* bits) = 0;//编码三帧原始语音，返回编码后数据字节长度

		virtual int decode(unsigned char* bits, short* frame, int crc = 0) = 0;//解码生成三帧原始语音，返回用掉码流的长度

		virtual int calculate_energy(unsigned char* bits, int len) = 0;//获取一个编码后音频数据包包能量,len 以字节为单位

		
		//混音一路
		//mix_buffer-- 混音buffer，混第一路时应该清零
		//frame－－待混音话音
		//frame_len－－－话音长度，以字节为单位
		//fscale－－加权参数,依路数变化，默认1.0
		//energy---输入语音能量,可选,默认0
		static int mix(short* mix_buffer, short *frame, int frame_len, double fscale, int energy)
		{
#if 1
			return 0;//mix_audio(mix_buffer, frame, frame_len * sizeof(char) / sizeof(short), fscale, energy);
#else
			//::memcpy(mix_buffer, frame, frame_len);
			//return frame_len;
#endif
		}


	protected:
		void* _encoder;
		void* _decoder;

	protected:
		CSSMutex _encode_mutext;//编码互斥量
		CSSMutex _decode_mutext;//解码互斥量

	};
}

#endif  // _AUDIO_CODEC_H_      
