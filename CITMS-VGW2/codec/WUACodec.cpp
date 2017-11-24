// **********************************************************************
// 作者: 傅佑铭
// 版本: 1.0
// 日期: 2011-01 ~ 2011-11
// 修改历史记录: 
// 日期, 作者, 变更内容
// **********************************************************************
#include "WUACodec.h"
#include "TimeConsuming.h"

using namespace ScheduleServer;

CWUACodec::CWUACodec()
{
}

CWUACodec::~CWUACodec()
{
}

int CWUACodec::encode(short* frame, unsigned char* bits)
{
	//CTimeConsuming tc('E', 1.0);
	CSSLocker lock(&_encode_mutext);

	return 0;
}

int CWUACodec::decode(unsigned char* bits, short* frame, int crc)
{
	CSSLocker lock(&_decode_mutext);

	return 0;
}

unsigned long CWUACodec::decode2(unsigned char* bits, unsigned long bits_len, void* frame)
{
	//CTimeConsuming tc('C', 1.0);

	CSSLocker lock(&_decode_mutext);
	
	return 0;
}

int CWUACodec::calculate_energy(unsigned char* bits, int len)
{
	return 0;
}
