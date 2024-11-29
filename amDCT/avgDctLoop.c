
#include <math.h>
#include <stdio.h>

#include "amDCTtypedefs.h"

#include "avgDctLoop.h"
#include "Utilities.h"
#include "BuildPreCompVals.h"
#include "emmintrin.h"

//__inline void avgAccum_xmm(uint8_t *dst, int16_t * const src, int32_t len, uint8_t round, uint8_t numShiftsHiBit);  BROKEN

__inline uint8_t   doBrightSmoothed(uint8_t* BrightProtectDifArr, uint8_t src, uint8_t smooth, uint8_t start);
__inline uint8_t   doDarkProtect(uint8_t* DarkProtectDifArr, uint8_t src, uint8_t smooth, uint8_t darkStart);

__inline uint8_t   doEndPointLimit(uint8_t* AccumLimitArr, uint8_t temp, uint8_t src);
__inline uint8_t   doEndPointBndLimit(uint8_t* AccumLimitArr, uint8_t* AccumLimitBoundArr, uint8_t temp, uint8_t src, uint8_t bndVal);


/*
  5 lookup tables are built to speed up the pixel by pixel processing described below.
  The lookup tables are designed to reduce the difference between 2 frames.
  The amount of reduction is determined by the value of a third frame.
  For example when expansion is done the amount of expansion that is done should be less if the source pixel is
  close to 0 or 255.  The amount of expansion should also smoothly taper off.  This is done by precomputing a table
  that for every source value (0-255) and any amount of difference between the source value and the expanded value (0-128)
  the table will contain the actual difference that is wanted.

  BrightProtectDifArr     [(srcVal << BRIGHT_SMOOTH_SHIFT)            + Absdiff]     bright protection processing.
  DarkProtectDifArr       [(srcVal << DARK_PROTECT_SHIFT)             + Absdiff]    dark protection processing.
  AccumLimitArr           [(temp   << ACCUM_LIMIT_SHIFT)              + Absdiff]    sharp  processing.
  AccumLimitBoundArr      [(temp   << ACCUM_LIMIT_BOUND_SHIFT)        + Absdiff]    expand processing.


  In the above tables the _SHIFT entries are #defines in amDCTtypedefs.h and specify the max size Absdiff can be for each array.
  In all cases the srcVal or temp are sized for 256 entries

  The above tables are built by the following routines.
    void buildAccumLimitArrVals     (*AccumLimitArr);
    void buildAccumLimitArrBoundVals(*AccumLimitBoundArr);
    void buildBrightProtectDifVals  (*BrightProtectDifArr, brightstart, brightAmt, BrightMax, expand);  if quality >= 3
    void buildDarkProtectDifVals    (*DarkProtectDifArr,   darkstart,   darkAmt)




          AVERAGE DCT SHIFTS ACCUMULATOR PROCESSING BY QUALITY

  The DCT SHIFTS ACCUMULATOR, BF_accumP, contains
    the sum of the results of running the frame through the specified DCT operations.  "see DctLoop()      in DctLoop.c"
    The frame is shifted up, down, left, or right,   shift number of times.            "see DispatchLoop() in DispatchLoop.c"

  To convert the "DCT SHIFTS ACCUMULATOR" output, stored in BF_accumP, to the new processed frame,
    each pixel in BF_accumP is converted to a newPixVal in the new frame using various functions depending the users arguments.

  For each summed pixel in BF_accumP[x]
    Take the average:    BF_accumP/"shift number of times"        newPixVal = (uint8_t)((BF_accumP[x] + round) >> numShiftsHiBit);

  Depending on the following arguments, up to 5 additional operations are done on each newPixVal to increase the quality of the frame.
      the newPixVal brightness,
      the boundary strength,                 // requires quality >= 3
      and the user arguments,
        quality >= 3,                      // sets doBound  to TRUE
        quant,                             // sets doSmooth to TRUE
        expand,                            // sets doExpand to TRUE
        sharpWAmt  and  sharpTAmt          // sets doSharp  to TRUE
        darkAmt    and  darkStart and quant > 0  // sets doDark   to TRUE  Prevents over-smoothing of dark areas.
        brightAmt  and  brightStart        // sets doBright to TRUE  Increases smoothing in bright areas.

  All of the additional operations are done using table lookups.
  The additional operations are
    bright    smoothing     doBrightSmoothed()
    dark      protection   doDarkProtect()

    end point limiting               doEndPointLimit()    the amt of change expand and sharpening do near pixel values 0 and 255 is reduced.
    end point and boundary limiting  doEndPointBndLimit() the amt of change expand    does is reduced   when near a boundary. quality >= 3


    if (doBright)    temp = doBrightSmoothed   (BrightProtectDifArr, temp,  smoothVal, brightStart);
    if (doSharp)     temp = doEndPointLimit    (AccumLimitArr,       temp,  srcVal);
    if (doExpand)    temp = doEndPointBndLimit (AccumLimitBoundArr,  temp,  srcVal,    boundaryStrength);
    if (doDark)      temp = doDarkProtect      (DarkProtectDifArr,   temp,  srcVal,    darkStart);





    Operations performed for each Processing pass for each quality.
  The shift argument to amDCT takes values 0-5 which are translated
  by DispatchLoop() into the values 1, 4, 8, 16, 32, 64 which are the actual number of shifts to be done by dctLoop()
  Each shift requires 1 pass through the DctLoop.
  The term "shift passes" used below means the number of shifts done in the DCTloop specified by the user argument shift.


  Quality == 1
    if (doBright)  8 passes through the DCTloop to create the smoothed frame.
    smoothing, expanding, and sharpening are done at the same time in "shift passes" through the DctLoop


  quality == 2 || quality == 3
    if (doBright)        8 passes through the DCTloop to create the smoothed frame.
    if (doSharp)          "shift passes" through the DctLoop.
    if (doSmooth || doExpand)   "shift passes" through the DctLoop.

    NOTE: The difference between quality 2 and quality 3 is
        quality >= 3 does block level adaptive processing.



  quality == 4
    Does boundary processing.
    if (doBright)  8 passes through the DCTloop to create the smoothed frame.
    if (doSharp)    "shift passes" through the DctLoop.
    if (doExpand)   "shift passes" through the DctLoop.
    if (doSmooth)   "shift passes" through the DctLoop.


*/











