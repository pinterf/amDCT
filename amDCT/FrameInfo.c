#include <math.h>

#include "FrameInfo.h"

#include "amDCTtypedefs.h"
#include "Memory.h"

#include "BuildPreCompVals.h"
#include "Utilities.h"
#include "avgDctLoop.h"

#include "Sharp.h"
#include "LumaFrameInfo.h"
#include "LumaBlockInfo.h"
#include "Matrix.h"
#include "DispatchLoop.h"

#include "DctLoop.h"
#include "Threading.h"

#include "blindPPcode.h"
#include "smoothFilters.h"
#include "MaxMinFilter.h"


void smoothedSource(FrameInfo_args* FrameInfoArgs, uint8_t type);
void sharpSource(FrameInfo_args* FrameInfoArgs);
void fixSaltPepperNoise(FrameInfo_args* FrameInfoArgs);
void fixSaltPepperNoise1(FrameInfo_args* FrameInfoArgs);
void fixSharpSmoothDif(FrameInfo_args* FrameInfoArgs);
void makeDeringMask(FrameInfo_args* FrameInfoArgs);
void maxMin_3x3mean(FrameInfo_args* FrameInfoArgs);








//  This is called once by amDCTmain for each frame.
//  It calls malloc_data(FrameInfoArgs)
//  It initializes all of the internal data structures.
//  It initializes the following lookup tables.
//    buildAccumLimitArr
//    buildAccumLimitArrSmoothBoundVals
//    buildAvgLumaBlockProtectVals
//    buildDarkProtectDifVals
//
int  create_frame_info(FrameInfo_args* FrameInfoArgs,
  uint8_t* psrc,
  //             uint8_t         *pf1,
  //             uint8_t         *nf1,
  uint8_t* pdst,
  uint8_t          quant_a,
  uint8_t          adapt_a,

  uint16_t         src_width,
  uint16_t         src_pitch,
  uint16_t         src_height,
  uint16_t         dst_width,
  uint16_t         dst_pitch,
  uint16_t         dst_height,





  uint8_t          brightStart,
  uint8_t          brightAmt,
  uint8_t          darkStart,
  uint8_t          darkAmt,

  uint8_t          doSmoothFlag,
  uint8_t          doSharpFlag,
  uint8_t          doExpandFlag,
  uint8_t          doBrightFlag,
  uint8_t          doDarkFlag,

  uint8_t          ncpu,
  uint8_t       quality,
  uint8_t          T2) {

  // In order to reduce edge of frame artifacts  amDCTmain.c has copied the original src frame into the center of a frame "psrc" that
  // has an 8 pixel wide boarder on all 4 sides.  That boarder is filled with the mirror of the 8 pixels edge of the original frame.
  // psrc points to the frame with the 8 pixel boarder surronding it.
  // src_width and src_height are the width and height of psrc.
  uint32_t       sizeY = src_width * src_height;   // Size of the luma "Y" plane in bytes
  unsigned int   numBlocks_sizeY = sizeY >> 6;                 // The number of blocks in the luma frame.
  unsigned int   numBlocks_high = 2 * ((src_height + 15) / 16);   // The number of rows in blocks rounded up + 1.
  unsigned int   numBlocks_wide = 2 * ((src_width + 15) / 16);

  // Clear FrameInfoArgs then store the argument info in it.
  memset(FrameInfoArgs, 0, sizeof(FrameInfo_args));        // FrameInfoArgs was allocated on the stack in amDCT.c

  FrameInfoArgs->sizeY = sizeY;                   // Size (number of bytes) of the luma "Y" plane.
  FrameInfoArgs->numBlocks_sizeY = numBlocks_sizeY;         // The number of blocks in the luma "Y" plane.
  FrameInfoArgs->numBlocks_high = numBlocks_high;          // The number of rows in blocks rounded up + 1.
  FrameInfoArgs->numBlocks_wide = numBlocks_wide;


  FrameInfoArgs->numBlocksY = numBlocks_high * numBlocks_wide;            // The total number of blocks in the frame. We store per block info in this many blocks.
  FrameInfoArgs->sizeBlocksY = numBlocks_high * numBlocks_wide * 64;         // Size of the buffer that the processing will be done on in pixels.  It should be 16 pixels wider and taller then the luma buffer.
  FrameInfoArgs->numBlocksWork = (numBlocks_high + 1) * (numBlocks_wide + 1);    // Size total number of blocks that the processing will be done on.   It should be 2 blocks wider and taller then the luma buffer.
  FrameInfoArgs->sizeBlocksWork = (numBlocks_high + 1) * (numBlocks_wide + 1) * 64; // Size of the buffer that the processing will be done on in pixels.  It should be 16 pixels wider and taller then the luma buffer.


  FrameInfoArgs->ncpu = ncpu;
  FrameInfoArgs->psrc = psrc;
  //  FrameInfoArgs->pf1           = pf1;       // Previous Frame
  //  FrameInfoArgs->nf1           = nf1;     // Next     Frame
  FrameInfoArgs->pdst = pdst;
  FrameInfoArgs->quant = quant_a;
  FrameInfoArgs->adapt = adapt_a;
  FrameInfoArgs->quant_a = quant_a;  // The unmodified quant argument
  FrameInfoArgs->adapt_a = adapt_a;  // The unmodified adapt argument
  FrameInfoArgs->src_width = src_width;
  FrameInfoArgs->src_pitch = src_pitch;
  FrameInfoArgs->src_height = src_height;
  FrameInfoArgs->dst_width = dst_width;
  FrameInfoArgs->dst_pitch = dst_pitch;
  FrameInfoArgs->dst_height = dst_height;
  FrameInfoArgs->quality = quality;
  FrameInfoArgs->T2 = T2;


  if (brightAmt == 0) brightStart = 255;
  if (darkAmt == 0) darkStart = 0;

  FrameInfoArgs->brightAmtUser = brightAmt;  // The unmodified User Specified value
  FrameInfoArgs->brightAmt = brightAmt;  // The Value Used Internally
  FrameInfoArgs->brightStartUser = brightStart;  // The unmodified User Specified value
  FrameInfoArgs->brightStart = brightStart;  // The Value Used Internally

  FrameInfoArgs->darkAmtUser = darkAmt;    // The unmodified User Specified value
  FrameInfoArgs->darkAmt = darkAmt;    // The Value Used Internally
  FrameInfoArgs->darkStartUser = darkStart;  // The unmodified User Specified value
  FrameInfoArgs->darkStart = darkStart;  // The Value Used Internally

  FrameInfoArgs->doBrightFlag = doBrightFlag;
  FrameInfoArgs->doDarkFlag = doDarkFlag;
  FrameInfoArgs->doSmoothFlag = doSmoothFlag;
  FrameInfoArgs->doExpandFlag = doExpandFlag;
  FrameInfoArgs->doSharpFlag = doSharpFlag;
  FrameInfoArgs->doBoundFlag = 0;

  //  if (quality >= 3)    FrameInfoArgs->doBoundFlag = 1;  // It probably makes more sense to set this to 1 in buildMasks()
  if (quality > 3)    FrameInfoArgs->doBoundFlag = 1;  // It probably makes more sense to set this to 1 in buildMasks()

  if (quality >= 4 && doExpandFlag) FrameInfoArgs->doBlkAdapExpandFlag = 1;


  FrameInfoArgs->buildMasksDone = 0;
  FrameInfoArgs->smoothFrameDone = 0;
  FrameInfoArgs->sharpFrameDone = 0;
  FrameInfoArgs->buildBrightProtectDifValsDone = 0;
  FrameInfoArgs->quality6Done = 0;
  FrameInfoArgs->LumaBlockInfoDone = 0;



  // Now we malloc all of the data we need for amDCT()
  // malloc_data() will use most of the information above to determine the amount of memory to malloc.
  // This allows a single routine free_data() which is called from various places in amDCT.c to free all of the malloced data.
  // There are a total of 39 malloc() calls in Memory.c of which 19 are called multiple times depending upon the
  // number of CPU's specified,  and 9 of those are called 31 times if adapt is set.
  // IT IS SO MUCH EASIER TO DEBUG MEMOY PROBLEMS, LEAKS, FORGOT TO ALLOCATE, ALLOCATED TWICE ETC. IF WE DO ALL MALLOC AND FREE IN ONE ROUTINE.
  if (malloc_data(FrameInfoArgs) != 0)
    return(1);

  // Setup "local" pointers to the memory that has just been maloced.
  FrameInfoArgs->frame_adaptSource = FrameInfoArgs->MemoryArgs->frame_adaptSource;        // The original source frame after adapt has been done.       Used as a reference frame and is not changed during processing.
  FrameInfoArgs->frame_workSource = FrameInfoArgs->MemoryArgs->frame_workSource;         // The source frame as it is being modified by each routine.
  FrameInfoArgs->frame_3x3sadm = FrameInfoArgs->MemoryArgs->frame_3x3sadm;
  FrameInfoArgs->frame_3x3mean = FrameInfoArgs->MemoryArgs->frame_3x3mean;
  FrameInfoArgs->frame_ImpulseNoise = FrameInfoArgs->MemoryArgs->frame_ImpulseNoise;
  FrameInfoArgs->frame_smoothed = FrameInfoArgs->MemoryArgs->frame_smoothed;
  FrameInfoArgs->frame_sharp = FrameInfoArgs->MemoryArgs->frame_sharp;
  FrameInfoArgs->frame_boundaries = FrameInfoArgs->MemoryArgs->frame_boundaries;          // (max9x9-min9x9) + sad
  FrameInfoArgs->frame_deringMask = FrameInfoArgs->MemoryArgs->frame_deringMask;
  FrameInfoArgs->frame_localMax9x9 = FrameInfoArgs->MemoryArgs->frame_localMax9x9;
  FrameInfoArgs->frame_localMin9x9 = FrameInfoArgs->MemoryArgs->frame_localMin9x9;
  FrameInfoArgs->frame_grainMask = FrameInfoArgs->MemoryArgs->frame_grainMask;
  FrameInfoArgs->frame_sharpSmoothDif = FrameInfoArgs->MemoryArgs->frame_sharpSmoothDif;





  // Build lookup tables for repetitive calculations.
  buildAccumLimitArr(FrameInfoArgs->MemoryArgs->AccumLimitArr, FrameInfoArgs->brightStart);
  buildAccumLimitArrSmoothBoundVals(FrameInfoArgs->MemoryArgs->AccumLimitSmoothBoundArr);
  buildAvgLumaBlockProtectVals(FrameInfoArgs->MemoryArgs->AvgLumaBlockProtectArr, quant_a, adapt_a, darkStart, darkAmt, brightStart, brightAmt, 255);

  buildDarkProtectDifVals(FrameInfoArgs->MemoryArgs->DarkProtectDifArr, FrameInfoArgs->darkStart, FrameInfoArgs->darkAmt);
  FrameInfoArgs->buildDarkProtectDifValsDone = 1;



  ////  FrameInfoArgs->pf1           = pf1;
  ////  FrameInfoArgs->nf1           = nf1;


                        // It might be interesting to add a 2 shift version.  See if either (diag up left, up right) or (vertical up, diag down right) are interesting.
  FrameInfoArgs->numShifts[0] = 1;   // Number of shifted frames that are added into the accumulator.
  FrameInfoArgs->numShifts[1] = 4;
  FrameInfoArgs->numShifts[2] = 8;
  FrameInfoArgs->numShifts[3] = 16;
  FrameInfoArgs->numShifts[4] = 32;
  FrameInfoArgs->numShifts[5] = 64;

  FrameInfoArgs->numShiftsHiBit[0] = 0;  // Number of bits to right shift the accumulator to do the average.
  FrameInfoArgs->numShiftsHiBit[1] = 2;
  FrameInfoArgs->numShiftsHiBit[2] = 3;
  FrameInfoArgs->numShiftsHiBit[3] = 4;
  FrameInfoArgs->numShiftsHiBit[4] = 5;
  FrameInfoArgs->numShiftsHiBit[5] = 6;

  FrameInfoArgs->round[0] = 0;  // Number to add to the accumulator so the result of the right shift will be rounded appropriately.
  FrameInfoArgs->round[1] = 2;
  FrameInfoArgs->round[2] = 4;
  FrameInfoArgs->round[3] = 8;
  FrameInfoArgs->round[4] = 16;
  FrameInfoArgs->round[5] = 32;



  FrameInfoArgs->num_frames = 1;
  //  if (pf1 != NULL) FrameInfoArgs->num_frames += 1;  // Previous frame processing
  //  if (nf1 != NULL) FrameInfoArgs->num_frames += 1;  // Next     frame processing
  //  MY_MEMCPY(&FrameInfoArgs->MemoryArgs.BF_pf1[0][0], pf1,  sizeof(uint8_t) * sizeY);
  //  MY_MEMCPY(&FrameInfoArgs->MemoryArgs.BF_nf1[0][0], nf1,  sizeof(uint8_t) * sizeY);


  MY_MEMCPY(FrameInfoArgs->MemoryArgs->frame_adaptSource, psrc, sizeof(uint8_t) * sizeY);
  MY_MEMCPY(FrameInfoArgs->MemoryArgs->frame_workSource, psrc, sizeof(uint8_t) * sizeY);
  MY_MEMCPY(FrameInfoArgs->MemoryArgs->DctLoopArgs[0].BF_srcP, psrc, sizeof(uint8_t) * sizeY);
  MY_MEMCPY(FrameInfoArgs->MemoryArgs->frame_grainMask, psrc, sizeof(uint8_t) * sizeY);   // JUST FOR TESTING

  FrameInfoArgs->SharpSmoothDifDone = 0;

  // If doDCTadapt is 1 then block adaptive processing is done in the DCT loop.
  //    doDCTadapt is Turned off, doDCTadapt==0, while in sharpSource() and smoothedSource().
  //    It is also    Turned off, if quality < 3
  FrameInfoArgs->doDCTadapt = 0;   // Turned off, doDCTadapt=0, while in sharpSource() and smoothedSource().
  if (FrameInfoArgs->quality >= 3) FrameInfoArgs->doDCTadapt = 1;  // If 1 then do smooth, expand, and sharp strength block adaptive processing in the DCT loop, otherwise just do full strength.


  return(0);
}  // END OF    create_frame_info()










