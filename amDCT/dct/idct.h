/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Inverse DCT header  -
 *
 *  Copyright(C) 2001-2011 Michael Militzer <michael@xvid.org>
 *
 *  This program is free software ; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation ; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY ; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program ; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * $Id: idct.h 1986 2011-05-18 09:07:40Z Isibaar $
 *
 ****************************************************************************/

#ifndef _IDCT_H_
#define _IDCT_H_

#include "../amDCTPortab.h"

typedef void (idctFunc)(short* const block);
typedef idctFunc* idctFuncPtr;

extern idctFuncPtr idct;

// C versions are not identical to SSE2
idctFunc idct_int32; // better use this one if 
idctFunc simple_idct_c; // Michael Niedermayer

#ifdef INTEL_INTRINSICS
idctFunc idct_sse2; // converted from skal version, IEEE 1180 compliant */
#endif

#endif              /* _IDCT_H_ */