// The Following 4 routines take the output from the DctLoop() processing,
//     which might have summed the pixel values up to 64 times,
//     and normalizes the pixel values back to their original 0-255 value range.
// avgDctLoopAccum()          is used for the main amDCT processing loop.
// avgDctLoopAccumSmoothed()  is used when creating a very smoothed frame.
// avgDctLoopAccumSharp()     is used when sharpening.
// avgDctLoopAccumDCT()       is used when displaying DCT values.  Currently only for debugging.
//                In the future possibly for advanced per block adaptivity.



// BF_accumP contains the sum of the values produced by the main processing loop.
// avgDctLoopAccum divides the sum by the number of times the main processing loop was called.
// The loop is only called a power of 2 times so we can use a round up and shift to do the average.
//
uint8_t  avgDctLoopAccum(FrameInfo_args* FrameInfoArgs) {
  uint16_t     src_width = FrameInfoArgs->src_width;
  uint16_t     src_height = FrameInfoArgs->src_height;

  uint8_t* PworkSource = FrameInfoArgs->frame_workSource;
  uint8_t* Psmoothed = FrameInfoArgs->frame_smoothed;
  uint8_t* Pframe_bound = FrameInfoArgs->frame_boundaries;
  uint8_t* PsrcMem = FrameInfoArgs->MemoryArgs->DctLoopArgs[0].BF_srcP;
  uint16_t* BF_accumP = FrameInfoArgs->MemoryArgs->DctLoopArgs[0].BF_accumP;

  uint8_t* BrightProtectDifArr = FrameInfoArgs->MemoryArgs->BrightProtectDifArr;
  uint8_t      brightStart = FrameInfoArgs->brightStart;
  uint8_t      doBright = FrameInfoArgs->doBrightFlag;

  uint8_t* DarkProtectDifArr = FrameInfoArgs->MemoryArgs->DarkProtectDifArr;
  uint8_t      darkStart = FrameInfoArgs->darkStart;
  uint8_t      doDark = FrameInfoArgs->doDarkFlag;

  uint8_t      doSharp = FrameInfoArgs->doSharpFlag;
  uint8_t      doExpand = FrameInfoArgs->doExpandFlag;
  uint8_t      doSmooth = FrameInfoArgs->doSmoothFlag;

  uint8_t* AccumLimitArr = FrameInfoArgs->MemoryArgs->AccumLimitArr;
  uint8_t* AccumLimitBoundArr = FrameInfoArgs->MemoryArgs->AccumLimitBoundArr;

  uint8_t      quality = FrameInfoArgs->quality;

  uint8_t      shift = FrameInfoArgs->shift;
  uint8_t     round = FrameInfoArgs->round[shift];
  uint8_t     numShiftsHiBit = FrameInfoArgs->numShiftsHiBit[shift];

  uint8_t     lastFilter = 0;
  uint16_t    h, x;


  if (quality == 1)  lastFilter = 1;

  if (quality == 2 || quality == 3) {
    if (doSharp == 0)                    lastFilter = 1;
    if (doSharp == 1 && doExpand == 0 && doSmooth == 0)  lastFilter = 1;
  }

  if (quality == 4) {
    if (doSmooth == 1) {     // Do Expand first.  This matches with quality 4 processing in aaFilters.c
      if (doSharp == 0 && doExpand == 0)  lastFilter = 1;
      if (doSharp != 0 && doExpand != 0)  doSharp = 0;
    }
    else {            // If doSmooth==0 do last filter if not doing either sharp or expand.
      if (doSharp == 0 || doExpand == 0)  lastFilter = 1;
      if (doSharp != 0 && doExpand != 0)  doSharp = 0;
    }
  }

  //round=0;
  for (h = 0; h < src_height; h++) {
    for (x = 0; x < src_width; x++) {
      uint8_t newPixVal = (uint8_t)((BF_accumP[x] + round) >> numShiftsHiBit);
      uint8_t smoothVal = Psmoothed[x];
      uint8_t srcVal = PworkSource[x];
      uint8_t boundVal = Pframe_bound[x];

      if (doSharp)  newPixVal = doEndPointLimit(AccumLimitArr, newPixVal, srcVal);
      if (doExpand)   newPixVal = doEndPointBndLimit(AccumLimitArr, AccumLimitBoundArr, srcVal, newPixVal, boundVal);

      if (lastFilter) {  // doDark is set in amDCTmain.c
        if (doBright)  newPixVal = doBrightSmoothed(BrightProtectDifArr, newPixVal, smoothVal, brightStart);
        if (doDark)    newPixVal = doDarkProtect(DarkProtectDifArr, newPixVal, srcVal, darkStart);
      }

      PsrcMem[x] = newPixVal;
      PworkSource[x] = newPixVal;
    }

    Psmoothed += src_width;
    Pframe_bound += src_width;
    PworkSource += src_width;
    PsrcMem += src_width;
    BF_accumP += src_width;
  }
  if (quality == 4) {
    if (doExpand == 1) FrameInfoArgs->doExpandFlag = 0;
    else if (doSharp == 1) FrameInfoArgs->doSharpFlag = 0;
  }
  if (quality == 2 || quality == 3) {
    if (doSharp == 1)      FrameInfoArgs->doSharpFlag = 0;
  }

  if (lastFilter) copySrcToDst(FrameInfoArgs);
  return(0);
}