//  Set info for a DCT Loop pass.
void    doFrameInfo(uint8_t      FrameQuant,
  uint8_t      quant,
  uint8_t      qtype,
  uint8_t      adapt,
  uint8_t      shift,
  uint8_t      matrix,
  uint8_t      expand,
  uint8_t      sharpWPos,
  uint8_t      sharpWAmt,
  uint8_t      sharpTPos,
  uint8_t      sharpTAmt,
  FrameInfo_args* FrameInfoArgs,
  uint8_t      showMask,
  uint8_t      T2) {


  FrameInfoArgs->FrameQuant = FrameQuant;
  FrameInfoArgs->quant = quant;
  FrameInfoArgs->qtype = qtype;
  FrameInfoArgs->adapt = adapt;
  FrameInfoArgs->shift = shift;
  FrameInfoArgs->matrix = matrix;
  FrameInfoArgs->expand = expand;
  FrameInfoArgs->sharpWPos = sharpWPos;
  FrameInfoArgs->sharpWAmt = sharpWAmt;
  FrameInfoArgs->sharpTPos = sharpTPos;
  FrameInfoArgs->sharpTAmt = sharpTAmt;
  FrameInfoArgs->showMask = showMask;
  FrameInfoArgs->T2 = T2;
  FrameInfoArgs->max_quant = MAX(quant, adapt);
  FrameInfoArgs->sharpMaxWT = MAX(sharpWAmt, sharpTAmt);

  buildAccumLimitArrBoundVals(FrameInfoArgs->MemoryArgs->AccumLimitBoundArr, expand);

  return;
}











