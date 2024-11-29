#ifndef _BLINDPPCODE_H_
#define _BLINDPPCODE_H_

#include "amDCTtypedefs.h"


void deblock_horiz(uint8_t* image, int height, int width, int stride, int quant);
void deblock_vert(uint8_t* image, int height, int width, int stride, int quant);

void deblock_horiz_DoDC(uint8_t* image, int height, int width, int stride, int quant);
void deblock_vert_DoDC(uint8_t* image, int height, int width, int stride, int quant);

void doDering(FrameInfo_args* FrameInfoArgs);
void dering(uint8_t* image, int height, int width, int quant);

#endif // _BLINDPPCODE_H_