//// This is used by sharpSource() in FrameInfo.c
//// It outputs the frame to frame_sharp without touching the src or dest frames.
//uint8_t  avgDctLoopAccumSharp(FrameInfo_args *FrameInfoArgs) {
//
//  uint16_t    *BF_accumP      = FrameInfoArgs->MemoryArgs->DctLoopArgs[0].BF_accumP;
//  uint8_t     *PsharpP         = FrameInfoArgs->frame_sharp;
//
//  uint8_t      shift          = FrameInfoArgs->shift;
//  uint8_t     round          = FrameInfoArgs->round[shift];
//  uint8_t     numShiftsHiBit = FrameInfoArgs->numShiftsHiBit[shift];
//  uint16_t     src_height     = FrameInfoArgs->src_height;
//  uint16_t     src_width      = FrameInfoArgs->src_width;
//
//  uint16_t h, x;
//  uint32_t idxRC=0;
//
//  for (h=0; h < src_height; h++) {    // THIS LOOP SHOULD BE SSE2 OPTIMIZED !!!!!!!!!!!!!!!!!!
//    for (x=0; x < src_width; x++) {
//      PsharpP[idxRC] = (uint8_t)((BF_accumP[idxRC] + round)>>numShiftsHiBit);
//      idxRC++;
//    }
//  }
//
//  _mm_empty()
//  return(0);
//

// avgDctLoopAccumSmoothed() is used to produce a very smoothed frame from the source frame.
// The very smoothed frame which is used for brightAmt-brightStart processing. It provides extra smoothing in
// bright areas of the picture. It discovers and reduces "bright highlight noise" caused by block encoding errors
// Video noise and encoding artifacts are often more noticeable in the bright areas.
//
// This routine is called by smoothedSource() in FrameInfo.c
// It outputs the very smoothed frame to FrameInfoArgs->frame_smoothed without touching the src or dest frames.
//
// It analyses the frame and determines the brightness of overly bright pixels, usually caused by encoding errors,
// and sets them to slightly darker values.
//
// Extra expand processing was tried because
//     expand makes the overly bright pixels even brighter,
//    they would be reduced more as expand is increased.
// but the extra processing does not seem to be needed.
//
//     0 <= avg <= brightStart <= maxSmooth <= maxSrc <= 255
//     dist1 = brightStart - avg             larger  more maxSmooth can be reduced.  I.E. trim more bright pixels
//     dist2 = maxSmooth   - brightStart     larger  more maxSmooth can be reduced.
//     dist3 = maxSrc      - maxSmooth       Smaller more maxSmooth can be reduced.
//     dist4 = 255         - maxSrc          larger  more maxSmooth can be reduced.