///////////////////////////////////////////////////////////////////
//
//           BUILDMASKS()
//
//  buildMasks() determines what auxiliary processing needs to be done.
//  buildMasks() or one of its subroutines, might be called multiple times during processing.
//         As a result it remembers what has been done to eliminate duplicate work.
//  It uses the following user arguments:
//
//      quality
//      expand
//    sharpWAmt   sharpTAmt
//      brightAmt   brightStart   (bright processing)
//    darkAmt     darkStart     (dark   processing)
//
//  What the various masks are used for.
//     smoothedSource       Provides a very smoothed frame
//      required for  bright processing
//      quality >= 4    LumaFrameInfo()
//                  boundary mask
//
//
//  It uses the following flags to keep from running routines more than once.  The flags are initialized to 0 by create_frame_info().
//      FrameInfoArgs->smoothFrameDone             // Flag  if 1 then the smooth frame has been made.  Set and Tested by doFrameInfo().
//      FrameInfoArgs->buildBrightProtectDifValsDone     // Flag  if 1 then Bright smooth lookup table built.  Set and Tested by doFrameInfo().
//      FrameInfoArgs->LumaBlockInfoDone           // Flag  if 1 then LumaBlockInfo() is run.  Set and Tested by doFrameInfo().
//
//////////////////////////////////////////////////////////////////////////////

