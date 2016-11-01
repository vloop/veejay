/* 
 * Linux VeeJay
 *
 * Copyright(C)2002 Niels Elburg <nwelburg@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License , or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307 , USA.
 */

#include "common.h"
#include <libvjmem/vjmem.h>
#include "kaleidoscope.h"

vj_effect *kaleidoscope_init(int w, int h)
{

	vj_effect *ve = (vj_effect *) vj_calloc(sizeof(vj_effect));
	ve->num_params = 1;
	ve->defaults = (int *) vj_calloc(sizeof(int) * ve->num_params);	/* default values */
	ve->limits[0] = (int *) vj_calloc(sizeof(int) * ve->num_params);	/* min */
	ve->limits[1] = (int *) vj_calloc(sizeof(int) * ve->num_params);	/* max */
	ve->defaults[0] = 0;
	ve->limits[0][0] = 0;	/* horizontal or vertical mirror */
	ve->limits[1][0] = 0;
	ve->sub_format = 1;
	ve->description = "Kaleidoscope !";
	ve->extra_frame = 0;
	ve->has_user = 0;
	ve->param_description = vje_build_param_list(ve->num_params, "H or V mode");
	return ve;
}

static void kaleidoscope(uint8_t * yuv[3], int width, int height)
{
///////////////////////////////////////////////////////////////////////////////
  // debugging functions to display digits - should be moved
	int setsegments (const int segments, const int x0, const int y0, const int seglen, const int Y, const int Cb, const int Cr){
	  // 0..5 (abcdef) clockwise from top, 6 (g) dash, 7 dot
	  // if (segnum>7) return(1);
	  unsigned int x, y, yi;
	    if(segments & 1) { // a: horizontal, top
	      yi=(y0-2*seglen)*width;
	      for (x=x0; x<x0+seglen; x++){
		yuv[0][yi+x] = Y;
		yuv[1][yi+x] = Cb;
		yuv[2][yi+x] = Cr;
	      }
	    }
	    if(segments & 2) { // b: vertical, top right
	      x=x0+seglen;
	      for(y=y0-seglen; y>y0-seglen*2; y--){
		yi=y*width;
		yuv[0][yi+x] = Y;
		yuv[1][yi+x] = Cb;
		yuv[2][yi+x] = Cr;
	      }
	    }
	    if(segments & 4) { // c: vertical, bottom right
	      x=x0+seglen;
	      for(y=y0; y>y0-seglen; y--){
		yi=y*width;
		yuv[0][yi+x] = Y;
		yuv[1][yi+x] = Cb;
		yuv[2][yi+x] = Cr;
	      }
	    }
	    if(segments & 8) { // d: horizontal, bottom
	      yi=y0*width;
	      for (x=x0; x<x0+seglen; x++){
		yuv[0][yi+x] = Y;
		yuv[1][yi+x] = Cb;
		yuv[2][yi+x] = Cr;
	      }
	    }
	    if(segments & 16) { // e: vertical, bottom left
	      x=x0;
	      for(y=y0; y>y0-seglen; y--){
		yi=y*width;
		yuv[0][yi+x] = Y;
		yuv[1][yi+x] = Cb;
		yuv[2][yi+x] = Cr;
	      }
	    }
	    if(segments & 32) { // f: vertical, top left
	      x=x0;
	      for(y=y0-seglen; y>y0-seglen*2; y--){
		yi=y*width;
		yuv[0][yi+x] = Y;
		yuv[1][yi+x] = Cb;
		yuv[2][yi+x] = Cr;
	      }
	    }
	    if(segments & 64) { // g: horizontal, middle
	      yi=(y0-seglen)*width;
	      for (x=x0; x<x0+seglen; x++){
		yuv[0][yi+x] = Y;
		yuv[1][yi+x] = Cb;
		yuv[2][yi+x] = Cr;
	      }
	    }
	    if(segments & 128) { // dot bottom right
	      yi=(y0+2)*width;
	      x=x0+seglen+2;
	      yuv[0][yi+x] = Y;
	      yuv[1][yi+x] = Cb;
	      yuv[2][yi+x] = Cr;
	    }
	    return(0);
	}
	
	int showdigit (const int digit, const int x0, const int y0, const int seglen, const int Y, const int Cb, const int Cr){
	  int gfedcba[16] = { 0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71 };
	  setsegments(gfedcba[digit & 0x0F], x0, y0, seglen, Y, Cb, Cr);
          return(0);
	}
	// showdigit(3, x0, y0, 10, 255, 0, 0);

	int showhexdigits (const int digits, const int x0, const int y0, const int seglen, const int Y, const int Cb, const int Cr){
	  showdigit(digits & 0x0F, x0+(seglen+5)*3, y0, seglen, Y, Cb, Cr);
	  showdigit((digits >> 4) & 0x0F, x0+(seglen+5)*2, y0, seglen, Y, Cb, Cr);
	  showdigit((digits >> 8) & 0x0F, x0+(seglen+5), y0, seglen, Y, Cb, Cr);
	  showdigit((digits >> 12) & 0x0F, x0, y0, seglen, Y, Cb, Cr);
	  return(0);
	}
	// const unsigned int seglen=12;
	// showhexdigits(0x0123, xc, yc, seglen, 255, 0, 0);
	// showhexdigits(0x0123, xc+1, yc+1, seglen, 255, 0, 0);
	// showhexdigits(0x4567, xc, yc-3*seglen, seglen, 255, 0, 0);
	// showhexdigits(0x89AB, xc, yc-6*seglen, seglen, 255, 0, 0);
	// showhexdigits(0xCDEF, xc, yc-9*seglen, seglen, 255, 0, 0);

///////////////////////////////////////////////////////////////////////////////
        int with_border=1;
	int x, xmin, xmax, y, yr;
        // x absolute
        // yr relative to triangle base 
	unsigned int yi, yi2; // offset dans le tableau
	uint8_t cb, cr;
	const float sin60 = sin(M_PI/3);
	// const float cos60 = cos(M_PI/3);
	// const float tan30 = tan(M_PI/6);
	const float sin2a = sin(2*M_PI/3);
	const float cos2a = cos(2*M_PI/3);
		
        // centre of the picture:
        const unsigned int yc = height/2;
	const unsigned int xc = width/2;
        // Width and height of the triangle
	const unsigned int wt = width/2;
	const unsigned int ht = wt*sin60;
        // x changes by half the triangle width while y moves across the triangle height
        const float xpente=0.5*wt/ht;
        // todo truncate if triangle is bigger than picture area
        // x0, y0 top left angle of the triangle
        const unsigned int y0=yc-ht/2;
        const unsigned int x0=xc-wt/2;

	const unsigned int seglen=12, lineheigth=3*seglen;
	x=xc-3*seglen;
	y=yc+lineheigth;
	showhexdigits(0x0123, x, y, seglen, 255, 0, 0);
	showhexdigits(0x0123, x+1, y+1, seglen, 255, 0, 0);
	y-=lineheigth;
	showhexdigits(0x4567, x, y, seglen, 255, 255, 0);
	showhexdigits(0x4567, x+1, y+1, seglen, 255, 255, 0);
	y-=lineheigth;
	showhexdigits(0x89AB, x, y, seglen, 255, 0, 255);
	showhexdigits(0x89AB, x+1, y+1, seglen, 255, 0, 255);
	y-=lineheigth;
	showhexdigits(0xCDEF, x, y, seglen, 255, 255, 255);
	showhexdigits(0xCDEF, x+1, y+1, seglen, 255, 255, 255);

	// center triangle - draw sides
	if (with_border){
	  for (yr = 0; yr < ht; yr++) {
		  yi = (y0+yr) * width;
		  xmin=x0+yr*xpente;
		  xmax=x0+wt-yr*xpente;
		  yuv[0][yi+xmin] = 255;
		  yuv[0][yi+xmax] = 255;
		  // for (x = xmin; x < xmax; x++) {
			  // yuv[0][yi+x] = 255;

			  /* cb = yuv[1][yi + x];
			  cr = yuv[2][yi + x]; */
			  /* yuv[1][yi + x] = cb;
			  yuv[2][yi + x] = cr; */
			  
		  // }
	  }
	  // center triangle - draw base
	  yi=y0*width;
	  for(x=x0; x<x0+wt; x++){
	    yuv[0][yi+x] = 255;
	  }
	}
	// top triangle
	unsigned int ymin=1;
	if (y0>ht) ymin=y0-ht;
	for (y = y0; y > ymin; y--) {
	        yi=y * width;
	        yr=y0-y;
		yi2=(y0+yr) * width;
                xmin=x0+yr*xpente;
                xmax=x0+wt-yr*xpente;
		for (x = xmin; x <= xmax; x++) {
			yuv[0][yi+x] = yuv[0][yi2+x];
			yuv[1][yi+x] = yuv[1][yi2+x];
			yuv[2][yi+x] = yuv[2][yi2+x];			
		}    
	}
	// bottom triangle
	unsigned int ymax=y0+ht+ht;
	if (ymax>height) ymax=height;
	for (y = y0+ht; y < ymax; y++) {
	        yi=y * width;
	        yr=y-(y0+ht);
		yi2=(y0+ht-yr) * width;
                xmin=xc-yr*xpente;
                xmax=xc+yr*xpente;
		for (x = xmin; x <= xmax; x++) {
		        /* central symmetry
			yuv[0][yi+x] = yuv[0][yi2+width-x];
			yuv[1][yi+x] = yuv[1][yi2+width-x];
			yuv[2][yi+x] = yuv[2][yi2+width-x];
			*/
			// Straight reflexion
			yuv[0][yi+x] = yuv[0][yi2+x];
			yuv[1][yi+x] = yuv[1][yi2+x];
			yuv[2][yi+x] = yuv[2][yi2+x];
		}    
	}
	// left triangle 1
	for (yr = 0; yr < ht; yr++) {
		yi = (y0+yr) * width;
                xmin=x0-wt-yr*xpente;
		if (xmin<0) xmin=0;
                xmax=x0+yr*xpente;
		if(with_border){
		  // Draw sides
		  yuv[0][yi+xmin] = 255;
		  yuv[0][yi+xmax] = 255;
		  yuv[1][yi+xmin] = 255;
		  yuv[1][yi+xmax] = 255;
		}
		for (x = xmin; x < xmax; x++) {
		        int xr, x2, y2, xr2, yr2, yi2;
			xr = x-x0;
			xr2=xr*cos2a+yr*sin2a;
			yr2=xr*sin2a-yr*cos2a;
			x2=xr2+x0;
			y2=yr2+y0;
			yi2=y2*width;
			
			yuv[0][yi+x] = yuv[0][yi2+x2];
			yuv[1][yi+x] = yuv[1][yi2+x2];
			yuv[2][yi+x] = yuv[2][yi2+x2];
			
		}
	}
	// left triangle 2
	for (yr = 0; yr < ht; yr++) {
		yi = (y0+yr) * width;
                xmin=x0-wt+yr*xpente;
		if (xmin<0) xmin=0;
                xmax=x0-yr*xpente;
		if(with_border){
		  // Draw sides
		  yuv[0][yi+xmin] = 255;
		  yuv[0][yi+xmax] = 255;
		  yuv[1][yi+xmin] = 255;
		  yuv[1][yi+xmax] = 255;
		}
		for (x = xmin; x < xmax; x++) {
		        int xr, x2, y2, xr2, yr2, yi2;
			xr = x-x0;
			xr2=xr*cos2a+yr*sin2a;
			yr2=xr*sin2a-yr*cos2a;
			x2=xr2+x0;
			y2=yr2+y0;
			yi2=y2*width;
			
			yuv[0][yi+x] = yuv[0][yi2+x2];
			yuv[1][yi+x] = yuv[1][yi2+x2];
			yuv[2][yi+x] = yuv[2][yi2+x2];
			
		}
	}
	// right triangle 1
	for (yr = 0; yr < ht; yr++) {
		yi = (y0+yr) * width;
                xmin=x0+wt-yr*xpente;
                xmax=x0+wt+yr*xpente;
		if (xmax>width) xmax=width;
		if(with_border){
		  // Draw sides
		  yuv[0][yi+xmin] = 255;
		  yuv[0][yi+xmax] = 255;
		  yuv[1][yi+xmin] = 255;
		  yuv[1][yi+xmax] = 255;
		}
		for (x = xmin; x < xmax; x++) {
		        int xr, x2, y2, xr2, yr2, yi2;
			xr = x-x0-wt;
			xr2=xr*cos2a-yr*sin2a;
			yr2=-xr*sin2a-yr*cos2a;
			x2=xr2+x0+wt;
			y2=yr2+y0;
			yi2=y2*width;
			
			yuv[0][yi+x] = yuv[0][yi2+x2];
			yuv[1][yi+x] = yuv[1][yi2+x2];
			yuv[2][yi+x] = yuv[2][yi2+x2];
			
		}
	}
	// right triangle 2
	for (yr = 0; yr < ht; yr++) {
		yi = (y0+yr) * width;
                xmin=x0+wt+yr*xpente;
                xmax=x0+wt+wt-yr*xpente;
		if (xmax>width) xmax=width;
		if(with_border){
		  // Draw sides
		  yuv[0][yi+xmin] = 255;
		  yuv[0][yi+xmax] = 255;
		  yuv[1][yi+xmin] = 255;
		  yuv[1][yi+xmax] = 255;
		}
		for (x = xmin; x < xmax; x++) {
		        int xr, x2, y2, xr2, yr2, yi2;
			xr = x-x0-wt;
			xr2=xr*cos2a-yr*sin2a;
			yr2=-xr*sin2a-yr*cos2a;
			x2=xr2+x0+wt;
			y2=yr2+y0;
			yi2=y2*width;
			
			yuv[0][yi+x] = yuv[0][yi2+x2];
			yuv[1][yi+x] = yuv[1][yi2+x2];
			yuv[2][yi+x] = yuv[2][yi2+x2];
			
		}
	}

}

void kaleidoscope_apply( VJFrame *frame, int type)
{
	switch (type) {
		case 0:
			kaleidoscope(frame->data, frame->width, frame->height);
		break;
	}
}
