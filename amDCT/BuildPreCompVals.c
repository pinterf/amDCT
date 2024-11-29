#include <math.h>   // Needed for pow()
#include <stdio.h>  // Needed for pow()

#include "amDCTtypedefs.h"

#include "Utilities.h"
#include "Font.h"

#include "BuildPreCompVals.h"



//////////
////  BUILD ARRAYS OF PRECOMPUTED VALUES
////    These routines initializes 9 tables containing precomputed values          
////       so darkAmt, expand, and sharp processing can do a table lookup
////       to determine the actual change made to a source pixel which is determined
////       by the value of the source pixel, amount of change made by the processing routines
////       and the argument values specified by the user.
////
////    Example: Doing DarkProtect would require 2 calls to the fitRange() routines PER PIXEL
////    which would require:
////       2 max
////       4 min
////      40 compares
////      24 add or subtract
////       8 multiplies
////       2 divide
////    instead of 1 table lookup.
////       DarkProtectDifArr[(src << DARK_PROTECT_SHIFT) + Absdiff]
////       Each table is 1 dimension and contains a maximum of 32768 uint8_t values.  See #include "amDCTtypedefs.h" 
////      256 possible src pixel values multiplied by 128 possible difference values.
/////////

  //      FUTURE ENHANCEMENTS!!!!!!!!!!!!
  // The max pixel value change between blocked and de-blocked is around 8.
  // We determine how much block edges and corners are allowed to have there detail enhanced
  // by using a measure of the frames overall blockiness from adaptWeightRaw as well as the  deblocking
  // pixel blockiness found around the block from deblockDif.
  // High frame blockiness and high pixel blockiness means NO detail enhancement.
  // Low frame blockiness and low pixel blockiness mean FULL detail enhancement.
  //
  // TODO: buildDarkProtectDifEdgeVals
  // Block edges don't get unsmoothed as much.
  // If there is a lot of blockiness in the frame, adaptWeightRaw is large, then
  // block edges should get unsmoothed even less.





///////////////////////
//
//  PROTECT DARK VALUES FROM OVER SMOOTHING
//
//  buildDarkProtectDifVals() computes the values for DarkProtectDifArr[] which allows
//    doDarkProtect() to do a table lookup instead of a Int_FitRange_Int() to smoothly limit the
//    changes made to the image.
//    buildDarkProtectDifVals() is called from FrameInfo.c
void buildDarkProtectDifVals(uint8_t* DarkProtectDifArr, uint8_t darkStart, uint8_t darkAmt) {
  uint8_t   diff1;
  uint8_t   srcVal;
  uint8_t   darkLow = 16;    // Values below this do not change.
  float     maxAbsDif;
  uint8_t   absDiff2;

  for (srcVal = 0; srcVal < 255; srcVal++) {
    for (diff1 = 0; diff1 < MAX_DARK_PROTECT_DIFF; diff1++) {

      if (darkAmt == 0 ||
        diff1 == 0 ||
        darkStart <= darkLow ||
        darkStart <= srcVal) {
        DarkProtectDifArr[(srcVal << DARK_PROTECT_SHIFT) + diff1] = 0;
        continue;
      }

      if (diff1 == 1) {
        DarkProtectDifArr[(srcVal << DARK_PROTECT_SHIFT) + diff1] = 1;
        continue;
      }

      // maxAbsDif  is the max diff value allowed at the current pixel brightness.
      maxAbsDif = Float_FitRange_Float((float)srcVal, (float)darkStart, (float)darkLow, (float)diff1, 1.0F);

      // The -1.0F below is used to calibrate the transition between amt=0 and amt = 1  
      absDiff2 = (uint8_t)Int_FitRange_Float((float)darkAmt, 31.0F, -1.0F, maxAbsDif, 0.0F);

      if (srcVal < absDiff2) absDiff2 = srcVal;

      DarkProtectDifArr[(srcVal << DARK_PROTECT_SHIFT) + diff1] = absDiff2;
    }
  }
}







