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

// Set these for live debugging
static int dbg1=0, dbg2=0, dbg3=0, dbg4=0;
///////////////////////////////////////////////////////////////////////////////

////////////////////////////////////
// Utilities - possibly redundant //
////////////////////////////////////
int
fill_rectangle (uint8_t ** yuv, const unsigned int width,
		const unsigned int height, const int x0, const int y0,
		const int x1, const int y1, const uint8_t Y, const uint8_t u,
		const uint8_t v)
{
  int yi,p;
  int xmin, xmax, ys, ytop, ybot;

  xmin = x0<x1 ? x0 : x1;
  xmax = x0>x1 ? x0 : x1;
  xmin = xmin < 0 ? 0 : xmin;	// Clip left
  if (xmin >= width)
    return (-1);		// Off-screen
  if (xmax < 0)
    return (-1);		// Off-screen
  xmax = xmax >= width ? width - 1 : xmax; // Clip right

  ytop = y0<y1 ? y0 : y1;
  ybot = y0<y1 ? y1 : y0;
  ytop = ytop<0 ? 0 : ytop;	// Clip top
  if (ytop >= height)
    return (-1);		// Off-screen
  if (ybot < 0)
    return (-1);		// Off-screen
  ybot = ybot >= height ? height - 1 : ybot ;  // Clip bottom

  for (ys = ytop, yi = ytop * width; ys <= ybot; ys++, yi += width)
    {
      // Can be parallelized?
      for (p=yi+xmin; p<=yi+xmax; p++)
	{
	  yuv[0][p] = Y;
	  yuv[1][p] = u;
	  yuv[2][p] = v;
	}
    }
  return (0);			// Done
}

int
fill_hparall (uint8_t ** yuv, const unsigned int width,
	      const unsigned int height, const int x0, const int y0,
	      const int x1, const int y1, const int xsize, const uint8_t Y,
	      const uint8_t u, const uint8_t v)
{
  int yi;
  int x, xtop, xmin, xbot, xmax, y, yr, ytop, ybot;
  int xoffset;
  float xslope;
  // x0, y0 should be top left and x1, y1 bottom left
  // This function will truncate to display as needed
  if (xsize == 0)
    return (0);  
  // Assume p0 is top, we may need to swap xbot and xtop later
  xtop = x0;
  xbot = x1;
  if (xsize<0){
    xtop+=xsize;
    xbot+=xsize;
  }

  ytop = y0;
  ybot = y1;
  if (ytop > ybot)
    {
      ytop = y1;
      xtop = x1;
      ybot = y0;
      xbot = x0;
    }

  if (ytop == ybot)
    {				// horizontal line
      xslope = 0;		// Will be used when ytop=ybot
      xoffset = abs(xsize) + abs(x1 - x0);	// Extend to segment length
    }
  else
    {
      xslope = (float) (xbot - xtop) / (ybot - ytop);	// Compute slope before truncate, keeping sign !!
      xoffset = abs(xsize);
      if (xoffset < abs(xslope))
	xoffset += abs(xslope) - 1;	// Avoid gaps in near-horizontal lines
      xoffset= xoffset < 1 ? 1 : xoffset;
    }

  // Truncate y if needed, adjusting xtop to match
  if (ytop < 0)
    {
      xtop -= ytop * xslope;	// FIXME ?
      ytop = 0;			// Truncate left
    }
  if (ybot >= height)
    {
      xbot-=(ybot-(height-1)) * xslope;
      ybot = height - 1;	// Truncate right
    }

  // Completely off-screen test after all adjustments
  if (ytop >= height)
    return (-1);		// Below screen
  if (ybot < 0)
    return (-1);		// Above screen
  xmin = xtop<xbot ? xtop : xbot;
  xmax = xtop<xbot ? xbot : xtop;
  // dbg1=xmin; dbg2=xmax; dbg3=xoffset; dbg4=xmax+xoffset;
  if (xmin>=(signed)width)
    return(-1); // Left of screen
  if ((xmax+xoffset<0)) 
    return(-1); // Right of screen
    
  for (y = ytop, yi = ytop * width; y <= ybot; y++, yi += width)
    {
      xmin = xtop + (y - ytop) * xslope;
      xmax = xmin + xoffset;
      if (xmin < 0)
	xmin = 0;
      if (xmin >= width)
	continue;		// Row off-screen
      if (xmax < 0)
	continue;		// Row off-screen
      if (xmax >= width)
	xmax = width - 1;
      for (x = xmin; x <= xmax; x++)
	{
	  yuv[0][yi + x] = Y;
	  yuv[1][yi + x] = u;
	  yuv[2][yi + x] = v;
	}
    }
  return (0);			// Done
}

