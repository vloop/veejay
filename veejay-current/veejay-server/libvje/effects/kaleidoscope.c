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

///////////////////////////////////////////////////////////////////////////////

////////////////////////////////////
// Utilities - possibly redundant //
////////////////////////////////////
int fill_rectangle(uint8_t ** yuv, const unsigned int width, const unsigned int height, const int x0, const int y0, const int x1, const int y1, const uint8_t y, const uint8_t u, const uint8_t v){
  int yi;
  int x, xmin, xmax, ys, ymin, ymax;
  
  xmin=x0;
  xmax=x1;
  if (xmin>xmax){
    xmin=x1;
    xmax=x0;
  }
  if (xmin<0) xmin=0;
  if (xmin>=width) return(-1); // Off-screen
  if (xmax<0) return(-1); // Off-screen
  if (xmax>=width) xmax=width-1;
  
  ymin=y0;
  ymax=y1;
  if (ymin>ymax){
    ymin=y1;
    ymax=y0;
  }
  if (ymin<0) ymin=0;
  if (ymin>=height) return(-1); // Off-screen
  if (ymax<0) return(-1); // Off-screen
  if (ymax>=height) ymax=height-1;
  
  yi=ymin * width;
  for (ys=ymin; ys<=ymax; ys++, yi+=width){
    for(x=xmin; x<=xmax; x++){
      yuv[0][yi+x] = y;
      yuv[1][yi+x] = u;
      yuv[2][yi+x] = v;
    }
  }
  return(0); // Done
}