uint8_t  avgDctLoopAccumSmoothed(FrameInfo_args* FrameInfoArgs) {

  uint16_t* BF_accumP = FrameInfoArgs->MemoryArgs->DctLoopArgs[0].BF_accumP;
  uint8_t* SmoothDeHighlightArr = FrameInfoArgs->MemoryArgs->SmoothDeHighlightArr;
  uint8_t* BrightProtectDifArr = FrameInfoArgs->MemoryArgs->BrightProtectDifArr;
  uint8_t* psmoothed = FrameInfoArgs->frame_smoothed;
  uint8_t* PworkSource = FrameInfoArgs->frame_workSource;

  uint8_t      shift = FrameInfoArgs->shift;
  uint8_t     round = FrameInfoArgs->round[shift];
  uint8_t     numShiftsHiBit = FrameInfoArgs->numShiftsHiBit[shift];
  uint16_t     src_height = FrameInfoArgs->src_height;
  uint16_t     src_width = FrameInfoArgs->src_width;
  uint8_t      expand = FrameInfoArgs->expand;
  uint8_t      brightStart = FrameInfoArgs->brightStart;
  uint8_t      brightAmt = FrameInfoArgs->brightAmt;
  //  uint16_t     T2           = FrameInfoArgs->T2;

  uint16_t    h, x;
  uint32_t    idxRC = 0;
  //  uint32_t    idxRC        = src_width*h + x;

  uint32_t     totalPixels = src_height * src_width;
  uint64_t     total = 0;
  uint64_t     totalSrc = 0;
  uint64_t     totalSmooth = 0;
  uint32_t   maxNumBrightPix = 1;
  uint32_t   numBright = 0;  // Number of pixels found larger than brightStart.
  uint32_t   numBrightSrc = 0;  // Number of pixels found larger than brightStart.
  uint32_t   numBrightSmooth = 0;  // Number of pixels found larger than brightStart.
  uint64_t   totalBright = 0;  // sum of pixel values found larger than brightStart.
  uint64_t   totalBrightSrc = 0;  // sum of pixel values found larger than brightStart.
  uint64_t   totalBrightSmooth = 0;  // sum of pixel values found larger than brightStart.
  uint16_t    minSmoothCutoff = brightStart;

  uint8_t     minSrc = 255;      // The minimum source value found.
  uint8_t     maxSrc = 0;        // The largest source value found.
  uint8_t     minSmooth = 255;         // The minimum smooth value found.
  uint8_t     maxSmooth = 0;           // The largest smooth value found.
  float     useMaxSmoothF = 0.0F;
  uint8_t     minMaxSmooth = brightStart;  // The minimum value allowed for maxSmooth.  Range brightStart to maxSmooth.
  int16_t    lowVal = brightStart;
  float      lowValF = (float)brightStart;
  uint8_t     avgSrc = 128;
  float     avgSrcF = 128.0F;
  uint8_t     avgSmooth = 128;
  float     avgSmoothF = 128.0F;
  uint8_t     avgBright = 128;
  float     avgBrightF = 128.0F;
  float     percentBrightF = 0.0F;

  uint32_t     countSrc[256];    // Histogram of the source   frame pixel values.
  uint32_t     countSmooth[256]; // Histogram of the Smoothed frame pixel values.
  MY_MEMSET(countSrc, 0, sizeof(countSrc));
  MY_MEMSET(countSmooth, 0, sizeof(countSmooth));


  // Build the smoothed  frame psmoothed
  // Build the smoothed  frame histogram countSmooth[256].
  // Build the source    frame histogram countSrc[256].

  // Find  the smoothed  frame max and min values. maxSmooth, minSmooth
  // Find  the src       frame max and min values. maxSrc,    minSrc
  // Find  the total of the src      frame values. totalSrc
  // Find  the total of the smoothed frame values. totalSmooth


  idxRC = 0;
  for (h = 0; h < src_height; h++) {
    for (x = 0; x < src_width; x++) {
      //
      //// COMPUTE THE SMOOTHED FRAME INFO.
      //uint8_t srcVal    = PworkSource[idxRC];
      int16_t srcVal = (int16_t)PworkSource[idxRC];
      //uint8_t smoothVal = (uint8_t)((BF_accumP[idxRC] + round)>>numShiftsHiBit);
      int16_t smoothVal = (int16_t)((BF_accumP[idxRC] + round) >> numShiftsHiBit);

      if (srcVal > 253) srcVal = 253;
      if (srcVal < 2)   srcVal = 2;

      if (smoothVal > 253) smoothVal = 253;
      if (smoothVal < 2)   smoothVal = 2;
      //if (smoothVal > 254 || smoothVal < 0) smoothVal = 253;

      psmoothed[idxRC] = (uint8_t)smoothVal;    // This builds the Very Smoothed frame. Mask number 7

      //
      ////   COMPUTE THE SMOOTHED FRAME METRICS.
      totalSmooth += smoothVal;               // The sum of all of the smoothed pixels
      countSmooth[smoothVal] = countSmooth[smoothVal] + 1; // Histogram of the Very smooth frame.

      if (maxSmooth < smoothVal) {
        maxSmooth = (uint8_t)smoothVal;
      }
      if (minSmooth > smoothVal) {
        minSmooth = (uint8_t)smoothVal;
      }
      if (brightStart < smoothVal) {
        totalBrightSmooth += smoothVal;  // The sum of all the bright pixels in the smooth frame.
        numBrightSmooth++;
      }

      totalSmooth += smoothVal;    // The sum of all of the smoothed pixels
      countSmooth[smoothVal]++;    // Histogram of the smoothed pixel values

      if (minSmooth > smoothVal) {  // Find the largest smoothed pixel value
        minSmooth = (uint8_t)smoothVal;
      }

      if (brightStart < minSmooth) {
        totalBrightSmooth += minSmooth;
        numBrightSmooth++;
      }

      ////   COMPUTE THE SOURCE FRAME INFO.
      totalSrc += srcVal;               // The sum of all of the source pixels
      countSrc[srcVal] = countSrc[srcVal] + 1;   // Histogram of the src frame.

      if (brightStart < srcVal) {
        totalBrightSrc += srcVal;  // The sum of all the bright pixels in the source frame.
        numBrightSrc++;
      }

      totalSrc += srcVal;      // The sum of all of the source pixels
      countSrc[srcVal]++;      // Histogram of the source pixel values

      if (minSrc > srcVal) {    // Find the smallest source pixel value
        minSrc = (uint8_t)srcVal;
      }

      if (maxSrc < srcVal) {    // Find the largest source pixel value
        maxSrc = (uint8_t)srcVal;
      }

      if (minSrc > srcVal) {    // Find the smallest source pixel value
        minSrc = (uint8_t)srcVal;
      }

      if (brightStart < minSrc) {
        totalBrightSrc += minSrc;
        numBrightSrc++;
      }

      idxRC++;
    }
  }

  // (minSmooth > minSrc) and (maxSmooth < maxSrc) should always be TRUE since smoothing tends tward the average.
//  if (minSmooth > minSrc) minSmooth = minSrc;
//  if (maxSmooth < maxSrc) maxSmooth = maxSrc;

  // Find the average brightness of the smoothed frame and the source frame.
  avgSrcF = (float)((float)totalSrc / (float)totalPixels);
  avgSrc = (uint8_t)ROUND_TOINT(avgSrcF);

  avgBrightF = (float)((float)totalBright / (float)numBright);
  avgBright = (uint8_t)ROUND_TOINT(avgBrightF);

  avgSmoothF = (float)((float)totalSmooth / (float)totalPixels);
  avgSmooth = (uint8_t)ROUND_TOINT(avgSmoothF);

  // SAVE DEBUG VALUES
  FrameInfoArgs->frame_SmoothMax = maxSmooth;
  FrameInfoArgs->frame_SmoothMin = minSmooth;
  FrameInfoArgs->frame_Max = maxSrc;
  FrameInfoArgs->frame_Min = minSrc;
  FrameInfoArgs->frame_SmoothAvgLuma = avgSmooth;
  FrameInfoArgs->frame_Avg = avgSrc;
  FrameInfoArgs->totalBrightSrc = totalBrightSrc;
  FrameInfoArgs->totalBrightSmooth = totalBrightSmooth;

  //  FrameInfoArgs->totalBrightSmooth   = totalBrightSmooth;
  //  FrameInfoArgs->totalBrightSrc      = totalBrightSrc;

  FrameInfoArgs->frame_avgBrightSmooth = (uint8_t)ROUND_TOINT(totalBrightSmooth / totalPixels);
  FrameInfoArgs->frame_avgBrightSrc = (uint8_t)ROUND_TOINT(totalBrightSrc / totalPixels);


  FrameInfoArgs->AvgLuma = (uint8_t)ROUND_TOINT(totalBrightSrc / totalPixels);


  //brightStart=205,brightAmt=31
  //darkAmt=darkAmt, darkStart=darkStart

    //   0 <= avgSrc <= brightStart <= maxSmooth <= maxSrc <= 255
    //
    //
    //  minSmooth   >=0              always true Obviously
    //  darkStart   >=0        always true Obviously
    //  avgSrc    >=0            always true Obviously
    //  avgSmooth   >=0        always true Obviously
    //
    //  minSrc      <= minSmooth  always true
    //  maxSmooth   <= maxSrc    always true
    //
    // brightStart  <= 255      always true Obviously
    // maxSmooth    <= 255      always true Obviously
    // maxSrc       <= 255      always true Obviously
    //
    //
    //
    //   dist1 = brightStart - avgSrc          larger  more maxSmooth can be reduced.  I.E. trim more bright pixels
    //   dist2 = brightStart - maxSmooth       larger  more maxSmooth can be reduced.
    //   dist3 = maxSrc      - maxSmooth       Smaller more maxSmooth can be reduced.
    //   dist4 = 255         - maxSrc          larger  more maxSmooth can be reduced.

    // The following code reduces the level of ultra bright highlights.
    // The ultra bright highlights are often introduced by noise or by bit starved block encoding.
    //
    // The ultra bright highlights are compressed downward by
    //   1: finding the new value the current brightest value of the smoothed frame will have.
    //
    //   2: Compressing the brightness range from brightStart to maxSmooth to the range brightStart to newMaxSmooth
    //    The compression uses gamma = XXX  so the brightest pixels are compressed more then the darker pixels.
    //
    // Two metrics are used to reduce the level of ultra bright highlights.
    // we determine a new maximum value "maxSmooth" for the smoothed frame.
    // maxSmooth is determined by using the brightest pixel value after the brightest 1 percent of the pixels have been discarded.
    // All values from brightStart to 255 are compressed to the range brightStart to maxSmooth.
    // Int_FitRange_Int() clips the output to the range brightStart, maxSmooth if avgSmooth is outside of the range brightStart, 255.


    // avgSrc      <= brightStart  brightStart = avgSrc      // THIS IS PROBABLY MORE COMPLLEX
    // brightStart <= maxSmooth

    // WE WANT THE SRC VALUE TO MOVE TO THE SMOOTHED VALUE DEPENDING ON THE BRIGHTNESS AND THE STRENGTH

    //  minMaxSmooth gets larger the smaller the percent of total pixels that are greater than brightStart.
    //  This keeps bright highlights in a dark frame from being washed out.
    //  minMaxSmooth  = (uint8_t)Int_FitRange_Int(avgSmooth, brightStart, 255, brightStart, maxSmooth);

  percentBrightF = (float)((numBright * 100) / totalPixels);
  FrameInfoArgs->percentBright = (uint8_t)ROUND_TOINT(percentBrightF);

  if (maxSmooth < brightStart) minMaxSmooth = brightStart;

  //minMaxSmooth = brightStart;
  //minMaxSmooth = (uint8_t)Int_FitRange_Int(avgSmooth, brightStart, 255, brightStart, maxSmooth);
  minMaxSmooth = (uint8_t)Int_FitRange_Int(brightStart, avgSmooth, 255, brightStart, maxSmooth);

  if (minMaxSmooth < brightStart) minMaxSmooth = brightStart;

  //  Float_FitRange_Float((float) curVal, (float)src1, (float)src2, (float)dst1, (float)dst2);

  minMaxSmooth = (uint8_t)Int_FitRange_Float(percentBrightF, 0.0F, 20.0F, (float)maxSmooth, (float)minMaxSmooth);
  //  minMaxSmooth = (uint8_t)Int_FitRange_float(percentBright,    0,  20,           maxSmooth,       minMaxSmooth);

  FrameInfoArgs->minMaxSmooth = minMaxSmooth;

  //maxNumBrightPix = ROUND_TOINT(totalPixels / 10);     //  10    %
  //maxNumBrightPix = ROUND_TOINT(totalPixels / 50);     //   2    %
  //maxNumBrightPix = ROUND_TOINT(totalPixels / 100);    //   1    %
  //maxNumBrightPix = ROUND_TOINT(totalPixels / 133);    //   0.75 %
  //maxNumBrightPix = ROUND_TOINT(totalPixels / 200);    //   0.5  %
  maxNumBrightPix = ROUND_TOINT(totalPixels / 200);

  maxNumBrightPix = Int_FitRange_Int(expand, 0, 31, ROUND_TOINT(totalPixels / 200), ROUND_TOINT(totalPixels / 100));

  FrameInfoArgs->maxNumBrightPix = maxNumBrightPix;    // SAVE VALUE FOR DEBUGGING
  total = 0;
  for (x = 255; x > minMaxSmooth; x--) {   // Find the darkest pixel value where less than x% of pixels are brighter.
    if (total > maxNumBrightPix) {
      minSmoothCutoff = x + 1;
      break;
    }
    total += countSmooth[x];
  }

  // minSmoothCutoff is the smallest value that a bright pixel will be dimmed to
  // It is in the range of  brightStart < minSmoothCutoff < maxSmooth.
  FrameInfoArgs->minSmoothCutoff = (uint8_t)minSmoothCutoff;

  if (minSmoothCutoff < minMaxSmooth) minSmoothCutoff = minMaxSmooth;

  FrameInfoArgs->minMaxSmooth = minMaxSmooth;

  useMaxSmoothF = ((float)maxSmooth + (float)minSmoothCutoff) / 2.0F;
  if (useMaxSmoothF < minSmoothCutoff) useMaxSmoothF = (float)minSmoothCutoff;
  if (useMaxSmoothF > maxSmooth)       useMaxSmoothF = (float)maxSmooth;

  if (useMaxSmoothF < brightStart + 3) maxSmooth = (uint8_t)brightStart + 3; //(uint8_t)ROUND_TOINT((brightStart + 255)/2.0F);
  else                               maxSmooth = (uint8_t)ROUND_TOINT(useMaxSmoothF);

  if (maxSmooth > 255)  maxSmooth = 255;

  FrameInfoArgs->maxSmooth = maxSmooth;

  // lowVal starts at brightStart "user argument"
  if (lowVal < minSmoothCutoff) lowVal = minSmoothCutoff;

  lowValF = Float_FitRange_Float(avgSmoothF, useMaxSmoothF, (float)minSmoothCutoff, useMaxSmoothF, (float)lowVal);


  // this needs to build the de highlight lookup table  using equvlent src value, max src value, smooth cutoff value which is lowest smooth value found for for src value <= bright start.
  //int
  //Int_FitRange_Int(
  //int curVal,
  //int src1,
  //int src2,
  //int dst1,
  //int dst2)



    // Build the values for the SmoothDeHighlightArr[] lookup table.
  buildSmoothDeHighlight(SmoothDeHighlightArr, avgSmooth, brightAmt, brightStart, useMaxSmoothF, lowValF);

  if (brightStart < maxSmooth) {
    idxRC = 0;
    for (h = 0; h < src_height; h++) {
      // THIS LOOP SHOULD BE SSE2 OPTIMIZED !!!!!!!!!!!!
      for (x = 0; x < src_width; x++) {
        uint8_t   newSmoothVal = psmoothed[idxRC];

        if (newSmoothVal < brightStart) {
          idxRC++;
          continue;
        }

        psmoothed[idxRC] = SmoothDeHighlightArr[newSmoothVal];
        idxRC++;
      }
    }
  }


  buildBrightProtectDifVals(BrightProtectDifArr, brightStart, brightAmt, maxSrc, expand);
  FrameInfoArgs->buildBrightProtectDifValsDone = 1;

#ifdef ARCH_IS_IA32
  _mm_empty();
#endif
  return(0);
}