///////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////
// debugging functions to display digits - should be moved //
/////////////////////////////////////////////////////////////
int
setsegments (uint8_t ** yuv, const int width, const int height, const int segments,
	     const int x0, const int y0, const int seglen, const int Y,
	     const int Cb, const int Cr, const float slope,
	     const unsigned int bold)
{
  // display binary-coded digit, 1 bit per segment
  // bits 0..5 (abcdef) clockwise from top, 6 (g) dash, 7 dot
  // x0, y0 = bottom left
  // TODO: slope, bold, xy bound checking using fill_hparall
  unsigned int x, y, yi;
  const int segwidth=bold+1; // Not bold --> width 1
  if (segments & 1)
    {				// segment a: horizontal, top
      x = x0 + segwidth + 2 * seglen * slope;
      y = y0 - 2 * seglen - 3*segwidth;
      fill_hparall (yuv, width, height, x, y, x - bold * slope,
		    y + bold, seglen, Y, Cb, Cr);
    }
  if (segments & 2)
    {				// segment b: vertical, top right
      x = x0 + seglen + segwidth + 2 * seglen * slope;
      y = y0 - 2 * seglen - 2 * segwidth;
      fill_hparall (yuv, width, height, x, y, x - seglen * slope,
		    y + seglen, bold + 1, Y, Cb, Cr);
    }
  if (segments & 4)
    {				// segment c: vertical, bottom right
      x = x0 + seglen + segwidth + seglen * slope;
      y = y0 - seglen - segwidth;
      fill_hparall (yuv, width, height, x, y, x - seglen * slope,
		    y + seglen, bold + 1, Y, Cb, Cr);
    }
  if (segments & 8)
    {				// segment d: horizontal, bottom
      x = x0 + segwidth;
      y = y0 - segwidth;
      fill_hparall (yuv, width, height, x, y, x - bold * slope,
		    y + bold, seglen, Y, Cb, Cr);
    }
  if (segments & 16)
    {				// segment e: vertical, bottom left
      x = x0 + seglen * slope;
      y = y0 - seglen - segwidth;
      fill_hparall (yuv, width, height, x, y, x - seglen * slope,
		    y + seglen, bold + 1, Y, Cb, Cr);
    }
  if (segments & 32)
    {				// segment f: vertical, top left
      x = x0 + 2 * seglen * slope;
      y = y0 - 2 * seglen - 2 * segwidth;
      fill_hparall (yuv, width, height, x, y, x - seglen * slope,
		    y + seglen, bold + 1, Y, Cb, Cr);
    }
  if (segments & 64)
    {				// segment g: horizontal, middle  
      x = x0 + segwidth+ seglen * slope;
      y = y0 - seglen -2*segwidth;
      fill_hparall (yuv, width, height, x, y, x - bold * slope,
		    y + bold, seglen, Y, Cb, Cr);
    }
  if (segments & 128)
    {				// dot bottom right FIXME
      y = y0;
      x = x0 + seglen + 3*segwidth;
      fill_hparall (yuv, width, height, x, y, x - bold * slope,
		    y + segwidth, segwidth, Y, Cb, Cr);
    }
  return (0);
}

int
showdigit (uint8_t ** yuv, const int width, const int height, const int digit,
	   const int x0, const int y0, const int seglen, const int Y,
	   const int Cb, const int Cr, const float slope, const unsigned int bold)
{
  int gfedcba[16] =
    { 0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F, 0x77, 0x7C,
0x39, 0x5E, 0x79, 0x71 };
  setsegments (yuv, width, height, gfedcba[digit & 0x0F], x0, y0, seglen, Y,
	       Cb, Cr, slope, bold);
  return (0);
}

// showdigit(3, x0, y0, 10, 255, 0, 0);

int
showhexdigits (uint8_t ** yuv, const int width, const int height, const unsigned int digits,
	       const int x0, const int y0, const int seglen, const int Y,
	       const int Cb, const int Cr, const float slope, const unsigned int bold)
{
  const int size=seglen+3*(bold+1);
  showdigit (yuv, width, height, digits & 0x0F, x0 + size * 3, y0,
	     seglen, Y, Cb, Cr, slope, bold);
  showdigit (yuv, width, height, (digits >> 4) & 0x0F, x0 + size * 2,
	     y0, seglen, Y, Cb, Cr, slope, bold);
  showdigit (yuv, width, height, (digits >> 8) & 0x0F, x0 + size, y0,
	     seglen, Y, Cb, Cr, slope, bold);
  showdigit (yuv, width, height, (digits >> 12) & 0x0F, x0, y0, seglen, Y, Cb,
	     Cr, slope, bold);
  return (0);
}

// const unsigned int seglen=12;
// showhexdigits(0x0123, xc, yc, seglen, 255, 0, 0);
// showhexdigits(0x0123, xc+1, yc+1, seglen, 255, 0, 0);
// showhexdigits(0x4567, xc, yc-3*seglen, seglen, 255, 0, 0);
// showhexdigits(0x89AB, xc, yc-6*seglen, seglen, 255, 0, 0);
// showhexdigits(0xCDEF, xc, yc-9*seglen, seglen, 255, 0, 0);

