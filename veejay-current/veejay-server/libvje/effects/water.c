/* EffecTV - Realtime Digital Video Effektor
 * Copyright (C) 2001-2003 FUKUCHI Kentaro
 *
 * RippleTV - Water ripple effect
 * Copyright (C) 2001 - 2002 FUKUCHI Kentaro
 * 
 * ported to Linux VeeJay by:
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


#include <config.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <libvjmem/vjmem.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include "common.h"
#include "rippletv.h"
#include "softblur.h"

typedef struct {
	uint8_t *ripple_data[4];
	uint8_t *diff_img;
 	int stat;
	signed char *vtable;
	int *map;
	int *map1;
	int *map2;
	int *map3;
	int map_h;
	int map_w;
	int have_img;
	int sqrtable[256];
	int point; 
	int impact;
	int last_fresh_rate;
	int loopnum;
	int lastmode;
	unsigned int wfastrand_val;
} water_t;

/*static uint8_t *ripple_data[3];
static int stat;
static signed char *vtable;
static int *map;
static int *map1, *map2, *map3;
static int map_h, map_w;
static uint8_t *diff_img = NULL;
static int have_img = 0;
static int sqrtable[256];
static const int point = 16;
static const int impact = 2;
//static const int loopnum = 2;
static int bgIsSet = 0;
*/

/* from EffecTV:
 * fastrand - fast fake random number generator
 * Warning: The low-order bits of numbers generated by fastrand()
 *          are bad as random numbers. For example, fastrand()%4
 *          generates 1,2,3,0,1,2,3,0...
 *          You should use high-order bits.
 */

static unsigned int wfastrand(water_t *w)
{
	return (w->wfastrand_val=w->wfastrand_val*1103515245+12345);
}

static void setTable(water_t *w)
{
	int i;

	for(i=0; i<128; i++) {
		w->sqrtable[i] = i*i;
	}

	for(i=1; i<=128; i++) {
		w->sqrtable[256-i] = -i*i;
	}
}
//easy/calm: 10,2,32
//flimmerin: 10,2,3
//flowing: 10,1,29
//p2 = wave speed
//

vj_effect *water_init(int width, int height)
{
    vj_effect *ve = (vj_effect *) vj_calloc(sizeof(vj_effect));
    ve->num_params = 5;
    ve->defaults = (int *) vj_calloc(sizeof(int) * ve->num_params);	/* default values */
    ve->limits[0] = (int *) vj_calloc(sizeof(int) * ve->num_params);	/* min */
    ve->limits[1] = (int *) vj_calloc(sizeof(int) * ve->num_params);	/* max */
    ve->limits[0][0] = 1;
    ve->limits[1][0] = 3600;
    ve->limits[0][1] = 1;
    ve->limits[1][1] = 16;
    ve->limits[0][2] = 1;
    ve->limits[1][2] = 31; //number of waves
    ve->limits[0][3] = 0;
    ve->limits[1][3] = 6;    // mode
    ve->limits[0][4] = 0;
    ve->limits[1][4] = 255; // threshold
    ve->defaults[0] = 10;
    ve->defaults[1] = 1;
    ve->defaults[2] = 10;
    ve->defaults[3] = 1;
    ve->defaults[4] = 45;
    ve->description = "Water ripples";
    ve->sub_format = 0;
    ve->extra_frame = 1;
    ve->has_user = 1;
    ve->user_data = NULL;
	ve->motion = 1;

	ve->param_description= vje_build_param_list(ve->num_params, "Refresh Frequency",
			"Wavespeed", "Decay", "Mode", "Threshold (motion)");

	ve->hints = vje_init_value_hint_list( ve->num_params );

	vje_build_value_hint_list( ve->hints, ve->limits[1][3], 3, 
			"Raindrops", "Motion detect I (preview)", "Water ripples", "Motion detect II (preview)",
		    "Drops and Ripples", "Motion detect III (preview)", "Decaying Motion"	);

    return ve;
}
#define HIS_LEN (8*25)
#define HIS_DEFAULT 2