// Build the masks necessary to clean up the noise and artifacts that can occur
// when doing sharpening or range expansion.
void  buildMasks(FrameInfo_args* FrameInfoArgs) {

  uint8_t  quality = FrameInfoArgs->quality;
  uint8_t radiusX = 1;
  uint8_t radiusY = 1;
  uint8_t strength = 1;


  //  Build the frames necessary for various showmask arguments.
  switch (FrameInfoArgs->showMask) {

  case SHOW_ADAPT_SRC:   // adapt
  case SHOW_WORK_SRC:   // adapt
    LumaBlockInfo(FrameInfoArgs);
    return;

  case SHOW_SMOOTHED:   // frame_smoothed
    smoothedSource(FrameInfoArgs, 0);  // If brightStart and brightAmt are set it also does bright dehighlighting.
    return;

  case SHOW_SHARP:    // frame_sharp
    sharpSource(FrameInfoArgs);
    return;

  case SHOW_BOUNDARIES:    // frame_boundaries
    smoothedSource(FrameInfoArgs, 0);
    LumaFrameInfo(FrameInfoArgs);
    return;

    //  case 10:    // frame_ImpulseNoise
    //    uint16_t   src_width  = FrameInfoArgs->src_width;
    //    uint16_t   src_height = FrameInfoArgs->src_height;
    //    uint8_t    *psrc       = FrameInfoArgs->psrc;
    //    UnDot(src_width, src_width, src_width, psrc, psrc, src_height);
    ////    copyFrameToDst(FrameInfoArgs, 5);
    ////return;
    ////    for (int i=0; i<FrameInfoArgs->sizeY; i++) FrameInfoArgs->frame_ImpulseNoise[i] = abs(FrameInfoArgs->psrc - FrameInfoArgs->frame_grainMask);
    //    makeDeringMask(FrameInfoArgs);
    //    smoothedSource(FrameInfoArgs, 0);
    //    LumaFrameInfo(FrameInfoArgs);
    ////    fixSaltPepperNoise(FrameInfoArgs);
    //    copyFrameToDst(FrameInfoArgs, 10);
    //    return;


  case SHOW_SADM:     //frame_3x3sadm
    radiusX = 1;
    radiusY = 1;
    strength = 1;
    gaussianBlur2(FrameInfoArgs, FrameInfoArgs->frame_workSource, FrameInfoArgs->frame_3x3mean, radiusX, radiusY, strength);

    radiusX = 1;
    radiusY = 1;
    sadWindow(FrameInfoArgs, FrameInfoArgs->frame_workSource, FrameInfoArgs->frame_3x3sadm, FrameInfoArgs->frame_3x3mean, radiusX, radiusY);
    return;

  case SHOW_3X3MEAN:   // frame_3x3mean
    radiusX = 1;
    radiusY = 1;
    strength = 1;
    gaussianBlur2(FrameInfoArgs, FrameInfoArgs->frame_workSource, FrameInfoArgs->frame_3x3mean, radiusX, radiusY, strength);
    return;

    //  case SHOW_DERING:  // frame_deringMask
    //    makeDeringMask(FrameInfoArgs);
    //    return;

    //  case 14:     // frame_grainMask
    //    sharpSource(FrameInfoArgs); //////   NOT CURRENTLY WORKING !!!!
    //    return;

  case SHOW_LOCALMAX9X9:     // frame_localMax9x9
    //smoothedSource(FrameInfoArgs, 0);
    radiusX = 4;
    radiusY = 4;
    maxFrame(FrameInfoArgs, FrameInfoArgs->frame_workSource, FrameInfoArgs->frame_localMax9x9, radiusX, radiusY);
    //LumaFrameInfo(FrameInfoArgs);
    return;

  case SHOW_LOCALMIN9X9:     // frame_localMin9x9
    //smoothedSource(FrameInfoArgs, 0);
    radiusX = 4;
    radiusY = 4;
    minFrame(FrameInfoArgs, FrameInfoArgs->frame_workSource, FrameInfoArgs->frame_localMin9x9, radiusX, radiusY);
    return;

    //  case 17:     // frame_sharpSmoothDif     // NOTE NOT CURRENTLY BEING USED!!! TMP USE FOR HIGHLIGHT ARRAY  in avgDctLoopAccumSmoothed()
    //    smoothedSource(FrameInfoArgs, 0);
    //    sharpSource(FrameInfoArgs);         // sharp is not currently being used. Using source smooth diff seems to work almost as well.
    //    fixSharpSmoothDif(FrameInfoArgs);
    //    return;
    //
    //  case 18:     // show DCT values of the input frame
    //    //smoothedSource(FrameInfoArgs, 0);
    //    //LumaFrameInfo(FrameInfoArgs);
    //    return;
    //
    //  case 19:     // show DCT values of the processed frame
    //    //smoothedSource(FrameInfoArgs, 0);
    //    //LumaFrameInfo(FrameInfoArgs);
    //    return;
    //
    //  case 20:     // // 9x9 max min dif
    //    smoothedSource(FrameInfoArgs, 0);
    //    maxFrame(FrameInfoArgs,  FrameInfoArgs->frame_smoothed, FrameInfoArgs->frame_localMax9x9, 4, 4);  // Build 9x9 max
    //    minFrame(FrameInfoArgs,  FrameInfoArgs->frame_smoothed, FrameInfoArgs->frame_localMin9x9, 4, 4);  // Build 9x9 min
    //
    ////    smoothedSource(FrameInfoArgs, 0);
    //    //LumaFrameInfo(FrameInfoArgs);
    //    return;
    //
    //  case 21:     // (9x9 max min avg) - src
    //    smoothedSource(FrameInfoArgs, 0);
    //    maxFrame(FrameInfoArgs,  FrameInfoArgs->frame_smoothed, FrameInfoArgs->frame_localMax9x9, 4, 4);  // Build 9x9 max
    //    minFrame(FrameInfoArgs,  FrameInfoArgs->frame_smoothed, FrameInfoArgs->frame_localMin9x9, 4, 4);  // Build 9x9 min
    //
    //    //LumaFrameInfo(FrameInfoArgs);
    //    return;
    //
    //  case 22:     // (9x9 max min avg) - src
    //    smoothedSource(FrameInfoArgs, 0);
    //    LumaBlockInfo(FrameInfoArgs);
    //    return;

  default:
    break;
  };




  // For quality 1 or 2 we only have work to do if "bright smoothing" has been requested.
  if (FrameInfoArgs->doBrightFlag == 1 && FrameInfoArgs->smoothFrameDone == 0) {
    smoothedSource(FrameInfoArgs, 0);        // Needed for doBrightSmoothed
    FrameInfoArgs->smoothFrameDone = 1;
  }

  if (quality <= 2) return;
  // END  quality <= 2

  // quality is >= 3
  if (FrameInfoArgs->doSharpFlag == 1 ||
    FrameInfoArgs->doExpandFlag == 1 ||
    FrameInfoArgs->doBrightFlag == 1) {

    // If needed Build the smoothed frame.
    //    Needed for doBrightSmoothed(), in avgDctLoop.c to do extra smoothing in bright
    //    Needed to build the boundaries mask in LumaFrameInfo.c to reduce halos.
    if (FrameInfoArgs->smoothFrameDone == 0) {
      smoothedSource(FrameInfoArgs, 0);     // Needed for doBrightSmoothed(), in avgDctLoop.c and to build the frame_boundaries mask in LumaFrameInfo.c to reduce halos.
      FrameInfoArgs->smoothFrameDone = 1;
    }

    if (quality >= 4)  LumaFrameInfo(FrameInfoArgs);  // Build the frame_boundaries mask to reduce halos.
  }

  if (quality >= 3 && FrameInfoArgs->LumaBlockInfoDone == 0) {  // Needed for more accurate frame adaptive values and for Block adaptive values.
    LumaBlockInfo(FrameInfoArgs);
    FrameInfoArgs->LumaBlockInfoDone = 1;
  }
  return;
}