//  THE _xmm VERSION OF THE LOOP IS BROKEN  SEE END OF THIS FILE
//avgAccum_xmm(PsharpP, BF_accumP, src_height*src_width, round, numShiftsHiBit);
//
//  _mm_empty()
//  return(0);




// This is used by sharpSource() in FrameInfo.c
// It outputs the frame to frame_sharp without touching the src or dest frames.
uint8_t  avgDctLoopAccumSharp(FrameInfo_args* FrameInfoArgs) {

  uint16_t* BF_accumP = FrameInfoArgs->MemoryArgs->DctLoopArgs[0].BF_accumP;
  uint8_t* PsharpP = FrameInfoArgs->frame_sharp;

  uint8_t      shift = FrameInfoArgs->shift;
  uint8_t     round = FrameInfoArgs->round[shift];
  uint8_t     numShiftsHiBit = FrameInfoArgs->numShiftsHiBit[shift];
  uint16_t     src_height = FrameInfoArgs->src_height;
  uint16_t     src_width = FrameInfoArgs->src_width;

  uint16_t h, x;
  uint32_t idxRC = 0;

  for (h = 0; h < src_height; h++) {    // THIS LOOP SHOULD BE SSE2 OPTIMIZED !!!!!!!!!!!!!!!!!!
    for (x = 0; x < src_width; x++) {
      PsharpP[idxRC] = (uint8_t)((BF_accumP[idxRC] + round) >> numShiftsHiBit);
      idxRC++;
    }
  }

#ifdef ARCH_IS_IA32
  _mm_empty();
#endif
  return(0);
}