static uint32_t histogram_[HIS_LEN];
static int n__ = 0;
static uint32_t keyv_ = 0;
static uint32_t keyp_ = 0;
int water_malloc(void **d, int width, int height)
{
	*d = (void*) vj_calloc(sizeof(water_t));
	water_t *w = (water_t*) *d;

	w->ripple_data[0] = (uint8_t*)vj_calloc(sizeof(uint8_t) * RUP8(width * height));
	if(!w->ripple_data[0]) return 0;

	w->diff_img = (uint8_t*)vj_calloc(sizeof(uint8_t) * RUP8(width * height * 2));
	if(!w->diff_img) return 0;

	w->map_h = height / 2 + 1;
	w->map_w = width / 2 + 1;

	w->map = (int*) vj_calloc (sizeof(int) * RUP8( w->map_h * w->map_w * 3));
	if(!w->map) return 0;

	w->vtable = (signed char*) vj_calloc( sizeof(signed char) * RUP8(w->map_w * w->map_h * 2));
	if(!w->vtable) return 0;

	w->map3 = w->map + w->map_w * w->map_h * 2;

	setTable(w);

	w->map1 = w->map;
	w->map2 = w->map + w->map_h*w->map_w;
	w->stat = 1;
	w->point = 16;
	w->impact = 2;
	w->loopnum = 2;
  
	return 1;
}

void water_free(void *ud) {

	water_t *w = (water_t*) ud;
	if(w) {
		if(w->ripple_data[0]) free(w->ripple_data[0]);
		if(w->map) free(w->map);	
		if(w->diff_img) free(w->diff_img);
		if(w->vtable) free(w->vtable);
		free(w);
	}
	w = NULL;
}


static inline void drop(water_t *w,int power)
{
	int x, y;
	int *p, *q;

	x = wfastrand(w)%(w->map_w-4)+2;
	y = wfastrand(w)%(w->map_h-4)+2;
	p = w->map1 + y*w->map_w + x;
	q = w->map2 + y*w->map_w + x;
	*p = power;
	*q = power;
	*(p-w->map_w) = *(p-1) = *(p+1) = *(p+w->map_w) = power/2;
	*(p-w->map_w-1) = *(p-w->map_w+1) = *(p+w->map_w-1) = *(p+w->map_w+1) = power/4;
	*(q-w->map_w) = *(q-1) = *(q+1) = *(q+w->map_w) = power/2;
	*(q-w->map_w-1) = *(q-w->map_w+1) = *(q+w->map_w-1) = *(p+w->map_w+1) = power/4;
}

static	void	drawmotionframe( VJFrame *f , water_t *w )
{
	int strides[4] = { 0, f->uv_len,f->uv_len, 0 };
	vj_frame_clear( f->data, strides, 128 );
	vj_frame_clear( f->data, strides, 128 );
	vj_frame_copy1( w->diff_img, f->data[0], f->width * f->height );
//	veejay_memcpy( f->data[0], w->diff_img, f->width * f->height );
}

/* globalactivity not used */
/*
static	int	globalactivity(VJFrame *f2, water_t *w, int in)
{
	int len = (f2->width * f2->height)/4;
	uint32_t sum = 0,min=0xffff,max=0;
	uint64_t activity_level1 = 0;
	uint64_t activity_level2 = 0;
	uint64_t activity_level3 = 0;
	uint64_t activity_level4 = 0;
	uint8_t *binary_img = w->diff_img;
	int i = 0;
	for( i = 0; i < len; i += 4 )
	{
		
		activity_level1 += binary_img[i];
		activity_level2 += binary_img[i+1];
		activity_level3 += binary_img[i+2];
		activity_level4 += binary_img[i+3];
	}
	uint32_t activity_level = ( (activity_level1>>8) + (activity_level2>>8) + (activity_level3>>8) + (activity_level4>>8));

	int current_his_len = 8;
	
	histogram_[ (n__%current_his_len) ] = activity_level;

	for( i = 0; i < current_his_len; i ++ )
	{
		sum += histogram_[i];
		if(histogram_[i] > max ) max = histogram_[i];
		if(histogram_[i] < min ) min = histogram_[i];
	}	
	if( (n__ % current_his_len)==0 )
	{
		keyp_ = keyv_;
		keyv_ = (sum > 0 ? (sum/current_his_len):0 );
	}

	if( n__ <= 1 )
		return in;

	int tmp = (( n__ - 1) % current_his_len) + 1;
	float q = 1.0f/(float) current_his_len * tmp;
	float diff = (float) keyv_ - (float) keyp_;
	float pu = keyp_ + ( q* diff);
	float wu = 1.0f/31;
	float pw = wu * pu;

	int res = (30 * pw);
	if( res < 1 )
		return 1;
	return res;
}
*/

