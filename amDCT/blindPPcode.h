#ifndef _BLINDPPCODE_H_
#define _BLINDPPCODE_H_

#include "amDCTtypedefs.h"


void deblock_horiz(uint8_t* image, int height, int width, int stride, int quant);
void deblock_vert(uint8_t* image, int height, int width, int stride, int quant);

void deblock_vert_lpf9(uint64_t* v_local, uint64_t* p1p2, uint8_t* v, int stride);
#ifndef ARCH_IS_X86_64
void deblock_vert_lpf9_mmx(uint64_t* v_local, uint64_t* p1p2, uint8_t* v, int stride);
#endif
void deblock_vert_lpf9_ssse3(uint64_t* v_local, uint64_t* p1p2, uint8_t* v, int stride);
void deblock_vert_lpf9_c(uint64_t* v_local, uint64_t* p1p2, uint8_t* v, int stride);

void deblock_horiz_DoDC(uint8_t* image, int height, int width, int stride, int quant);
void deblock_vert_DoDC(uint8_t* image, int height, int width, int stride, int quant);

void deblock_horiz_lpf9(uint8_t* v, int stride, int QP);
#ifndef ARCH_IS_X86_64
void deblock_horiz_lpf9_mmx(uint8_t* v, int stride, int QP);
#endif
void deblock_horiz_lpf9_ssse3(uint8_t* v, int stride, int QP);
void deblock_horiz_lpf9_c(uint8_t* v, int stride, int QP);

void doDering(FrameInfo_args* FrameInfoArgs);

void dering(uint8_t* image, int height, int width, int quant);
#ifndef ARCH_IS_X86_64
void dering_mmx(uint8_t* image, int height, int width, int quant);
#endif
void dering_sse41(uint8_t* image, int height, int width, int quant);
void dering_c(uint8_t* image, int height, int width, int quant);

#endif // _BLINDPPCODE_H_
