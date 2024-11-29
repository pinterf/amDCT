
#ifndef _BUILD_PRECOMP_VALS_H_
#define _BUILD_PRECOMP_VALS_H_

#include "amDCTtypedefs.h" 


// Used by avgDCTLoop
void   buildDarkProtectDifVals(uint8_t* DarkProtectDifArr, uint8_t darkstart, uint8_t darkAmt);
void buildBrightProtectDifVals(uint8_t* BrightProtectDifArr, uint8_t brightstart, uint8_t brightAmt, uint8_t srcMax, uint8_t expand);
//void buildBrightProtectDifVals(uint8_t *BrightProtectDifArr, uint8_t brightstart, uint8_t brightAmt, uint8_t srcMax, uint8_t smoothMax, uint8_t expand);

void buildAccumLimitArr(uint8_t* AccumLimitArr, uint8_t brightStart);

void buildSmoothDeHighlight(uint8_t* SmoothDeHighlightArr, float avg, uint8_t brightAmt, uint8_t brightStart, float useMaxSmoothF, float lowValF);
void buildBoundaryStrengthVals(uint8_t* BoundaryStrengthArr);

void buildAccumLimitArrBoundVals(uint8_t* AccumLimitBoundArr, uint8_t expand);
void buildAccumLimitArrSmoothBoundVals(uint8_t* AccumLimitSmoothBoundArr);               // THE ARRAY IS NOT USED!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


// Used by LumaBlockInfo
void buildAvgLumaBlockProtectVals(uint8_t* AvgLumaBlockProtectArr, uint8_t Quant, uint8_t FrameQuant, uint8_t darkStart, uint8_t darkAmt, uint8_t brightStart, uint8_t brightAmt, uint8_t brightMax);
void buildBlkQuantNormVals(uint8_t* QtypeNormArr, uint8_t quant_a, uint8_t adapt_a);




//void buildAccumLimitArrVals(uint8_t *AccumLimitArr); 


#endif // _BUILD_PRECOMP_VALS_H_
