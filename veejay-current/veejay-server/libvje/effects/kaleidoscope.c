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
	unsigned int x, xmin, xmax, yr;
        // x absolu
        // yr relatif à la base 
	unsigned int yi; // offset dans le tableau
	uint8_t p, cb, cr;
	const float sin60 = sin(M_PI/3);
	// const float cos60 = cos(M_PI/3);
	// const float tan30 = tan(M_PI/6);
        // centre de l'écran:
        const unsigned int yc = height/2;
	const unsigned int xc = width/2;
        // Largeur et hauteur du triangle
	const unsigned int wt = width/2;
	const unsigned int ht = wt*sin60;
        // x se décale de la moitié de la largeur pendant que y parcourt la hauteur
        const float xpente=0.5*wt/ht;
        // todo tronquer si le triangle dépasse l'écran	
        // x0, y0 angle supérieur gauche
        const unsigned int y0=yc-ht/2;
        const unsigned int x0=xc-wt/2;
	for (yr = 0; yr < ht; yr++) {
		yi = (y0+yr) * width;
                xmin=x0+yr*xpente;
                xmax=x0+wt-yr*xpente;
		for (x = xmin; x < xmax; x++) {
			yuv[0][yi+x] = 255;

			/* cb = yuv[1][yi + x];
			cr = yuv[2][yi + x]; */
			/* yuv[1][yi + x] = cb;
			yuv[2][yi + x] = cr; */
			
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
