#ifndef _TRANSFER_ADD_H_
#define _TRANSFER_ADD_H_

#include "amDCTtypedefs.h" 


void copy_add4_16to16(uint16_t* dst, uint16_t* const src1, uint16_t* const src2, uint16_t* const src3, uint32_t len);
void copy_add4_16to16_sse2(uint16_t* dst, uint16_t* const src1, uint16_t* const src2, uint16_t* const src3, uint32_t len);
void copy_add4_16to16_c(uint16_t* dst, uint16_t* const src1, uint16_t* const src2, uint16_t* const src3, uint32_t len);

void copy_add_16to16_clpsrc(uint16_t* dst, int16_t* const src, int32_t stride);
void copy_add_16to16_clpsrc_sse2(uint16_t* dst, int16_t* const src, int32_t stride);
#ifndef ARCH_IS_X86_64
void copy_add_16to16_clpsrc_xmm(uint16_t* dst, int16_t* const src, int32_t stride);
#endif
void copy_add_16to16_clpsrc_c(uint16_t* dst, int16_t* src, uint32_t stride);

void copy_16to16_dctblk_c(int16_t* dst, int16_t* src, uint32_t stride);
void copy_16to32_dctblk_c(int32_t* dst, int16_t* src, uint32_t stride);


void copy_add_16to16(uint16_t* dst, uint16_t* const src, int32_t len);
void copy_add_16to16_sse2(uint16_t* dst, uint16_t* const src, int32_t len);
#ifndef ARCH_IS_X86_64
void copy_add_16to16_xmm(uint16_t* dst, uint16_t* const src, int32_t len);
#endif
void copy_add_16to16_c(uint16_t* dst, uint16_t* src, uint32_t len);

void copy_add3_16to16(uint16_t* dst, uint16_t* src1, uint16_t* src2, uint32_t len);
void copy_add3_16to16_sse2(uint16_t* dst, uint16_t* src1, uint16_t* src2, uint32_t len);
#ifndef ARCH_IS_X86_64
void copy_add3_16to16_xmm(uint16_t* dst, uint16_t* src1, uint16_t* src2, uint32_t len);
#endif

#if 0
// not used
void copy_add4_16to16_xmm(uint16_t* dst, uint16_t* const src1, uint16_t* const src2, uint16_t* const src3, uint32_t len);
void working_copy_add4_16to16_xmm(uint16_t * dst, const uint16_t* const src1, const uint16_t* const src2, const uint16_t* const src3, uint32_t len)
#endif

void  test_transfer_16to8copy_c(uint8_t* dst, int16_t* src, uint32_t stride);
void test_transfer_16to16copy_c(uint16_t* dst, int16_t* src, uint32_t stride);

#endif // _TRANSFER_ADD_H_