static  void    motiondetect(VJFrame *f, VJFrame *f2, int threshold, water_t *w)
{
        const int len = f->len;
        uint8_t *bg = w->diff_img + len;
        uint8_t *in = f2->data[0];
        if(!w->have_img)
        {
                //softblur_apply( f2, 0);
                //veejay_memcpy(bg, f2->data[0], len );
                vj_frame_copy1( f2->data[0],bg, len );
		w->have_img = 1;
                return;
        }

        int i;
        uint8_t pp1;
        for(i = 0; i < len ; i ++ ) {
                pp1 = abs(bg[i] - in[i]);
                if(pp1 > threshold ) {
                        w->diff_img[i] = pp1;
                } else {
                        w->diff_img[i] = 0;
// (w->diff_img[i] + 0)>>1;
                } 
        }

        int *p = w->map1 + w->map_w + 1;
        int *q = w->map2 + w->map_w + 1;        
        int width = f->width;
        int x,y,h;
        uint8_t *d = w->diff_img + width + 2;
        for( y = w->map_h - 2; y > 0 ; y -- ) { 
                for( x = w->map_w - 2 ; x > 0 ; x -- ) {
                        h = (int) *d + (int) *(d+1) + (int) *(d+width) + (int) *(d+width+1);
                        if(h>0) {
                                *p = h << ( w->point + w->impact - 8 );
                                *q = *p;                        
                        }
                        p ++; q ++;
                        d += 2;
                }
                d += width + 2;
                p += 2;
                q += 2; 
        }
        
}


static	void	motiondetect2(VJFrame *f, VJFrame *f2, int threshold, water_t *w)
{
	const int len = f->len;
	uint8_t *bg = w->diff_img + len;
	uint8_t *in = f2->data[0];
	if(!w->have_img)
	{
		softblur_apply( f2, 0);
		//	veejay_memcpy(bg, f2->data[0], len );
		vj_frame_copy1( f2->data[0], bg, len );
		w->have_img = 1;
		return;
	}

	int i;
	uint8_t pp1;
	for(i = 0; i < len ; i ++ ) {
		pp1 = abs(bg[i] - in[i]);
		if(pp1 > threshold ) {
			w->diff_img[i] = 0xff-pp1;
		} else {
			w->diff_img[i] = 0;
// (w->diff_img[i] + 0)>>1;
		} 
	}

	int *p = w->map1 + w->map_w + 1;
	int *q = w->map2 + w->map_w + 1;	
	int width = f->width;
	int x,y,h;
	uint8_t *d = w->diff_img + width + 2;
	for( y = w->map_h - 2; y > 0 ; y -- ) {	
		for( x = w->map_w - 2 ; x > 0 ; x -- ) {
			h = (int) *d + (int) *(d+1) + (int) *(d+width) + (int) *(d+width+1);
			if(h>0) {
				*p = h << ( w->point + w->impact - 8 );
				*q = *p;			
			}
			p ++; q ++;
			d += 2;
		}
		d += width + 2;
		p += 2;
		q += 2;	
	}
	
}


