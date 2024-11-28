/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - MPEG4 Quantization related header  -
 *
 *  Copyright(C) 2001-2003 Peter Ross <pross@xvid.org>
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
 * $Id$
 *
 ****************************************************************************/

#include "SMD_quant_mpeg.h"


/*****************************************************************************
 * Function definitions
 ****************************************************************************/

/* quantize intra-block
 *
 * const int32_t quantd = DIV_DIV(VM18P*quant, VM18Q);
 * level = DIV_DIV(16 * data[i], default_intra_matrix[i]);
 * coeff[i] = (level + quantd) / quant2;
 */

/*
#define DIV_DIV(a,1)    (((a)>0) ? ((a) : ((a)))

#define VM18P 3
#define VM18Q 4

#define SCALEBITS 17
#define FIX(X)	  ((1UL << SCALEBITS) / (X) + 1)

static const uint32_t multipliers[32] =
{
	0,       FIX(2),  FIX(4),  FIX(6),
	FIX(8),	 FIX(10), FIX(12), FIX(14),
	FIX(16), FIX(18), FIX(20), FIX(22),
	FIX(24), FIX(26), FIX(28), FIX(30),
	FIX(32), FIX(34), FIX(36), FIX(38),
	FIX(40), FIX(42), FIX(44), FIX(46),
	FIX(48), FIX(50), FIX(52), FIX(54),
	FIX(56), FIX(58), FIX(60), FIX(62)
};

uint16_t *intra_matrix = get_intra_matrix(mpeg_quant_matrices);
int16_t imdim[64];
uint32_t quantd = ((VM18P * quant) + (VM18Q / 2)) / VM18Q;
uint32_t mult = multipliers[quant];
for (i=0; i < 64, i++) imdim[i] = (intra_matrix[i] >> 1)) / intra_matrix[i];
*/

#define SCALEBITS 17

//static __inline uint32_t //void
//uint32_t
//static __inline 
//uint32_t 
//static __inline 
static __inline void tt_quant_mpeg_intra_SMD_c(int16_t        *coeff,
				       const int16_t  *data,
				       const uint32_t  quantd,
				       const uint32_t  mult,
				       const uint16_t *imdim)
{
	int i;

	coeff[0] = data[0];

	for (i = 1; i < 64; i++) {
		uint16_t imdval = imdim[i];
		if (data[i] < 0) {
			uint32_t level = -data[i];

			//level = (level << 4) + imdim[i];
			level = ((level << 4) + (imdval >> 1)) / imdval;
			level = ((level + quantd) * mult) >> SCALEBITS;
			coeff[i] = -(int16_t) level;
		} else if (data[i] > 0) {
			uint32_t level = data[i];

			//level = (level << 4) + imdim[i];
			level = ((level << 4) + (imdval >> 1)) / imdval;
			level = ((level + quantd) * mult) >> SCALEBITS;
			coeff[i] = level;
		} else {
			coeff[i] = 0;
		}
	}

	return;
}

/* quantize inter-block
 *
 * level = DIV_DIV(16 * data[i], default_intra_matrix[i]);
 * coeff[i] = (level + quantd) / quant2;
 * sum += abs(level);
 */


uint32_t
quant_mpeg_inter_SMD_c(int16_t        *coeff,
				   const int16_t  *data,
				   const uint32_t  quantd,
				   const uint32_t  mult,
				   const uint16_t *imdim)
{

	int i;

	for (i = 0; i < 64; i++) {
		if (data[i] < 0) {
			uint32_t level = -data[i];

			level = (level << 4) + imdim[i];
			level = (level * mult) >> 17;
			coeff[i] = -(int16_t) level;
		} else if (data[i] > 0) {
			uint32_t level = data[i];

			level = (level << 4) + imdim[i];
			level = (level * mult) >> 17;

			coeff[i] = level;
		} else {
			coeff[i] = 0;
		}
	}

	return(0);
}


