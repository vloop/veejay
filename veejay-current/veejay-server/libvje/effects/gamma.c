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
#include "gamma.h"

static __thread int gamma_flag = 0;
static __thread uint8_t table[256];

vj_effect *gamma_init(int w, int h)
{
    vj_effect *ve = (vj_effect *) vj_calloc(sizeof(vj_effect));
    ve->num_params = 1;
    ve->defaults = (int *) vj_calloc(sizeof(int) * ve->num_params);	/* default values */
    ve->limits[0] = (int *) vj_calloc(sizeof(int) * ve->num_params);	/* min */
    ve->limits[1] = (int *) vj_calloc(sizeof(int) * ve->num_params);	/* max */
    ve->defaults[0] = 124;
    ve->limits[0][0] = 0;
    ve->limits[1][0] = 500;
    ve->description = "Gamma Correction";
    ve->extra_frame = 0;
    ve->sub_format = -1;
	ve->has_user = 0;
	ve->param_description = vje_build_param_list(ve->num_params, "Gamma");
    ve->parallel = 1;
	return ve;
}

static void gamma_setup(int width, int height,
			double gamma_value)
{
    int i;
    double val;

    for (i = 0; i < 256; i++) {
		val = i / 256.0;
		val = pow(val, gamma_value);
		val = 256.0 * val;
		table[i] = val;
    }
}

void gamma_apply(VJFrame *frame, int gamma_value)
{
	unsigned int i;
	const int len = frame->len;
	uint8_t *Y = frame->data[0];
    
    if (gamma_value != gamma_flag) {
		gamma_setup(frame->width, frame->height, (double) (gamma_value / 100.0));
		gamma_flag = gamma_value;
	}

    for (i = 0; i < len; i++) {
		Y[i] = (uint8_t) table[Y[i]];
    }
}
