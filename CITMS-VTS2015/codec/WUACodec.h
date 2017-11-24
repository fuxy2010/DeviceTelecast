// **********************************************************************
// 作者: 傅佑铭
// 版本: 1.0
// 日期: 2011-01 ~ 2011-11
// 修改历史记录: 
// 日期, 作者, 变更内容
// **********************************************************************
#ifndef _AMRNB_CODEC_H_
#define _AMRNB_CODEC_H_

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "GeneralDef.h"
#include "AudioCodec.h"

namespace ScheduleServer
{
	class CWUACodec : public CAudioCodec
	{
	public:
		CWUACodec();
		virtual ~CWUACodec();

		virtual int encode(short* frame, unsigned char* bits);

		virtual int decode(unsigned char* bits, short* frame, int crc = 0);

		unsigned long decode2(unsigned char* bits, unsigned long bits_len, void* frame);

		virtual int calculate_energy(unsigned char* bits, int len);//获取一个编码后音频数据包包能量

	private:
	};
}

#endif  //_AMRNB_CODEC_H_      
