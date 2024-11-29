#include <math.h>
#include <stdio.h>

#include "amDCT.h"
#include "Memory.h"
#include "amDCTtypedefs.h"

#include "amFilters.h"

#include "DispatchLoop.h"
#include "LumaBlockInfo.h"
#include "FrameInfo.h"
#include "Threading.h"
#include "DctLoop.h"
#include "avgDctLoop.h"
#include "Adapt.h"
#include "Sharp.h"
#include "Matrix.h"
#include "blindPPcode.h"
#include "Utilities.h"
//#include "Grain.h"


#include "Font.h"

// #include "BuildPerBlockMatrix.h"
//void  BuildPerBlockMatrix(FrameInfo_args *FrameInfoArgs);



/*

TODO ----------------------

WHAT IS workSource USED FOR!!!
psrcMemP only exists until a clear_data_work_buffers() is done and should be considered temporary.

SSE routines for new code


CODE DOC:
walk a source frame the system.


NOTES ----------------
sharpening should be done before any change in picture size.  Run it before video enhancer super resolution.

sharpening will sharpen (using default values sharpW_NumPix=5, sharpT_NumPix=7) lines that are 1-3 pixels wide that have a small brightness difference with their neighbors.
It does not directly sharpen edges between 2 flat areas that differ in brightness.
Unfortunately edges, especially with large changes in brightness, often produce a myriad of small brightness variations which sharpening will amplify.
  This is reduced, when doing quality=4, byusing a boundary mask to reduce sharpenning along strong edges.

range expand will expand the contrast along edges. It will tend to transform an edge function that looks like  (step function) to this (new values)
  also show for a softer edge

talk about do limit change controlling level change and reducing artifacts.

SEE
  http://reference.wolfram.com/mathematica/ref/RangeFilter.html
  http://reference.wolfram.com/mathematica/ref/GradientFilter.html
  http://reference.wolfram.com/mathematica/ref/ImageDeconvolve.html
  http://reference.wolfram.com/mathematica/ref/TotalVariationFilter.html
  http://reference.wolfram.com/mathematica/ref/RidgeFilter.html

    see Mathematica for the following
  MedianDeviation filter may be useful.
  RidgeFilter may be useful in finding block notches and block anti-aliasing.
  It might help in dealing with noise and edges.
  It might provide a mask for areas where expand and sharp should be stronger and smooth should be weaker.


The system is adaptive at the frame level, the block level, and the pixel level.
The strength of the adaptivity is controlled by adapt. It has a range of 0-31.
adapt = 0 does no adaptation.  31 provides the most adaptation.
Adaptation works by determining how blocky the frame as a whole is.
Then by how blocky each block is.  The blockiness of each block is determined by the
Increase of the sad of the block when it has been fully range expanded.
when smoothing the variable quant is increased from the user specified quant value up to 31.
Range expansion operates on a per block bases and is reduced for blocks with high blockiness.
Sharpening is also reduced on a per block bases for blocks with high blockiness.
For Range expansion and Sharpening The variables that influence the amount of decrease
are the adapt variable, the sharpening variable, and the range expansion variable.
These are used to create 31 levels of sharpening or range expansion that will be used
on a per block basis in DctLoop.  Which level is chosen is determined by the amount of
blockiness assigned to each block.
*/


//  Some GLOBAL debug values
//static int max_debug3=0;
//static int min_debug3=9999;
//
//static int max_debug4=0;
//static int min_debug4=9999;
//
//
//static int max_debug5=0;
//static int min_debug5=9999;
//
//static int max_debug6=0;
//static int min_debug6=9999;
//
//static int max_debug7=0;
//static int min_debug7=9999;





