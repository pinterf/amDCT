#include <math.h>


#include "LumaFrameInfo.h"
#include "FrameInfo.h"
#include "SmoothFilters.h"
#include "MaxMinFilter.h"
#include "Utilities.h"
#include "BuildPreCompVals.h"





void  LumaFrameInfo(FrameInfo_args* FrameInfoArgs) {

  uint16_t  src_height = FrameInfoArgs->src_height;
  uint16_t  src_width = FrameInfoArgs->src_width;

  uint8_t* psmoothed = FrameInfoArgs->frame_smoothed;
  uint8_t* frame_3x3mean = FrameInfoArgs->frame_3x3mean;
  uint8_t* frame_3x3sadm = FrameInfoArgs->frame_3x3sadm;
  uint8_t* frame_boundaries = FrameInfoArgs->frame_boundaries;
  uint8_t* frame_localMax9x9 = FrameInfoArgs->frame_localMax9x9;
  uint8_t* frame_localMin9x9 = FrameInfoArgs->frame_localMin9x9;
  uint8_t* BoundaryStrengthArr = FrameInfoArgs->MemoryArgs->BoundaryStrengthArr;

  uint8_t  T2 = FrameInfoArgs->showMask;

  int   h, x;
  int   temp;


  if (FrameInfoArgs->LumaFrameInfoDone == 1) return;
  FrameInfoArgs->LumaFrameInfoDone = 1;

  buildBoundaryStrengthVals(BoundaryStrengthArr);

  gaussianBlur2(FrameInfoArgs, psmoothed, frame_3x3mean, 1, 1, 1);   // Build 3x3 mean frame.

  maxFrame(FrameInfoArgs, psmoothed, frame_localMax9x9, 4, 4);       // Build 9x9 max
  minFrame(FrameInfoArgs, psmoothed, frame_localMin9x9, 4, 4);       // Build 9x9 min

  sadWindow(FrameInfoArgs, psmoothed, FrameInfoArgs->frame_3x3sadm, FrameInfoArgs->frame_3x3mean, 1, 1);  // Build the 3x3 sad

  for (h = 0; h < src_height; h++) {
    for (x = 0; x < src_width; x++) {
      uint32_t idxRC = src_width * h + x;

      temp = ROUND_TOINT((float)((float)(frame_localMax9x9[idxRC] - frame_localMin9x9[idxRC]) * .75F) + (frame_3x3sadm[idxRC] * 1.4F));
      if (T2 == 9 || T2 == 11) temp = ROUND_TOINT((float)((float)(frame_localMax9x9[idxRC] - frame_localMin9x9[idxRC]) * .75F) + (frame_3x3sadm[idxRC] * 2.5F)) + 5;

      if (temp < 0)   temp = 0;
      if (temp > 255) temp = 255;
      frame_boundaries[idxRC] = BoundaryStrengthArr[(uint8_t)temp];
    }
  }


  return;

}