int fill_hparall (uint8_t ** yuv, const unsigned int width, const unsigned int height, const int x0, const int y0, const int x1, const int y1, const int xsize, const uint8_t y, const uint8_t u, const uint8_t v){
  int yi;
  int x, xtop, xmin, xbot, xmax, yr, ymin, ymax;
  float xslope;
  // x0, y0 should be top left and x1, y1 bottom left
  // This function handles truncate to display
  // TODO vertical slope is rectangle
  if(x0==x1 || y0==y1) return(-2);
  xtop=x0;
  xbot=x1;
  xmin=x0;
  xmax=x1;
  if (xmin>xmax){
    xmin=x1;
    xmax=x0;
  }
  // if (xmin>=width) return(-1); // Completely off-screen
  // if (xmax<0) return(-1); // Completely off-screen
  
  ymin=y0;
  ymax=y1;
  if (ymin>ymax){
    ymin=y1; xtop=x1;
    ymax=y0; xbot=x0;
  }
  xslope=(float)(xbot-xtop)/(ymax-ymin);// Compute slope before truncate !!

  if (ymin<0) ymin=0; // Truncate left - TODO should fix x
  if (ymin>=height) return(-1); // Completely off-screen
  if (ymax<0) return(-1); // Completely off-screen
  if (ymax>=height) ymax=height-1; // Truncate right - TODO should fix x
  
  yi=ymin * width;
  for (yr=0; yr<=ymax-ymin; yr++, yi+=width){
    xmin=xtop+yr*xslope;
    xmax=xmin+xsize;
    if (xmin<0) xmin=0;
    if (xmin>=width) continue; // Row off-screen
    if (xmax<0) continue; // Row off-screen
    if (xmax>=width) xmax=width-1;
    for(x=xmin; x<=xmax; x++){
      yuv[0][yi+x] = y;
      yuv[1][yi+x] = u;
      yuv[2][yi+x] = v;
    }
  }
  return(0); // Done
}
///////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////
// debugging functions to display digits - should be moved //
/////////////////////////////////////////////////////////////
int setsegments (uint8_t ** yuv, int width, int height, const int segments, const int x0, const int y0, const int seglen, const int Y, const int Cb, const int Cr, const float slope, const int bold){
  // display binary-coded digit, 1 bit per segment
  // 0..5 (abcdef) clockwise from top, 6 (g) dash, 7 dot
  // TODO: slope, bold, xy bound checking using fill_hparall
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

int showdigit (uint8_t ** yuv, int width, int height, const int digit, const int x0, const int y0, const int seglen, const int Y, const int Cb, const int Cr){
  int gfedcba[16] = { 0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71 };
  setsegments(yuv, width, height, gfedcba[digit & 0x0F], x0, y0, seglen, Y, Cb, Cr, 0.0, 0);
  return(0);
}
// showdigit(3, x0, y0, 10, 255, 0, 0);

int showhexdigits (uint8_t ** yuv, int width, int height, const int digits, const int x0, const int y0, const int seglen, const int Y, const int Cb, const int Cr){
  showdigit(yuv, width, height, digits & 0x0F, x0+(seglen+5)*3, y0, seglen, Y, Cb, Cr);
  showdigit(yuv, width, height, (digits >> 4) & 0x0F, x0+(seglen+5)*2, y0, seglen, Y, Cb, Cr);
  showdigit(yuv, width, height, (digits >> 8) & 0x0F, x0+(seglen+5), y0, seglen, Y, Cb, Cr);
  showdigit(yuv, width, height, (digits >> 12) & 0x0F, x0, y0, seglen, Y, Cb, Cr);
  return(0);
}
// const unsigned int seglen=12;
// showhexdigits(0x0123, xc, yc, seglen, 255, 0, 0);
// showhexdigits(0x0123, xc+1, yc+1, seglen, 255, 0, 0);
// showhexdigits(0x4567, xc, yc-3*seglen, seglen, 255, 0, 0);
// showhexdigits(0x89AB, xc, yc-6*seglen, seglen, 255, 0, 0);
// showhexdigits(0xCDEF, xc, yc-9*seglen, seglen, 255, 0, 0);

///////////////////////////////////////////////////////////////////////////////

static int frame_index = 0; // TODO refactor malloc/prepare

vj_effect *kaleidoscope_init(int w, int h)
{
  
  vj_effect *ve = (vj_effect *) vj_calloc(sizeof(vj_effect));
  ve->num_params = 7;
  ve->defaults = (int *) vj_calloc(sizeof(int) * ve->num_params);	/* default values */
  ve->limits[0] = (int *) vj_calloc(sizeof(int) * ve->num_params);	/* min */
  ve->limits[1] = (int *) vj_calloc(sizeof(int) * ve->num_params);	/* max */
  ve->defaults[0] = 1;
  ve->defaults[1] = 300;
  ve->defaults[2] = 0;
  ve->defaults[3] = 255;
  ve->defaults[4] = 255;
  ve->defaults[5] = 255;
  ve->defaults[6] = 100;
  ve->limits[0][0] = 0;	// triangle or rectangle
  ve->limits[1][0] = 2;
  ve->limits[0][1] = 0; // triangle or rectangle width per thousand
  ve->limits[1][1] = 1000;
  ve->limits[0][2] = 0; // Border size pct
  ve->limits[1][2] = 100;
  ve->limits[0][3] = 0;
  ve->limits[1][3] = 255; // red
  ve->limits[0][4] = 0;
  ve->limits[1][4] = 255; // green
  ve->limits[0][5] = 0;
  ve->limits[1][5] = 255; // blue
  ve->limits[0][6] = 0; // Total light loss
  ve->limits[1][6] = 100; // 100% light reflection
  
  ve->sub_format = 1;
  ve->description = "Kaleidoscope";
  ve->extra_frame = 0;
  ve->has_user = 0;
  ve->param_description = vje_build_param_list(ve->num_params, "Triangle/Square", "Width", "Border size", "Border red", "Border green", "Border blue", "Reflection");
  return ve;
}

static void kaleidoscope( VJFrame* frame, const unsigned int type, const int wt_pct, const unsigned int border_size_pct, int border_r, int border_g, int border_b, const int light_pct)
{
  // Probably a few off by one and rounding errors left
  // Square "kaleidoscope" is not geometrically correct, some reflections should be simultaneously straight and inverted
  // TODO: inverted triangle/diagonal (as extra types)
  // TODO: compute initial reflections in a buffer so that they are never truncated and reflection 2 can always use reflection 1
  uint8_t ** yuv = frame->data;
  int width = frame->width;
  int height = frame->height;
  double timecode = frame->timecode;
  
  const unsigned int debugging=0;
  
  const float alpha = M_PI/3;
  const float sin_a = sin(alpha); // about .866
  const float sin_2a = sin(2*alpha);
  const float cos_2a = cos(2*alpha);
  
  const float light_coeff = light_pct/100.0;
  
  // centre of the picture (absolute):
  const int yc = height/2;
  const int xc = width/2;
  // Width and height of the original triangle
  const int wt = wt_pct * (width + height)/1000;
  const int ht = wt*sin_a;
  // x changes by half the triangle width while y moves across the triangle height
  const float xslope=0.5*wt/ht; // about .577
  // x0, y0 top left angle of the source triangle or rectangle
  const int y0=yc-ht/2;
  const int x0=xc-wt/2;
  // x1, y1 top right angle of the source triangle
  const int y1=y0;
  const int x1=x0+wt;
  // x2, y2 bottom angle of the source triangle
  const int y2=y0+ht;
  const int x2=xc;
  // x1, y2 bottom right of source rectangle
  
  // general work variables
  int x, xr, xmin, xmax, xrmin, xrmax, y, yr; // ymin, ymax;
  // x, y, xmin, xmax absolute; xr, yr relative
  // Signed so that triangle truncate works!!
  int x_offset;
  unsigned int yi, yi2; // offset in pixel array
  // uint8_t cb, cr;
  uint8_t border_y,border_u,border_v;
  _rgb2yuv( border_r,border_g,border_b,border_y,border_u,border_v );
  int border_size, border_mod_size, top_size;
  
  /////////////
  // Framing //
  /////////////
  // Should be moved to separate functions or maybe separate effect
  
  // draw original triangle borders
  if (border_size_pct>0 && wt>0){
    border_size=wt*border_size_pct/300; // For triangle
    border_mod_size=border_size/sin_a;
    
    if(type==1){ // Triangle
      top_size=2*ht*border_size/(wt*sin_a);
      // TODO border type, 0, 1, 2 or 3 sides, with or without summit ==> 16 types
      
      // original triangle - draw sides
      // fill_hparall causes artefacts at bigger size and width
      // fill_hparall(yuv, width, height, x0, y0, x2, y2, border_size/sin_a, border_y, border_u, border_v);
      // fill_hparall(yuv, width, height, x1-(border_size/sin_a), y1, x2-(border_size/sin_a), y2, border_size/sin_a, border_y, border_u, border_v);
      fill_hparall(yuv, width, height, x0-border_mod_size, y0, x2-border_mod_size, y2, 2*border_mod_size, border_y, border_u, border_v); // Double width
      fill_hparall(yuv, width, height, x1-border_mod_size, y1, x2-border_mod_size, y2, 2*border_mod_size, border_y, border_u, border_v); // Double width
      /*
      for (yr = border_size; yr <= ht-top_size; yr++) {
	if(y0+yr>=0 && y0+yr<height){
	  yi = (y0+yr) * width;
	  xmin=x0+yr*xslope;
	  xmax=x1-yr*xslope+1;
	  for(xr=0; xr<=border_size/sin_a; xr++){
	    if (xmin+xr>=0 && xmin+xr<width){
	      yuv[0][yi+xmin+xr] = border_y;
	      yuv[1][yi+xmin+xr] = border_u;
	      yuv[2][yi+xmin+xr] = border_v;
	    }
	    if (xmax-xr>=0 && xmax-xr<width){
	      yuv[0][yi+xmax-xr] = border_y;
	      yuv[1][yi+xmax-xr] = border_u;
	      yuv[2][yi+xmax-xr] = border_v;
	    }
	  }
	}
      }
      */
      
      // original triangle - draw trapezoidal base (at top)
      fill_rectangle(yuv, width, height, x0, y0-border_size, x1, y0+border_size, border_y, border_u, border_v); // Double width
/*
      for (y=y0; y<=y0+border_size; y++){
	if(y>=0 && y<height){
	  yi=y*width;
	  for(x=x0+(y-y0)*xslope; x<=x1-(y-y0)*xslope+1; x++){
	    if (x>=0 && x<width){
	      yuv[0][yi+x] = border_y;
	      yuv[1][yi+x] = border_u;
	      yuv[2][yi+x] = border_v;
	    }
	  }
	}
      }
*/
      /* Part of sides
      // original triangle - draw triangular summit (at bottom)
      if(top_size>0){
	for (yr=0; yr<=top_size; yr++){
	  y=y2-yr;
	  if(y>=0 && y<height){
	    yi=y*width;
	    for(x=x2-yr*xslope; x<=x2+yr*xslope; x++){
	      if (x>=0 && x<width){
		yuv[0][yi+x] = border_y;
		yuv[1][yi+x] = border_u;
		yuv[2][yi+x] = border_v;
	      }
	    }
	  }
	}
      }
      */
      
      
    }else{ // Original rectangle borders
      border_size=wt*border_size_pct/200; // For rectangle
      fill_rectangle(yuv, width, height, x0, y0, x0+border_size, y2, border_y, border_u, border_v);
      fill_rectangle(yuv, width, height, x1, y1, x1-border_size, y2, border_y, border_u, border_v);
      fill_rectangle(yuv, width, height, x0+border_size, y0, x1-border_size, y0+border_size, border_y, border_u, border_v);
      fill_rectangle(yuv, width, height, x0+border_size, y2-border_size, x1-border_size, y2, border_y, border_u, border_v);
    }
    
    if(type==2 && ht>0){ // Original rectangle diagonal
      float dborder_size=2*border_size/sin(M_PI/4); // FIXME rectangle is not a square
      fill_hparall(yuv, width, height, x0-dborder_size/2, y0, x1-dborder_size/2, y2, dborder_size, border_y, border_u, border_v);
    }
    
  }
  
  
  
  // Debugging display
  if(debugging){
    const unsigned int seglen=12, lineheigth=3*seglen;
    x=xc-3*seglen;
    y=yc+lineheigth;
    showhexdigits(yuv, width, height, border_size/xslope, x, y, seglen, 255, 0, 0);
    showhexdigits(yuv, width, height, border_size/xslope, x+1, y+1, seglen, 255, 0, 0);
    y-=lineheigth;
    showhexdigits(yuv, width, height, border_size, x, y, seglen, 255, 255, 0);
    showhexdigits(yuv, width, height, border_size, x+1, y+1, seglen, 255, 255, 0);
    y-=lineheigth;
    showhexdigits(yuv, width, height, frame_index, x, y, seglen, 0, 255, 255);
    showhexdigits(yuv, width, height, frame_index, x+1, y+1, seglen, 0, 255, 255);
    y-=lineheigth;
    showhexdigits(yuv, width, height, (int)timecode, x, y, seglen, 0, 0, 255);
    showhexdigits(yuv, width, height, (int)timecode, x+1, y+1, seglen, 0, 0, 255);
    return;
  }
  
  
  // reflection work variables (s for source)
  int xs, ys, xrs, yrs, xr2, yr2;
  
  ///////////////////////
  // First reflections //
  ///////////////////////
  // beware of bounds, a read outside the frame can cause a core dump
  if(type==1){ // Triangle
    for (yr = 0; yr <= ht; yr++) {
      y=y0+yr;
      if(y>=0 && y<height){
	yi = y * width;
	
	// left triangle 1
	xmin=x0-yr*xslope;
	xmax=x0+yr*xslope;
	if (xmin<0)
	  xmin=0;
	if (xmax<0)
	  xmax=0;
	// if (xmax>width) xmax=width; // should not happen on left reflections
	for (x = xmin; x <= xmax; x++) {
	  xr = x - x0;
	  xrs = xr*cos_2a + yr*sin_2a;
	  yrs = xr*sin_2a - yr*cos_2a;
	  xs = x0 + xrs;
	  ys = y0 + yrs;
	  if(xs>=0 && xs<width && ys>=0 && ys<height){
	    yi2 = ys * width;
	    yuv[0][yi+x] = yuv[0][yi2+xs]*light_coeff;
	    yuv[1][yi+x] = yuv[1][yi2+xs];
	    yuv[2][yi+x] = yuv[2][yi2+xs];
	  }else{
	    yuv[0][yi+x] = 0;
	    yuv[1][yi+x] = 0;
	    yuv[2][yi+x] = 0;
	  }
	}

	// right triangle 1
	// FIXME visual artefact on one edge, off by one?
	xmin=x1-yr*xslope;
	xmax=x1+yr*xslope;
	if (xmin<0) xmin=0; // should not happen on right reflections
	if (xmax<0) xmax=0;
	if (xmin>=width) xmin=width-1;
	if (xmax>=width) xmax=width-1;
	for (x = xmin; x <= xmax; x++) {
	  xr = x - x1;
	  // yr already set correctly
	  xrs = xr*cos_2a - yr*sin_2a;
	  yrs = - xr*sin_2a - yr*cos_2a;
	  xs = x1 + xrs;
	  ys = y1 + yrs;
	  if(xs>=0 && xs<width && ys>=0 && ys<height){
	    yi2 = ys * width;
	    yuv[0][yi+x] = yuv[0][yi2+xs]*light_coeff;
	    yuv[1][yi+x] = yuv[1][yi2+xs];
	    yuv[2][yi+x] = yuv[2][yi2+xs];
	  }  
	}
      }
    }

    // 2nd triangular reflection
    for (yr = 0; yr <= ht; yr++) {
      y=y0+yr;
      if(y>=0 && y<height){
	yi = y * width;
	ys = y2 - yr;
        if(ys>=0 && ys<height){
	  // yi2 = ys * width; // Outside x-loop for reflection
	  
	  // left triangle 2
	  // left triangle 1 reflection = source triangle rotation = right triangle 1 inversion !!
	  // TODO skip completely if off-screen
	  xmin=x0-wt+yr*xslope;
	  xmax=x0-yr*xslope;
	  if (xmin<0) xmin=0;
	  if (xmax>=width) xmax=width-1;
	  if (xmax>=0 && xmin<width){
	    for (x = xmin; x <= xmax; x++) {
	      // source triangle rotation -2a
	      xr = x-x0; // <0 for left triangle 2
	      xrs = xr*cos_2a + yr*sin_2a;
	      yrs = -xr*sin_2a + yr*cos_2a;
	      xs=x0+xrs;
	      ys=y0+yrs;
	      // 1st right reflection's reflection
	      // Mostly ok but what if right triangle 1 is partly off-screen? will create visible artefacts
	      // xs=x+(wt*1.5);
	      if(xs>=0 && xs<width && ys>=0 && ys<height){
		yi2=width*ys; // Inside x-loop for rotation
		yuv[0][yi+x] = yuv[0][yi2+xs]*light_coeff*light_coeff;
		yuv[1][yi+x] = yuv[1][yi2+xs];
		yuv[2][yi+x] = yuv[2][yi2+xs];
	      }
	    }
	  }
	  
	  // right triangle 2
	  // right triangle 1 reflection = source triangle rotation !!
	  // TODO skip completely if off-screen
	  xmin=x1+yr*xslope;
	  xmax=x1+wt-yr*xslope;
	  if (xmin<0) xmin=0;
	  if (xmax>=width) xmax=width-1;
	  if (xmax>=0 && xmin<width){
	    for (x = xmin; x <= xmax; x++) {
	      // rotation +2a
	      xr = x-x1; // <0 for left triangle 2
	      xrs = xr*cos_2a - yr*sin_2a;
	      yrs = xr*sin_2a + yr*cos_2a;
	      xs=x1+xrs;
	      ys=y1+yrs;
	      // 1st right reflection's reflection
	      // xs=x-(wt*1.5);
	      if(xs>=0 && xs<width && ys>=0 && ys<height){
		yi2=width*ys; // For rotation
		yuv[0][yi+x] = yuv[0][yi2+xs]*light_coeff*light_coeff;
		yuv[1][yi+x] = yuv[1][yi2+xs];
		yuv[2][yi+x] = yuv[2][yi2+xs];
	      }
	    }
	  }
	}
      }
    }
    
    // Third triangular reflection
    for (yr = 0; yr <= ht; yr++) {
      y=y0+yr;
      if(y>=0 && y<height){
	yi = y * width;
	yr2=y-y2; // Relative to bottom of original triangle
	
	// Half right triangle 3
	// right triangle 2 reflection = right triangle 1 rotation !!
	// needs 1st y loop to be complete, hence has its own y loop
	// only left half needed ?
	// TODO skip completely if off-screen
	xmin=x1+wt-yr*xslope; // From top
	xmax=x1+wt ; // +yr*xslope; // comment out slope for left half only
	if (xmin<0) xmin=0; // should not happen on right reflections
	if (xmax<0) xmax=0;
	if (xmax>=width) xmax=width-1;
	if (xmin>=width) xmin=width-1;
	for (x = xmin; x <= xmax; x++) {
	  xr2 = x-(x2+wt);
	  // rotation -2a
	  xrs = xr2*cos_2a + yr2*sin_2a;
	  yrs = - xr2*sin_2a + yr2*cos_2a;
	  xs=x2+wt+xrs;
	  ys=y2+yrs;
	  if(xs>=0 && xs<width && ys>=0 && ys<height){
	    yi2=width*ys;
	    yuv[0][yi+x] = yuv[0][yi2+xs]*light_coeff;
	    yuv[1][yi+x] = yuv[1][yi2+xs];
	    yuv[2][yi+x] = yuv[2][yi2+xs];
	  }
	}
	
	// Half left triangle 3
	// left triangle 2 reflection = left triangle 2 rotation !!
	// TODO skip completely if off-screen
	xmin=x0-wt; // Right half-triangle only
	xmax=x0-wt+yr*xslope;
	if (xmin<0)
	  xmin=0;
	if (xmax<0)
	  xmax=0;
	if (xmax>=width) xmax=width-1;
	if (xmin>=width) xmin=width-1;
	for (x = xmin; x <= xmax; x++) {
	  xr2 = x-(x2-wt); // <0 for left triangle 3
	  // rotation -2a
	  xrs = xr2*cos_2a - yr2*sin_2a;
	  yrs = xr2*sin_2a + yr2*cos_2a;
	  xs=x2-wt+xrs;
	  ys=y2+yrs;
	  if(xs>=0 && xs<width && ys>=0 && ys<height){
	    yi2=width*ys;
	    yuv[0][yi+x] = yuv[0][yi2+xs]*light_coeff;
	    yuv[1][yi+x] = yuv[1][yi2+xs];
	    yuv[2][yi+x] = yuv[2][yi2+xs];
	  }
	}    
      }
    }
  }
  
  if(type==2 && ht>0){ // Diagonally split rectangle pseudo-reflection
    // TODO real reflection would require a different transform (keep pseudo as an option for visual fun)
    for (yr = 0; yr <= ht; yr++) {
      y=y0+yr;
      if(y>=0 && y<height && y2-yr>=0 && y2-yr<height){
	yi = y * width;
	yi2=(y2-yr) * width;
	for(xr=0; xr<yr*wt/ht; xr++){
	  if(x0+xr>=0 && x0+xr<width && x1-xr>=0 && x1-xr<width){
	    yuv[0][yi+x0+xr] = yuv[0][yi2+x1-xr]*light_coeff;
	    yuv[1][yi+x0+xr] = yuv[1][yi2+x1-xr];
	    yuv[2][yi+x0+xr] = yuv[2][yi2+x1-xr];
	  }
	}
      }
    }
  }
  
  if(type==0 || type==2){ // Rectangle - needs first lateral reflections before copy
    xmax=x0;
    xmin=x1;
    if(xmax>=0){ // Extra space left of initial reflections
      for (yr = 0; yr <= ht; yr++) {
	y=y0+yr;
	if(y>=0 && y<height){
	  yi = y * width;
	  for (xr=wt; xr>=0;xr--){ // Reverse scan (leftward)
	    xs=xmax+xr;
	    x=xmax-xr;
	    if(x>=0 && x<width && xs>=0 && xs<width){
	      yuv[0][yi+x] = yuv[0][yi+xs]*light_coeff;
	      yuv[1][yi+x] = yuv[1][yi+xs];
	      yuv[2][yi+x] = yuv[2][yi+xs];
	    }
	  }
	}
      }
    }
    if(xmin<width){ // Extra space right of initial reflections
      for (yr = 0; yr <= ht; yr++) {
	y=y0+yr;
	if(y>=0 && y<height){
	  yi = y * width;
	  for (xr=0; xr<=wt;xr++){ // Forward scan (rightward)
	    xs=xmin-xr;
	    x=xmin+xr;
	    if(x>=0 && x<width && xs>=0 && xs<width){
	      yuv[0][yi+x] = yuv[0][yi+xs]*light_coeff;
	      yuv[1][yi+x] = yuv[1][yi+xs];
	      yuv[2][yi+x] = yuv[2][yi+xs];
	    }
	  }
	}
      }
    }
  }

  // FIXME some triangle border artifacts could be cleaned by re-drawing borders for initial reflections
  // But would better fix possibly off-by-one reflection
  
  /////////////////////////
  // lateral reflections //
  /////////////////////////
  // FIXME attenuation wrong for edge half-triangles reflections
  // Attenuation factor should be squared only outside those half-triangles
  x_offset=2*wt;
  xmax=x0-1-wt; // Start one pixel left of first reflection, right to left
  xmin=x1+1+wt; // Start one pixel right of first reflection, left to right
  if (type==1){
    x_offset=3*wt;
  }
  if(xmax>=0){ // Extra space left of initial reflections
    for (yr = 0; yr <= ht; yr++) {
      y=y0+yr;
      if(y>=0 && y<height){
	yi = y * width;
	for (x=xmax; x>=0;x--){ // Reverse scan so it can copy itself
	  // Main pattern is 1 or 3 * wt wide and starts at x0-wt
	  yuv[0][yi+x] = yuv[0][yi+x+x_offset]*light_coeff*light_coeff;
	  yuv[1][yi+x] = yuv[1][yi+x+x_offset];
	  yuv[2][yi+x] = yuv[2][yi+x+x_offset];
	}
      }
    }
  }
  if(xmin<width){ // Extra space right of initial reflections
    for (yr = 0; yr <= ht; yr++) {
      y=y0+yr;
      if(y>=0 && y<height){
	yi = y * width;
	for (x=xmin; x<width;x++){ // Forward scan so it can copy itself
	  // Main pattern is 1 or 3 * wt wide and starts at x0-wt
	  yuv[0][yi+x] = yuv[0][yi+x-x_offset]*light_coeff*light_coeff;
	  yuv[1][yi+x] = yuv[1][yi+x-x_offset];
	  yuv[2][yi+x] = yuv[2][yi+x-x_offset];
	}
      }
    }
  }
  
  if (type==1){ // Ugly partial fix for wrong attenuation
      // Multiples copies may have occurred, this only fixes the 1st one
      // This fix should be applied after 1st and before 2nd copy instead of after all copies
      for (yr = 0; yr <= ht; yr++) {
      y=y0+yr;
      if(y>=0 && y<height){
	yi = y * width;
	yr2=y-y2; // Relative to bottom of original triangle
	
	// Half right triangle 3
	// right triangle 2 reflection = right triangle 1 rotation !!
	// needs 1st y loop to be complete, hence has its own y loop
	// only left half needed ?
	// TODO skip completely if off-screen
	xmin=x1+wt;// -yr*xslope; // From top
	xmax=x1+wt+yr*xslope; // comment out slope for left half only
	if (xmin<0) xmin=0; // should not happen on right reflections
	if (xmax<0) xmax=0;
	if (xmax>=width) xmax=width-1;
	if (xmin>=width) xmin=width-1;
	// if (xmax>width) xmax=width; // should not happen on left reflections
	for (x = xmin; x <= xmax; x++) {
	  xr2 = x-(x2+wt);
	  // rotation -2a
	  xrs = xr2*cos_2a + yr2*sin_2a;
	  yrs = - xr2*sin_2a + yr2*cos_2a;
	  xs=x2+wt+xrs;
	  ys=y2+yrs;
	  if(xs>=0 && xs<width && ys>=0 && ys<height){
	    yi2=width*ys;
	    yuv[0][yi+x] = yuv[0][yi2+xs] * light_coeff;
	    yuv[1][yi+x] = yuv[1][yi2+xs];
	    yuv[2][yi+x] = yuv[2][yi2+xs];
	  }
	}
	// Half left triangle 3
	// left triangle 2 reflection = left triangle 2 rotation !!
	// TODO skip completely if off-screen
	xmin=x0-wt-yr*xslope; // Right half-triangle only
	xmax=x0-wt; // +yr*xslope;
	if (xmin<0)
	  xmin=0;
	if (xmax<0)
	  xmax=0;
	if (xmax>=width) xmax=width-1;
	if (xmin>=width) xmin=width-1;
	for (x = xmin; x <= xmax; x++) {
	  xr2 = x-(x2-wt); // <0 for left triangle 3
	  // rotation -2a
	  xrs = xr2*cos_2a - yr2*sin_2a;
	  yrs = xr2*sin_2a + yr2*cos_2a;
	  xs=x2-wt+xrs;
	  ys=y2+yrs;
	  if(xs>=0 && xs<width && ys>=0 && ys<height){
	    yi2=width*ys;
	    yuv[0][yi+x] = yuv[0][yi2+xs] * light_coeff;
	    yuv[1][yi+x] = yuv[1][yi2+xs];
	    yuv[2][yi+x] = yuv[2][yi2+xs];
	  }
	}    
      }
    }
  }
  //////////////////////////
  // vertical reflections //
  //////////////////////////
  if (y0>0) { // First reflections above original
    for (y=y0;y>y0-ht && y>0;y--){
      yi = y * width;
      yr = y - y0;
      yi2 = (y0-yr)* width; // Source 1 triangle height below, inverted
      for(x=0;x<width;x++){
	yuv[0][yi+x] = yuv[0][yi2+x]*light_coeff;
	yuv[1][yi+x] = yuv[1][yi2+x];
	yuv[2][yi+x] = yuv[2][yi2+x];
	
      }
    }
  }
  
  if (y2<height) { // First reflections below original
    for (y=y2;y<y2+ht && y<height;y++){
      yi = y * width;
      yr = y-y2; // will remain positive
      ys = y2-yr;
      yi2 = ys * width; // Source 1 triangle height above, inverted
      for(x=0;x<width;x++){
	yuv[0][yi+x] = yuv[0][yi2+x]*light_coeff;
	yuv[1][yi+x] = yuv[1][yi2+x];
	yuv[2][yi+x] = yuv[2][yi2+x];
	
      }
    }
  }
  
  if (y0>ht) { // Extra space above initial reflections
    for (y=y0-ht;y>0;y--){
      yi = y * width;
      yi2 = yi + 2*ht* width;
      for(x=0;x<width;x++){
	yuv[0][yi+x] = yuv[0][yi2+x]*light_coeff;
	yuv[1][yi+x] = yuv[1][yi2+x];
	yuv[2][yi+x] = yuv[2][yi2+x];
	
      }
    }
  }
  
  if (y2+ht<height) { // Extra space below initial reflections
    // TODO uses vj_frame_copy; is there a vj_blit?
    // int len = width*height;
    // int strides[4] = { len,len,len, 0 };
    // vj_frame_copy( yuv, buf, strides );
    for (y=y2+ht;y<height;y++){
      yi = y * width;
      yi2 = yi - 2*ht*width;
      for(x=0;x<width;x++){
	yuv[0][yi+x] = yuv[0][yi2+x]*light_coeff;
	yuv[1][yi+x] = yuv[1][yi2+x];
	yuv[2][yi+x] = yuv[2][yi2+x];
	
      }
    }
  }
  
}

void kaleidoscope_apply( VJFrame *frame, int type, int wt, int border, int border_r, int border_g, int border_b, int light_pct )
{
  kaleidoscope(frame, type, wt, border, border_r, border_g, border_b, light_pct);
  frame_index++;
}
