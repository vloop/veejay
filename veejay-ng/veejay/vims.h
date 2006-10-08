/* veejay - Linux VeeJay
 *           (C) 2002-2004 Niels Elburg <nelburg@looze.net> 
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#ifndef VIMS_H
#define VIMS_H

/*
 * VIMS selectors
 */

enum {
	VIMS_MAX		=	650,
};

enum {
/* general controls */
	VIMS_FULLSCREEN				=	40,
	VIMS_QUIT				=	600,
	VIMS_BEZERK				=	11,
	VIMS_DEBUG_LEVEL			=	12,
	VIMS_RESIZE_SCREEN			=	41,
	VIMS_SCREENSHOT				=	13,
	VIMS_SAMPLE_NEW				=	100,
	VIMS_SAMPLE_SELECT			=	101,
	VIMS_SAMPLE_DEL				=	102,
	VIMS_SAMPLE_COPY			=	103,
	VIMS_SAMPLE_LOAD			=	104,
	VIMS_SAMPLE_SAVE			=	105,

	VIMS_SAMPLE_PLAY_FORWARD		=	110,
	VIMS_SAMPLE_PLAY_BACKWARD		=	111,
	VIMS_SAMPLE_PLAY_STOP			=	112,
	VIMS_SAMPLE_SKIP_FRAME			=	113,
	VIMS_SAMPLE_PREV_FRAME			=	114,
	VIMS_SAMPLE_SKIP_SECOND			=	115,
	VIMS_SAMPLE_PREV_SECOND			=	116,
	VIMS_SAMPLE_GOTO_START			=	117,
	VIMS_SAMPLE_GOTO_END			=	118,
	VIMS_SAMPLE_SET_FRAME			=	119,
	VIMS_SAMPLE_SET_SPEED			=	120,
	VIMS_SAMPLE_SET_SLOW			=	121,
	VIMS_SAMPLE_SET_PROPERTIES		=	123,
	VIMS_SAMPLE_EDL_PASTE_AT		=	150,
	VIMS_SAMPLE_EDL_COPY			=	151,
	VIMS_SAMPLE_EDL_DEL			=	152,
	VIMS_SAMPLE_EDL_CROP			=	153,
	VIMS_SAMPLE_EDL_CUT			=	154,
	VIMS_SAMPLE_EDL_ADD			=	155,
	VIMS_SAMPLE_EDL_SAVE			=	158,
	VIMS_SAMPLE_EDL_LOAD			=	159,
	VIMS_SAMPLE_EDL_EXPORT			=	156,
	VIMS_SAMPLE_CHAIN_SET_ACTIVE		=	130,
	VIMS_SAMPLE_CHAIN_CLEAR			=	131,
	VIMS_SAMPLE_CHAIN_LIST			=	132,
	VIMS_SAMPLE_CHAIN_SET_ENTRY		=	133,
	VIMS_SAMPLE_CHAIN_ENTRY_SET_FX		=	134,
	VIMS_SAMPLE_CHAIN_ENTRY_SET_PRESET	=	135,
	VIMS_SAMPLE_CHAIN_ENTRY_SET_ACTIVE	=	136,
	VIMS_SAMPLE_CHAIN_ENTRY_SET_VALUE	=	137,
	VIMS_SAMPLE_CHAIN_ENTRY_SET_INPUT	=	138,
	VIMS_SAMPLE_CHAIN_ENTRY_CLEAR		=	139,
	VIMS_SAMPLE_ATTACH_OUT_PARAMETER	=	180,
	VIMS_SAMPLE_DETACH_OUT_PARAMETER	=	181,
	
	VIMS_SAMPLE_BIND_OUTP_OSC		=	190,
	VIMS_SAMPLE_RELEASE_OUTP_OSC		=	191,
	VIMS_SAMPLE_OSC_START			=	192,
	VIMS_SAMPLE_OSC_STOP			=	193,
	
	VIMS_SAMPLE_CONFIGURE_RECORDER		=	124,
	VIMS_SAMPLE_START_RECORDER		=	125,
	VIMS_SAMPLE_STOP_RECORDER		=	126,

	VIMS_PERFORMER_SETUP_PREVIEW		=	20,
	VIMS_PERFORMER_GET_PREVIEW		=	21,

	VIMS_SAMPLEBANK_LIST			=	1,
	VIMS_SAMPLEBANK_ADD			=	2,
	VIMS_SAMPLEBANK_DEL			=	3,

	VIMS_GET_FRAME				=	5,

	VIMS_SET_PORT				=	99,
	VIMS_GET_PORT				=	97,
	
};
#endif