////////////////////////////
//
//  EXTRA SMOOTHING FOR BRIGHT  PIXELS.
//
// srcVal      The brightness of the current pixel.
// srcMax      The brightest Pixel found in the src frame.
// brightstart The brightness value were the extra smoothing starts.   User argument.
// brightAmt   The Maximum amount of smoothing applied.                User argument.
// expand      The amount of range expansion being done.   
//         Being more aggressive with smoothing, using smoothMax vs. srcMax, 
//          will decreases some of the range expansion bright halos.
//
// This routine is called by avgDctLoopAccumSmoothed()
// This is used when doing quality > 2 and range expansion.  This helps reduce halos.
// It slightly reduces BrightMax, moves it toward brightstart, depending on how large expand is.
void buildBrightProtectDifVals(uint8_t* BrightProtectDifArr, uint8_t brightstart, uint8_t brightAmt, uint8_t srcMax, uint8_t expand) {
  uint8_t   diff1;
  uint8_t   srcVal;

  for (srcVal = 0; srcVal < 255; srcVal++) {
    for (diff1 = 0; diff1 < MAX_BRIGHT_SMOOTH_DIFF; diff1++) {
      float     brightAmtF = (float)brightAmt;
      float     maxbrightF = (float)255.0F;
      float     maxAbsDif;

      if (diff1 == 0) {
        BrightProtectDifArr[(srcVal << BRIGHT_SMOOTH_SHIFT) + diff1] = 0;
        continue;
      }

      maxbrightF = Float_FitRange_Float((float)expand, 31.0F, 0.0F, (float)srcMax, (float)255);

      if (maxbrightF < brightstart) {
        BrightProtectDifArr[(srcVal << BRIGHT_SMOOTH_SHIFT) + diff1] = 0;
        continue;
      }

      maxAbsDif = Float_FitRange_Float((float)srcVal, (float)brightstart, maxbrightF, 0.0F, (float)diff1);
      maxAbsDif = (uint8_t)Int_FitRange_Float(brightAmtF, 31.0F, 0.0F, maxAbsDif, 0.0F);   // Change the first 0.0F to -1.0F to calibrate the transition between amt=0 and amt=1

      BrightProtectDifArr[(srcVal << BRIGHT_SMOOTH_SHIFT) + diff1] = (uint8_t)maxAbsDif;
    }
  }
}





////////////////////////////
//
//  THIS IS USED TO REMOVE VERY BRIGHT HIGHLIGHTS FROM THE SMOOTHED FRAME.
//
// This routine is called from avgDctLoopAccumSmoothed()
// This is used when doing quality > 2 and range expansion to help reduce halos.
// It slightly reduces BrightMax, moving it toward brightstart, by an amount determined by how large expand is.  
// smoothMax <= srcMax   since smoothing tends toward the average.
void buildSmoothDeHighlight(uint8_t* SmoothDeHighlightArr, float avg, uint8_t brightAmt, uint8_t brightStart, float useMaxSmoothF, float lowValF) {
  uint8_t srcVal;
  float  newSmoothValF;
  float   highValF, tempF;


  for (srcVal = 0; srcVal < 255; srcVal++) {
    newSmoothValF = (float)srcVal;

    if (newSmoothValF <= lowValF) {
      SmoothDeHighlightArr[srcVal] = srcVal;
      continue;
    }

    highValF = Float_FitRange_Float((float)brightAmt, 0.0F, 31.0F, (float)((brightStart + 255) / 2.0F), 255.0F);
    highValF = Float_FitRange_Float(newSmoothValF, useMaxSmoothF, avg, highValF, (float)brightStart);

    //  The smaller gamma is "0.225F"  the more compressed the bright highlights become. 
    //  tempF   = Float_fitRangeGamma_Float(newSmoothValF, highValF, (float)brightStart, 0.225F, useMaxSmoothF, lowValF);
    //  tempF   = Float_fitRangeGamma_Float(newSmoothValF, highValF, (float)brightStart, 0.525F, useMaxSmoothF, lowValF);
    tempF = Float_FitRange_Float(newSmoothValF, highValF, (float)brightStart, useMaxSmoothF, lowValF);  // NO GAMMA

    if (tempF <= lowValF) tempF = lowValF;


    SmoothDeHighlightArr[srcVal] = (uint8_t)(ROUND_TOINT(tempF));
  }
}