//  for (h = 0; h < src_height; h++) {
//    for (x = 0; x < src_width; x++) {
//      uint8_t newPixVal = (uint8_t)((BF_accumP[x] + round) >> numShiftsHiBit);
//      uint8_t smoothVal = Psmoothed[x];
//      uint8_t srcVal    = PworkSource[x];
//      uint8_t boundVal  = Pframe_bound[x];
//
//      if (doSharp)  newPixVal = doEndPointLimit(AccumLimitArr, newPixVal, srcVal);
//      if (doExpand)   newPixVal = doEndPointBndLimit(AccumLimitArr,  AccumLimitBoundArr, srcVal, newPixVal, boundVal);
//
//      if (lastFilter) {  // doDark is set in amDCTmain.c
//        if (doBright)  newPixVal = doBrightSmoothed(BrightProtectDifArr, newPixVal, smoothVal, brightStart);
//        if (doDark)    newPixVal =    doDarkProtect(  DarkProtectDifArr, newPixVal,    srcVal,   darkStart);
//      }
//
//      PsrcMem[x]     = newPixVal;
//      PworkSource[x] = newPixVal;
//    }
//
//    Psmoothed    += src_width;
//    Pframe_bound += src_width;
//    PworkSource  += src_width;
//    PsrcMem      += src_width;
//    BF_accumP    += src_width;
//  }






 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//   The following routines are used to change the difference between the new pixel value and the original pixel value.
