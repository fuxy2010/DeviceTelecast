#include "Sizescale.h"

void CSizescale::vs_scanline_resample_nearest_Y (unsigned char * dest, unsigned char * src, int src_width, int n, int *accumulator, int increment)
{
	int acc = *accumulator;
	int i;
	int j;
	int x;

	for (i = 0; i < n; i++) {
		j = acc >> 16;
		x = acc & 0xffff;
		dest[i] = (x < 32768 || j + 1 >= src_width) ? src[j] : src[j + 1];

		acc += increment;
	}

	*accumulator = acc;
}

void CSizescale::vs_scanline_resample_linear_Y (unsigned char * dest, unsigned char * src, int src_width, int n, int *accumulator, int increment)
{
	int acc = *accumulator;
	int i;
	int j;
	int x;

	for (i = 0; i < n; i++) 
	{
		j = acc >> 16;
		x = acc & 0xffff;

		if (j + 1 < src_width)
			dest[i] = (src[j] * (65536 - x) + src[j + 1] * x) >> 16;
		else
			dest[i] = src[j];

		acc += increment;
	}

	*accumulator = acc;
}

void CSizescale::orc_merge_linear_u8 (unsigned char * d1, const unsigned char * s1, const unsigned char * s2, int p1, int p2, int n)
{
	int i;
	char var0;
	char *ptr0;
	char var4;
	const char *ptr4;
	char var5;
	const char *ptr5;
	const short var16 = 128;
	const short var17 = 8;
	const int var24 = p1;
	const int var25 = p2;
	short var32;
	short var33;
	short var34;
	short var35;
	short var36;

	ptr0 = (char *) d1;
	ptr4 = (char *) s1;
	ptr5 = (char *) s2;

	for (i = 0; i < n; i++) 
	{
		var4 = *ptr4;
		ptr4++;
		var5 = *ptr5;
		ptr5++;
		/* 0: mulubw */
		var32 = (unsigned char) var4 *(unsigned char) var24;
		/* 1: mulubw */
		var33 = (unsigned char) var5 *(unsigned char) var25;
		/* 2: addw */
		var34 = var32 + var33;
		/* 3: addw */
		var35 = var34 + var16;
		/* 4: shruw */
		var36 = ((unsigned short) var35) >> var17;
		/* 5: convwb */
		var0 = var36;
		*ptr0 = var0;
		ptr0++;
	}
}

void CSizescale::vs_scanline_merge_linear_Y (unsigned char * dest, unsigned char * src1, unsigned char * src2, int n, int x)
{
	unsigned int value = x >> 8;

	if (value == 0)
	{
		memcpy (dest, src1, n);
	}
	else 
	{
		orc_merge_linear_u8 (dest, src1, src2, 256 - value, value, n);
	}
}

void CSizescale::vs_image_scale_nearest_Y (const VSImage * dest, const VSImage * src,  unsigned char * tmpbuf)
{
	int acc;
	int y_increment;
	int x_increment;
	int i;
	int j;
	int xacc;

	if (dest->height == 1)
		y_increment = 0;
	else
		y_increment = ((src->height - 1) << 16) / (dest->height - 1);

	if (dest->width == 1)
		x_increment = 0;
	else
		x_increment = ((src->width - 1) << 16) / (dest->width - 1);

	acc = 0;
	for (i = 0; i < dest->height; i++) 
	{
		j = acc >> 16;

		xacc = 0;
		vs_scanline_resample_nearest_Y (dest->pixels + i * dest->stride,
			src->pixels + j * src->stride, src->width, dest->width, &xacc,
			x_increment);

		acc += y_increment;
	}
}