static	void	motiondetect3(VJFrame *f, VJFrame *f2, int threshold, water_t *w)
{
	const int len= f->len;
	uint8_t *bg = w->diff_img + len;
	uint8_t *in = f2->data[0];
	if(!w->have_img)
	{
		softblur_apply( f2, 0);
	//	veejay_memcpy(bg, f2->data[0], len );
		vj_frame_copy1( f2->data[0], bg, len );
		w->have_img = 1;
		return;
	}

	int i;
	uint8_t pp1;
	for(i = 0; i < len ; i ++ ) {
		pp1 = abs(bg[i] - in[i]);
		if(pp1 < threshold ) {
			w->diff_img[i] = pp1;
		} else {
			w->diff_img[i] = 0;
	// (w->diff_img[i] + 0)>>1;
		} 
	}

	int *p = w->map1 + w->map_w + 1;
	int *q = w->map2 + w->map_w + 1;	
	int width = f->width;
	int x,y,h;
	uint8_t *d = w->diff_img + width + 2;
	for( y = w->map_h - 2; y > 0 ; y -- ) {	
		for( x = w->map_w - 2 ; x > 0 ; x -- ) {
			h = (int) *d + (int) *(d+1) + (int) *(d+width) + (int) *(d+width+1);
			if(h>0) {
				*p = h << ( w->point + w->impact - 8 );
				*q = *p;			
			}
			p ++; q ++;
			d += 2;
		}
		d += width + 2;
		p += 2;
		q += 2;	
	}
	
}	
	

static void raindrop(water_t *w)
{
	static int period = 0;
	static int rain_stat = 0;
	static unsigned int drop_prob = 0;
	static int drop_prob_increment = 0;
	static int drops_per_frame_max = 0;
	static int drops_per_frame = 0;
	static int drop_power = 0;

	int i;

	if(period == 0) {
		switch(rain_stat) {
		case 0:
			period = (wfastrand(w)>>23)+100;
			drop_prob = 0;
			drop_prob_increment = 0x00ffffff/period;
			drop_power = (-(wfastrand(w)>>28)-2)<<w->point;
			drops_per_frame_max = 2<<(wfastrand(w)>>30); // 2,4,8 or 16
			rain_stat = 1;
			break;
		case 1:
			drop_prob = 0x00ffffff;
			drops_per_frame = 1;
			drop_prob_increment = 1;
			period = (drops_per_frame_max - 1) * 16;
			rain_stat = 2;
			break;
		case 2:
			period = (wfastrand(w)>>22)+1000;
			drop_prob_increment = 0;
			rain_stat = 3;
			break;
		case 3:
			period = (drops_per_frame_max - 1) * 16;
			drop_prob_increment = -1;
			rain_stat = 4;
			break;
		case 4:
			period = (wfastrand(w)>>24)+60;
			drop_prob_increment = -(drop_prob/period);
			rain_stat = 5;
			break;
		case 5:
		default:
			period = (wfastrand(w)>>23)+500;
			drop_prob = 0;
			rain_stat = 0;
			break;
		}
	}
	switch(rain_stat) {
	default:
	case 0:
		break;
	case 1:
	case 5:
		if((wfastrand(w)>>8)<drop_prob) {
			drop(w,drop_power);
		}
		drop_prob += drop_prob_increment;
		break;
	case 2:
	case 3:
	case 4:
		for(i=drops_per_frame/16; i>0; i--) {
			drop(w,drop_power);
		}
		drops_per_frame += drop_prob_increment;
		break;
	}
	period--;
}

