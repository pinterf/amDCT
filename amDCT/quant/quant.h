/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - (de)Quantization related header  -
 *
 *  Copyright(C) 2003 Edouard Gomez <ed.gomez@free.fr>
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
 * $Id: quant.h 1985 2011-05-18 09:02:35Z Isibaar $
 *
 ****************************************************************************/

#ifndef _QUANT_H_
#define _QUANT_H_

#include <stdint.h>  

/*****************************************************************************
 * Common API for Intra (de)Quant functions
 ****************************************************************************/

typedef uint32_t(quant_intraFunc) (int16_t* coeff,
  const int16_t* data,
  const uint32_t quant,
  const uint32_t dcscalar,
  const uint16_t* mpeg_quant_matrices);

typedef quant_intraFunc* quant_intraFuncPtr;

/* Global function pointers */
extern quant_intraFuncPtr quant_h263_intra;
extern quant_intraFuncPtr quant_mpeg_intra;
extern quant_intraFuncPtr dequant_h263_intra;
extern quant_intraFuncPtr dequant_mpeg_intra;

/*****************************************************************************
 * Known implementation of Intra (de)Quant functions
 ****************************************************************************/

 /* Quant functions */
quant_intraFunc quant_h263_intra_c;
quant_intraFunc quant_mpeg_intra_c;

#ifdef INTEL_INTRINSICS
quant_intraFunc quant_h263_intra_sse2;
quant_intraFunc quant_mpeg_intra_sse2;
#endif

/* DeQuant functions */
quant_intraFunc dequant_h263_intra_c;
quant_intraFunc dequant_mpeg_intra_c;

#ifdef INTEL_INTRINSICS
quant_intraFunc dequant_h263_intra_sse2;
quant_intraFunc dequant_mpeg_intra_sse2;
#endif


/*****************************************************************************
 * Common API for Inter (de)Quant functions
 ****************************************************************************/

typedef uint32_t(quant_interFunc) (int16_t* coeff,
  const int16_t* data,
  const uint32_t quant,
  const uint16_t* mpeg_quant_matrices);

typedef quant_interFunc* quant_interFuncPtr;

/* Global function pointers */
extern quant_interFuncPtr quant_h263_inter;
extern quant_interFuncPtr quant_mpeg_inter;
extern quant_interFuncPtr dequant_h263_inter;
extern quant_interFuncPtr dequant_mpeg_inter;

/*****************************************************************************
 * Known implementation of Inter (de)Quant functions
 ****************************************************************************/

quant_interFunc quant_h263_inter_c;
quant_interFunc quant_mpeg_inter_c;
quant_interFunc dequant_h263_inter_c;
quant_interFunc dequant_mpeg_inter_c;

#ifdef INTEL_INTRINSICS
quant_interFunc quant_h263_inter_sse2;
quant_interFunc quant_mpeg_inter_ssse3;
quant_interFunc dequant_h263_inter_sse2;
quant_interFunc dequant_mpeg_inter_sse41;
#endif

#endif /* _QUANT_H_ */