//   The change is done by using precomputed lookup tables that are created once per frame.  See code in BuildPreCompVals.c
//   This provides a huge speedup in overall processing.
//
//    doEndPointLimit()
//    doEndPointBndLimit()
//    doBrightSmoothed()
//    doDarkProtect()
//
//   They are called from avgDctLoopAccum()
//      if (doSharp)  newPixVal = doEndPointLimit(AccumLimitArr, newPixVal, srcVal);
//      if (doExpand)   newPixVal = doEndPointBndLimit(AccumLimitArr,  AccumLimitBoundArr, srcVal, newPixVal, boundVal);
//
//      if (lastFilter) {
//         if (doBright)  newPixVal = doBrightSmoothed(BrightProtectDifArr, newPixVal, smoothVal, brightStart);
//        if (doDark)    newPixVal =    doDarkProtect(DarkProtectDifArr,   srcVal,    newPixVal, darkStart);
//      }




//     As pixel values "newPixVal" approach 0 or 255 it uses AccumLimitArr[] to gradually move the value of the pixel toward the pixel value of the "src" pixel.
__inline uint8_t   doEndPointLimit(uint8_t* AccumLimitArr, uint8_t newPixVal, uint8_t src) {

  int16_t diff = src - newPixVal;

  if (diff != 0) {
    uint8_t Absdiff = (uint8_t)abs(diff);
    uint8_t absDiff2 = AccumLimitArr[(src << ACCUM_LIMIT_SHARP_SHIFT) + Absdiff];     // Do limit expand.  absDiff2 <= Absdiff

    if (diff >= 0) diff = absDiff2;
    else           diff = -absDiff2;

    src = (uint8_t)(src - diff);
  }

  return(src);
}




//     As pixel values "newPixVal" approach 0 or 255 it uses AccumLimitArr[]      to gradually move the value of the pixel         toward the pixel value of the "src" pixel.
//     If the pixel value is near a boundary,  it then  uses AccumLimitBoundArr[] to gradually move the value of the pixel further toward the pixel value of the "src" pixel.
__inline uint8_t   doEndPointBndLimit(uint8_t* AccumLimitArr, uint8_t* AccumLimitBoundArr, uint8_t newPixVal, uint8_t src, uint8_t bndVal) {

  int16_t diff = src - newPixVal;

  if (diff != 0) {
    uint8_t Absdiff = (uint8_t)abs(diff);
    uint8_t absDiff2 = AccumLimitArr[(src << ACCUM_LIMIT_SHARP_SHIFT) + Absdiff];     // Do limit expand.  absDiff2 <= Absdiff

    absDiff2 = AccumLimitBoundArr[(bndVal << ACCUM_LIMIT_SMOOTH_BOUND_SHIFT) + absDiff2];

    if (diff >= 0) diff = absDiff2;
    else           diff = -absDiff2;

    src = (uint8_t)(src - diff);
  }

  return(src);
}





//      For pixel values "newPixVal" larger than brightStart it gradually moves the value of the pixel toward the pixel value of the very smoothed frame "smooth"
__inline uint8_t   doBrightSmoothed(uint8_t* BrightProtectDifArr, uint8_t newPixVal, uint8_t smooth, uint8_t brightStart) {

  int16_t diff = newPixVal - smooth;
  int16_t Absdiff = (uint8_t)abs(diff);

  if (newPixVal <= brightStart) return(newPixVal);


  if (newPixVal > brightStart && Absdiff) {
    uint8_t absDiff2 = BrightProtectDifArr[(newPixVal << BRIGHT_SMOOTH_SHIFT) + Absdiff];
    int16_t diff2;

    if (diff >= 0) diff2 = absDiff2;
    else           diff2 = -absDiff2;

    newPixVal = (uint8_t)(newPixVal - diff2);
  }

  if (newPixVal <= brightStart) return(brightStart);

  return(newPixVal);
}




