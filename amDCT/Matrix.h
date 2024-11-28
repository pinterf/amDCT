#ifndef _MATRIX_H_
#define _MATRIX_H_

#include "amDCTtypedefs.h"


void set_matrix(FrameInfo_args *FrameInfoArgs, uint8_t matrix_Num, uint8_t qtype, uint8_t quant, uint8_t range_expand);

void set_sharp_matrix(FrameInfo_args *FrameInfoArgs); 

void init_matrix(uint16_t * quant_intra_matrix, uint16_t * quant_inter_matrix, uint8_t matrix, uint8_t qtype);

void build_block_matrix(int16_t *dct_block, 
			uint8_t qtype, 
			uint8_t quant, 
			uint16_t *qtype11_block_matrix, 
			uint16_t *qtype11_block_matrix_quant,	
			uint16_t *quant_intra_matrix,
			uint16_t *quant_inter_matrix); 

#endif 
// _MATRIX_H_