///////////////////////////////////////////////////////////////////////////////

static int frame_index = 0;	// TODO refactor malloc/prepare

vj_effect *
kaleidoscope_init (int w, int h)
{

  vj_effect *ve = (vj_effect *) vj_calloc (sizeof (vj_effect));
  ve->num_params = 7;
  ve->defaults = (int *) vj_calloc (sizeof (int) * ve->num_params);	/* default values */
  ve->limits[0] = (int *) vj_calloc (sizeof (int) * ve->num_params);	/* min */
  ve->limits[1] = (int *) vj_calloc (sizeof (int) * ve->num_params);	/* max */
  ve->defaults[0] = 1;
  ve->defaults[1] = 300;
  ve->defaults[2] = 0;
  ve->defaults[3] = 255;
  ve->defaults[4] = 255;
  ve->defaults[5] = 255;
  ve->defaults[6] = 100;
  ve->limits[0][0] = 0;		// triangle or rectangle
  ve->limits[1][0] = 2;
  ve->limits[0][1] = 0;		// triangle or rectangle width per thousand
  ve->limits[1][1] = 1000;
  ve->limits[0][2] = 0;		// Border size pct
  ve->limits[1][2] = 100;
  ve->limits[0][3] = 0;
  ve->limits[1][3] = 255;	// red
  ve->limits[0][4] = 0;
  ve->limits[1][4] = 255;	// green
  ve->limits[0][5] = 0;
  ve->limits[1][5] = 255;	// blue
  ve->limits[0][6] = 0;		// Total light loss
  ve->limits[1][6] = 100;	// 100% light reflection

  ve->sub_format = 1;
  ve->description = "Kaleidoscope";
  ve->extra_frame = 0;
  ve->has_user = 0;
  ve->param_description =
    vje_build_param_list (ve->num_params, "Triangle/Square", "Width",
			  "Border size", "Border red", "Border green",
			  "Border blue", "Reflection");
  return ve;
}