// Build a very smoothed frame.
//   It should be free of halos otherwise expand will make them much worse.
// fixSaltPepperNoise() should be run first since it keeps outliers from influencing the smoothed frame.
// smoothedSource() does the equivalent of
//           amDCT(quality=1, qtype=4, quant=6,  adapt=0, matrix=1, shift=1)
void smoothedSource(FrameInfo_args* FrameInfoArgs, uint8_t type) {

  uint8_t use_qtype = 2;   // I did say very smoothed.
  uint8_t use_matrix = MATRIX_FLAT3; // Flat_3  all values are 3  This is really a place holder since qtypes == 2 or 4, use the much stronger h263 matricies.
  uint8_t use_quant = 8;   // 8 works well with qtype=2,  6 works well with qtype=4.
  uint8_t use_expand = 0;
  uint8_t shift = 2;
  uint8_t use_ncpu = 1;   // NOTE use_ncpu must be <= FrameInfoArgs->ncpu since that is the most the user told us to use.
  // When only doing a total of 4 shifts, all that we need, the threading overhead negates the speedup of using more then 1 cpu.
  //  IS THIS TRUE OF LARGE FRAMES AS WELL AS SMALL FRAMES.   SPEED TEST THIS

  uint8_t* psrc = FrameInfoArgs->MemoryArgs->DctLoopArgs[0].BF_srcP;
  //  uint8_t  *psmoothed       = FrameInfoArgs->frame_smoothed;
  uint8_t* BF_tmp_8P = FrameInfoArgs->MemoryArgs->DctLoopArgs[0].BF_tmp_8P;
  //  uint8_t  *frame_grainMask = FrameInfoArgs->frame_grainMask;
  uint32_t  sizeY = FrameInfoArgs->sizeY;

  uint8_t   save_use_qtype = FrameInfoArgs->qtype;
  uint8_t   save_matrix = FrameInfoArgs->matrix;
  uint8_t   save_quant = FrameInfoArgs->quant;
  uint8_t   quality = FrameInfoArgs->quality;
  uint8_t   save_ncpu = FrameInfoArgs->ncpu;
  uint8_t   save_expand = FrameInfoArgs->expand;
  uint8_t   save_Num_shift = FrameInfoArgs->shift;
  uint8_t   save_doDCTadapt = FrameInfoArgs->doDCTadapt;

  //  uint16_t  src_height      = FrameInfoArgs->src_height;
  uint16_t  src_width = FrameInfoArgs->src_width;

  uint8_t   src_is1080p = 0;
  //  uint8_t   src_is1080p = 1;  // TESTING ONLY !!!!!!!!!!!!!!!!!!!!!!!!!!!!  0 IS THE CORRECT VALUE !!!!!!!!!!!!!!!!!


    // TODO EXPAND BLOCK ARTIFACT DIFFERNCES BETWEEN INTERLACED AND NON  INTERLACED AS WELL AS 4k 1080 720 480 AND SMALLER THAN 480.
    //      ALSO DIFFERENT ENCODING SYSTEMS.
    // 1080i often shows a great deal more blockiness than lower resolution formats so we do extra processing on the smoothed frame.
    // 1080i is specified as being 1920 pixels wide.
    //  720p is specified as being 1280 pixels wide.
    //       so split the difference and you get 1600 pixels wide. Anything wider is considered 1080p

  if (src_width > 100) {  // TESTING TURN ON 1080p MODE
    //if (src_width > 1600) {
    src_is1080p = 1;
    use_qtype = 4;
    use_quant = 6;
  }


  if (quality <= 4) {
    shift = 2;  // 2 a bit smoother.
    use_ncpu = 2;
  }
  if (quality >= 5) {
    shift = 3;  // 3 a very small improvement.
    use_ncpu = 4;
  }

  if (use_ncpu > save_ncpu)  use_ncpu = save_ncpu;  // Don't use more CPUs than the user specified.

  /*
    // grain() uses the following smoothing arguments.
    if (type == 1) {
      use_qtype  = 4;
      use_matrix = MATRIX_FLAT3;  // matrix_252_Flat_3
      use_quant  = 1;
      use_ncpu   = 1;
      use_expand = 0;
      shift      = 0;
    }

    // grain() also uses the following smoothing arguments.
    if (type == 2) {
      use_qtype  = 1;
      use_matrix = MATRIX_FLAT3; // matrix_252_Flat_3
      use_quant  = 1;
      use_ncpu   = 1;
      use_expand = 0;
      shift      = 0;
    }
  */


  FrameInfoArgs->qtype = use_qtype;
  FrameInfoArgs->matrix = use_matrix;
  FrameInfoArgs->quant = use_quant;
  FrameInfoArgs->ncpu = use_ncpu;
  FrameInfoArgs->expand = use_expand;
  FrameInfoArgs->shift = shift;
  FrameInfoArgs->doDCTadapt = 0;  // Don't use block level adaptive strength.


  // Save the psrc
  MY_MEMCPY(BF_tmp_8P, psrc, sizeY);

  //  UnDot(src_width, src_width, src_width, psrc, psrc, src_height);

  /*
    if (quality >= 4) {
      if (src_is1080p == 1) {
        dering(psmoothed, src_height, src_width, 11);
        deblock_horiz_DoDC(psmoothed, src_height, src_width, src_width, 6);
        deblock_vert_DoDC( psmoothed, src_height, src_width, src_width, 6);
      }
      else {
        dering(psmoothed, src_height, src_width, 5);  // dering greater than 5 provides no useful change.
        deblock_horiz_DoDC(psmoothed, src_height, src_width, src_width, 3);
        deblock_vert_DoDC( psmoothed, src_height, src_width, src_width, 3);
      }
    }
  */
  set_matrix(FrameInfoArgs, use_matrix, use_qtype, use_quant, use_expand);
  dup_cpu_data(FrameInfoArgs);
  DispatchLoop(FrameInfoArgs);
  avgDctLoopAccumSmoothed(FrameInfoArgs);   // avgDctLoopAccumSmoothed() outputs the changes into frame_smoothed.
  clear_data_work_buffers(FrameInfoArgs);

  /*
   if (quality >= 4) {
     if (src_is1080p == 1) {
       gaussianBlur2(FrameInfoArgs, psmoothed, FrameInfoArgs->frame_3x3mean, 3, 3, 1);
       MY_MEMCPY(psmoothed, FrameInfoArgs->frame_3x3mean, sizeY);
       dering(psmoothed, src_height, src_width, 11);
       deblock_horiz_DoDC(psmoothed, src_height, src_width, src_width, 6);
       deblock_vert_DoDC( psmoothed, src_height, src_width, src_width, 6);
       }
     else {
       dering(psmoothed, src_height, src_width, 5);  // dering greater than 5 provides no useful change.
       deblock_horiz_DoDC(psmoothed, src_height, src_width, src_width, 3);
       deblock_vert_DoDC( psmoothed, src_height, src_width, src_width, 3);
     }
   }
   */

  MY_MEMCPY(psrc, BF_tmp_8P, sizeY);

  FrameInfoArgs->qtype = save_use_qtype;
  FrameInfoArgs->matrix = save_matrix;
  FrameInfoArgs->quant = save_quant;
  FrameInfoArgs->ncpu = save_ncpu;
  FrameInfoArgs->expand = save_expand;
  FrameInfoArgs->shift = save_Num_shift;
  FrameInfoArgs->doDCTadapt = save_doDCTadapt;

  if (type == 0)  FrameInfoArgs->smoothFrameDone = 1;

  return;
}