void	water_apply(void *user_data, VJFrame *frame, VJFrame *frame2, int width, int height, int fresh_rate, int loopnum, int decay, int mode, int threshold )
{
	int x, y, i;
	int dx, dy;
	int h, v;
	int wi, hi;
	int *p, *q, *r;
	signed char *vp;
	uint8_t *src,*dest;
	const int len = frame->len;
	water_t *w = (water_t*) user_data;

	if(w->last_fresh_rate != fresh_rate)
	{
		w->last_fresh_rate = fresh_rate;
		veejay_memset( w->map, 0, (w->map_h*w->map_w*2*sizeof(int)));
	}
	if(w->lastmode != mode )
	{
		veejay_memset( w->map, 0, (w->map_h*w->map_w*2*sizeof(int)));
		w->have_img = 0;
		w->lastmode = mode;
	}
	vj_frame_copy1( frame->data[0], w->ripple_data[0],len);

	dest = frame->data[0];
	src = w->ripple_data[0];

	w->loopnum = loopnum;

	switch(mode) {
		case 0: raindrop(w); w->have_img = 0; break;
		case 1: 
			motiondetect(frame,frame2,threshold,w);	
			drawmotionframe(frame,w);
			return; 
		case 2: motiondetect(frame,frame2,threshold,w);
			break;
		case 3:
			motiondetect2(frame,frame2,threshold,w);
			drawmotionframe(frame,w);
			return;
			break;
		case 4:
			motiondetect2(frame,frame2,threshold,w);
			break;
		case 5:
			motiondetect3(frame,frame2,threshold,w);
			drawmotionframe(frame,w);
			return;	
		case 6:
			motiondetect3(frame,frame2,threshold,w);
			break;
		default:
		break;
	}

	/* simulate surface wave */
	wi = w->map_w;
	hi = w->map_h;
	
	/* This function is called only 30 times per second. To increase a speed
	 * of wave, iterates this loop several times. */
	for(i=w->loopnum; i>0; i--) {
		/* wave simulation */
		p = w->map1 + wi + 1;
		q = w->map2 + wi + 1;
		r = w->map3 + wi + 1;
		for(y=hi-2; y>0; y--) {
			for(x=wi-2; x>0; x--) {
				h = *(p-wi-1) + *(p-wi+1) + *(p+wi-1) + *(p+wi+1)
				  + *(p-wi) + *(p-1) + *(p+1) + *(p+wi) - (*p)*9;
				h = h >> 3;
				v = *p - *q;
				v += h - (v >> decay);
				*r = v + *p;
				p++;
				q++;
				r++;
			}
			p += 2;
			q += 2;
			r += 2;
		}

		/* low pass filter */
		p = w->map3 + wi + 1;
		q = w->map2 + wi + 1;
		for(y=hi-2; y>0; y--) {
			for(x=wi-2; x>0; x--) {
				h = *(p-wi) + *(p-1) + *(p+1) + *(p+wi) + (*p)*60;
				*q = h >> 6;
				p++;
				q++;
			}
			p+=2;
			q+=2;
		}

		p = w->map1;
		w->map1 = w->map2;
		w->map2 = p;
	}

	vp = w->vtable;
	p = w->map1;
	for(y=hi-1; y>0; y--) {
		for(x=wi-1; x>0; x--) {
			/* difference of the height between two voxel. They are twiced to
			 * emphasise the wave. */
			vp[0] = w->sqrtable[((p[0] - p[1])>>(w->point-1))&0xff]; 
			vp[1] = w->sqrtable[((p[0] - p[wi])>>(w->point-1))&0xff]; 
			p++;
			vp+=2;
		}
		p++;
		vp+=2;
	}

	hi = height;
	wi = width;
	vp = w->vtable;

	for(y=0; y<hi; y+=2) {
		for(x=0; x<wi; x+=2) {
			h = (int)vp[0];
			v = (int)vp[1];
			dx = x + h;
			dy = y + v;
			if(dx<0) dx=0;
			if(dy<0) dy=0;
			if(dx>=wi) dx=wi-1;
			if(dy>=hi) dy=hi-1;
			dest[0] = src[dy*wi+dx];

			i = dx;

			dx = x + 1 + (h+(int)vp[2])/2;
			if(dx<0) dx=0;
			if(dx>=wi) dx=wi-1;
			dest[1] = src[dy*wi+dx];

			dy = y + 1 + (v+(int)vp[w->map_w*2+1])/2;
			if(dy<0) dy=0;
			if(dy>=hi) dy=h-1;
			dest[wi] = src[dy*wi+i];

			dest[wi+1] = src[dy*wi+dx];
			dest+=2;
			vp+=2;
		}
		dest += wi;
		vp += 2;
	}


}