static void
kaleidoscope (VJFrame * frame, const unsigned int type, const int wt_pct,
	      const unsigned int border_size_pct, int border_r, int border_g,
	      int border_b, const int light_pct)
{
  // Probably a few off by one and rounding errors left
  // Square "kaleidoscope" is not geometrically correct, some reflections should be simultaneously straight and inverted
  // TODO: inverted triangle/diagonal, center hexagon/cross reflections only (as extra types)
  // TODO: compute initial reflections in a buffer so that they are never truncated and reflection 2 can always use reflection 1
  uint8_t **yuv = frame->data;
  const int width = frame->width;
  const int height = frame->height;
  double timecode = frame->timecode;

  const unsigned int debugging = 0;

  const float alpha = M_PI / 3;
  const float sin_a = sin (alpha);	// about .866
  const float sin_2a = sin (2 * alpha);
  const float cos_2a = cos (2 * alpha);

  const float light_coeff = light_pct / 100.0;

  // centre of the picture (absolute):
  const int yc = height / 2;
  const int xc = width / 2;
  // Width and height of the original triangle
  const int wt = wt_pct * (width + height) / 1000;
  const int ht = wt * sin_a;
  // x changes by half the triangle width while y moves across the triangle height
  const float xslope = 0.5 * wt / ht;	// about .577
  // x0, y0 top left angle of the source triangle or rectangle
  const int y0 = yc - ht / 2;
  const int x0 = xc - wt / 2;
  // x1, y1 top right angle of the source triangle
  const int y1 = y0;
  const int x1 = x0 + wt;
  // x2, y2 bottom angle of the source triangle
  const int y2 = y0 + ht;
  const int x2 = xc;
  // x1, y2 bottom right of source rectangle

  // general work variables
  int x, xr, xmin, xmax, xrmin, xrmax, y, yr;	// ytop, ybot;
  // x, y, xmin, xmax absolute; xr, yr relative
  // Signed so that triangle truncate works!!
  int x_offset;
  unsigned int yi, yi2;		// offset in pixel array
  // uint8_t cb, cr;
  uint8_t border_y, border_u, border_v;
  _rgb2yuv (border_r, border_g, border_b, border_y, border_u, border_v);
  int border_size = 0, border_mod_size = 0, top_size = 0;

  /////////////
  // Framing //
  /////////////
  // Should be moved to separate functions or maybe separate effect

  // draw original triangle borders
  if (border_size_pct > 0 && wt > 0)
    {
      border_size = wt * border_size_pct / 300;	// For triangle
      border_mod_size = border_size / sin_a;
      if (border_size>0 && border_mod_size<1)
	border_mod_size=1;

      if (type == 1)
	{			// Triangle
	  // TODO border type, 0, 1, 2 or 3 sides, with or without summit ==> 16 types

	  // original triangle - draw sides
	  // fill_hparall may cause artefacts at bigger size and width
	  fill_hparall (yuv, width, height, x0 - border_mod_size, y0, x2 - border_mod_size, y2, 2 * border_mod_size, border_y, border_u, border_v);	// Double width
	  fill_hparall (yuv, width, height, x1 - border_mod_size, y1, x2 - border_mod_size, y2, 2 * border_mod_size, border_y, border_u, border_v);	// Double width

	  // original triangle - draw trapezoidal base (at top)
	  // The slanted parts have already been drawn by fill_hparall
	  fill_rectangle (yuv, width, height, x0, y0 - border_size, x1, y0 + border_size, border_y, border_u, border_v);	// Double width

	  /*
	     top_size=2*ht*border_size/(wt*sin_a);
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


	}
      else
	{			// Original rectangle borders
	  border_size = wt * border_size_pct / 200;	// For rectangle
	  fill_rectangle (yuv, width, height, x0, y0, x0 + border_size, y2,
			  border_y, border_u, border_v);
	  fill_rectangle (yuv, width, height, x1, y1, x1 - border_size, y2,
			  border_y, border_u, border_v);
	  fill_rectangle (yuv, width, height, x0 + border_size, y0,
			  x1 - border_size, y0 + border_size, border_y,
			  border_u, border_v);
	  fill_rectangle (yuv, width, height, x0 + border_size,
			  y2 - border_size, x1 - border_size, y2, border_y,
			  border_u, border_v);
	}

      if (type == 2 && ht > 0)
	{			// Original rectangle diagonal
	  float dborder_size = 2 * border_size / sin (M_PI / 4);	// FIXME rectangle is not a square
	  fill_hparall (yuv, width, height, x0 - dborder_size / 2, y0,
			x1 - dborder_size / 2, y2, dborder_size, border_y,
			border_u, border_v);
	}
    }



  // Debugging display
  if (debugging)
    {
      const unsigned int bold=2;
      const float slope = 0.2;
      const unsigned int seglen = 12, lineheigth = 3 * (seglen+bold+1);
      int d1, d2, d3, d4;
      d1=dbg1; d2=dbg2; d3=dbg3; d4=dbg4; // Freeze values in case showhexdigits changes them!
      x = xc - 3 * seglen;
      y = yc + lineheigth;

      // fill_hparall (yuv, width, height, x, y + 4, x + 50, y + 4, 1, 255, 128, 128);	// Horizontal
      // fill_hparall (yuv, width, height, x, y + 6, x + 50, y + 7, 1, 255, 128, 128);	// Near-horizontal
      fill_hparall (yuv, width, height, x, -10, x + 50, 10, 10, 255, 128, 128);	// truncate top      
      fill_hparall (yuv, width, height, x, height-10, x + 50, height+10, 10, 255, 128, 128);	// truncate bottom      
      fill_hparall (yuv, width, height, -10, y+10, 50, y+30, 10, 255, 128, 128);	// truncate left
      fill_hparall (yuv, width, height, width+10, y+10, width-10, y+30, 10, 255, 128, 128);	// truncate right
      
      showhexdigits (yuv, width, height, border_size, x, y, seglen, // FIXME d1
		     255, 0, 0, slope, bold);
      //showhexdigits(yuv, width, height, border_size/xslope, x+1, y+1, seglen, 255, 0, 0);// Pseudo-bold
      y -= lineheigth;
      showhexdigits (yuv, width, height, d2, x, y, seglen, 255, 255,
		     0, slope, bold);
      y -= lineheigth;
      showhexdigits (yuv, width, height, d3, x, y, seglen, 0, 255,
		     255, slope, bold);
      y -= lineheigth;
      showhexdigits (yuv, width, height, d4, x, y, seglen, 0, 0,
		     255, slope, bold);
      return;
    }


  // reflection work variables (s for source)
  int xs, ys, xrs, yrs, xr2, yr2;

  ///////////////////////
  // First reflections //
  ///////////////////////
  // beware of bounds, a read outside the frame can cause a core dump
  if (type == 1)
    {				// Triangle
      for (yr = 0; yr <= ht; yr++)
	{
	  y = y0 + yr;
	  if (y >= 0 && y < height)
	    {
	      yi = y * width;

	      // left triangle 1
	      xmin = x0 - yr * xslope;
	      xmax = x0 + yr * xslope;
	      if (xmin < 0)
		xmin = 0;
	      if (xmax < 0)
		xmax = 0;
	      // if (xmax>width) xmax=width; // should not happen on left reflections
	      for (x = xmin; x <= xmax; x++)
		{
		  xr = x - x0;
		  xrs = xr * cos_2a + yr * sin_2a;
		  yrs = xr * sin_2a - yr * cos_2a;
		  xs = x0 + xrs;
		  ys = y0 + yrs;
		  // Is there a clever way to set limits and move this out of the inner loop?
		  if (xs >= 0 && xs < width && ys >= 0 && ys < height)
		    {
		      yi2 = ys * width;
		      yuv[0][yi + x] = yuv[0][yi2 + xs] * light_coeff;
		      yuv[1][yi + x] = yuv[1][yi2 + xs];
		      yuv[2][yi + x] = yuv[2][yi2 + xs];
		    }
		  else
		    {
		      yuv[0][yi + x] = 0;
		      yuv[1][yi + x] = 0;
		      yuv[2][yi + x] = 0;
		    }
		}

	      // right triangle 1
	      // FIXME visual artefact on one edge, off by one?
	      xmin = x1 - yr * xslope;
	      xmax = x1 + yr * xslope;
	      if (xmin < 0)
		xmin = 0;	// should not happen on right reflections
	      if (xmax < 0)
		xmax = 0;
	      if (xmin >= width)
		xmin = width - 1;
	      if (xmax >= width)
		xmax = width - 1;
	      for (x = xmin; x <= xmax; x++)
		{
		  xr = x - x1;
		  // yr already set correctly
		  xrs = xr * cos_2a - yr * sin_2a;
		  yrs = -xr * sin_2a - yr * cos_2a;
		  xs = x1 + xrs;
		  ys = y1 + yrs;
		  if (xs >= 0 && xs < width && ys >= 0 && ys < height)
		    {
		      yi2 = ys * width;
		      yuv[0][yi + x] = yuv[0][yi2 + xs] * light_coeff;
		      yuv[1][yi + x] = yuv[1][yi2 + xs];
		      yuv[2][yi + x] = yuv[2][yi2 + xs];
		    }
		}
	    }
	}
      // Defeat rounding, redraw edge; ugly but works
      // except when wt ~ width with large borders
      if (border_size > 0)
	{
// ok for large size, KO for small (overwrites source triangle)
//      fill_rectangle(yuv, width, height, x2-wt, y2-border_size, x2, y2, border_y*light_coeff, border_u, border_v);
//      fill_rectangle(yuv, width, height, x2, y2-border_size, x2+wt, y2, border_y*light_coeff, border_u, border_v);
	  fill_rectangle (yuv, width, height, x2 - wt, y2 - border_size,
			  x2 - border_mod_size / 2, y2,
			  border_y * light_coeff, border_u, border_v);
	  fill_rectangle (yuv, width, height, x2 + border_mod_size / 2,
			  y2 - border_size, x2 + wt, y2,
			  border_y * light_coeff, border_u, border_v);
	}

      // 2nd triangular reflection
      // Moved to its own y-loop so that it can reuse 1st reflections
      // Otherwise it would attempt to use parts that have not been drawn yet
      for (yr = 0; yr <= ht; yr++)
	{
	  y = y0 + yr;
	  if (y >= 0 && y < height)
	    {
	      yi = y * width;
	      ys = y2 - yr;
	      if (ys >= 0 && ys < height)
		{
		  // yi2 = ys * width; // Outside x-loop for reflection

		  // left triangle 2
		  // left triangle 1 reflection = source triangle rotation = right triangle 1 inversion
		  // but only source triangle is known good
		  // TODO skip completely if off-screen
		  xmin = x0 - wt + yr * xslope;
		  xmax = x0 - yr * xslope;
		  if (xmin < 0)
		    xmin = 0;
		  if (xmax >= width)
		    xmax = width - 1;
		  if (xmax >= 0 && xmin < width)
		    {
		      for (x = xmin; x <= xmax; x++)
			{
			  // source triangle rotation -2a
			  xr = x - x0;	// <0 for left triangle 2
			  xrs = xr * cos_2a + yr * sin_2a;
			  yrs = -xr * sin_2a + yr * cos_2a;
			  xs = x0 + xrs;
			  ys = y0 + yrs;
			  if (xs >= 0 && xs < width && ys >= 0 && ys < height)
			    {
			      yi2 = width * ys;	// Inside x-loop for rotation
			      yuv[0][yi + x] =
				yuv[0][yi2 + xs] * light_coeff * light_coeff;
			      yuv[1][yi + x] = yuv[1][yi2 + xs];
			      yuv[2][yi + x] = yuv[2][yi2 + xs];
			    }
			}
		    }

		  // right triangle 2
		  // right triangle 1 reflection = source triangle rotation !!
		  // TODO skip completely if off-screen
		  xmin = x1 + yr * xslope;
		  xmax = x1 + wt - yr * xslope;
		  if (xmin < 0)
		    xmin = 0;
		  if (xmax >= width)
		    xmax = width - 1;
		  if (xmax >= 0 && xmin < width)
		    {
		      for (x = xmin; x <= xmax; x++)
			{
			  // rotation +2a
			  xr = x - x1;	// <0 for left triangle 2
			  xrs = xr * cos_2a - yr * sin_2a;
			  yrs = xr * sin_2a + yr * cos_2a;
			  xs = x1 + xrs;
			  ys = y1 + yrs;
			  if (xs >= 0 && xs < width && ys >= 0 && ys < height)
			    {
			      yi2 = width * ys;	// For rotation
			      yuv[0][yi + x] =
				yuv[0][yi2 + xs] * light_coeff * light_coeff;
			      yuv[1][yi + x] = yuv[1][yi2 + xs];
			      yuv[2][yi + x] = yuv[2][yi2 + xs];
			    }
			}
		    }
		}
	    }
	}
      // Defeat rounding, redraw edge; ugly but works
      if (border_size > 0)
	{
	  fill_rectangle (yuv, width, height, x0 - wt + border_size, y0,
			  x0 - border_size, y0 + border_size,
			  border_y * light_coeff * light_coeff, border_u,
			  border_v);
	  fill_rectangle (yuv, width, height, x1 + border_size, y0,
			  x1 + wt - border_size, y0 + border_size,
			  border_y * light_coeff * light_coeff, border_u,
			  border_v);
	}

      // Third triangular reflection
      // One half of the triangle on each side
      // Results in finished rectangular area, easier to further mirror
      for (yr = 0; yr <= ht; yr++)
	{
	  y = y0 + yr;
	  if (y >= 0 && y < height)
	    {
	      yi = y * width;
	      yr2 = y - y2;	// Relative to bottom of original triangle

	      // Half right triangle 3
	      // right triangle 2 reflection = right triangle 1 rotation !!
	      // needs 1st y loop to be complete, hence has its own y loop
	      // only left half needed ?
	      // TODO skip completely if off-screen
	      xmin = x1 + wt - yr * xslope;	// From top
	      xmax = x1 + wt;	// +yr*xslope; // comment out slope for left half only
	      if (xmin < 0)
		xmin = 0;	// should not happen on right reflections
	      if (xmax < 0)
		xmax = 0;
	      if (xmax >= width)
		xmax = width - 1;
	      if (xmin >= width)
		xmin = width - 1;
	      for (x = xmin; x <= xmax; x++)
		{
		  xr2 = x - (x2 + wt);
		  // rotation -2a
		  xrs = xr2 * cos_2a + yr2 * sin_2a;
		  yrs = -xr2 * sin_2a + yr2 * cos_2a;
		  xs = x2 + wt + xrs;
		  ys = y2 + yrs;
		  if (xs >= 0 && xs < width && ys >= 0 && ys < height)
		    {
		      yi2 = width * ys;
		      yuv[0][yi + x] =
			yuv[0][yi2 + xs] * light_coeff * light_coeff;
		      yuv[1][yi + x] = yuv[1][yi2 + xs];
		      yuv[2][yi + x] = yuv[2][yi2 + xs];
		    }
		}

	      // Half left triangle 3
	      // left triangle 2 reflection = left triangle 2 rotation !!
	      // TODO skip completely if off-screen
	      xmin = x0 - wt;	// Right half-triangle only
	      xmax = x0 - wt + yr * xslope;
	      if (xmin < 0)
		xmin = 0;
	      if (xmax < 0)
		xmax = 0;
	      if (xmax >= width)
		xmax = width - 1;
	      if (xmin >= width)
		xmin = width - 1;
	      for (x = xmin; x <= xmax; x++)
		{
		  xr2 = x - (x2 - wt);	// <0 for left triangle 3
		  // rotation -2a
		  xrs = xr2 * cos_2a - yr2 * sin_2a;
		  yrs = xr2 * sin_2a + yr2 * cos_2a;
		  xs = x2 - wt + xrs;
		  ys = y2 + yrs;
		  if (xs >= 0 && xs < width && ys >= 0 && ys < height)
		    {
		      yi2 = width * ys;
		      yuv[0][yi + x] =
			yuv[0][yi2 + xs] * light_coeff * light_coeff;
		      yuv[1][yi + x] = yuv[1][yi2 + xs];
		      yuv[2][yi + x] = yuv[2][yi2 + xs];
		    }
		}
	    }
	}
      // Edge redrawing deferred to attenuation ugly fix
    }

  if (type == 2 && ht > 0)
    {				// Diagonally split rectangle pseudo-reflection
      // TODO real reflection would require a different transform (keep pseudo as an option for visual fun)
      for (yr = 0; yr <= ht; yr++)
	{
	  y = y0 + yr;
	  if (y >= 0 && y < height && y2 - yr >= 0 && y2 - yr < height)
	    {
	      yi = y * width;
	      yi2 = (y2 - yr) * width;
	      for (xr = 0; xr < yr * wt / ht; xr++)
		{
		  if (x0 + xr >= 0 && x0 + xr < width && x1 - xr >= 0
		      && x1 - xr < width)
		    {
		      yuv[0][yi + x0 + xr] =
			yuv[0][yi2 + x1 - xr] * light_coeff;
		      yuv[1][yi + x0 + xr] = yuv[1][yi2 + x1 - xr];
		      yuv[2][yi + x0 + xr] = yuv[2][yi2 + x1 - xr];
		    }
		}
	    }
	}
    }

  if (type == 0 || type == 2)
    {				// Rectangle - needs first lateral reflections before copy
      xmax = x0;
      xmin = x1;
      if (xmax >= 0)
	{			// Extra space left of initial reflections
	  for (yr = 0; yr <= ht; yr++)
	    {
	      y = y0 + yr;
	      if (y >= 0 && y < height)
		{
		  yi = y * width;
		  for (xr = wt; xr >= 0; xr--)
		    {		// Reverse scan (leftward)
		      xs = xmax + xr;
		      x = xmax - xr;
		      if (x >= 0 && x < width && xs >= 0 && xs < width)
			{
			  yuv[0][yi + x] = yuv[0][yi + xs] * light_coeff;
			  yuv[1][yi + x] = yuv[1][yi + xs];
			  yuv[2][yi + x] = yuv[2][yi + xs];
			}
		    }
		}
	    }
	}
      if (xmin < width)
	{			// Extra space right of initial reflections
	  for (yr = 0; yr <= ht; yr++)
	    {
	      y = y0 + yr;
	      if (y >= 0 && y < height)
		{
		  yi = y * width;
		  for (xr = 0; xr <= wt; xr++)
		    {		// Forward scan (rightward)
		      xs = xmin - xr;
		      x = xmin + xr;
		      if (x >= 0 && x < width && xs >= 0 && xs < width)
			{
			  yuv[0][yi + x] = yuv[0][yi + xs] * light_coeff;
			  yuv[1][yi + x] = yuv[1][yi + xs];
			  yuv[2][yi + x] = yuv[2][yi + xs];
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
  x_offset = 2 * wt;
  xmax = x0 - 1 - wt;		// Start one pixel left of first reflection, right to left
  xmin = x1 + 1 + wt;		// Start one pixel right of first reflection, left to right
  if (type == 1)
    {
      x_offset = 3 * wt;
    }
  if (xmax >= 0)
    {				// Extra space left of initial reflections
      for (yr = 0; yr <= ht; yr++)
	{
	  y = y0 + yr;
	  if (y >= 0 && y < height)
	    {
	      yi = y * width;
	      for (x = xmax; x >= 0; x--)
		{		// Reverse scan so it can copy itself
		  // Main pattern is 1 or 3 * wt wide and starts at x0-wt
		  yuv[0][yi + x] =
		    yuv[0][yi + x + x_offset] * light_coeff * light_coeff;
		  yuv[1][yi + x] = yuv[1][yi + x + x_offset];
		  yuv[2][yi + x] = yuv[2][yi + x + x_offset];
		}
	    }
	}
    }
  if (xmin < width)
    {				// Extra space right of initial reflections
      for (yr = 0; yr <= ht; yr++)
	{
	  y = y0 + yr;
	  if (y >= 0 && y < height)
	    {
	      yi = y * width;
	      for (x = xmin; x < width; x++)
		{		// Forward scan so it can copy itself
		  // Main pattern is 1 or 3 * wt wide and starts at x0-wt
		  yuv[0][yi + x] =
		    yuv[0][yi + x - x_offset] * light_coeff * light_coeff;
		  yuv[1][yi + x] = yuv[1][yi + x - x_offset];
		  yuv[2][yi + x] = yuv[2][yi + x - x_offset];
		}
	    }
	}
    }

  if (type == 1)
    {				// Ugly partial fix for wrong attenuation
      // Multiples copies may have occurred, this only fixes the 1st one
      // This fix should be applied after 1st and before 2nd copy instead of after all copies
      for (yr = 0; yr <= ht; yr++)
	{
	  y = y0 + yr;
	  if (y >= 0 && y < height)
	    {
	      yi = y * width;
	      yr2 = y - y2;	// Relative to bottom of original triangle

	      // Half right triangle 3
	      // right triangle 2 reflection = right triangle 1 rotation !!
	      // needs 1st y loop to be complete, hence has its own y loop
	      // only left half needed ?
	      // TODO skip completely if off-screen
	      xmin = x1 + wt;	// -yr*xslope; // From top
	      xmax = x1 + wt + yr * xslope;	// comment out slope for left half only
	      if (xmin < 0)
		xmin = 0;	// should not happen on right reflections
	      if (xmax < 0)
		xmax = 0;
	      if (xmax >= width)
		xmax = width - 1;
	      if (xmin >= width)
		xmin = width - 1;
	      // if (xmax>width) xmax=width; // should not happen on left reflections
	      for (x = xmin; x <= xmax; x++)
		{
		  xr2 = x - (x2 + wt);
		  // rotation -2a
		  xrs = xr2 * cos_2a + yr2 * sin_2a;
		  yrs = -xr2 * sin_2a + yr2 * cos_2a;
		  xs = x2 + wt + xrs;
		  ys = y2 + yrs;
		  if (xs >= 0 && xs < width && ys >= 0 && ys < height)
		    {
		      yi2 = width * ys;
		      yuv[0][yi + x] =
			yuv[0][yi2 + xs] * light_coeff * light_coeff;
		      yuv[1][yi + x] = yuv[1][yi2 + xs];
		      yuv[2][yi + x] = yuv[2][yi2 + xs];
		    }
		}
	      // Half left triangle 3
	      // left triangle 1 reflection = left triangle 2 rotation !!
	      // TODO skip completely if off-screen
	      xmin = x0 - wt - yr * xslope;	// Right half-triangle only
	      xmax = x0 - wt;	// +yr*xslope;
	      if (xmin < 0)
		xmin = 0;
	      if (xmax < 0)
		xmax = 0;
	      if (xmax >= width)
		xmax = width - 1;
	      if (xmin >= width)
		xmin = width - 1;
	      for (x = xmin; x <= xmax; x++)
		{
		  xr2 = x - (x2 - wt);	// <0 for left triangle 3
		  // rotation -2a
		  xrs = xr2 * cos_2a - yr2 * sin_2a;
		  yrs = xr2 * sin_2a + yr2 * cos_2a;
		  xs = x2 - wt + xrs;
		  ys = y2 + yrs;
		  if (xs >= 0 && xs < width && ys >= 0 && ys < height)
		    {
		      yi2 = width * ys;
		      yuv[0][yi + x] =
			yuv[0][yi2 + xs] * light_coeff * light_coeff;
		      yuv[1][yi + x] = yuv[1][yi2 + xs];
		      yuv[2][yi + x] = yuv[2][yi2 + xs];
		    }
		}
	    }
	}
      // Defeat rounding, redraw edge; ugly but works
      if (border_size > 0)
	{
	  fill_rectangle (yuv, width, height, x2 - 2 * wt + border_size, y2,
			  x2 - wt - border_size, y2 - border_size,
			  border_y * light_coeff * light_coeff * light_coeff,
			  border_u, border_v);
	  fill_rectangle (yuv, width, height, x2 + wt + border_size, y2,
			  x2 + 2 * wt - border_size, y2 - border_size,
			  border_y * light_coeff * light_coeff * light_coeff,
			  border_u, border_v);
	}

    }
  //////////////////////////
  // vertical reflections //
  //////////////////////////
  if (y0 > 0)
    {				// First reflections above original
      for (y = y0; y > y0 - ht && y >= 0; y--)
	{
	  yi = y * width;
	  yr = y - y0;
	  yi2 = (y0 - yr) * width;	// Source 1 triangle height below, inverted
	  for (x = 0; x < width; x++)
	    {
	      yuv[0][yi + x] = yuv[0][yi2 + x] * light_coeff;
	      yuv[1][yi + x] = yuv[1][yi2 + x];
	      yuv[2][yi + x] = yuv[2][yi2 + x];

	    }
	}
    }

  if (y2 < height)
    {				// First reflections below original
      for (y = y2; y < y2 + ht && y < height; y++)
	{
	  yi = y * width;
	  yr = y - y2;		// will remain positive
	  ys = y2 - yr;
	  yi2 = ys * width;	// Source 1 triangle height above, inverted
	  for (x = 0; x < width; x++)
	    {
	      yuv[0][yi + x] = yuv[0][yi2 + x] * light_coeff;
	      yuv[1][yi + x] = yuv[1][yi2 + x];
	      yuv[2][yi + x] = yuv[2][yi2 + x];

	    }
	}
    }

  if (y0 > ht)
    {				// Extra space above initial reflections
      for (y = y0 - ht; y >= 0; y--)
	{
	  yi = y * width;
	  yi2 = yi + 2 * ht * width;
	  for (x = 0; x < width; x++)
	    {
	      yuv[0][yi + x] = yuv[0][yi2 + x] * light_coeff * light_coeff;
	      yuv[1][yi + x] = yuv[1][yi2 + x];
	      yuv[2][yi + x] = yuv[2][yi2 + x];

	    }
	}
    }

  if (y2 + ht < height)
    {				// Extra space below initial reflections
      // TODO uses vj_frame_copy; is there a vj_blit?
      // int len = width*height;
      // int strides[4] = { len,len,len, 0 };
      // vj_frame_copy( yuv, buf, strides );
      for (y = y2 + ht; y < height; y++)
	{
	  yi = y * width;
	  yi2 = yi - 2 * ht * width;
	  for (x = 0; x < width; x++)
	    {
	      yuv[0][yi + x] = yuv[0][yi2 + x] * light_coeff * light_coeff;
	      yuv[1][yi + x] = yuv[1][yi2 + x];
	      yuv[2][yi + x] = yuv[2][yi2 + x];

	    }
	}
    }

}

void
kaleidoscope_apply (VJFrame * frame, int type, int wt, int border,
		    int border_r, int border_g, int border_b, int light_pct)
{
  kaleidoscope (frame, type, wt, border, border_r, border_g, border_b,
		light_pct);
  frame_index++;
}