void CSizescale::vs_image_scale_linear_Y (const VSImage * dest, const VSImage * src, unsigned char * tmpbuf)
{
	int acc;
	int y_increment;
	int x_increment;
	unsigned char *tmp1;
	unsigned char *tmp2;
	int y1;
	int y2;
	int i;
	int j;
	int x;
	int dest_size;
	int xacc;

	if (dest->height == 1)
		y_increment = 0;
	else
		y_increment = ((src->height - 1) << 16) / (dest->height - 1);

	if (dest->width == 1)
		x_increment = 0;
	else
		x_increment = ((src->width - 1) << 16) / (dest->width - 1);

	dest_size = dest->width;

	tmp1 = tmpbuf;
	tmp2 = tmpbuf + dest_size;

	acc = 0;
	xacc = 0;
	y2 = -1;
	vs_scanline_resample_linear_Y (tmp1, src->pixels, src->width, dest->width, &xacc, x_increment);
	y1 = 0;
	for (i = 0; i < dest->height; i++) 
	{
		j = acc >> 16;
		x = acc & 0xffff;

		if (x == 0) 
		{
			if (j == y1) 
			{
				memcpy (dest->pixels + i * dest->stride, tmp1, dest_size);
			} else if (j == y2) 
			{
				memcpy (dest->pixels + i * dest->stride, tmp2, dest_size);
			}
			else 
			{
				xacc = 0;
				vs_scanline_resample_linear_Y (tmp1, src->pixels + j * src->stride, src->width, dest->width, &xacc, x_increment);
				y1 = j;
				memcpy (dest->pixels + i * dest->stride, tmp1, dest_size);
			}
		}
		else 
		{
			if (j == y1) 
			{
				if (j + 1 != y2) 
				{
					xacc = 0;
					vs_scanline_resample_linear_Y (tmp2, src->pixels + (j + 1) * src->stride, src->width, dest->width, &xacc, x_increment);
					y2 = j + 1;
				}
				vs_scanline_merge_linear_Y (dest->pixels + i * dest->stride, tmp1, tmp2, dest->width, x);
			} 
			else if (j == y2) 
			{
				if (j + 1 != y1) 
				{
					xacc = 0;
					vs_scanline_resample_linear_Y (tmp1,
						src->pixels + (j + 1) * src->stride, src->width, dest->width,
						&xacc, x_increment);
					y1 = j + 1;
				}
				vs_scanline_merge_linear_Y (dest->pixels + i * dest->stride,
					tmp2, tmp1, dest->width, x);
			} else {
				xacc = 0;
				vs_scanline_resample_linear_Y (tmp1, src->pixels + j * src->stride,
					src->width, dest->width, &xacc, x_increment);
				y1 = j;
				xacc = 0;
				vs_scanline_resample_linear_Y (tmp2,
					src->pixels + (j + 1) * src->stride, src->width, dest->width, &xacc,
					x_increment);
				y2 = (j + 1);
				vs_scanline_merge_linear_Y (dest->pixels + i * dest->stride,
					tmp1, tmp2, dest->width, x);
			}
		}

		acc += y_increment;
	}
}

void CSizescale::scale(unsigned char* dest, int dest_w, int dest_h, unsigned char* src, int src_w, int src_h)
{
	VSImage dest_y;
	VSImage dest_u;
	VSImage dest_v;
	VSImage src_y;
	VSImage src_u;
	VSImage src_v;

	unsigned char *tmp_buf = (unsigned char *)malloc(dest_w*4*2);
	if(tmp_buf == NULL) return;

	dest_y.pixels = dest;
	dest_y.width = dest_w;
	dest_y.height = dest_h;
	dest_y.stride = dest_w;

	dest_u.pixels = dest + dest_w*dest_h;
	dest_u.width = dest_w/2;
	dest_u.height = dest_h/2;
	dest_u.stride = dest_w/2;

	dest_v.pixels = dest + dest_w*dest_h*5/4;
	dest_v.width = dest_w/2;
	dest_v.height = dest_h/2;
	dest_v.stride = dest_w/2;

	src_y.pixels = src;
	src_y.width = src_w;
	src_y.height = src_h;
	src_y.stride = src_w;

	src_u.pixels = src + src_w*src_h;
	src_u.width = src_w/2;
	src_u.height = src_h/2;
	src_u.stride = src_w/2;

	src_v.pixels = src + src_w*src_h*5/4;
	src_v.width = src_w/2;
	src_v.height = src_h/2;
	src_v.stride = src_w/2;

	vs_image_scale_nearest_Y (&dest_y, &src_y, tmp_buf);
	vs_image_scale_nearest_Y (&dest_u, &src_u, tmp_buf);
	vs_image_scale_nearest_Y (&dest_v, &src_v, tmp_buf);
	
	//vs_image_scale_linear_Y (&dest_y, &src_y, tmp_buf);
	//vs_image_scale_linear_Y (&dest_u, &src_u, tmp_buf);
	//vs_image_scale_linear_Y (&dest_v, &src_v, tmp_buf);
	
	free(tmp_buf);
}

