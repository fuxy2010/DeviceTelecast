#ifndef _TRANSCODING_H_
#define _TRANSCODING_H_

class CTranscoding
{
public:
	CTranscoding();
	~CTranscoding();

public:
	static void matrxCovert(int n,unsigned char *data);
	static void initLookupTable();
	static void RGBtoYUV(unsigned long w,unsigned long h,unsigned char * rgbData,unsigned char * y,unsigned char * u,unsigned char *v);
	static void YUY2toYUV(unsigned char *data,int width,int height,unsigned char *yuv_buf);
};

#endif