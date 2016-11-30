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

#ifndef KALEIDOSCOPE_H
#define KALEIDOSCOPE_H
vj_effect *kaleidoscope_init();
void kaleidoscope_apply( VJFrame *frame, int with_triangles, int wt, int border, int border_r, int border_g, int border_b, int light_pct);
void kaleidoscope_free();
int fill_rectangle (uint8_t ** yuv, const unsigned int width,
		const unsigned int height, const int x0, const int y0,
		const int x1, const int y1, const uint8_t Y, const uint8_t u,
		const uint8_t v);
int fill_hparall (uint8_t ** yuv, const unsigned int width,
	      const unsigned int height, const int x0, const int y0,
	      const int x1, const int y1, const int xsize, const uint8_t Y,
	      const uint8_t u, const uint8_t v);
int showhexdigits (uint8_t ** yuv, const int width, const int height, const unsigned int digits,
	       const int x0, const int y0, const int seglen, const int Y,
	       const int Cb, const int Cr, const float slope, const unsigned int bold);

#endif