////////////////////////////
//
//  LIMIT SHARP TAKE INTO ACCOUNT AMOUNT OF RANGE EXPANSION
//
//  buildAccumLimitBoundArrVals() computes the values for AccumLimitBoundArr[] which allows
//  avgDctLoopAccumLimitExpand() and avgDctLoopAccumLimitSharp()
//  to do a table lookup instead of a Int_FitRange_Int() to smoothly reduce the
//  amount of change in the image.
//
//  This is called from FrameInfo.c when the filter is being initialized.
//
//            UPGRADE TO ALLOW
// The amount of change "sharpening" allowed varies according to the brightness of the source pixel.
// It becomes smaller the brighter the pixel. Noise and block defects are more noticeable in bright areas.
// It becomes greater the darker the pixel.  This should be set using a spline curve.  FUTURE ENHACEMENT IN FitRange routines.
// To try.  if (darkstart < brightstart) center=avg(darkstart,brightstart)
//            replace 127 with average do current comp
//          else darkstart to 0 increase diff
//         brightstart to 255 decreases diff.
// WE ALSO WANT 2 ARRAYS.  ONE for diff greater than 1 and a SECOND for diff < 1
//               This will allow In dark areas values getting darker to have more change then values getting brighter.
//               And obviously for bright pixels the opposite would be true.

//void buildAccumLimitArr(uint8_t *AccumLimitArr, uint8_t sharp, uint8_t expand, uint8_t brightStart, uint8_t brightAmt, uint8_t darkStart, uint8_t darkAmt) {    // DON'T NEED EXPAND ARG !!!!!!!!!!!!!!!!!
void buildAccumLimitArr(uint8_t* AccumLimitArr, uint8_t brightStart) {
  uint8_t   diff;
  uint8_t   srcVal;
  uint8_t   maxDiff = MAX_ACCUM_LIMIT_SHARP_DIFF - 1;

  if (brightStart < 128) brightStart = 128;

  for (srcVal = 0; srcVal <= 127; srcVal++) {
    for (diff = 0; diff < maxDiff; diff++) {
      // dark to bright
      uint8_t temp2 = (uint8_t)ROUND_TOINT(Float_FitRange_Float((float)srcVal, 127.0F, (float)(255 - brightStart), (float)diff, 0.0F));    // bright to mid

      //uint8_t temp3   = (int16_t)ROUND_TOINT(Float_FitRange_Float((float)srcVal,  127.0F, 0.0F,  (float)diff, ((float)(diff)/0.95F)));  //   dark to mid
      uint8_t temp3 = (uint8_t)ROUND_TOINT(Float_FitRange_Float((float)srcVal, 127.0F, 0.0F, (float)diff, ((float)(diff) / 1.15F)));  //   dark to mid

      AccumLimitArr[((255 - srcVal) << ACCUM_LIMIT_SHARP_SHIFT) + diff] = temp2;  // bright to mid
      AccumLimitArr[(srcVal << ACCUM_LIMIT_SHARP_SHIFT) + diff] = temp3;          //   dark to mid
    }
  }
}








