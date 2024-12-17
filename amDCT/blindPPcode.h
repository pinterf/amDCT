#ifndef _BLINDPPCODE_H_
#define _BLINDPPCODE_H_

#include "amDCTtypedefs.h"


void deblock_horiz(uint8_t* image, int height, int width, int stride, int quant);
void deblock_vert(uint8_t* image, int height, int width, int stride, int quant);

void deblock_vert_lpf9(uint64_t* v_local, uint64_t* p1p2, uint8_t* v, int stride);
#ifdef INTEL_INTRINSICS
void deblock_vert_lpf9_ssse3(uint64_t* v_local, uint64_t* p1p2, uint8_t* v, int stride);
#endif
void deblock_vert_lpf9_c(uint64_t* v_local, uint64_t* p1p2, uint8_t* v, int stride);

void deblock_horiz_DoDC(uint8_t* image, int height, int width, int stride, int quant);
void deblock_vert_DoDC(uint8_t* image, int height, int width, int stride, int quant);

void deblock_horiz_lpf9(uint8_t* v, int stride, int QP);
#ifdef INTEL_INTRINSICS
void deblock_horiz_lpf9_ssse3(uint8_t* v, int stride, int QP);
#endif
void deblock_horiz_lpf9_c(uint8_t* v, int stride, int QP);

void doDering(FrameInfo_args* FrameInfoArgs);

void dering(uint8_t* image, int height, int width, int quant);
#ifdef INTEL_INTRINSICS
void dering_sse41(uint8_t* image, int height, int width, int quant);
#endif
void dering_c(uint8_t* image, int height, int width, int quant);

int deblock_horiz_DC_on(uint8_t* v, int stride, int QP);
int deblock_horiz_default_filter(uint8_t* v, int stride, int QP);

int deblock_horiz_useDC(uint8_t* v, int stride, int moderate_h);
#ifdef INTEL_INTRINSICS
int deblock_horiz_useDC_sse42(uint8_t* v, int stride, int moderate_h);
#endif
int deblock_horiz_useDC_c(uint8_t* v, int stride, int moderate_h);

void deblock_vert_default_filter(uint8_t* v, int stride, int QP);
#ifdef INTEL_INTRINSICS
void deblock_vert_default_filter_sse2(uint8_t* v, int stride, int QP);
#endif
void deblock_vert_default_filter_c(uint8_t* v, int stride, int QP);


int deblock_vert_DC_on(uint8_t* v, int stride, int QP);
void deblock_vert_default_filter_ORIGINAL(uint8_t* v, int stride, int QP);

int deblock_vert_useDC(uint8_t* v, int stride, int moderate_v);
#ifdef INTEL_INTRINSICS
int deblock_vert_useDC_sse2(uint8_t* v, int stride, int moderate_v);
#endif
int deblock_vert_useDC_c(uint8_t* v, int stride, int moderate_v);

void deblock_vert_choose_p1p2(uint8_t* v, int stride, uint64_t* p1p2, int QP);
#ifdef INTEL_INTRINSICS
void deblock_vert_choose_p1p2_sse2(uint8_t* v, int stride, uint64_t* p1p2, int QP);
#endif
void deblock_vert_choose_p1p2_c(uint8_t* v, int stride, uint64_t* p1p2, int QP);

void deblock_vert_copy_and_unpack(int stride, uint8_t* source, uint64_t* dest, int n);
#ifdef INTEL_INTRINSICS
void deblock_vert_copy_and_unpack_sse2(int stride, uint8_t* source, uint64_t* dest, int n);
#endif
void deblock_vert_copy_and_unpack_c(int stride, uint8_t* source, uint64_t* dest, int n);

#endif // _BLINDPPCODE_H_