int   amFilters(FrameInfo_args* FrameInfoArgs) {

  uint8_t        quality = FrameInfoArgs->quality;
  uint8_t         quant = FrameInfoArgs->quant;
  uint8_t         shift = FrameInfoArgs->shift;
  uint8_t         matrix = FrameInfoArgs->matrix;
  uint8_t         qtype = FrameInfoArgs->qtype;

  uint8_t         expand = FrameInfoArgs->expand;

  uint8_t         sharpWPos = FrameInfoArgs->sharpWPos;
  uint8_t         sharpWAmt = FrameInfoArgs->sharpWAmt;
  uint8_t         sharpTPos = FrameInfoArgs->sharpTPos;
  uint8_t         sharpTAmt = FrameInfoArgs->sharpTAmt;

  uint8_t         doSmoothFlag = FrameInfoArgs->doSmoothFlag;
  uint8_t         doExpandFlag = FrameInfoArgs->doExpandFlag;
  uint8_t         doSharpFlag = FrameInfoArgs->doSharpFlag;
  uint8_t         doBrightFlag = FrameInfoArgs->doBrightFlag;

  uint8_t         showMask = FrameInfoArgs->showMask;
  uint8_t         T2 = FrameInfoArgs->T2;

  uint8_t         use_quant = FrameInfoArgs->quant;
  uint8_t         FrameQuant = FrameInfoArgs->quant;
  uint8_t         use_adapt = FrameInfoArgs->adapt;

  //  uint8_t         *psrc        = FrameInfoArgs->psrc;
  // uint16_t          src_width    = FrameInfoArgs->src_width;
  // uint16_t          src_height   = FrameInfoArgs->src_height;



  //    UNCOMMENT Outbuf and sbuf   FOR DEBUGGING
  //  uint8_t         Outbuf[160];    // USED FOR DEBUG MESSAGES
  //  uint8_t         *sbuf   = Outbuf;



  /*
  //////////////////
  // START DEBUG  //
  //////////////////
  //
  //
  //  DEBUG---- RETURN VARIOUS FRAMES FOR DEBUGGING AND TUNNING
  //            showMask == 4 is done later after  the pre deblocking step and the pre smoothing are done.
  //

  //sprintf_s(sbuf, sizeof(sbuf), "use_quant %2d", use_quant);
  //DrawYV12(psrc, src_height, src_width, 1, 1, sbuf);


    if ((showMask > 0 && showMask != 4)) { // || T2==XX) {
      uint8_t  *psharpSmoothDif = FrameInfoArgs->frame_sharpSmoothDif;


      // We explicitly call buildMasks() since we may want to look at the masks even if the arguments to amDCT were only for smoothing.
      if (showMask == 11) FrameInfoArgs->quality = 5; // deringMask is only created at quality >= 5
      buildMasks(FrameInfoArgs);


      MY_MEMCPY(psharpSmoothDif, psrc, sizeof(uint8_t) * FrameInfoArgs->sizeIntFramesY);

      deblock_horiz_DoDC(psharpSmoothDif, src_height, src_width, src_width, quant);
      deblock_vert_DoDC(psharpSmoothDif, src_height, src_width, src_width, quant);

      copyFrameToDst(FrameInfoArgs, showMask);

      if (T2==33) { // Printout variables on the screen
        float  numblocks = (float)FrameInfoArgs->numBlocksY;
        //float  usedBlks  = (float)FrameInfoArgs->MemoryArgs->usedBlks;

        sprintf_s(sbuf, sizeof(sbuf), "FrameQuant=%3d", (int)FrameInfoArgs->FrameQuant);
        DrawYV12(pdst, dst_height, dst_width, 0, 1, sbuf);
        Draw16x16Block(pdst, dst_height, dst_width, 20, 1, FrameInfoArgs->brightMax);   // draw a block of brightness brightMax 20 characters in 1 row down.

        sprintf_s(sbuf, sizeof(sbuf), "frameAvgDetE=%3d", (int)FrameInfoArgs->frameAvgDetE);
        DrawYV12(pdst, dst_height, dst_width, 0, 2, sbuf);
        Draw16x16Block(pdst, dst_height, dst_width, 20, 2, FrameInfoArgs->frame_SmoothMax);

        sprintf_s(sbuf, sizeof(sbuf), "sadEDetFrameAvg=%4d", (int)FrameInfoArgs->sadEDetFrameAvg);
        DrawYV12(pdst, dst_height, dst_width, 0, 3, sbuf);

        sprintf_s(sbuf, sizeof(sbuf), "totalBlks=%4d", (int)numblocks);
        DrawYV12(pdst, dst_height, dst_width, 0, 4, sbuf);

        sprintf_s(sbuf, sizeof(sbuf), "perUsedBlks=%5.1f", FrameInfoArgs->MemoryArgs->percentBlksUsed);
        DrawYV12(pdst, dst_height, dst_width, 0, 5, sbuf);


  //      sprintf_s(sbuf, sizeof(sbuf), "frameDetDifECsad=%5.1f", FrameInfoArgs->frameDetDifECsad);
  //      DrawYV12(pdst, dst_height, dst_width, 0, 6, sbuf);
  //      Draw16x16Block(pdst, dst_height, dst_width, 23, 6, FrameInfoArgs->frameDetDifECsad);


  //      sprintf_s(sbuf, sizeof(sbuf), "mean3x3BrightMax=%3d", FrameInfoArgs->mean3x3BrightMax);
  //      DrawYV12(pdst, dst_height, dst_width, 0, 4, sbuf);
  //      Draw16x16Block(pdst, dst_height, dst_width, 20, 4, FrameInfoArgs->mean3x3BrightMax);

      }

  //    free_data(FrameInfoArgs);
  //    _mm_empty()
      return(0);
    }
  //
  //  END   DEBUG---- RETURN VARIOUS FRAMES FOR DEBUGGING AND TUNNING
  //
  //
  ////////////////
  // END DEBUG  //
  ////////////////
  */






  /*  PUT BACK IN !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

 //////////////////
 //     PREPROCESS FRAMES THAT ARE GOING TO BE SHARPENNED MORE THAN A SMALL AMOUNT.
 //     THIS REMOVES NOISE THAT WOULD CAUSE SHARPENING ARTIFACTS
 //////////////////

   if ((sharpWAmt >= 4 || sharpTAmt >= 4) && quality >= 2) {    //&& (T2==2||T2==3)) {   // When quality==1 presmoothing doesn't help

     if ((quality == 2 && expand <= 14 && (sharpWAmt <  8 && sharpTAmt <  8)) || // Cases where doing an undot() is good enough and much faster.
       (quality == 2 && expand <= 12 && (sharpWAmt < 10 && sharpTAmt < 10)) ||
       (quality == 3 && expand <= 11 && (sharpWAmt <  6 && sharpTAmt <  6)) ||
       (quality == 4 && expand <= 10 && (sharpWAmt <  5 && sharpTAmt <  5)) || showMask == SHOW_PRESMOOTH_1) {


         UnDot(src_width, src_width, src_width, psrc, psrc, src_height);
         if (showMask == SHOW_PRESMOOTH_2) { // DEBUG Pre Smoothing
           copyFrameToDst(FrameInfoArgs, 5);
           _mm_empty()
           return(0);
         }
     }
     else {
       // Very Mild Pre Smoothing Variables.
       uint8_t use_shift_PreSmooth     = 2;
       uint8_t use_qtype_PreSmooth     = 1;
       uint8_t use_expand_PreSmooth    = 0;
       uint8_t use_matrix_PreSmooth    = MATRIX_PRESMOOTH;
       uint8_t use_sharpWAmt_PreSmooth = 0;
       uint8_t use_sharpTAmt_PreSmooth = 0;
       uint8_t use_quant_PreSmooth     = 10;
       uint8_t  use_adapt_PreSmooth     = 0;

       if (quality == 2 && shift >  3)  use_shift_PreSmooth = 3;
       if (quality == 3 && shift >  3)  use_shift_PreSmooth = shift-1;
       if (quality == 4 && shift >= 3)  use_shift_PreSmooth = shift;


       if (FrameInfoArgs->buildMasksDone == 0) buildMasks(FrameInfoArgs);


       // Do Mild smoothing
       doFrameInfo(FrameQuant,
             use_quant_PreSmooth,
             use_qtype_PreSmooth,
             use_adapt_PreSmooth,
             use_shift_PreSmooth,
             use_matrix_PreSmooth,

             use_expand_PreSmooth,
             sharpWPos,
             use_sharpWAmt_PreSmooth,
             sharpTPos,
             use_sharpTAmt_PreSmooth,

             FrameInfoArgs,

             showMask,
             T2);


 // NOTE TRY USING DERING MASK TO SHOW AREAS THAT SHOULD HAVE STRONGER PRESMOOTHING DONE !!!!!!!!!!
       set_matrix(FrameInfoArgs, use_matrix_PreSmooth, use_qtype_PreSmooth, use_quant_PreSmooth, use_expand_PreSmooth);
       dup_cpu_data(FrameInfoArgs);
       DispatchLoop(FrameInfoArgs);
       avgDctLoopAccum(FrameInfoArgs);
       clear_data_work_buffers(FrameInfoArgs);
     }
   }   //   END PREPROCESS FRAMES


   if (showMask == SHOW_PRESMOOTH_2) {
     copyFrameToDst(FrameInfoArgs, SHOW_PRESMOOTH_2);
     return(0);
   }

 /////////////////////
 // END PREPROCESS  //
 /////////////////////

 */





 //   We ARE ONLY SMOOTHING THE BRIGHT AREAS.  NO OTHER PROCESSING TO DO
  if (doBrightFlag == 1 && doSharpFlag == 0 && doExpandFlag == 0 && doSmoothFlag == 0) {
    uint8_t save_quality_Smooth = quality;

    uint8_t use_qtype_Smooth = qtype;
    uint8_t use_shift_Smooth = shift;
    uint8_t use_matrix_Smooth = matrix;
    uint8_t use_expand_Smooth = 0;    // Turn off range expansion
    uint8_t use_sharpWAmt_Smooth = 0;    // Turn off sharpening
    uint8_t use_sharpTAmt_Smooth = 0;    // Turn off sharpening
    uint8_t use_quant_Smooth = 0;    // Turn off quant smoothing.  Bright area smoothing stays on.

    doFrameInfo(FrameQuant,
      use_quant_Smooth,
      use_qtype_Smooth,
      use_adapt,
      use_shift_Smooth,
      use_matrix_Smooth,

      use_expand_Smooth,
      sharpWPos,
      use_sharpWAmt_Smooth,
      sharpTPos,
      use_sharpTAmt_Smooth,

      FrameInfoArgs,

      showMask,
      T2);


    FrameInfoArgs->quality = 4;

    if (FrameInfoArgs->buildMasksDone == 0) buildMasks(FrameInfoArgs);

    FrameInfoArgs->quality = save_quality_Smooth;

    set_matrix(FrameInfoArgs, use_matrix_Smooth, use_qtype_Smooth, use_quant_Smooth, use_expand_Smooth);
    dup_cpu_data(FrameInfoArgs);
    DispatchLoop(FrameInfoArgs);
    avgDctLoopAccum(FrameInfoArgs);

    return(0);
  }



  //  The user just wants a mask.
  if (quality >= 3 && showMask > 0) {
    buildMasks(FrameInfoArgs);
    copyFrameToDst(FrameInfoArgs, showMask);

    return(0);
  }




  /////////////////////////
  // START QUALITY == 1  //
  /////////////////////////
    // Do range expand, sharp, and smooth in 1 pass


  if (quality == 1) {
    uint8_t use_shift = shift;
    uint8_t use_qtype = qtype;
    uint8_t use_expand = expand;
    uint8_t use_matrix = matrix;
    uint8_t use_sharpWAmt = sharpWAmt;
    uint8_t use_sharpTAmt = sharpTAmt;
    uint8_t use_quantq1 = use_quant;

    doFrameInfo(FrameQuant,
      use_quantq1,
      use_qtype,
      use_adapt,
      use_shift,
      use_matrix,

      use_expand,
      sharpWPos,
      use_sharpWAmt,
      sharpTPos,
      use_sharpTAmt,

      FrameInfoArgs,

      showMask,
      T2);

    if (FrameInfoArgs->buildMasksDone == 0) buildMasks(FrameInfoArgs);

    set_matrix(FrameInfoArgs, use_matrix, use_qtype, use_quant, use_expand);

    // The 1 tells Sharp to set up all of the sharpening info but not to run it in its own pass.
    if (doSharpFlag == 1) {
      Sharp(FrameInfoArgs, 1);
    }

    dup_cpu_data(FrameInfoArgs);
    DispatchLoop(FrameInfoArgs);

    avgDctLoopAccum(FrameInfoArgs);

    return(0);
  }
  /////////////////////////
  // END QUALITY == 1    //
  /////////////////////////








  /////////////////////////
  // START QUALITY == 2 or QUALITY == 3
  /////////////////////////
    // Do sharp in 1 pass and
    // do range expand, and smooth in a second pass
  if (quality == 2 || quality == 3) {

    //    clear_data_work_buffers(FrameInfoArgs);


        // START OF SHARP //
    if (doSharpFlag == 1) {
      uint8_t use_shift_Shrp = shift;
      uint8_t use_qtype_Shrp = 21; // Use the special sharpening qtype that bypasses the smoothing code.
      uint8_t use_expand_Shrp = 0;
      uint8_t use_matrix_Shrp = MATRIX_SHARP;
      uint8_t use_sharpWAmt = sharpWAmt;
      uint8_t use_sharpTAmt = sharpTAmt;
      uint8_t use_quant_Shrp = use_quant;


      doFrameInfo(FrameQuant,
        use_quant_Shrp,
        use_qtype_Shrp,
        use_adapt,
        use_shift_Shrp,
        use_matrix_Shrp,

        use_expand_Shrp,
        sharpWPos,
        use_sharpWAmt,
        sharpTPos,
        use_sharpTAmt,

        FrameInfoArgs,

        showMask,
        T2);

      if (FrameInfoArgs->buildMasksDone == 0) buildMasks(FrameInfoArgs);

      set_matrix(FrameInfoArgs, use_matrix_Shrp, use_qtype_Shrp, use_quant_Shrp, use_expand_Shrp);

      Sharp(FrameInfoArgs, 1);
      dup_cpu_data(FrameInfoArgs);
      DispatchLoop(FrameInfoArgs);

      avgDctLoopAccum(FrameInfoArgs);
      clear_data_work_buffers(FrameInfoArgs);
    }
    // END OF SHARP //




    // If we are not doing any smoothing "quant == 0" then just do the Range Expansion code.
    if (quant == 0 && expand > 0) {
      uint8_t use_quant_RE = 0;
      uint8_t use_qtype_RE = 22; // Use the special range expansion qtype that bypasses the smoothing code.
      uint8_t use_sharpWAmt_RE = 0;
      uint8_t use_sharpTAmt_RE = 0;
      uint8_t use_matrix_RE = MATRIX_RANGE_EXPAND;
      uint8_t use_shift_RE = 2;  // In this case you don't get any better results adding more shifts.

      doFrameInfo(FrameQuant,
        use_quant_RE,
        use_qtype_RE,
        use_adapt,
        use_shift_RE,
        use_matrix_RE,

        expand,
        sharpWPos,
        use_sharpWAmt_RE,
        sharpTPos,
        use_sharpTAmt_RE,

        FrameInfoArgs,

        showMask,
        T2);

      if (FrameInfoArgs->buildMasksDone == 0) buildMasks(FrameInfoArgs);

      set_matrix(FrameInfoArgs, use_matrix_RE, use_qtype_RE, use_quant_RE, expand);
      dup_cpu_data(FrameInfoArgs);
      DispatchLoop(FrameInfoArgs);
      avgDctLoopAccum(FrameInfoArgs);

      clear_data_work_buffers(FrameInfoArgs);
    }


    // Do smoothing or combined expansion and smoothing  if expand > 0
    // Do the combined range expansion and smoothing code.
    if (quant > 0) {
      uint8_t use_shift_Smooth = shift;
      uint8_t use_qtype_Smooth = qtype;
      uint8_t use_expand_Smooth = expand;
      uint8_t use_matrix_Smooth = matrix;
      uint8_t use_sharpWAmt_Smooth = 0;
      uint8_t use_sharpTAmt_Smooth = 0;
      uint8_t use_quant_Smooth = use_quant;

      doFrameInfo(FrameQuant,
        use_quant_Smooth,
        use_qtype_Smooth,
        use_adapt,
        use_shift_Smooth,
        use_matrix_Smooth,

        use_expand_Smooth,
        sharpWPos,
        use_sharpWAmt_Smooth,
        sharpTPos,
        use_sharpTAmt_Smooth,

        FrameInfoArgs,

        showMask,
        T2);

      if (FrameInfoArgs->buildMasksDone == 0) buildMasks(FrameInfoArgs);

      set_matrix(FrameInfoArgs, use_matrix_Smooth, use_qtype_Smooth, use_quant_Smooth, use_expand_Smooth);
      dup_cpu_data(FrameInfoArgs);
      DispatchLoop(FrameInfoArgs);
      avgDctLoopAccum(FrameInfoArgs);

      clear_data_work_buffers(FrameInfoArgs);
    }


    return(0);
  }
  ///////////////////////
  // END QUALITY == 2 or QUALITY == 3
  ///////////////////////




  /////////////////////////
  ////
  //// START OF Large Contrast Block Edge Processing.
  //// This code can be moddled from the following AVS script code
  ////
  ////    smoothed = orig1.amDCT(qtype=qtype, quant=1, matrix=252, shift=0, quality=1)
  ////     return(orig1.ShowDiff(smoothed, mult=8))
  ////
  /////////////////////////
  //    if (quant > 0) {
  //      uint8_t use_qtype_Smooth     = 1;   // We take advabtage of a bug in the matrix processing for this routine to work,
  //      uint8_t use_shift_Smooth     = 0;   // NO SHIFTING
  //      uint8_t use_matrix_Smooth    = 252;  // MUST BE 252
  //      uint8_t use_expand_Smooth    = 0;    // Turn off range expansion
  //      uint8_t use_sharpWAmt_Smooth = 0;    // Turn off sharpening
  //      uint8_t use_sharpTAmt_Smooth = 0;    // Turn off sharpening
  //      uint8_t use_quant_Smooth     = 1;
  //
  //      doFrameInfo(FrameQuant,
  //            use_quant_Smooth,
  //            use_qtype_Smooth,
  //            use_adapt,
  //            use_shift_Smooth,
  //            use_matrix_Smooth,
  //
  //            use_expand_Smooth,
  //            sharpWPos,
  //            use_sharpWAmt_Smooth,
  //            sharpTPos,
  //            use_sharpTAmt_Smooth,
  //
  //            FrameInfoArgs,
  //
  //            showMask,
  //            T2);
  //
  //      //if (FrameInfoArgs->buildMasksDone == 0) buildMasks(FrameInfoArgs);
  //
  //      set_matrix(FrameInfoArgs, use_matrix_Smooth, use_qtype_Smooth, use_quant_Smooth, use_expand_Smooth);
  //      dup_cpu_data(FrameInfoArgs);
  //      DispatchLoop(FrameInfoArgs);
  //      avgDctLoopAccum(FrameInfoArgs);
  //
  //      //  Now build the diff mask. between the source frame and the smoothed frame mask  NEED TO FIND A MASK TO USE AND RENAME IT !!!!!
  //      // Processd each 8x8 block in the mask to produce the block strength.
  //    }
  //    // END OF Large Contrast Block Edge Processing
  //    //
  //    //
  //    //





  /////////////////////////
  // START QUALITY >= 4  //
  /////////////////////////




  /*
                    THIS CAUSES HORRIBLE ARTIFACTS
  ///////////////////////
  //
  // START OF Large Contrast Block Edge Processing.
  // This code can be modeled from the following .avs script code
  //
  //    smoothed = orig1.amDCT(qtype=qtype, quant=1, matrix=252, shift=0, quality=1)
  //     return(orig1.ShowDiff(smoothed, mult=8))
  //
  //     THE ROUTINE THAT IMPLIMENTS THIS SHOULD BE RUN AT THE BEGINNING OF THE ADAPT CODE   ???????????????
  // IT IS USED AS 1 INPUT TO THE BLOCK ADAPTIVE PER BLOCK MATRIX WEIGHTING SYSTEM.
  //
  // quant=1 and matrix=252 taske advantage of a bug
  // that overflow the the quant dequant routines so that
  // BLOCKS that cover a large dark bright transition have the transition magnified.
  // The amount of magnification is one measure that is used to control the per block adaptivity.
  //
  ///////////////////////
      if (quant > 0) {
        uint8_t use_qtype_Smooth     = 1;   // We take advabtage of a bug in the matrix processing for this routine to work,
        uint8_t use_shift_Smooth     = 0;   // NO SHIFTING
        uint8_t use_matrix_Smooth    = 252;  // MUST BE 252
        uint8_t use_expand_Smooth    = 0;    // Turn off range expansion
        uint8_t use_sharpWAmt_Smooth = 0;    // Turn off sharpening
        uint8_t use_sharpTAmt_Smooth = 0;    // Turn off sharpening
        uint8_t use_quant_Smooth     = 1;

        doFrameInfo(FrameQuant,
              use_quant_Smooth,
              use_qtype_Smooth,
              use_adapt,
              use_shift_Smooth,
              use_matrix_Smooth,

              use_expand_Smooth,
              sharpWPos,
              use_sharpWAmt_Smooth,
              sharpTPos,
              use_sharpTAmt_Smooth,

              FrameInfoArgs,

              showMask,
              T2);

        //if (FrameInfoArgs->buildMasksDone == 0) buildMasks(FrameInfoArgs);

        set_matrix(FrameInfoArgs, use_matrix_Smooth, use_qtype_Smooth, use_quant_Smooth, use_expand_Smooth);
        dup_cpu_data(FrameInfoArgs);
        DispatchLoop(FrameInfoArgs);
        avgDctLoopAccum(FrameInfoArgs);

        //  Now build the diff mask. between the source frame and the smoothed frame mask  NEED TO FIND A MASK TO USE AND RENAME IT !!!!!
        // Processd each 8x8 block in the mask to produce the block strength.
      }
      // END OF Large Contrast Block Edge Processing
      //
      //
      //
  */



  // Do sharp, range expand, and smooth each in their own pass
  //if (quality >= 4) {
  if (quality == 4) {

    //
    //
    // START OF RANGE EXPAND //
    if (expand > 0) {
      clear_data_work_buffers(FrameInfoArgs);

      // Set values for the range expansion call that may be different than the user arguments.
      uint8_t use_quant_RE = 0;   // Turn off smoothing
      uint8_t use_qtype_RE = 22;  // Use the special range expansion qtype that bypasses the smoothing code.
      uint8_t use_matrix_RE = MATRIX_RANGE_EXPAND;
      uint8_t use_shift_RE = 2;   // Default number of shifts for range expansion.
      uint8_t use_sharpWAmt_RE = 0;   // Turn off sharpening.
      uint8_t use_sharpTAmt_RE = 0;

      // Set the number of shifts done for range expansion.
      //   2 (8 shifts) is the default. There is not much difference at larger shift values.
      //   There is a slight reduction in horizontal line artifacts at high expand values when using use_shift_RE = 3
      //   Using use_shift_RE 1 (4 shifts) is terrible.
      //   If quant is 0 and sharpening is 0 then use the value of the shift argument to amDCT() as the range expansion number of shifts.
      if (quality == 4 && expand > 4) use_shift_RE = 3;
      if (quality == 4 && expand > 13) use_shift_RE = 4;
      if (doSharpFlag == 0 && doSmoothFlag == 0)  use_shift_RE = shift;

      doFrameInfo(FrameQuant,
        use_quant_RE,
        use_qtype_RE,
        use_adapt,
        use_shift_RE,
        use_matrix_RE,

        expand,
        sharpWPos,
        use_sharpWAmt_RE,
        sharpTPos,
        use_sharpTAmt_RE,

        FrameInfoArgs,

        showMask,
        T2);

      if (FrameInfoArgs->buildMasksDone == 0) buildMasks(FrameInfoArgs);

      set_matrix(FrameInfoArgs, use_matrix_RE, use_qtype_RE, use_quant_RE, expand);
      dup_cpu_data(FrameInfoArgs);
      DispatchLoop(FrameInfoArgs);
      avgDctLoopAccum(FrameInfoArgs);

      // Doing a buildMasks() after a range expand gets rid of a very small amount of noise. !!! NOTE BUILDMASKS NEEDS TO ACCOUNT FOR THIS CASE !!!!!!!!!!!!!!!!
      //if(quality >= 6 && (sharpWAmt > 4 || sharpTAmt > 4)) {
      //  buildMasks(FrameInfoArgs);
      //}
    }
    // END OF RANGE EXPAND //
    //
    //
    //



    clear_data_work_buffers(FrameInfoArgs);



    //
    //
    //
    // START OF SHARP //
    if (doSharpFlag == 1) {
      uint8_t use_quant_Shrp = 0;    // Turn off smoothing
      uint8_t use_shift_Shrp = shift;
      uint8_t use_qtype_Shrp = 21;  // Use the special sharp qtype that bypasses the smoothing code.
      uint8_t use_expand_Shrp = 0;   // Turn off Range Expansion
      uint8_t use_matrix_Shrp = MATRIX_SHARP;

      doFrameInfo(FrameQuant,
        use_quant_Shrp,
        use_qtype_Shrp,
        use_adapt,
        use_shift_Shrp,
        use_matrix_Shrp,

        use_expand_Shrp,
        sharpWPos,
        sharpWAmt,
        sharpTPos,
        sharpTAmt,

        FrameInfoArgs,

        showMask,
        T2);

      if (FrameInfoArgs->buildMasksDone == 0) buildMasks(FrameInfoArgs);

      set_matrix(FrameInfoArgs, use_matrix_Shrp, use_qtype_Shrp, use_quant_Shrp, use_expand_Shrp);

      Sharp(FrameInfoArgs, 1);
      dup_cpu_data(FrameInfoArgs);
      DispatchLoop(FrameInfoArgs);
      avgDctLoopAccum(FrameInfoArgs);
    }
    // END OF SHARP //
    //
    //


    clear_data_work_buffers(FrameInfoArgs);


    //
    //
    //
    // START OF SMOOTHING
    if (quant > 0) {
      uint8_t use_qtype_Smooth = qtype;
      uint8_t use_shift_Smooth = shift;
      uint8_t use_matrix_Smooth = matrix;
      uint8_t use_expand_Smooth = 0;    // Turn off range expansion
      uint8_t use_sharpWAmt_Smooth = 0;    // Turn off sharpening
      uint8_t use_sharpTAmt_Smooth = 0;    // Turn off sharpening
      uint8_t use_quant_Smooth = use_quant;

      doFrameInfo(FrameQuant,
        use_quant_Smooth,
        use_qtype_Smooth,
        use_adapt,
        use_shift_Smooth,
        use_matrix_Smooth,

        use_expand_Smooth,
        sharpWPos,
        use_sharpWAmt_Smooth,
        sharpTPos,
        use_sharpTAmt_Smooth,

        FrameInfoArgs,

        showMask,
        T2);

      if (FrameInfoArgs->buildMasksDone == 0) buildMasks(FrameInfoArgs);

      set_matrix(FrameInfoArgs, use_matrix_Smooth, use_qtype_Smooth, use_quant_Smooth, use_expand_Smooth);
      dup_cpu_data(FrameInfoArgs);
      DispatchLoop(FrameInfoArgs);
      avgDctLoopAccum(FrameInfoArgs);

    }
    // END OF SMOOTHING
    //
    //
    //

    return(0);
  }
  /////////////////////////
  //   END QUALITY >= 4  //
  /////////////////////////



  if (quality >= 4) return(0);


















  //  WHAT FOLLOWS IS EXPERIMENTAL !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  //     THINGS THAT DON'T WORK YET
  //      preProcessEdges(FrameInfoArgs);
  //      buildPerBlockMatrix(FrameInfoArgs);
  //      fixSharpSmoothDif(FrameInfoArgs)
  //       fixHaloNoise(FrameInfoArgs)

  /////////////////////////
  // START QUALITY >= 5  //     TESTING FOR ADAPTIVE PER BLOCK CREATED MATRIX
  /////////////////////////
    // Do sharp, range expand, and smooth each in their own pass
    //if (quality >= 4) {
  if (quality >= 5) {

    //    if (quality >= 6) {
    //      preProcessEdges(FrameInfoArgs);
    //    }

    if (quality == 5) {
      /*
            FrameInfoArgs->showMask = 18;
            buildPerBlockMatrix(FrameInfoArgs);
            FrameInfoArgs->showMask = 0;
            //copyFrameToDst(&FrameInfoArgs, 18);
      //      return(0);
          clear_data_work_buffers(FrameInfoArgs);
      */

      //    LumaBlockInfo(FrameInfoArgs);
       /*
            //Now Do PreSmooth
            FrameInfoArgs->T2 = 88;
            if (quant > 0) {
              uint8_t use_qtype_Smooth     = qtype;
              uint8_t use_shift_Smooth     = 0; //shift;
              uint8_t use_matrix_Smooth    = matrix;
              uint8_t use_expand_Smooth    = 0;    // Turn off range expansion
              uint8_t use_sharpWAmt_Smooth = 0;    // Turn off sharpening
              uint8_t use_sharpTAmt_Smooth = 0;    // Turn off sharpening
              uint8_t use_quant_Smooth     = use_quant;

              doFrameInfo(FrameQuant,
                    use_quant_Smooth,
                    use_qtype_Smooth,
                    use_adapt,
                    use_shift_Smooth,
                    use_matrix_Smooth,

                    use_expand_Smooth,
                    sharpWPos,
                    use_sharpWAmt_Smooth,
                    sharpTPos,
                    use_sharpTAmt_Smooth,

                    FrameInfoArgs,

                    showMask,
                    T2);

              if (FrameInfoArgs->buildMasksDone == 0) buildMasks(FrameInfoArgs);

              //set_matrix(FrameInfoArgs, use_matrix_Smooth, use_qtype_Smooth, use_quant_Smooth, use_expand_Smooth);
              dup_cpu_data(FrameInfoArgs);
              DispatchLoop(FrameInfoArgs);

              avgDctLoopAccum(FrameInfoArgs);

              clear_data_work_buffers(FrameInfoArgs);

            }
      */
      //FrameInfoArgs->T2 = 88;
      if (quant > 0) {
        uint8_t use_qtype_Smooth = 5;
        uint8_t use_shift_Smooth = 0; //shift;
        uint8_t use_matrix_Smooth = matrix;
        uint8_t use_expand_Smooth = 0;    // Turn off range expansion
        uint8_t use_sharpWAmt_Smooth = 0;    // Turn off sharpening
        uint8_t use_sharpTAmt_Smooth = 0;    // Turn off sharpening
        uint8_t use_quant_Smooth = use_quant;

        doFrameInfo(FrameQuant,
          use_quant_Smooth,
          use_qtype_Smooth,
          use_adapt,
          use_shift_Smooth,
          use_matrix_Smooth,

          use_expand_Smooth,
          sharpWPos,
          use_sharpWAmt_Smooth,
          sharpTPos,
          use_sharpTAmt_Smooth,

          FrameInfoArgs,

          showMask,
          T2);

        if (FrameInfoArgs->buildMasksDone == 0) buildMasks(FrameInfoArgs);

        set_matrix(FrameInfoArgs, use_matrix_Smooth, use_qtype_Smooth, use_quant_Smooth, use_expand_Smooth);
        dup_cpu_data(FrameInfoArgs);
        DispatchLoop(FrameInfoArgs);

        avgDctLoopAccum(FrameInfoArgs);

        clear_data_work_buffers(FrameInfoArgs);

      }
    }

    //return(0);

    FrameInfoArgs->showMask = 0;
    //LumaBlockInfo(FrameInfoArgs);


//    if (quality >= 6) {
//      preProcessEdges(FrameInfoArgs);
//    }


    //
    //
    // START OF RANGE EXPAND //
    if (expand > 0) {
      clear_data_work_buffers(FrameInfoArgs);

      // Set values for the range expansion call that may be different than the user arguments.
      uint8_t use_quant_RE = 0;   // Turn off smoothing
      uint8_t use_qtype_RE = 22;  // Use the special range expansion qtype that bypasses the smoothing code.
      uint8_t use_matrix_RE = MATRIX_RANGE_EXPAND;
      uint8_t use_shift_RE = 2;   // Default number of shifts for range expansion.
      uint8_t use_sharpWAmt_RE = 0;   // Turn off sharpening.
      uint8_t use_sharpTAmt_RE = 0;

      // Set the number of shifts done for range expansion.
      //   2 (8 shifts) is the default. There is not much difference at larger shift values.
      //   There is a slight reduction in horizontal line artifacts at high expand values when using use_shift_RE = 3
      //   Using use_shift_RE 1 (4 shifts) is terrible.
      //   If quant is 0 and sharpening is 0 then use the value of the shift argument to amDCT() as the range expansion number of shifts.
      if (quality == 4 && expand > 4) use_shift_RE = 3;
      if (quality == 4 && expand > 13) use_shift_RE = 4;
      if (quality >= 5 && expand > 4) use_shift_RE = 4;
      if (doSharpFlag == 0 && doSmoothFlag == 0)  use_shift_RE = shift;

      doFrameInfo(FrameQuant,
        use_quant_RE,
        use_qtype_RE,
        use_adapt,
        use_shift_RE,
        use_matrix_RE,

        expand,
        sharpWPos,
        use_sharpWAmt_RE,
        sharpTPos,
        use_sharpTAmt_RE,

        FrameInfoArgs,

        showMask,
        T2);

      if (FrameInfoArgs->buildMasksDone == 0) buildMasks(FrameInfoArgs);

      set_matrix(FrameInfoArgs, use_matrix_RE, use_qtype_RE, use_quant_RE, expand);
      dup_cpu_data(FrameInfoArgs);
      DispatchLoop(FrameInfoArgs);
      avgDctLoopAccum(FrameInfoArgs);

      // Doing a buildMasks() after a range expand gets rid of a very small amount of noise. !!! NOTE BUILDMASKS NEEDS TO ACCOUNT FOR THIS CASE !!!!!!!!!!!!!!!!
      if (quality >= 6 && (sharpWAmt > 4 || sharpTAmt > 4)) {
        buildMasks(FrameInfoArgs);
      }
    }
    // END OF RANGE EXPAND //
    //
    //
    //



    clear_data_work_buffers(FrameInfoArgs);



    //
    //
    //
    // START OF SHARP //
    if (doSharpFlag == 1) {
      uint8_t use_quant_Shrp = 0;    // Turn off smoothing
      uint8_t use_shift_Shrp = shift;
      uint8_t use_qtype_Shrp = 21;  // Use the special sharp qtype that bypasses the smoothing code.
      uint8_t use_expand_Shrp = 0;   // Turn off Range Expansion
      uint8_t use_matrix_Shrp = MATRIX_SHARP;

      doFrameInfo(FrameQuant,
        use_quant_Shrp,
        use_qtype_Shrp,
        use_adapt,
        use_shift_Shrp,
        use_matrix_Shrp,

        use_expand_Shrp,
        sharpWPos,
        sharpWAmt,
        sharpTPos,
        sharpTAmt,

        FrameInfoArgs,

        showMask,
        T2);

      if (FrameInfoArgs->buildMasksDone == 0) buildMasks(FrameInfoArgs);

      set_matrix(FrameInfoArgs, use_matrix_Shrp, use_qtype_Shrp, use_quant_Shrp, use_expand_Shrp);

      Sharp(FrameInfoArgs, 1);
      dup_cpu_data(FrameInfoArgs);
      DispatchLoop(FrameInfoArgs);
      avgDctLoopAccum(FrameInfoArgs);

      //      doDering(FrameInfoArgs);              // no difference
      //if (T2==28)  doDeringDarkProtect(FrameInfoArgs);  // very small difference, wide spread over dark areas. probably not worth it.

    }
    // END OF SHARP //
    //
    //


    clear_data_work_buffers(FrameInfoArgs);


    //
    //
    //
    // START OF SMOOTHING
    if (quant > 0) {
      uint8_t use_qtype_Smooth = qtype;
      uint8_t use_shift_Smooth = shift;
      uint8_t use_matrix_Smooth = matrix;
      uint8_t use_expand_Smooth = 0;    // Turn off range expansion
      uint8_t use_sharpWAmt_Smooth = 0;    // Turn off sharpening
      uint8_t use_sharpTAmt_Smooth = 0;    // Turn off sharpening
      uint8_t use_quant_Smooth = use_quant;

      doFrameInfo(FrameQuant,
        use_quant_Smooth,
        use_qtype_Smooth,
        use_adapt,
        use_shift_Smooth,
        use_matrix_Smooth,

        use_expand_Smooth,
        sharpWPos,
        use_sharpWAmt_Smooth,
        sharpTPos,
        use_sharpTAmt_Smooth,

        FrameInfoArgs,

        showMask,
        T2);

      if (FrameInfoArgs->buildMasksDone == 0) buildMasks(FrameInfoArgs);

      set_matrix(FrameInfoArgs, use_matrix_Smooth, use_qtype_Smooth, use_quant_Smooth, use_expand_Smooth);
      dup_cpu_data(FrameInfoArgs);
      DispatchLoop(FrameInfoArgs);

      avgDctLoopAccum(FrameInfoArgs);

    }
    // END OF SMOOTHING
    //
    //
    //
  }

  /////////////////////////
  //   END QUALITY >= 5  //
  /////////////////////////



  //  OUTPUT DEBUG INFO AS NEEDED FOR TESTING  !!!!!!!!

  //if (FrameInfoArgs->max_debug7 > max_debug7) {
  //  max_debug7 = FrameInfoArgs->max_debug7;
  //}


  //  FrameInfoArgs->frame_Max       = frame_Max;
  //   FrameInfoArgs->frame_Min       = frame_Min;
  //  FrameInfoArgs->frame_SmoothMax = frame_SmoothMax;
  //   FrameInfoArgs->frame_SmoothMin = frame_SmoothMin;

  //sprintf_s(sbuf, sizeof(sbuf), "        Max = %3d  Min = %3d", FrameInfoArgs->frame_Max, FrameInfoArgs->frame_Min);
  //DrawYV12(pdst, dst_height, dst_width, 0, 0, sbuf);
  //
  //sprintf_s(sbuf, sizeof(sbuf), "smooth  Max = %3d  Min = %3d", FrameInfoArgs->frame_SmoothMax, FrameInfoArgs->frame_SmoothMin);
  //DrawYV12(pdst, dst_height, dst_width, 0, 1, sbuf);



  //sprintf_s(sbuf, sizeof(sbuf), "quant_a=%2d frQnt=%2d adapt_a=%2d", quant_a, FrameQuant, adapt_a);
  //sprintf_s(sbuf, sizeof(sbuf), "max_debug7 = %2d", max_debug7);
  //DrawYV12(pdst, dst_height, dst_width, 0, 0, sbuf);



  //sprintf_s(sbuf, sizeof(sbuf), "use_quant %2d", use_quant);
  //DrawYV12(psrc, src_height, src_width, 1, 1, sbuf);


  //sprintf_s(sbuf, sizeof(sbuf), "maxSmooth=%3d maxSrc=%3d", (int)FrameInfoArgs->frame_SmoothMax, (int)FrameInfoArgs->frame_Max);
  //  FrameInfoArgs->frame_SmoothMax = maxSmooth;
  //  FrameInfoArgs->frame_Max       = maxSrc;

  //DrawYV12(pdst, (int)FrameInfoArgs->dst_height, (int)FrameInfoArgs->dst_width, 1, 1, sbuf);
  //DrawYV12(FrameInfoArgs->psrc, (int)FrameInfoArgs->dst_height, (int)FrameInfoArgs->dst_width, 2, 2, sbuf);

  if (T2 == 255) goto debug;


  return(0);






  ////////////////////  DEBUG  PRINT VALUES ON RETURNED FRAME   //////////////////////////////////
debug:

  //
  //// /*
  //      //adaptTemp = Int_FitRange_Int(FrameInfoArgs->sumAbDif2, 23000, 11, adapt_a, quant_a);
  ////adaptTemp = Int_FitRange_Int(FrameInfoArgs->sumAbDif,  FrameInfoArgs->sumAbDifPixCnt, 0, adapt_a, quant_a);
  ////temp      = Int_FitRange_Int(FrameInfoArgs->sumAbDif2, FrameInfoArgs->sumAbDifPixCnt, 0, adapt_a, quant_a);
  //         //temp = (FrameInfoArgs->sumAbDifPixCnt * 10) / FrameInfoArgs->sumAbDif2;
  //
  //sprintf_s(sbuf, sizeof(sbuf), "AvgDetE %7d AvgDetC %7d", (uint16_t)FrameInfoArgs->frameAvgDetE); //, (uint16_t)FrameInfoArgs->frameAvgDetC);
  //sprintf_s(sbuf, sizeof(sbuf), "AvgDetE %9d", (int16_t)FrameInfoArgs->frameAvgDetE); //, (uint16_t)FrameInfoArgs->frameAvgDetC);
  //    sprintf_s(sbuf, sizeof(sbuf), "Qnt=%2d frQnt=%2d useQnt=%2d max7=%2d", (int)use_quant, (int)FrameQuant, (int)FrameInfoArgs->adaptWeight, max_debug7);
  //DrawYV12(pdst, FrameInfoArgs->dst_height, FrameInfoArgs->dst_width, 0, 0, sbuf);
  //
  //
  ////sprintf_s(sbuf, sizeof(sbuf), "totalPer %2d avgtotalPer %2d", (int)FrameInfoArgs->totalPercentedge, (int)FrameInfoArgs->avgtotalPercentedge);
  ////sprintf_s(sbuf, sizeof(sbuf), "totalPer %2d avgtotalPer %2d", (int)FrameInfoArgs->totalPercentedge, (int)(FrameInfoArgs->totalPercentedge / FrameInfoArgs->numBlocks_sizeY));
  ////DrawYV12(pdst, dst_height, dst_width, 0, 4, sbuf);
  //
  //
  //sprintf_s(sbuf, sizeof(sbuf), "zeroBlksPercent1=%2d Percent2=%2d", (int)FrameInfoArgs->zeroBlksPercentedge, (int)FrameInfoArgs->zeroBlksPercentedge2);
  //DrawYV12(pdst, dst_height, dst_width, 0, 5, sbuf);
  //
  //sprintf_s(sbuf, sizeof(sbuf), "LumaBlk percentBlksUsed =%2d", (int)FrameInfoArgs->percentBlksUsed );
  //DrawYV12(pdst, dst_height, dst_width, 0, 2, sbuf);
  //
  //sprintf_s(sbuf, sizeof(sbuf), "adapt   percentBlksUsed =%2d", (int)FrameInfoArgs->percentBlksUsed2 );
  //DrawYV12(pdst, dst_height, dst_width, 0, 3, sbuf);
  //
  ///*
  //if (FrameInfoArgs->percentBlksUsed > max_debug4) max_debug4=FrameInfoArgs->percentBlksUsed;  // max_debug4 = 88
  //if (FrameInfoArgs->percentBlksUsed < min_debug4) min_debug4=FrameInfoArgs->percentBlksUsed;  // min_debug4 = 44-57 sometimes = 1   T2==6 min 30 if corners not used
  //sprintf_s(sbuf, sizeof(sbuf), "MaxPcntBlkUsed=%2d MinPcntBlk=%2d", max_debug4, min_debug4);
  //DrawYV12(pdst, dst_height, dst_width, 0, 5, sbuf);
  //
  //
  //
  //if (FrameInfoArgs->sumDifDivUsed * 100 > max_debug5) max_debug5=FrameInfoArgs->sumDifDivUsed * 100;  // max_debug5 = 429
  //if (FrameInfoArgs->sumDifDivUsed * 100 < min_debug5) min_debug5=FrameInfoArgs->sumDifDivUsed * 100;  // min_debug5 = 123
  //sprintf_s(sbuf, sizeof(sbuf), "MaxsumDifDiv %2d MinsumDifDiv %2d", max_debug5, min_debug5);
  //DrawYV12(pdst, dst_height, dst_width, 0, 8, sbuf);
  //
  //
  ////sprintf_s(sbuf, sizeof(sbuf), "sumDifDivUsed %2d", (int)(FrameInfoArgs->sumDifDivUsed * 100));
  //sprintf_s(sbuf, sizeof(sbuf), "sumAbDifUsed %2d", (int)(FrameInfoArgs->sumAbDif));
  //DrawYV12(pdst, dst_height, dst_width, 0, 6, sbuf);
  //
  //sprintf_s(sbuf, sizeof(sbuf), "frameDetDifECsad %3d avg %3d", (int)(FrameInfoArgs->frameDetDifECsad), (int)(FrameInfoArgs->framedetDifECavg));
  //DrawYV12(pdst, dst_height, dst_width, 0, 7, sbuf);
  //
  ////sprintf_s(sbuf, sizeof(sbuf), "framedetDifECavg %3d", (int)(FrameInfoArgs->framedetDifECavg));
  //sprintf_s(sbuf, sizeof(sbuf), "framedetDifECsadDIVavg %3d", (int)((FrameInfoArgs->frameDetDifECsad / FrameInfoArgs->framedetDifECavg) + .5));
  //DrawYV12(pdst, dst_height, dst_width, 0, 9, sbuf);
  //
  ////sprintf_s(sbuf, sizeof(sbuf), "sadCMed %3d sadEMed %3d dif %3d", (int)(FrameInfoArgs->sadCMedFrameAvg  + .5), (int)(FrameInfoArgs->sadEMedFrameAvg + .5), abs((int)(FrameInfoArgs->sadCMedFrameAvg  + .5) - (int)(FrameInfoArgs->sadEMedFrameAvg + .5)));
  ////sprintf_s(sbuf, sizeof(sbuf), "sC1Med %3d sCMed %3d dif %3d", (int)(FrameInfoArgs->sadC1MedFrameAvg  + .5), (int)(FrameInfoArgs->sadCMedFrameAvg + .5), ((int)(FrameInfoArgs->sadCMedFrameAvg  + .5) - (int)(FrameInfoArgs->sadC1MedFrameAvg + .5)));
  //sprintf_s(sbuf, sizeof(sbuf), "sC1Med %3d sCMed %3d dif %3d", (int)(FrameInfoArgs->sadC1MedFrameAvg  + .5), (int)(FrameInfoArgs->sadCMedFrameAvg + .5), (int)(((FrameInfoArgs->sadC1MedFrameAvg) / (FrameInfoArgs->sadCMedFrameAvg)) * 10.0));
  //DrawYV12(pdst, dst_height, dst_width, 0, 10, sbuf);
  //*/
  ////sprintf_s(sbuf, sizeof(sbuf), "adaptPercentPixUsedTotalPix %3d", (int)(FrameInfoArgs->PercentPixUsedTotalPix));
  //sprintf_s(sbuf, sizeof(sbuf), "adpPcentPixUsed %3d", (int)(FrameInfoArgs->PercentPixUsedTotalPix));
  ////sprintf_s(sbuf, sizeof(sbuf), "sadEMedDetFrameAvg %3d", (int)(FrameInfoArgs->sadEMedDetFrameAvg / 290.0));
  //DrawYV12(pdst, dst_height, dst_width, 0, 6, sbuf);
  //
  ////sprintf_s(sbuf, sizeof(sbuf), "LumBlkPercentPixUsedTotalPix %3d", (int)(FrameInfoArgs->frameDetDifECsad));
  ////DrawYV12(pdst, dst_height, dst_width, 0, 7, sbuf);
  //
  ///*
  //
  //////sprintf_s(sbuf, sizeof(sbuf), "difBinCount %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d", FrameInfoArgs->difBinCount[0],FrameInfoArgs->difBinCount[1],FrameInfoArgs->difBinCount[2],FrameInfoArgs->difBinCount[3],FrameInfoArgs->difBinCount[4],FrameInfoArgs->difBinCount[5],FrameInfoArgs->difBinCount[6],FrameInfoArgs->difBinCount[7],FrameInfoArgs->difBinCount[8],FrameInfoArgs->difBinCount[9],FrameInfoArgs->difBinCount[10]);
  ////sprintf_s(sbuf, sizeof(sbuf), "DBC %3d %3d %2d %1d %1d %1d %1d", difBinCount[0],difBinCount[1],difBinCount[2],difBinCount[3],difBinCount[4],difBinCount[5],difBinCount[6]); //,difBinCount[7],difBinCount[8],difBinCount[9],difBinCount[10],difBinCount[11]);
  ////DrawYV12(pdst, dst_height, dst_width, 0, 7, sbuf);
  ////
  ////sprintf_s(sbuf, sizeof(sbuf), "DBC %3d %3d %2d %1d %1d %1d %1d %1d %1d %1d", difBinCount[7],difBinCount[8],difBinCount[9],difBinCount[10],difBinCount[11],difBinCount[12],difBinCount[13],difBinCount[14],difBinCount[15],difBinCount[16]); //,difBinCount[10],difBinCount[11]);
  ////DrawYV12(pdst, dst_height, dst_width, 0, 8, sbuf);
  //*/
  //
  //
  ////if (FrameInfoArgs->framepixCnt < 1) FrameInfoArgs->framepixCnt = 1;
  ////temp = (int)(((float)FrameInfoArgs->difBlockTotalUsed / (float)FrameInfoArgs->framepixCnt) * 100.0);
  ////temp2 = (int)(((float)FrameInfoArgs->difBinCount[1] / (float)FrameInfoArgs->framepixCnt) * 100.0);
  ////temp3 = (int)(((float)FrameInfoArgs->difBinCount[2] / (float)FrameInfoArgs->framepixCnt) * 100.0);
  //
  ///*
  ////if (temp > max_debug3) max_debug3=temp;
  ////if (temp < min_debug3) min_debug3=temp;
  //if (temp > max_debug3) max_debug3=temp;  // max_debug3 = 19  13 typical
  //if (temp < min_debug3) min_debug3=temp;  // min_debug3 = 2    3 typical
  //sprintf_s(sbuf, sizeof(sbuf), "maxPcnt %2d minPcnt %2d", max_debug3, min_debug3);  // WORKS BETTER when NOT using corners.
  //DrawYV12(pdst, dst_height, dst_width, 0, 2, sbuf);
  //*/
  ////sprintf_s(sbuf, sizeof(sbuf), "PcntUsd=%2d B1=%2d B2=%2d Rest=%2d", temp, temp2, temp3, (temp - (temp2 + temp3)));
  ////DrawYV12(pdst, dst_height, dst_width, 0, 2, sbuf);
  //
  //
  ////sprintf_s(sbuf, sizeof(sbuf), "max_debug3=%2d min_debug3=%2d", max_debug3, min_debug3);
  ////DrawYV12(pdst, dst_height, dst_width, 0, 9, sbuf);
  //
  //
  //
  ////sprintf_s(sbuf, sizeof(sbuf), "qEdgeUse %2d", FrameInfoArgs->adaptWeight1);
  ////DrawYV12(pdst, dst_height, dst_width, 0, 2, sbuf);
  ////
  //////sprintf_s(sbuf, sizeof(sbuf), "adap2 %2d", FrameInfoArgs->adaptWeight2);
  //////DrawYV12(pdst, dst_height, dst_width, 0, 3, sbuf);
  //////
  //////sprintf_s(sbuf, sizeof(sbuf), "adap3 %2d", FrameInfoArgs->adaptWeight3);
  //////DrawYV12(pdst, dst_height, dst_width, 0, 4, sbuf);
  //////
  //////sprintf_s(sbuf, sizeof(sbuf), "adap4 %2d", FrameInfoArgs->adaptWeight4);
  //////DrawYV12(pdst, dst_height, dst_width, 0, 5, sbuf);
  //////
  ////sprintf_s(sbuf, sizeof(sbuf), "adapttemp %2d numBlocks_sizeY %2d", FrameInfoArgs->adaptTemp, (int)FrameInfoArgs->numBlocks_sizeY);
  ////DrawYV12(pdst, dst_height, dst_width, 0, 8, sbuf);
  //
  ////sprintf_s(sbuf, sizeof(sbuf), "adap%2d", FrameInfoArgs->adapt);
  ////DrawYV12(pdst, dst_height, dst_width, 1, 1, sbuf);
  ////sprintf_s(sbuf, sizeof(sbuf), "PixCntUsed  %6d", FrameInfoArgs->blkPixCountUsed);
  ////DrawYV12(pdst, dst_height, dst_width, 0, 5, sbuf);
  ////sprintf_s(sbuf, sizeof(sbuf), "SumAbDif   %5d", FrameInfoArgs->sumAbDif);
  ////DrawYV12(pdst, dst_height, dst_width, 0, 6, sbuf);
  //////sprintf_s(sbuf, sizeof(sbuf), "AbDif2 %2d", FrameInfoArgs->sumAbDif2);
  //////DrawYV12(pdst, dst_height, dst_width, 0, 3, sbuf);
  ////sprintf_s(sbuf, sizeof(sbuf), "pixCount %5d", FrameInfoArgs->sumAbDifPixCnt);
  ////DrawYV12(pdst, dst_height, dst_width, 0, 7, sbuf);
  ////sprintf_s(sbuf, sizeof(sbuf), "adaptUse  %2d", adaptTemp);
  ////DrawYV12(pdst, dst_height, dst_width, 0, 5, sbuf);
  ////sprintf_s(sbuf, sizeof(sbuf), "adaptUse2  %2d", temp);
  ////DrawYV12(pdst, dst_height, dst_width, 0, 6, sbuf);
  ////*/


  return(0);
}


///////////////////////
// END QUALITY >= 4  //
///////////////////////




///////////////////////
//////////////////////
// WE SHOULD NEVER GET HERE
//  free_data(FrameInfoArgs);

//  return(0);

//}