////////////////////////////
//
//  LIMIT EXPAND AND SHARP near BOUNDARIES
//
//  if (doExpand)   newPixVal = doLimitSharpBound(AccumLimitArr,  AccumLimitBoundArr, srcVal, newPixVal, boundVal);
//
//  buildAccumLimitBoundArrVals() computes the values for AccumLimitBoundArr[] which allows
//  avgDctLoopAccumLimitExpand() and avgDctLoopAccumLimitSharp()
//  to do a table lookup instead of a Int_FitRange_Int() to smoothly reduce the
//  amount of change near boundaries in the image.
//
//  This is called from FrameInfo.c when the filter is being initialized.
void buildAccumLimitArrBoundVals(uint8_t* AccumLimitBoundArr, uint8_t expand) {
  uint16_t  bndBrightness;
  uint8_t   diff;
  uint8_t   diff2;
  float     maxBright;

  // Decrease the amount of range expansion at boundaries as the amount of expansion increases.
  maxBright = (float)Float_FitRange_Float((float)expand, 0.0F, 31.0F, 200.0F, 100.0F);
  //  maxBright = (float)Float_FitRange_Float((float)expand, 0.0F, 31.0F, 240.0F, 100.0F);

  for (bndBrightness = 0; bndBrightness <= 255; bndBrightness++) {
    for (diff = 0; diff < MAX_ACCUM_LIMIT_BOUND_DIFF - 1; diff++) {  // The max change seen using the reference frame is about 22
      // As the brightness of the boundary mask gets larger, we are closer to a bright dark boundary, 
      //    decrease diff2, which is the amount of expansion allowed for the pixel.
      diff2 = (uint8_t)Int_FitRange_Float((float)bndBrightness, 64.0F, maxBright, (float)diff / 9.0F, (float)diff);   // new
      AccumLimitBoundArr[(bndBrightness << ACCUM_LIMIT_BOUND_SHIFT) + diff] = diff2;
    }
  }
}









////////////////////////////
//
//  LIMIT EXPAND AND SHARP near BOUNDARIES  THAT ARE WITHIN THE BRIGHT-SMOOTHED,   BRIGHTNESS RANGE.        NOT USED!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//  buildAccumLimitArrSmoothBoundVals() computes the values for AccumLimitSmoothBoundArr[] which allows
//  avgDctLoopAccumLimitExpand() and avgDctLoopAccumLimitSharp()
//  to do a table lookup instead of a Int_FitRange_Int() to smoothly reduce the
//  amount of change near boundaries in the image taking into account the brightStart argument.
//
//  This is called from FrameInfo.c when the filter is being initialized.
void buildAccumLimitArrSmoothBoundVals(uint8_t* AccumLimitSmoothBoundArr) {
  uint16_t  temp;
  uint8_t   diff;

  for (temp = 0; temp <= 255; temp++) {
    for (diff = 0; diff < MAX_ACCUM_LIMIT_SMOOTH_BOUND_DIFF; diff++) {  // The max change seen using the smoothed frame is about 40
      //AccumLimitSmoothBoundArr[(temp << ACCUM_LIMIT_SMOOTH_BOUND_SHIFT) + diff] = (uint8_t)Int_FitRange_Int(temp, 25, 128, diff>>1, 0);
      AccumLimitSmoothBoundArr[(abs(temp - diff) << ACCUM_LIMIT_SMOOTH_BOUND_SHIFT) + diff] = (uint8_t)Int_FitRange_Int(temp, 25, 128, diff >> 1, 0);
    }
  }
}








