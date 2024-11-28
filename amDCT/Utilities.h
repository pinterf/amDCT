#ifndef _UTILITIES_H_
#define _UTILITIES_H_

#include "amDCTtypedefs.h" 

#include "FitRange.h" 


 


int   copyFrameToDst(FrameInfo_args *FrameInfoArgs, uint8_t showMask); // For debugging

void  copySrcToDst_noArgs(const uint8_t *psrc, uint8_t *pdst, uint16_t src_width, uint16_t dst_width, uint16_t dst_pitch, uint16_t dst_height);
void  copySrcToDst(FrameInfo_args *FrameInfoArgs);
void  copySrcToDst2(uint8_t *src, uint8_t *dst, uint32_t src_width, uint32_t dst_width, uint32_t dst_height);

int   showBlockDetail(FrameInfo_args *FrameInfoArgs, uint8_t showMask);

void  UnDot(int src_pit, int dst_pit, int row_size, const uint8_t *srcp, uint8_t *dstp, int FldHeight);

void  DrawYV12(uint8_t *dst, unsigned int dst_height, unsigned int dst_width, int x1, int y1, const char *s);
void  Draw16x16Block(uint8_t *dst, unsigned int dst_height, unsigned int dst_width, int x1, int y1, const char val); 

 



#endif // _UTILITIES_H_
