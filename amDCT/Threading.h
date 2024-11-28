
#ifndef _THREADING_H_
#define _THREADING_H_

#include "amDCT.h"		// needed for BLKSIZE and MAX_BLOCK_ADAPT.
#include "amDCTtypedefs.h"


void setShift(uint8_t starti, uint8_t startj, FrameInfo_args *args);
void startDctLoop(FrameInfo_args *args);
unsigned int __stdcall DctLoopThread(DctLoop_args *args);
	
	
#endif _THREADING_H_