///////////////////////////
//
//  LIMIT BLOCK SMOOTHING BASED UPON THE AVERAGE LUMA OF A BLOCK WHEN DOING  BLOCK ADAPTIVE SMOOTHING
//
//  AvgLumaBlockProtectArr[] is used in LumaBlockInfo()
//  temp2 = Int_FitRange_Int(LumaPerBlockArgs[curBlock].AvgLuma, 92, 16, temp2, 0); // Blocks in dark areas don't show blockiness as much, reduce smoothing.
//
//
//  This is called from FrameInfo.c when the filter is being initialized.
void buildAvgLumaBlockProtectVals(uint8_t* AvgLumaBlockProtectArr, uint8_t quant_a, uint8_t adapt_a, uint8_t darkStart, uint8_t darkAmt, uint8_t brightStart, uint8_t brightAmt, uint8_t brightMax) {
  uint16_t  avgLuma;    // 0-255  The average luma of the block.
  uint8_t   DifEC_EC2;  // 0-31   The current smoothing value of the block.
  float     tempF;
  float     blockQuantF;
  uint8_t   blockQuant;

  for (avgLuma = 0; avgLuma <= 255; avgLuma++) {
    for (DifEC_EC2 = 0; DifEC_EC2 < 32; DifEC_EC2++) {

      blockQuantF = Float_FitRange_Float((float)(DifEC_EC2 + 1), 0.0F, 11.0F, (float)quant_a, (float)adapt_a);

      tempF = Float_FitRange_Float((float)avgLuma, (float)brightStart, (float)brightMax, blockQuantF, (float)adapt_a);  // Should avgLuma be modified by block range???
      blockQuantF = Float_FitRange_Float((float)brightAmt, 0.0F, 31.0F, blockQuantF, tempF);

      tempF = Float_FitRange_Float((float)avgLuma, (float)darkStart, 0.0F, blockQuantF, (float)adapt_a);
      blockQuant = (uint8_t)Int_FitRange_Float((float)darkAmt, 0.0F, 31.0F, blockQuantF, tempF);

      if (blockQuant > adapt_a) blockQuant = adapt_a;

      AvgLumaBlockProtectArr[(abs(avgLuma - DifEC_EC2) << ACCUM_LIMIT_SMOOTH_BOUND_SHIFT) + DifEC_EC2] = blockQuant;
    }
  }
}






////////////////////////////
//
//  LIMIT BLOCK ADAPTIVE SMOOTHING TO THE RANGE quant_a to adapt_a
//
//  called from  LumaBlockInfo()
//
void buildBlkQuantNormVals(uint8_t* QtypeNormArr, uint8_t quant_a, uint8_t adapt_a) {
  if (quant_a > adapt_a) adapt_a = quant_a;
  for (uint8_t quant = 0; quant <= 31; quant++) {
    QtypeNormArr[quant] = (uint8_t)Int_FitRange_Int(quant, 0, 31, quant_a, adapt_a);
  }
}










////////////////////////////
//
//  PRECOMPUTE BOUNDARY STRENGTH CALCULATIONS.
//
//  ONLY called from  FrameInfo.c  buildMasks() LumaFrameInfo.c which calls buildBoundaryStrengthVals()  when the filter is being initialized.
//
////////////
void buildBoundaryStrengthVals(uint8_t* BoundaryStrengthArr) {
  for (int idx = 0; idx <= 255; idx++) {
    float idxF = (float)idx;
    float tempF = idxF;
    int   temp;

    tempF = Float_fitRangeGamma_Float(tempF, 0.0F, 255.0F, 1.99F, 0.0F, 255.0F);
    tempF = Float_FitRange_Float(tempF, 225.0F, 50.0F, 255.0F, 0.0F);      // Expand the range.  Makes the mask cleaner.
    temp = Int_FitRange_Float(idxF, 32.0F, 120.0F, tempF / 3.5F, tempF);   // Reduce boundary strenth in dark areas.

    if (temp < 0)   temp = 0;
    if (temp > 255) temp = 255;
    BoundaryStrengthArr[idx] = (uint8_t)temp;
  }
}







//////////////   OLD CODE FROM HERE TO END  /////////////////


