#include "stdafx.h"
#include "Transcoding.h"

float RGBYUV02990[256],RGBYUV05870[256],RGBYUV01140[256];
float RGBYUV01684[256],RGBYUV03316[256];
float RGBYUV04187[256],RGBYUV00813[256];

CTranscoding::CTranscoding()
{
	//...
}

CTranscoding::~CTranscoding()
{
	//...
}

void CTranscoding::matrxCovert(int n,unsigned char *data)
{
	unsigned char temp;
	for (int i=0;i<n/2;i++)
	{
		temp=*(data+i);
		*(data+i)=*(data+n-i-1);
		*(data+n-i-1)=temp;
	}
}

void CTranscoding::initLookupTable()
{
	for (int i=0;i<256;i++)
	{
		RGBYUV02990[i] = (float)0.2990 * i;
		RGBYUV05870[i] = (float)0.5870 * i;
		RGBYUV01140[i] = (float)0.1140 * i;
		RGBYUV01684[i] = (float)0.1684 * i;
		RGBYUV03316[i] = (float)0.3316 * i;
		RGBYUV04187[i] = (float)0.4187 * i;
		RGBYUV00813[i] = (float)0.0813 * i;
	}
}

void CTranscoding::RGBtoYUV(unsigned long w,unsigned long h,unsigned char * rgbData,unsigned char * y,unsigned char * u,unsigned char *v)
{
	initLookupTable();//��ʼ�����ұ�
	unsigned char *ytemp = NULL;
	unsigned char *utemp = NULL;
	unsigned char *vtemp = NULL;
	utemp = (unsigned char *)malloc(w*h);
	vtemp = (unsigned char *)malloc(w*h);

	unsigned long i,nr,ng,nb,nSize;
	//��ÿ�����ؽ��� rgb -> yuv��ת��
	for (i=0,nSize=0;nSize<w*h*3;nSize+=3)
	{
		nb = rgbData[nSize];
		ng = rgbData[nSize+1];
		nr = rgbData[nSize+2];
		y[i] = (unsigned char)(RGBYUV02990[nr]+RGBYUV05870[ng]+RGBYUV01140[nb]);
		utemp[i] = (unsigned char)(-RGBYUV01684[nr]-RGBYUV03316[ng]+nb/2+128);
		vtemp[i] = (unsigned char)(nr/2-RGBYUV04187[ng]-RGBYUV00813[nb]+128);
		i++;
	}
	//��u�źż�v�źŽ��в���
	int k = 0;
	for (i=0;i<h;i+=2)
		for(unsigned long j=0;j<w;j+=2)
		{
			u[k]=(utemp[i*w+j]+utemp[(i+1)*w+j]+utemp[i*w+j+1]+utemp[(i+1)*w+j+1])/4;
			v[k]=(vtemp[i*w+j]+vtemp[(i+1)*w+j]+vtemp[i*w+j+1]+vtemp[(i+1)*w+j+1])/4;
			k++;
		}
		//��y��u��v �źŽ��п��봦��
		for (i=0;i<w*h;i++)
		{
			if(y[i]<16)
				y[i] = 16;
			if(y[i]>235)
				y[i] = 235;
		}
		for(i=0;i<h*w/4;i++)
		{
			if(u[i]<16)
				u[i] = 16;
			if(v[i]<16)
				v[i] = 16;
			if(u[i]>240)
				u[i] = 240;
			if(v[i]>240)
				v[i] = 240;
		}
		if(utemp)
			free(utemp);
		if(vtemp)
			free(vtemp);
}

void CTranscoding::YUY2toYUV(unsigned char *data,int width,int height,unsigned char *yuv_buf)