// Build the sharp frame which will be used to help clean the original frame of areas that would produce artifacts when doing sharpening.
void sharpSource(FrameInfo_args* FrameInfoArgs) {

  uint8_t use_quality = 1;  // Do it fast.
  uint8_t use_qtype = 22; // 22 does range expansion.  Using 23 which does range expansion and sharpening makes no difference.
  uint8_t use_matrix = MATRIX_SHARP;
  uint8_t use_quant = 0; //1;
  uint8_t use_ncpu = 1;
  uint8_t use_expand = 12; //31;  // was 28 was 21
  uint8_t use_shift = 2;

  uint8_t save_quality = FrameInfoArgs->quality;
  uint8_t save_qtype = FrameInfoArgs->qtype;
  uint8_t save_matrix = FrameInfoArgs->matrix;
  uint8_t save_quant = FrameInfoArgs->quant;
  uint8_t save_ncpu = FrameInfoArgs->ncpu;
  uint8_t save_expand = FrameInfoArgs->expand;
  uint8_t save_shift = FrameInfoArgs->shift;
  uint8_t save_doDCTadapt = FrameInfoArgs->doDCTadapt;


  uint8_t shift = 2;
  if (FrameInfoArgs->quality <= 2) shift = 1;  // 1 has some blockiness across some gradients.
  if (FrameInfoArgs->quality >= 3) shift = 2;  // 2 gets rid of most blockiness across gradients.
  //  if (FrameInfoArgs->quality >= 5 && FrameInfoArgs->T2==61)       shift = 3;  // 3 almost no change from 2.
  use_shift = shift;
  use_shift = 2;

  //if (FrameInfoArgs->T2 != 140)   use_shift =  0;
  //  if (FrameInfoArgs->ncpu >= 2 && use_shift >= 3) {
  //      use_ncpu = 2;
  //  }


  FrameInfoArgs->quality = use_quality;
  FrameInfoArgs->qtype = use_qtype;
  FrameInfoArgs->matrix = use_matrix;
  FrameInfoArgs->quant = use_quant;
  FrameInfoArgs->ncpu = use_ncpu;
  FrameInfoArgs->expand = use_expand;
  FrameInfoArgs->shift = use_shift;



  set_matrix(FrameInfoArgs, use_matrix, use_qtype, use_quant, use_expand);

  FrameInfoArgs->doDCTadapt = 0;  // Don't use block level adaptive strength.  We want to see the uglies.
  dup_cpu_data(FrameInfoArgs);
  DispatchLoop(FrameInfoArgs);
  avgDctLoopAccumSharp(FrameInfoArgs);   // avgDctLoopAccumSharp() outputs the changes into frame_sharp.
  clear_data_work_buffers(FrameInfoArgs);


  FrameInfoArgs->doDCTadapt = save_doDCTadapt;
  FrameInfoArgs->quality = save_quality;
  FrameInfoArgs->qtype = save_qtype;
  FrameInfoArgs->matrix = save_matrix;
  FrameInfoArgs->quant = save_quant;
  FrameInfoArgs->ncpu = save_ncpu;
  FrameInfoArgs->expand = save_expand;
  FrameInfoArgs->shift = save_shift;

  return;
}