/*
////////////////////////////
//
//  LIMIT EXPAND AND SHARP as they approach brightStart
//
//  if (doExpand)   newPixVal = doLimitSharpBound(AccumLimitArr,  AccumLimitBoundArr, srcVal, newPixVal, boundVal);
//
//  buildAccumLimitBoundArrVals() computes the values for AccumLimitBoundArr[] which allows
//  avgDctLoopAccumLimitExpand() and avgDctLoopAccumLimitSharp()
//  to do a table lookup instead of a Int_FitRange_Int() to smoothly reduce the
//  amount of change near boundaries in the image.
//
//  This is called from FrameInfo.c when the filter is being initialized.
void buildAccumLimitArrBoundVals2(uint8_t *AccumLimitBoundArr, uint8_t expand, uint8_t brightStart) {
  uint16_t  bndBrightness;
  uint8_t   diff;
  float     diff2;
  uint8_t   diff3;
  float     maxBright;

  // Decrease the amount of range expansion at boundaries as the amount of expansion increases.
  maxBright = (float)Float_FitRange_Float((float)expand, 0.0F, 31.0F, 200.0F, 100.0F);

  for (bndBrightness = 0; bndBrightness <= 255; bndBrightness++) {
    for (diff = 0; diff < MAX_ACCUM_LIMIT_BOUND_DIFF - 1; diff++) {  // The max change seen using the reference frame is about 22
      // As the brightness of the boundary mask gets larger, we are closer to a bright dark boundary, decrease diff2, the amount of expansion allowed for the pixel.
      diff2 = Float_FitRange_Float((float)bndBrightness, 64.0F, maxBright, (float)diff / 9.0F, (float)diff);
      diff3 = (uint8_t)Int_FitRange_Float((float)brightStart, 64.0F, maxBright, (float)diff / 9.0F, (float)diff);
      AccumLimitBoundArr[(bndBrightness << ACCUM_LIMIT_BOUND_SHIFT) + diff] = (uint8_t)ROUND_TOINT(diff2);
    }
  }
}
*/


////////////////////////////
//
//  LIMIT BLOCK SMOOTHING BASED UPON THE AVERAGE LUMA OF A BLOCK WHEN DOING BLOCK ADAPTIVE SMOOTHING
//
//  called from  LumaBlockInfo()
//  temp2 = Int_FitRange_Int(LumaPerBlockArgs[curBlock].AvgLuma, 92, 16, temp2, 0); // Blocks in dark areas don't show blocks as much, reduce smoothing.
//  FrameInfoArgs->MemoryArgs->DctLoopArgs[0].blkAdaptSmoothAmt[curBlock] = (uint8_t)Int_FitRange_Int(temp2, 1, 31, FrameInfoArgs->quant, FrameInfoArgs->adapt); // Bring range between quant and adapt;
//
//
//  This is called from FrameInfo.c when the filter is being initialized.
//void buildQtypeNormVals(uint8_t *QtypeNormArr, uint8_t quant_a, uint8_t adapt_a) {
//  for (uint8_t quant = 0; quant <= 31; quant++) {
//    QtypeNormArr[quant] = (uint8_t)Int_FitRange_Int(quant, 0, 31, quant_a, adapt_a);
//  }
//}
////////////////////////////




////////////////////////////
//
//  LIMIT BLOCK SMOOTHING BASED UPON THE AVERAGE LUMA OF A BLOCK WHEN DOING qtype>1 && qtype<=4  SAME AS ABOVE BUT USES GAMMA INSTEAD OF STRAIGHT LINE FIT.
//
//  called from  LumaBlockInfo()
//
//  if (avgLumaC >= 127) tempfloat1 = Int_FitRange_Float(avgLumaC, 127, 255, sadC/2, sadC);
//  else                 tempfloat1 = Int_FitRange_Float(avgLumaC, 126,   0, sadC/2, sadC);
//  if (tempfloat1 < 4.0) tempfloat1 = 0.0;
//  tempfloat1 = Float_fitRangeGamma_Float(tempfloat1, 6.0, 1920.0, 6.0, 0.0, 255.0);

/*
//  This is called from FrameInfo.c when the filter is being initialized.
void buildQtypeNormVals(uint8_t *QtypeNormArr, uint8_t quant_a, uint8_t adapt_a) {
  for (uint8_t quant = 0; quant <= 31; quant++) {
    QtypeNormArr[quant] = (uint8_t)Float_fitRangeGamma_Float(tempfloat1, 6.0, 1920.0, 6.0, 0.0, 255.0);
//    Int_FitRange_Int(quant, 0, 31, quant_a, adapt_a);
  }
}
*/
////////////////////////////












