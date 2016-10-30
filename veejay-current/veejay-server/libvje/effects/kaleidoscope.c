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
	unsigned int x, xmin, xmax, y, yr;
        // x absolute
        // yr relative to triangle base 
	unsigned int yi, yi2; // offset dans le tableau
	uint8_t cb, cr;
	const float sin60 = sin(M_PI/3);
	// const float cos60 = cos(M_PI/3);
	// const float tan30 = tan(M_PI/6);
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
	// center triangle - draw sides
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
	// top triangle
	unsigned int ymin=1;
	if (y0>ht) ymin=y0-ht;
	for (y = y0; y > ymin; y--) {
	        yi=y * width;
	        yr=y0-y;
		yi2=(y0+yr) * width;
                xmin=x0+yr*xpente;
                xmax=x0+wt-yr*xpente;
		for (x = xmin; x < xmax; x++) {
			yuv[0][yi+x] = yuv[0][yi2+x];
			yuv[1][yi+x] = yuv[1][yi2+x];
			yuv[2][yi+x] = yuv[2][yi2+x];			
		}    
	}
	// bottom triangle
	unsigned int ymax=y0+ht+ht;
	if (ymax>height) ymax=height-1;
	for (y = y0+ht; y < ymax; y++) {
	        yi=y * width;
	        yr=y-(y0+ht);
		yi2=(y0+ht-yr) * width;
                xmin=xc-yr*xpente;
                xmax=xc+yr*xpente;
		for (x = xmin; x < xmax; x++) {
			yuv[0][yi+x] = yuv[0][yi2+x];
			yuv[1][yi+x] = yuv[1][yi2+x];
			yuv[2][yi+x] = yuv[2][yi2+x];
		}    
	}
	// debugging functions to display digits - should be factored out
	int setsegment (int segnum, int x0, int y0, int seglen, int Y, int Cb, int Cr){
	  // 0..5 clockwise from top, 6 dash, 7 dot
	  if (segnum>7) return(1);
	  unsigned int x, y, yi;
	  switch(segnum){
	    case 0: // horizontal, top
	      yi=(y0-2*seglen)*width;
	      for (x=x0; x<x0+seglen; x++){
		yuv[0][yi+x] = Y;
		yuv[1][yi+x] = Cb;
		yuv[2][yi+x] = Cr;
	      }
	      break;
	    case 1: // vertical, top right
	      x=x0+seglen;
	      for(y=y0-seglen; y>y0-seglen*2; y--){
		yi=y*width;
		yuv[0][yi+x] = Y;
		yuv[1][yi+x] = Cb;
		yuv[2][yi+x] = Cr;
	      }
	      break;      
	    case 2: // vertical, bottom right
	      x=x0+seglen;
	      for(y=y0; y>y0-seglen; y--){
		yi=y*width;
		yuv[0][yi+x] = Y;
		yuv[1][yi+x] = Cb;
		yuv[2][yi+x] = Cr;
	      }
	      break;      
	    case 3: // horizontal, bottom
	      yi=y0*width;
	      for (x=x0; x<x0+seglen; x++){
		yuv[0][yi+x] = Y;
		yuv[1][yi+x] = Cb;
		yuv[2][yi+x] = Cr;
	      }
	      break;      
	    case 4: // vertical, bottom left
	      x=x0;
	      for(y=y0; y>y0-seglen; y--){
		yi=y*width;
		yuv[0][yi+x] = Y;
		yuv[1][yi+x] = Cb;
		yuv[2][yi+x] = Cr;
	      }
	      break;      
	    case 5: // vertical, top left
	      x=x0;
	      for(y=y0-seglen; y>y0-seglen*2; y--){
		yi=y*width;
		yuv[0][yi+x] = Y;
		yuv[1][yi+x] = Cb;
		yuv[2][yi+x] = Cr;
	      }
	      break;      
	    case 6: // horizontal, middle
	      yi=(y0-seglen)*width;
	      for (x=x0; x<x0+seglen; x++){
		yuv[0][yi+x] = Y;
		yuv[1][yi+x] = Cb;
		yuv[2][yi+x] = Cr;
	      }
	      break;
	    case 7: // dot bottom right
	      yi=(y0+2)*width;
	      x=x0+seglen+2;
	      yuv[0][yi+x] = Y;
	      yuv[1][yi+x] = Cb;
	      yuv[2][yi+x] = Cr;
	      }
	}
	// setsegment(0, xc, yc+30, 10, 255, 0, 0);
	// setsegment(1, xc, yc+30, 10, 255, 0, 0);
	// setsegment(5, xc, yc+30, 10, 255, 0, 0);
	// setsegment(6, xc, yc+30, 10, 255, 0, 0);
	int showdigit (int digit, int x0, int y0, int seglen, int Y, int Cb, int Cr){
	  switch (digit & 0x0F){
	    case 0:
	      setsegment(0, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(1, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(2, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(3, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(4, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(5, x0, y0, seglen, Y, Cb, Cr);
	      break;
	    case 1:
	      setsegment(1, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(2, x0, y0, seglen, Y, Cb, Cr);
	      break;
	    case 2:
	      setsegment(0, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(1, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(3, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(4, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(6, x0, y0, seglen, Y, Cb, Cr);
	      break;
	    case 3:
	      setsegment(0, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(1, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(2, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(3, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(6, x0, y0, seglen, Y, Cb, Cr);
	      break;
	    case 4:
	      setsegment(1, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(2, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(5, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(6, x0, y0, seglen, Y, Cb, Cr);
	      break;
	    case 5:
	      setsegment(0, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(5, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(3, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(2, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(6, x0, y0, seglen, Y, Cb, Cr);
	      break;
	    case 6:
	      setsegment(0, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(2, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(3, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(4, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(5, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(6, x0, y0, seglen, Y, Cb, Cr);
	      break;
	    case 7:
	      setsegment(0, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(1, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(2, x0, y0, seglen, Y, Cb, Cr);
	      break;
	    case 8:
	      setsegment(0, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(1, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(2, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(3, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(4, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(5, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(6, x0, y0, seglen, Y, Cb, Cr);
	      break;
	    case 9:
	      setsegment(0, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(1, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(2, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(3, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(5, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(6, x0, y0, seglen, Y, Cb, Cr);
	      break;
	    case 10: // 0x0A
	      setsegment(0, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(1, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(2, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(4, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(5, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(6, x0, y0, seglen, Y, Cb, Cr);
	      break;
	    case 11: // 0x0B
	      setsegment(2, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(3, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(4, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(5, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(6, x0, y0, seglen, Y, Cb, Cr);
	      break;
	    case 12: // 0x0C
	      setsegment(0, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(3, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(4, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(5, x0, y0, seglen, Y, Cb, Cr);
	      break;
	    case 13: // 0x0D
	      setsegment(1, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(2, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(3, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(4, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(6, x0, y0, seglen, Y, Cb, Cr);
	      break;
	    case 14: // 0x0E
	      setsegment(0, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(3, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(4, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(5, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(6, x0, y0, seglen, Y, Cb, Cr);
	      break;
	    case 15: // 0x0F
	      setsegment(0, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(4, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(5, x0, y0, seglen, Y, Cb, Cr);
	      setsegment(6, x0, y0, seglen, Y, Cb, Cr);
	      break;
	  }
	  return(0);
	}
	// showdigit(3, x0, y0, 10, 255, 0, 0);
	int showhexdigits (int digits, int x0, int y0, int seglen, int Y, int Cb, int Cr){
	  showdigit(digits & 0x0F, x0+(seglen+5)*3, y0, seglen, Y, Cb, Cr);
	  showdigit((digits >> 4) & 0x0F, x0+(seglen+5)*2, y0, seglen, Y, Cb, Cr);
	  showdigit((digits >> 8) & 0x0F, x0+(seglen+5), y0, seglen, Y, Cb, Cr);
	  showdigit((digits >> 12) & 0x0F, x0, y0, seglen, Y, Cb, Cr);
	  return(0);
	}
	const unsigned int seglen=12;
	showhexdigits(0x0123, xc, yc, seglen, 255, 0, 0);
	showhexdigits(0x0123, xc+1, yc+1, seglen, 255, 0, 0);
	// showhexdigits(0x4567, xc, yc-3*seglen, seglen, 255, 0, 0);
	// showhexdigits(0x89AB, xc, yc-6*seglen, seglen, 255, 0, 0);
	// showhexdigits(0xCDEF, xc, yc-9*seglen, seglen, 255, 0, 0);
}

void kaleidoscope_apply( VJFrame *frame, int type)
{
	switch (type) {
		case 0:
			kaleidoscope(frame->data, frame->width, frame->height);
		break;
	}
}