//      For pixel values "newPixVal" smaller than darkStart it gradually moves the smooth pixel value toward the newPixVal pixel value.
__inline uint8_t   doDarkProtect(uint8_t* DarkProtectDifArr, uint8_t newPixVal, uint8_t src, uint8_t darkStart) {

  int16_t diff = newPixVal - src;
  uint8_t Absdiff = (uint8_t)abs(diff);

  if (newPixVal < darkStart && Absdiff) {
    uint8_t absDiff2 = DarkProtectDifArr[(newPixVal << DARK_PROTECT_SHIFT) + Absdiff];
    int16_t diff2;

    if (diff >= 0) diff2 = absDiff2;
    else           diff2 = -absDiff2;

    newPixVal = (uint8_t)(newPixVal - diff2);
  }

  return(newPixVal);
}










// /*
// FROM HERE TO END OF FILE IN NOT CURRENTLY BEING USED  POSSIBLY FOR FUTURE RELEASE !!!

// This is used by LumaBlockInfo() in LumaBlockInfo.c to produce a per block matrix for presmoothing.
// It outputs the frame to frame_grainMask without touching the src or dest frames.
uint8_t  avgDctLoopAccumDCT(FrameInfo_args* FrameInfoArgs) {

  //  uint8_t     *PworkSource   = FrameInfoArgs->frame_workSource;
  //  uint8_t    *PsrcMem        = FrameInfoArgs->MemoryArgs->DctLoopArgs[0].BF_srcP;

  uint16_t* BF_accumP = FrameInfoArgs->MemoryArgs->DctLoopArgs[0].BF_accumP;
  uint8_t* PgrainP = FrameInfoArgs->frame_grainMask;

  //  uint8_t      shift          = FrameInfoArgs->shift;
  //  uint8_t     round          = FrameInfoArgs->round[shift];
  //  uint8_t     numShiftsHiBit = FrameInfoArgs->numShiftsHiBit[shift];
  uint16_t     src_height = FrameInfoArgs->src_height;
  uint16_t     src_width = FrameInfoArgs->src_width;

  uint16_t h, x;
  //  uint32_t idxRC=0;

  for (h = 0; h < src_height; h++) {
    // THIS LOOP SHOULD BE SSE2 OPTIMIZED !!!!!!!!!!!!!!!!!!
    for (x = 0; x < src_width; x++) {
      //uint8_t temp = abs(BF_accumP[idxRC]);
      uint16_t temp = (uint16_t)abs(BF_accumP[x]);
      //temp = (uint8_t)Int_fitRangeGamma_Int(temp, 0, 255, 1.8, 64, 255);
      if (temp > 255) temp = 255;

      PgrainP[x] = (uint8_t)temp;
      //PsrcMem[x]     = temp;
      //PworkSource[x] = temp;

//      PgrainP[idxRC] = (uint8_t)((BF_accumP[idxRC] + round)>>numShiftsHiBit);
//      PgrainP[idxRC] = (uint8_t)Int_fitRangeGamma_Int(abs(BF_accumP[idxRC]), 0, 255, 1.8, 42, 255);
//      PgrainP[idxRC] = (uint8_t)Int_fitRangeGamma_Int(abs(BF_accumP[idxRC]), 0, 255, 1.8, 64, 255);
      //PgrainP[idxRC] = (uint8_t)temp;
//      PworkSource[idxRC] = (uint8_t)temp;
//      PsrcMem[idxRC] = (uint8_t)temp;
      //idxRC++;
    }
    //PworkSource  += src_width;
    //PsrcMem      += src_width;
    PgrainP += src_width;
    BF_accumP += src_width;
  }

#ifdef ARCH_IS_IA32
  _mm_empty();
#endif
  return(0);
}
//*/


/*
// PsharpP[idxRC]  = (uint8_t)((BF_accumP[idxRC] + round)>>numShiftsHiBit);
// dst = PsharpP changes to uint8_t
// len = src_width
// src = BF_accumP
//__forceinline
__inline void avgAccum_xmm(uint8_t *dst, int16_t * const src, int32_t len, uint8_t round, uint8_t numShiftsHiBit) {

  __asm {
     ALIGN 16
     mov     edx, dst
     mov     esi, src
     mov     ecx, len

  MainLoop:
    movapd    xmm0, [EDX]
    movapd    xmm1, [ESI]
    movapd    xmm2, [EDX + 8]
    movapd    xmm3, [ESI + 16]
    movapd    xmm4, [EDX + 16]
    movapd    xmm5, [ESI + 32]
    movapd    xmm6, [EDX + 24]
    movapd    xmm7, [ESI + 48]

    paddw     xmm0, xmm1
    paddw     xmm2, xmm3
    paddw     xmm4, xmm5
    paddw     xmm6, xmm7

    movapd   [EDX], xmm0
    movapd   [EDX + 8], xmm2
    movapd   [EDX + 16], xmm4
    movapd   [EDX + 24], xmm6

    add       EDX, 32
    add       ESI, 64
    sub       ECX, 32
    jnz       MainLoop

    emms
  }

  return;
}
*/

