#ifndef _SIZE_SCALE_H_H_
#define _SIZE_SCALE_H_H_

#include <stdio.h>
#include <stdlib.h>
#include <cstring>


typedef struct 
{
	unsigned char *pixels;
	int width;
	int height;
	int stride;
}VSImage;

class CSizescale
{
public:
	static void vs_scanline_resample_nearest_Y (unsigned char * dest, unsigned char * src, int src_width, int n, int *accumulator, int increment);
	static void vs_scanline_resample_linear_Y (unsigned char * dest, unsigned char * src, int src_width, int n, int *accumulator, int increment);
	static void orc_merge_linear_u8 (unsigned char * d1, const unsigned char * s1, const unsigned char * s2, int p1, int p2, int n);
	static void vs_scanline_merge_linear_Y (unsigned char * dest, unsigned char * src1, unsigned char * src2, int n, int x);
	static void vs_image_scale_nearest_Y (const VSImage * dest, const VSImage * src,  unsigned char * tmpbuf);
	static void vs_image_scale_linear_Y (const VSImage * dest, const VSImage * src, unsigned char * tmpbuf);
	static void scale(unsigned char* dest, int dest_w, int dest_h, unsigned char* src, int src_w, int src_h);
};




#endif
