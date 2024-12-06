#ifndef _DCTLOOP_H_
#define _DCTLOOP_H_


#include "amDCTtypedefs.h"
//#include "Matrix.h"

void transfer_8to16copy(int16_t* dst, uint8_t* src, uint32_t stride);
#ifndef ARCH_IS_X86_64
void transfer_8to16copy_xmm(int16_t* dst, uint8_t* src, uint32_t stride);
#endif
void transfer_8to16copy_sse2(int16_t* dst, uint8_t* src, uint32_t stride);
void transfer_8to16copy_c(int16_t* dst, uint8_t* src, uint32_t stride);

void quantDequant(int16_t* dct_block, const uint16_t* qtype1_matrix, const uint16_t* qtype1_matrix_quant);
#ifndef ARCH_IS_X86_64
void quantDequant_xmm(int16_t* dct_block, const uint16_t* qtype1_matrix, const uint16_t* qtype1_matrix_quant);
#endif
void quantDequant_sse2(int16_t* dct_block, const uint16_t* qtype1_matrix, const uint16_t* qtype1_matrix_quant);
void quantDequant_c(int16_t* dct_block, const uint16_t* qtype1_matrix, const uint16_t* qtype1_matrix_quant);

void quantDequant_shift14_c(int16_t* dct_block, const uint16_t* qtype1_matrix, const uint16_t* qtype1_matrix_quant);

void DctLoop(int starti, int startj, DctLoop_args* args);

#endif // _DCTLOOP_H_

