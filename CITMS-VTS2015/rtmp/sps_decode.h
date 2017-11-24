#ifndef __SPS_DECODE_H__
#define __SPS_DECODE_H__

#include <stdio.h>
#include <stdint.h>
#include <math.h>

typedef  unsigned int UINT;
typedef  unsigned char BYTE;
typedef  unsigned long DWORD;

#ifdef __cplusplus
extern "C"
{
#endif

UINT Ue(BYTE *pBuff, UINT nLen, UINT &nStartBit);

int Se(BYTE *pBuff, UINT nLen, UINT &nStartBit);

DWORD u(UINT BitCount, BYTE * buf, UINT &nStartBit);

void de_emulation_prevention(BYTE* buf, unsigned int* buf_size);

int h264_decode_sps(unsigned char * buf, unsigned int nLen, int& width, int& height, int& fps);

#ifdef __cplusplus
};
#endif

#endif