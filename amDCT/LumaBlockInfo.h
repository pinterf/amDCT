#ifndef _LUMABLOCKINFO_H_
#define _LUMABLOCKINFO_H_


//#include "amDCTtypedefs.h"
#include "BuildPreCompVals.h" 

void analyse_dct(FrameInfo_args *FrameInfoArgs, unsigned int blkRow, unsigned int blkCol, int16_t *dct_block, int8_t type);			
void LumaBlockInfo(FrameInfo_args *FrameInfoArgs);

#endif // _LUMABLOCKINFO_H_ 
