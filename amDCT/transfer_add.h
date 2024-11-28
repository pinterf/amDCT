#ifndef _TRANSFER_ADD_H_
#define _TRANSFER_ADD_H_

#include "amDCTtypedefs.h" 


void copy_add_8to16_mmx(uint16_t *dst, const uint8_t * const src, uint32_t len);
void copy_add_8to16_xmm(uint16_t *dst, const uint8_t * const src, uint32_t len);

void  copy_add_16to16_mmx(uint16_t *dst, const uint16_t * const src, uint32_t len);
void copy_add3_16to16_mmx(uint16_t *dst, const uint16_t * const src1, const uint16_t * const src2, uint32_t len);
//void copy_add4_16to16_mmx(uint16_t *dst, const uint16_t * const src1, const uint16_t * const src2, const uint16_t * const src3, uint32_t len);
//void copy_add4_16to16_mmx(uint16_t *dst, uint16_t * const src1, uint16_t * const src2, uint16_t * const src3, uint32_t len);
void copy_add4_16to16_c(uint16_t *dst, uint16_t * const src1, uint16_t * const src2, uint16_t * const src3, uint32_t len);


void copy_add_16to16_clpsrc_c(uint16_t *dst,  int16_t *src, uint32_t stride);
void copy_16to16_clpsrc_c(    uint16_t *dst,  int16_t *src, uint32_t stride);

void copy_add_16to16_dctblk_c(int16_t *dst,  int16_t *src, uint32_t stride);
void copy_16to16_dctblk_c(    int16_t *dst,  int16_t *src, uint32_t stride);
void copy_16to32_dctblk_c(    int32_t *dst,  int16_t *src, uint32_t stride);

void test_copy_add_16to16_c(uint16_t *dst, uint16_t *src, uint32_t len);


void  copy_add_16to16_xmm(uint16_t *dst, uint16_t * const src, int32_t len);
void copy_add3_16to16_xmm(uint16_t *dst, uint16_t * src1, uint16_t * src2, uint32_t len);
void copy_add4_16to16_xmm(uint16_t *dst, uint16_t * const src1, uint16_t * const src2, uint16_t * const src3, uint32_t len);

void copy_8to16_len_c(uint16_t *dst,  int16_t *src, uint32_t len);
void copy_add_8to16_len_c_2(uint16_t *dst,  uint8_t *src, uint32_t len);

void  test_transfer_16to8copy_c(uint8_t  *dst, int16_t *src, uint32_t stride);
void test_transfer_16to16copy_c(uint16_t *dst, int16_t *src, uint32_t stride);
void test_transfer_16to16copy_limited_c(uint16_t *dst, int16_t *src, int16_t *orig, uint32_t stride, int16_t sharpStren);

#endif // _TRANSFER_ADD_H_
