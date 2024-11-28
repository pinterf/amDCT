
#ifndef _SMOOTHFILTERS_H_
#define _SMOOTHFILTERS_H_

#include <math.h>

#include "FrameInfo.h"

#include "amDCTtypedefs.h"

//void 	boxBlur(        FrameInfo_args *FrameInfoArgs, uint8_t radius);
//void	gaussianBlur(   FrameInfo_args *FrameInfoArgs, uint8_t radius); 
//void	gaussianBlur2(  FrameInfo_args *FrameInfoArgs, uint8_t radius); 
void	gaussianBlur(   FrameInfo_args *FrameInfoArgs, uint8_t radiusX, uint8_t radiusY); 
void    gaussianBlur2(  FrameInfo_args *FrameInfoArgs, uint8_t *src, uint8_t *dst, uint8_t radiusX, uint8_t radiusY, uint8_t strength); 
//void    LumaFrameMedian(FrameInfo_args *FrameInfoArgs, uint8_t *src, uint8_t *dst, uint8_t radius);
//void ctmf(
//        const unsigned char* const src, unsigned char* const dst,
//        const int width, const int height,
//        const int src_step, const int dst_step,
//        const int r, const int cn, const long unsigned int memsize
//        );
        
/*
 *      NOTE: LumaFrameMedian() Provides an interface to ctmf.c - Constant-time median filtering which has the copyright listed below.
 *
 * ctmf.c - Constant-time median filtering
 * Copyright (C) 2006  Simon Perreault
 *
 * Reference: S. Perreault and P. Hébert, "Median Filtering in Constant Time",
 * IEEE Transactions on Image Processing, September 2007.
 *
 * This program has been obtained from http://nomis80.org/ctmf.html. No patent
 * covers this program, although it is subject to the following license:
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact:
 *  Laboratoire de vision et systèmes numériques
 *  Pavillon Adrien-Pouliot
 *  Université Laval
 *  Sainte-Foy, Québec, Canada
 *  G1K 7P4
 *
 *  perreaul@gel.ulaval.ca
 */


#endif // _SMOOTHFILTERS_H_


