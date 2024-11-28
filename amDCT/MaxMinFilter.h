
#ifndef _MAXMINFILTERS_H_
#define _MAXMINFILTERS_H_

#include <math.h>

#include "amDCTtypedefs.h"


void  maxFrame(FrameInfo_args *FrameInfoArgs, uint8_t *src, uint8_t *dst, uint8_t radiusX, uint8_t radiusY);
void  minFrame(FrameInfo_args *FrameInfoArgs, uint8_t *src, uint8_t *dst, uint8_t radiusX, uint8_t radiusY);

void sadWindow(FrameInfo_args *FrameInfoArgs, uint8_t *src, uint8_t *dst, uint8_t *mean, uint8_t radiusX, uint8_t radiusY); 



#endif // _MAXMINFILTERS_H_


