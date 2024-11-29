#ifndef _AVGDCTLOOP_H_
#define _AVGDCTLOOP_H_

#include "amDCTtypedefs.h" 

#include "BuildPreCompVals.h" 


uint8_t        avgDctLoopAccumDCT(FrameInfo_args* FrameInfoArgs);
uint8_t           avgDctLoopAccum(FrameInfo_args* FrameInfoArgs);
uint8_t   avgDctLoopAccumSmoothed(FrameInfo_args* FrameInfoArgs);
uint8_t      avgDctLoopAccumSharp(FrameInfo_args* FrameInfoArgs);

//uint8_t    justBrightSmoothed(FrameInfo_args *FrameInfoArgs);

//uint8_t    doDeringDarkProtect(FrameInfo_args *FrameInfoArgs);



#endif//  _AVGDCTLOOP_H_
