
#include <math.h>   // Needed for pow()
#include <stdio.h>  // Needed for pow()

#include "amDCT.h"
#include "amDCTtypedefs.h"  // needed for uint32_t etc.

#include "avgDctLoop.h"

#include "BuildPreCompVals.h"

#include "Utilities.h"

#include "Font.h"

#include "blindPPcode.h"



#include <smmintrin.h> // SSE4.2





//       REMOVE AFTE MORE TESTING
//
//// This is only called from amDCT.c if there is no processing to do.
//// It is called before FrameInfoArgs has been created.
//// Putting the code here makes it easier to keep the algorithms the same.
////void copySrcToDst_noArgs(const uint8_t *psrc, uint8_t *pdst, uint16_t src_width, uint16_t dst_width, uint16_t dst_pitch, uint16_t dst_height) {
////
////const  uint8_t *psrcP = psrc; // The frame within the boarder starts boarder rows down and boarder pixels in
////    uint8_t *pdstP = pdst;
////    uint32_t h;
////
////  for (h=0; h < dst_height; h++) {
////    MY_MEMCPY(pdstP, psrcP, dst_width);
////    pdstP += dst_pitch;
////    psrcP += src_width;
////  }
////
////  return;
////}
//
//
////void copySrcToDst2(uint8_t *src, uint8_t *dst, uint32_t src_width, uint32_t dst_width, uint32_t dst_height) {
////
////  uint8_t *psrcP = src + src_width*8 + 8; // The frame within the boarder starts boarder rows down and boarder pixels in
////  uint8_t *pdstP = dst;
////  uint32_t h;
////
////  for (h=0; h < dst_height; h++) {
////    MY_MEMCPY(pdstP, psrcP, dst_width);
////    pdstP += dst_width;     // SHOULD THIS BE dst_PITCH !!!!!!!!!!!
////    psrcP += src_width;
////  }
////
////  return;
////}


// This is only used in amDCTmain.c  It copies the source frame to the destination frame.
void copySrcToDst(FrameInfo_args* FrameInfoArgs) {
  uint8_t* psrcP = FrameInfoArgs->frame_workSource + (FrameInfoArgs->src_width * 8) + 8; // The frame within the boarder starts boarder rows down and boarder pixels in
  uint8_t* pdstP = FrameInfoArgs->pdst;
  uint16_t dst_width = FrameInfoArgs->dst_width;
  uint16_t dst_pitch = FrameInfoArgs->dst_pitch;
  uint16_t src_width = FrameInfoArgs->src_width;
  uint32_t h;

  for (h = 0; h < FrameInfoArgs->dst_height; h++) {
    MY_MEMCPY(pdstP, psrcP, dst_width);
    pdstP += dst_pitch;
    psrcP += src_width;
  }

  return;
}
//      if (showMask==4 || showMask==7) {  // THIS CODE WORKS !!!!!!!!!!!!!!!!!!!!!!!!!!
//      uint8_t  *psrcP2     = FrameInfoArgs.frame_workSource + (FrameInfoArgs.src_width * 8) + 8; // The frame within the boarder starts boarder rows down and boarder pixels in
//      uint8_t  *pdstP2     = FrameInfoArgs.pdst;
//      uint16_t  dst_width2 = FrameInfoArgs.dst_width;
//      uint16_t  dst_pitch2 = FrameInfoArgs.dst_pitch;
//      uint16_t  src_width2 = FrameInfoArgs.src_width;
//      uint32_t  h2;
//
//      for (h2=0; h2 < FrameInfoArgs.dst_height; h2++) {
//        MY_MEMCPY(pdstP2, psrcP2, dst_width2);
//        pdstP2 += dst_pitch2;
//        psrcP2 += src_width2;
//      //  psrcP2 += FrameInfoArgs.src_pitch;
//      }
//      FREE_AND_RETURN_0;  // The FREE_AND_RETURN_0 MACRO is defined at top of this file.
////      return(0);
//    }



//    uint8_t *psrc_ref     = FrameInfoArgs->frame_workSource;
//    uint8_t *psrc_orig    = FrameInfoArgs->psrc; // + FrameInfoArgs->src_width*8 + 8;
//  uint8_t      *psrc_mem   = FrameInfoArgs->MemoryArgs->DctLoopArgs[0].BF_srcP; // + FrameInfoArgs->src_width*8 + 8;
//  uint8_t      *psrc_mem   = FrameInfoArgs->MemoryArgs->DctLoopArgs[0].BF_srcP + FrameInfoArgs->src_width*8 + 8;
//  uint8_t *psrc_mem  = FrameInfoArgs->MemoryArgs->DctLoopArgs[0].BF_srcP + FrameInfoArgs->src_pitch*8 + 8;
//  uint8_t  *psrc_mem  = FrameInfoArgs->MemoryArgs->DctLoopArgs[0].BF_srcP + FrameInfoArgs->src_pitch*8 + 8;

//  uint8_t  *psrc_mem  = FrameInfoArgs->frame_psrc_mem; // + FrameInfoArgs->src_pitch*8 + 8;    //   = FrameInfoArgs->MemoryArgs->DctLoopArgs[0].BF_srcP; // + FrameInfoArgs->src_pitch*8 + 8;
  //FrameInfoArgs->frame_psrc_mem       = FrameInfoArgs->MemoryArgs->DctLoopArgs[0].BF_srcP; // + FrameInfoArgs->src_pitch*8 + 8;

//  uint8_t      *pdst       = FrameInfoArgs->pdst;
//  unsigned int   src_width  = FrameInfoArgs->src_width;
//  unsigned int   src_pitch  = FrameInfoArgs->src_pitch;
//  unsigned int   width      = FrameInfoArgs->src_width;
//  unsigned int   dst_height = FrameInfoArgs->dst_height;
//  unsigned int   dst_width  = FrameInfoArgs->dst_width;
//  unsigned int   dst_pitch  = FrameInfoArgs->dst_width;

//  uint8_t *psrc_mem  = FrameInfoArgs->MemoryArgs->DctLoopArgs[0].BF_srcP + FrameInfoArgs->src_pitch*8 + 8;
//  uint8_t  *psrc_mem  = FrameInfoArgs->MemoryArgs->DctLoopArgs[0].BF_srcP + FrameInfoArgs->src_pitch*8 + 8;
//  uint8_t  *psrc_mem  = FrameInfoArgs->frame_psrc_mem; // + FrameInfoArgs->src_pitch*8 + 8;    //   = FrameInfoArgs->MemoryArgs->DctLoopArgs[0].BF_srcP; // + FrameInfoArgs->src_pitch*8 + 8;
  //FrameInfoArgs->frame_psrc_mem       = FrameInfoArgs->MemoryArgs->DctLoopArgs[0].BF_srcP; // + FrameInfoArgs->src_pitch*8 + 8;
//  uint8_t *psrcP     = FrameInfoArgs->frame_workSource + (FrameInfoArgs->src_width * 8) + 8; // The frame within the boarder starts boarder rows down and boarder pixels in
//  uint8_t      *pdst       = FrameInfoArgs->pdst;
//    uint8_t *psrc_ref     = FrameInfoArgs->frame_workSource;
//    uint8_t *psrc_orig    = FrameInfoArgs->psrc; // + FrameInfoArgs->src_width*8 + 8;

//  uint8_t      *psrc_mem   = FrameInfoArgs->MemoryArgs->DctLoopArgs[0].BF_srcP; // + FrameInfoArgs->src_width*8 + 8;
//  uint8_t      *psrc_mem   = FrameInfoArgs->MemoryArgs->DctLoopArgs[0].BF_srcP + FrameInfoArgs->src_width*8 + 8;


//  uint8_t *psrc_mem  = FrameInfoArgs->MemoryArgs->DctLoopArgs[0].BF_srcP + FrameInfoArgs->src_pitch*8 + 8;
//  uint8_t  *psrc_mem  = FrameInfoArgs->MemoryArgs->DctLoopArgs[0].BF_srcP + FrameInfoArgs->src_pitch*8 + 8;

//  uint8_t  *psrc_mem  = FrameInfoArgs->frame_psrc_mem; // + FrameInfoArgs->src_pitch*8 + 8;    //   = FrameInfoArgs->MemoryArgs->DctLoopArgs[0].BF_srcP; // + FrameInfoArgs->src_pitch*8 + 8;

  //FrameInfoArgs->frame_psrc_mem       = FrameInfoArgs->MemoryArgs->DctLoopArgs[0].BF_srcP; // + FrameInfoArgs->src_pitch*8 + 8;
//  uint8_t *psrcP     = FrameInfoArgs->frame_workSource + (FrameInfoArgs->src_width * 8) + 8; // The frame within the boarder starts boarder rows down and boarder pixels in


//  frameSize;      The size of the src frame, including the 8 bit boarder on each side.                                     // The size of the frame in pixels.
//  numBlocksY      The total number of 8x8 blocks in the source frame. It is equal to numBlocks_high*numBlocks_wide.
//  sizeBlocksWork  The size of the src frame, including a 16 bit boarder on each side. Size of work buffers in DctLoop.c    // The size of the work buffer in pixels.
//  numBlocksWork   The total number of blocks in the work buffer.                                                           // The size of the work buffer in blocks.

//      REUSING SRC_PITCH !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// Debugging Routine used to show various internal frames

        //width = src_pitch;
        //width = src_pitch;
        //width = src_width;
        //width = dst_width;

//  if (showMask==4)   psrcP = psrc_mem                            + width*8 + 8;  //BAD   // Output the frame after pre deblocking step.
// showMask 4 and 7 not working !!!!!!!!
//  if (showMask==1)   psrcP = FrameInfoArgs->frame_refSource    + width*8 + 8;           // The frame within the boarder starts boarder rows down and boarder pixels in
//  if (showMask==2)   psrcP = FrameInfoArgs->frame_adaptSource     + src_pitch*8 + 8;       // The frame after adapt()
//  if (showMask==3)   psrcP = FrameInfoArgs->frame_workSource     + src_pitch*8 + 8;         // The frame after adapt() and PreSmooth() and brightSmooth() have been run.
//  if (showMask==3)   psrcP = FrameInfoArgs->frame_workSource     + width*8 + 8;         // The frame after adapt() and PreSmooth() and brightSmooth() have been run.
//  if (showMask==4)   psrcP = psrc_mem                            + width*8 + 8;  //BAD   // Output the frame after pre deblocking step.
//  if (showMask==4)   psrcP = psrc_mem                            + src_pitch*8 + 8;  //BAD   // Output the frame after pre deblocking step.
//  if (showMask==4)   psrcP = psrc_mem                            + src_pitch*8 + 8;  //BAD   // Output the frame after pre deblocking step.
//  if (showMask==5)   psrcP = FrameInfoArgs->psrc                 +  width*8 + 8;         // Output the frame after pre deblocking step.

//  uint16_t dst_width = FrameInfoArgs->dst_width;
//  uint16_t dst_pitch = FrameInfoArgs->dst_pitch;
//  uint16_t src_width = FrameInfoArgs->src_width;
//  uint16_t src_pitch = FrameInfoArgs->src_pitch;

//FrameInfoArgs->frame_psrc_mem       = FrameInfoArgs->MemoryArgs->DctLoopArgs[0].BF_srcP; // + FrameInfoArgs->src_pitch*8 + 8;
//  uint8_t  *psrc_mem  = FrameInfoArgs->MemoryArgs->DctLoopArgs[0].BF_srcP + FrameInfoArgs->src_pitch*8 + 8;
//  uint8_t  *psrc_mem  = FrameInfoArgs->frame_psrc_mem; // + FrameInfoArgs->src_pitch*8 + 8;    //   = FrameInfoArgs->MemoryArgs->DctLoopArgs[0].BF_srcP; // + FrameInfoArgs->src_pitch*8 + 8;


//  frameSize  includes the 8x8 boarder

//  uint8_t       *psrcP = psrc_mem;
//  uint8_t       *pdstP = pdst;
//  unsigned int h;




//  if (showMask==6)   psrcP = FrameInfoArgs->frame_psrc_mem       + (src_width*8 + 8);     // Output the frame after pre smoothing. Same frame, different number so can debug the presmoothing step.

int copyFrameToDst(FrameInfo_args* FrameInfoArgs, uint8_t showMask) {

  //  uint8_t *psrcP     = FrameInfoArgs->frame_workSource + (FrameInfoArgs->src_width * 8) + 8; // The frame within the boarder starts boarder rows down and boarder pixels in
  //  uint16_t dst_width  = FrameInfoArgs->dst_width;
  //  uint16_t dst_height = FrameInfoArgs->dst_height;

  //  uint16_t dst_pitch = FrameInfoArgs->dst_pitch;
  uint16_t src_width = FrameInfoArgs->src_width;
  //  uint16_t src_pitch = FrameInfoArgs->src_pitch;

  //                     = FrameInfoArgs->MemoryArgs->DctLoopArgs[0].BF_srcP;   // + FrameInfoArgs->src_pitch*8 + 8;
  //  uint8_t  *psrc_mem  = FrameInfoArgs->MemoryArgs->DctLoopArgs[0].BF_srcP + (FrameInfoArgs->src_pitch * 8) + 8;

  uint8_t* psrcP = FrameInfoArgs->frame_workSource + (FrameInfoArgs->src_pitch * 8) + 8; // The frame within the boarder starts boarder rows down and boarder pixels in
  //  uint8_t *psrcP2    = FrameInfoArgs->frame_workSource + (FrameInfoArgs->src_pitch * 8) + 8;  // The frame within the boarder starts boarder rows down and boarder pixels in

  uint8_t* pdstP2 = FrameInfoArgs->pdst;
  uint16_t dst_width2 = FrameInfoArgs->dst_width;
  uint16_t dst_pitch2 = FrameInfoArgs->dst_pitch;
  uint16_t src_width2 = FrameInfoArgs->src_width;
  uint32_t h = 0;
  uint32_t h2 = 0;

  unsigned int   width = FrameInfoArgs->src_width;

  //  uint8_t *pdstP     = FrameInfoArgs->pdst;

  if (showMask > 21) {
    showBlockDetail(FrameInfoArgs, showMask);
    return(0);
  }


  //  e  showmask5  =  5   # SHOW_DEBLOCK        black screen    5   access violation
  //  f  showmask6  =  6   # SHOW_PRESMOOTHED    black screen    5     same


  if (showMask == 0) showMask = 1;

  if (showMask == 1)   psrcP = FrameInfoArgs->frame_refSource + (src_width * 8 + 8);
  if (showMask == 2)   psrcP = FrameInfoArgs->frame_adaptSource + (src_width * 8 + 8);
  if (showMask == 3)   psrcP = FrameInfoArgs->frame_workSource + (src_width * 8 + 8);
  if (showMask == 4)   psrcP = FrameInfoArgs->frame_workSource + (src_width * 8 + 8);
  if (showMask == 5)   psrcP = FrameInfoArgs->frame_sharp + (src_width * 8 + 8);
  if (showMask == 6)   psrcP = FrameInfoArgs->MemoryArgs->DctLoopArgs[0].BF_srcP + (src_width * 8 + 8);     // Output the frame after pre smoothing. Same frame, different number so can debug the presmoothing step.
  if (showMask == 7)   psrcP = FrameInfoArgs->frame_smoothed + (src_width * 8 + 8);
  if (showMask == 8)   psrcP = FrameInfoArgs->frame_sharp + (src_width * 8 + 8);
  if (showMask == 9)   psrcP = FrameInfoArgs->frame_boundaries + (src_width * 8 + 8);
  if (showMask == 10)   psrcP = FrameInfoArgs->frame_ImpulseNoise + (src_width * 8 + 8);
  if (showMask == 11)   psrcP = FrameInfoArgs->frame_3x3sadm + (src_width * 8 + 8);
  if (showMask == 12)   psrcP = FrameInfoArgs->frame_3x3mean + (src_width * 8 + 8);
  if (showMask == 13)   psrcP = FrameInfoArgs->frame_deringMask + (src_width * 8 + 8);
  if (showMask == 14)   psrcP = FrameInfoArgs->frame_grainMask + (src_width * 8 + 8);
  if (showMask == 15)   psrcP = FrameInfoArgs->frame_localMax9x9 + (src_width * 8 + 8);
  if (showMask == 16)   psrcP = FrameInfoArgs->frame_localMin9x9 + (src_width * 8 + 8);
  if (showMask == 17)   psrcP = FrameInfoArgs->frame_sharpSmoothDif + (src_width * 8 + 8);
  if (showMask == 18)   psrcP = FrameInfoArgs->frame_workSource + (src_width * 8 + 8);


  if (showMask == 20) {  // 9x9 max min dif
    uint8_t* psrc = FrameInfoArgs->psrc;// + width*8 + 8;
    uint8_t* pMax9x9 = FrameInfoArgs->frame_localMax9x9;
    uint8_t* pMin9x9 = FrameInfoArgs->frame_localMin9x9;
    unsigned int src_height = FrameInfoArgs->src_height;
    unsigned int x;
    for (h = 0; h < src_height; h++) {
      for (x = 0; x < src_width; x++) {
        psrc[x] = (uint8_t)abs(pMax9x9[x] - pMin9x9[x]);
      }
      psrc += width;
      pMax9x9 += width;
      pMin9x9 += width;
    }
    psrcP = FrameInfoArgs->psrc + src_width * 8 + 8;
  }

  for (h2 = 0; h2 < FrameInfoArgs->dst_height; h2++) {
    //    MY_MEMCPY(pdstP2, psrcP2, dst_width2);
    MY_MEMCPY(pdstP2, psrcP, dst_width2);
    pdstP2 += dst_pitch2;
    //    psrcP2 += src_width2;
    psrcP += src_width2;
  }
  return(0);
}

/*
//  if (showMask==4)   psrcP = psrc_mem                            + src_pitch*8 + 8;  //BAD   // Output the frame after pre deblocking step.
//  if (showMask==7)   psrcP = FrameInfoArgs->frame_smoothed       + src_pitch*8 + 8;  // BAD

//  if (showMask==4)   psrcP = psrc_mem                            + src_width*8 + 8;  //BAD   // Output the frame after pre deblocking step.
//  if (showMask==7)   psrcP = FrameInfoArgs->frame_smoothed       + src_width*8 + 8;  // BAD

//  if (showMask==4)   psrcP = psrc_mem                            + src_width*8 + 8;  //BAD   // Output the frame after pre deblocking step.
//  if (showMask==4)   psrcP = FrameInfoArgs->frame_workSource;//     + width*8 + 8;         // The frame after adapt() and PreSmooth() and brightSmooth() have been run.psrcP = psrc_mem  + src_width*8 + 8;  //BAD   // Output the frame after pre deblocking step.
//  if (showMask==7)   psrcP = FrameInfoArgs->frame_smoothed       + src_width*8 + 8;  // BAD

//psrcP = FrameInfoArgs->frame_workSource     + width*8 + 8;         // The frame after adapt() and PreSmooth() and brightSmooth() have been run.

//  if (showMask==6)   psrcP = psrc_mem;//                            +     width*8 + 8;         // Output the frame after pre smoothing. Same frame, different number so can debug the presmoothing step.
////  if (showMask==7)   psrcP = FrameInfoArgs->frame_smoothed       + src_pitch*8 + 8;  // BAD
//  if (showMask==8)   psrcP = FrameInfoArgs->frame_sharp          + src_width*8 + 8;
//  if (showMask==9)   psrcP = FrameInfoArgs->frame_boundaries     + src_width*8 + 8;
//  if (showMask==10)   psrcP = FrameInfoArgs->frame_ImpulseNoise   + src_width*8 + 8;
//  if (showMask==11)   psrcP = FrameInfoArgs->frame_3x3sadm        + src_width*8 + 8;
//  if (showMask==12)   psrcP = FrameInfoArgs->frame_3x3mean        + src_width*8 + 8;
//  if (showMask==13)   psrcP = FrameInfoArgs->frame_deringMask     + src_width*8 + 8;
//  if (showMask==14)   psrcP = FrameInfoArgs->frame_grainMask      + src_width*8 + 8;
//  if (showMask==15)   psrcP = FrameInfoArgs->frame_localMax9x9    + src_width*8 + 8;
//  if (showMask==16)   psrcP = FrameInfoArgs->frame_localMin9x9    + src_width*8 + 8;
//  if (showMask==17)   psrcP = FrameInfoArgs->frame_sharpSmoothDif + src_width*8 + 8;
//  if (showMask==18)   psrcP = FrameInfoArgs->frame_workSource     + src_width*8 + 8;
//  if (showMask==19)   psrcP = FrameInfoArgs->frame_workSource     + src_width*8 + 8;
//  if (showMask==22)   psrcP = FrameInfoArgs->frame_workSource     + src_width*8 + 8;


//  psrcP = FrameInfoArgs->psrc + src_width*8 + 8;
//  psrcP = FrameInfoArgs->frame_workSource + (FrameInfoArgs->src_width * 8) + 8; // The frame within the boarder starts boarder rows down and boarder pixels in
//  uint8_t       *pdstP = pdst;

  // This copies the processed source frame to the destination frame.
//  for (h=0; h < dst_height; h++) {
//    MY_MEMCPY(pdstP, psrcP, dst_width);
//    pdstP += dst_pitch;
////    pdstP += dst_width;
////    psrcP += src_pitch; //width;
//    psrcP += src_width + 16;
//  }
//
//  return(0);
*/
/*
  if (showMask==20) {  // 9x9 max min dif
    uint8_t *psrc    = FrameInfoArgs->psrc;// + width*8 + 8;
    uint8_t *pMax9x9 = FrameInfoArgs->frame_localMax9x9;
    uint8_t *pMin9x9 = FrameInfoArgs->frame_localMin9x9;
    unsigned int src_height = FrameInfoArgs->src_height;
    unsigned int x;
    for (h=0; h < src_height; h++) {
      for (x=0; x < src_width; x++) {
        psrc[x] = (uint8_t)abs(pMax9x9[x] - pMin9x9[x]);
      }
      psrc    += width;
      pMax9x9 += width;
      pMin9x9 += width;
    }
    psrcP = FrameInfoArgs->psrc + src_width*8 + 8;
  }

  if (showMask==21) {  // (9x9 max min avg) - src
    uint8_t *psrc    = FrameInfoArgs->psrc; // + width*8 + 8;
    uint8_t *pMax9x9 = FrameInfoArgs->frame_localMax9x9; // + src_width*8 + 8;
    uint8_t *pMin9x9 = FrameInfoArgs->frame_localMin9x9; // + src_width*8 + 8;
    unsigned int src_height = FrameInfoArgs->src_height;
    unsigned int x;
    for (h=0; h < src_height; h++) {
      for (x=0; x < src_width; x++) {
        //int temp = (int)abs(((pMax9x9[x] + pMin9x9[x] + 1)>>1) - psrc[x])<<3;
        int temp = (int)abs((pMax9x9[x] + pMin9x9[x] + 1)>>1); // - psrc[x])<<3;
        if (temp > 255) temp = 255;
        psrc[x] = (uint8_t)temp;

      }
      psrc    += width; //src_width;
      pMax9x9 += width; //src_width;
      pMin9x9 += width; //src_width;
    }
  }
//*/

/*
// This is only used in amDCTmain.c  It copies the source frame to the destination frame.
void copySrcToDst(FrameInfo_args *FrameInfoArgs) {
  uint8_t *psrcP     = FrameInfoArgs->frame_workSource + (FrameInfoArgs->src_width * 8) + 8; // The frame within the boarder starts boarder rows down and boarder pixels in
  uint8_t *pdstP     = FrameInfoArgs->pdst;
  uint16_t dst_width = FrameInfoArgs->dst_width;
  uint16_t dst_pitch = FrameInfoArgs->dst_pitch;
  uint16_t src_width = FrameInfoArgs->src_width;
  uint32_t h;

  for (h=0; h < FrameInfoArgs->dst_height; h++) {
    MY_MEMCPY(pdstP, psrcP, dst_width);
    pdstP += dst_pitch;
    psrcP += src_width;
  }

  return;
}
*/
//copySrcToDst(FrameInfoArgs);
//return(0);

    //psrcP += src_width;
//
////  psrcP = FrameInfoArgs->psrc + src_width*8 + 8;
//  psrcP = FrameInfoArgs->frame_workSource + (FrameInfoArgs->src_width * 8) + 8; // The frame within the boarder starts boarder rows down and boarder pixels in
////  uint8_t       *pdstP = pdst;
//
//  // This copies the processed source frame to the destination frame.
//  for (h=0; h < dst_height; h++) {
//    MY_MEMCPY(pdstP, psrcP, dst_width);
//    pdstP += dst_pitch;
////    pdstP += dst_width;
////    psrcP += src_pitch; //width;
//    psrcP += src_width;
//  }
//
//  return(0);
//}
// */



// Debugging Routine.
// This routine returns a frame that shows various per block information as shades of grey blocks.
// The block values have been normalized to the range 16-255 so that the smaller values can be seen.
int showBlockDetail(FrameInfo_args* FrameInfoArgs, uint8_t showMask) {

  unsigned int h, x;
  int detail;

  uint8_t* psrc = FrameInfoArgs->MemoryArgs->DctLoopArgs[0].BF_srcP;
  //    uint8_t *psrc           = &FrameInfoArgs->psrc;
  uint16_t   src_height = FrameInfoArgs->src_height;
  uint16_t   src_width = FrameInfoArgs->src_width;
  unsigned int   numBlocks_wide = FrameInfoArgs->numBlocks_wide;
  unsigned int   numBlocks_high = FrameInfoArgs->numBlocks_high;

  uint8_t* psrcP = psrc;

  detail = 0;
  for (h = 0; h < src_height; h++) {
    for (x = 0; x < src_width; x++) {
      int blk_detail = 0;
      //      int blk_detail2 = 0;
      int blk_detail_temp = 0;
      float blk_detailF = 0.0F;
      //      double blk_detailD = 0.0;
      int blkNum = ((h >> 3) * (src_width >> 3)) + (x >> 3);
      if (blkNum >= (int)(numBlocks_high * numBlocks_wide)) blkNum = numBlocks_high * numBlocks_wide - 1;
      if (blkNum < 0) blkNum = 0;
      // /*
      switch (showMask) {
        // These values are for the source frame.
//      case 20:
//        blk_detail = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].brightMax;
//        break;

//      case 21:
//        blk_detail = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].MaxLuma;      // The brightest pixel after the 2 brightest pixels have been dicarded.
//        break;

      case 22:
        //blk_detail = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sumBright;      // The brightest pixel after the 2 brightest pixels have been dicarded.
        //blk_detail = Int_FitRange_Int((int)FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sumBright, 2040, 0, 255, 32);
        blk_detail = Int_FitRange_Int((int)FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sumBright, 64, 0, 255, 32);
        break;

        //      case 22:
        //        blk_detail = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].AvgC;
        //        break;

      case 23:
        blk_detail = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].AvgLuma;
        break;

      case 24:
        blk_detail = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].MinLuma;
        break;

      case 25:
        blk_detail = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].brightMin;
        break;

      case 26:
        blk_detail = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].maxMinDif;    // Range of values in block
        break;

      case 27:  // range is 0-MAX_BLOCK_ADAPT
        //blk_detail = Int_FitRange_Int((int)FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sadE1f, 2560, 0, 255, 0);
        blk_detail = Int_FitRange_Int((int)FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sadE1f, 1060, 0, 255, 32);
        //blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sadE1f, 2608, 0, 255, 16);
        break;

        // The whole block is 8x8 = 64
        // The Max possible sad is 8192 = 64 * 128   This will occure if half of the pixels in block are 0 and the other half are 255
      case 28:
        //blk_detail = Int_FitRange_Int((int)FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sadf, 1152, 0, 255, 16);
        blk_detail = Int_FitRange_Int((int)FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sadf, 2152, 0, 255, 32);
        //blk_detail = Int_FitRange_Int((int)FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sadf, 4096, 0, 255, 0);  // FOR HD TEST CLIP
        //blk_detail = Int_FitRange_Int((int)FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sadf, 6144, 0, 255, 32);  // FOR VR4 TEST CLIP
        //if (blk_detail <= 32) blk_detail = 0;
        break;


        // The center block is 6x6 = 36
        // The Max possible sad is 4608 = 36 * 128   This will occure if half of the pixels in block are 0 and the other half are 255
      case 29:
        blk_detail = Int_FitRange_Int((int)FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sadCf, 1152, 0, 255, 32);
        //blk_detail = Int_FitRange_Int((int)FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sadCf, 1660, 0, 255, 16);
        blk_detail = Int_FitRange_Int((int)FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sadCf, 2040, 0, 255, 32);
        //blk_detail = Int_FitRange_Int((int)FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sadCf, 640, 0, 255, 32);
        //blk_detail = Int_FitRange_Int((int)FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sadCf, 140, 0, 32, 255);
        //blk_detail = Int_FitRange_Int((int)FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sadCf, 2304, 0, 255, 32);
        //blk_detail = Int_FitRange_Int((int)FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sadCf, 3456, 0, 255, 32);    // FOR VR4 TEST CLIP
        //if (blk_detail <= 32) blk_detail = 0;
        break;

        // The Edge of block is 9x9 - 6x6 = 64
        // The Max possible sad is 4608 = 36 * 128   This will occure if half of the pixels in block are 0 and the other half are 255
      case 30:
        //blk_detail = Int_FitRange_Int((int)FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sadEf, 1152, 0, 255, 32);
        //blk_detail = Int_FitRange_Int((int)FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sadEf, 1660, 0, 255, 32);
        //blk_detail = Int_FitRange_Int((int)FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sadEf, 4096, 0, 255, 32);
        blk_detail = Int_FitRange_Float(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sadEf, 11.0, 0.0, 255.0, 16.0);
        //blk_detail = Int_FitRange_Int((int)FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sadEf, 7176, 0, 255, 32);
        //blk_detail = Int_FitRange_Int((int)FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sadEf, 2304, 0, 255, 32);
        //blk_detail = Int_FitRange_Int((int)FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sadEf, 3456, 0, 255, 32);    // FOR VR4 TEST CLIP
        //if (blk_detail <= 32) blk_detail = 0;
        break;

      case 31:  // range is 0-32
        //blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].det, MAX_BLOCK_ADAPT << 1, 0, 255, 0);  // sad divided by range;
        blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].det, MAX_BLOCK_ADAPT, 0, 255, 0);  // sad divided by range;
        break;

      case 32:  // range is 0-3072
        //blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].detC, 82512, 0, 255, 0);  // sad divided by range;
        blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].detC, 16, 0, 255, 0);  // sad divided by range;
        //        blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].detC4, 1024, 0, 32, 255);  // sad divided by range;
                //blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].detC4, 4096, 0, 32, 255);  // sad divided by range;
                //blk_detail = Int_FitRange_Int((int)FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sadCf, 2128, 0, 255, 32);  // sad divided by range;
        break;

      case 33:  // range is 0-32
        blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].detE, 31, 0, 255, 0);  // sad divided by range;
        //        blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].detE, 1024, 0, 255, 0);  // sad divided by range;
        break;

      case 34:// range is 1-15
        //blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].detDifEC, 31, 0, 255, 16);  // sad divided by range;
        blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].detDifEC, 512, 0, 255, 0);  // sad divided by range;
        //blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].detDifEC, MAX_BLOCK_ADAPT - 1, 0, 255, 0) + 16;  // sad divided by range;
        break;

      case 35:  //
        blk_detail = Int_FitRange_Float(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].std_Dev, 88, 0, 255, 0);
        break;

      case 36:  //
        //blk_detail = Int_FitRange_Float(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].std_DevC, 44, 0, 255, 0);
        blk_detail = Int_FitRange_Float(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].std_DevC, 128, 0, 255, 0);
        break;

      case 37://
        blk_detail = Int_FitRange_Float(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].std_DevE, 144, 0, 255, 0);
        break;

      case 38:
        //blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].coeff8_energy, 2097152, 0, 255, 0) + 16; // 2097152 = <<21  // add 16 to make low value block visible.
        blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].detDifEC2, 32, 0, 255, 0); // 2097152 = <<21  // add 16 to make low value block visible.
        if (blk_detail > 255) blk_detail = 255;
        //blk_detail = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].coeff8_energy; // always 0 !!!!!!!!!!!
        break;

      case 39:
        //blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].coeff8_energy, 2097152, 0, 255, 0) + 16; // 2097152 = <<21  // add 16 to make low value block visible.
        //blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].detDifEC4, 31, 0, 255, 16); // 2097152 = <<21  // add 16 to make low value block visible.
        blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].detDifEC4, 12, 0, 255, 16); // 2097152 = <<21  // add 16 to make low value block visible.
        if (blk_detail > 255) blk_detail = 255;
        //blk_detail = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].coeff8_energy; // always 0 !!!!!!!!!!!
        break;


        //      case 38:
        //        //blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].coeff8_energy, 2097152, 0, 255, 0) + 16; // 2097152 = <<21  // add 16 to make low value block visible.
        //        blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].coeff8_energy, MAX_BLOCK_ADAPT, 0, 255, 0) + 16; // 2097152 = <<21  // add 16 to make low value block visible.
        //        if (blk_detail > 255) blk_detail = 255;
        //        //blk_detail = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].coeff8_energy; // always 0 !!!!!!!!!!!
        //        break;


        //      case 39:
        //        //blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].coeff8_energy, 2097152, 0, 255, 0) + 16; // 2097152 = <<21  // add 16 to make low value block visible.
        //        blk_detail = Int_FitRange_Int((int)FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].blkSumEdge1, 8048, 0, 255, 0) + 16; // 4000 gives interesting block edge smothing mask when diffed with src, then mult dif*8.    2097152 = <<21  // add 16 to make low value block visible.
        //        if (blk_detail > 255) blk_detail = 255;
        //        //blk_detail = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].coeff8_energy; // always 0 !!!!!!!!!!!
        //        break;


      case 40:
        blk_detail = (int)FrameInfoArgs->frameAvgDetE;
        break;

      case 41:
        blk_detail = (int)FrameInfoArgs->frameAvgDetC;
        break;

      case 42:
        blk_detail = Int_FitRange_Int((int)FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].blkAdaptSmooth, 32, 0, 255, 0) + 16;
        //blk_detail = Int_FitRange_Int((int)FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].blkAdaptSmooth, 32, 0, 0, 255) + 16;
        break;



        // These values are for the sharpened frame.

      case 43:
        blk_detail = Int_FitRange_Int((int)FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].maxMinDif, 255, 0, 255, 16);
        //blk_detail = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].adapt_aC;
        break;


      case 44:
        blk_detail = Int_FitRange_Int((int)FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sadNormEC, 32, 0, 255, 32);
        break;

        /*
              case 41:adapt_a
                blk_detail = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sharp_MaxLuma;      // The brightest pixel after the 4 brightest pixels have been dicarded.
                blk_detail = Int_FitRange_Int((int)FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sadCf, 1660, 0, 255, 16);

                break;

              case 42:
                blk_detail = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sharp_MedianLuma;
                break;

              case 43:
                blk_detail = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sharp_AvgLuma;
                break;

              case 44:
                blk_detail = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sharp_MinLuma;
                break;

              case 45:
                blk_detail = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sharp_brightMin;
                break;

              case 46:
                blk_detail = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sharp_maxMinDif;    // Range of values in block
                break;

              case 47:
                blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sharp_sad, MAX_BLOCK_ADAPT, 0, 255, 16);
                break;

              case 48:
                blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sharp_det, 32, 0, 255, 0);  // sad divided by range;
                break;

              case 49:
                blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sharp_detC, 4608, 0, 255, 0);  // sad divided by range;
                break;

              case 50:
                blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sharp_detE, 32, 0, 255, 0);    // sad divided by range;
                break;

              case 51:
                //blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sharp_detDifEC, 8, 0, 255, 0);  // sad divided by range;
                blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sharp_detDifEC, 16, 0, 255, 0);  // sad divided by range;
                break;

              case 52:
                //blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sharp_coeff8_energy, 4194304, 0, 255, 0) + 16; // 4194304 = <<22  // add 16 to make low value block visible.
                blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sharp_coeff8_energy, MAX_BLOCK_ADAPT - 1, 0, 255, 0) + 16; // 2097152 = <<21  // add 16 to make low value block visible.
                //blk_detail = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sharp_coeff8_energy;
                if (blk_detail > 255) blk_detail = 255;
                //blk_detail = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sharp_coeff8_energy;
                break;


              case 53:  //
                blk_detail = Int_FitRange_Float(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].variance, 4096, 0, 255, 0);
                break;

              case 54:  //
                blk_detail = Int_FitRange_Float(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].varianceC, 3072, 0, 255, 0);
                break;

              case 55://
                blk_detail = Int_FitRange_Float(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].varianceE, 4096, 0, 255, 0);
                break;
        */

      case 56:
        //blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].coeff8_energy, 2097152, 0, 255, 0) + 16; // 2097152 = <<21  // add 16 to make low value block visible.
        blk_detail = Int_FitRange_Float(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].blkSadEdge1, 3272.0, 0.0, 255.0, 0.0) + 32; // 2097152 = <<21  // add 16 to make low value block visible.
        //blk_detail = Int_FitRange_Float(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].blkSadEdge1, 128.0, 0.0, 255.0, 0.0) + 16; // 2097152 = <<21  // add 16 to make low value block visible.
        if (blk_detail > 255) blk_detail = 255;
        //blk_detail = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].coeff8_energy; // always 0 !!!!!!!!!!!
        break;

      case 57:
        //blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].coeff8_energy, 2097152, 0, 255, 0) + 16; // 2097152 = <<21  // add 16 to make low value block visible.
        //        blk_detail = Int_FitRange_Float(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].blkMeanEdge1, 255.0, 0.0, 255.0, 0.0) + 16; // 2097152 = <<21  // add 16 to make low value block visible.
//        blk_detail = (int)FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].blkMeanEdge1;
//        if (blk_detail > 255) blk_detail = 255;
        //blk_detail = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].coeff8_energy; // always 0 !!!!!!!!!!!
        break;


      case 58:
        //blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].coeff8_energy, 2097152, 0, 255, 0) + 16; // 2097152 = <<21  // add 16 to make low value block visible.
        blk_detail = Int_FitRange_Float(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].blkSumCent, 128.0, 0.0, 255.0, 0.0) + 16; // 2097152 = <<21  // add 16 to make low value block visible.
        if (blk_detail > 255) blk_detail = 255;
        //blk_detail = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].coeff8_energy; // always 0 !!!!!!!!!!!
        break;


      case 59:
        //blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].coeff8_energy, 2097152, 0, 255, 0) + 16; // 2097152 = <<21  // add 16 to make low value block visible.
        blk_detail = Int_FitRange_Float(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].blkSadCent, 2558.0, 0.0, 255.0, 0.0) + 16; // 2097152 = <<21  // add 16 to make low value block visible.
        if (blk_detail > 255) blk_detail = 255;
        //blk_detail = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].coeff8_energy; // always 0 !!!!!!!!!!!
        break;




        // These values are other block values.

      case 60:
        blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sadIncrease, MAX_BLOCK_ADAPT - 1, 0, 255, 0) + 16;
        break;

      case 61:
        blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].rangeIncrease, MAX_BLOCK_ADAPT - 1, 0, 255, 0) + 16;
        break;

        //      case 62:
        //        //        blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].coeff8_energyIncrease, MAX_BLOCK_ADAPT - 1, 0, 255, 0) + 16;
        //        //        blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].coeff8_energyIncrease, 19, 0, 255, 0) + 32;
        //        blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].coeff8_energyIncrease, 18, 0, 255, 32);
        //        if (blk_detail <= 32) blk_detail = 0;
        //        break;

      case 63:
        blk_detail_temp = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sad3x3avg + ((FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sad + 1) >> 1);
        blk_detail = Int_FitRange_Int(blk_detail_temp, MAX_BLOCK_ADAPT, 0, 255, 0) + 16;
        //blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sad3x3avg, MAX_BLOCK_ADAPT - 1, 0, 255, 0) + 16;
        //blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sad3x3avg, 9, 0, 255, 0) + 16;
        break;

        //      case 64:
        //        blk_detail = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].adaptDebAmt<<4;
        //        break;
        //
        //      case 65:
        //        blk_detail = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].adaptDebAmtRaw<<3;
        //        break;
        //      }


        //      case 64:
        //        blk_detail = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].absAvgSharpSmooth << 3; //<<5;
        //        break;

      case 65:
        blk_detail = (FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].adaptDebAmt2) + 32;
        break;

      case 66:
        //        //blk_detail = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].blkDetailECMod1;
        //        //blk_detail = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].blkDetailECMod1;
        //        //sadmIncrease and the referenceSource coeff8_energy
        //        blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sadIncrease + FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].coeff8_energyIncrease, MAX_BLOCK_ADAPT - 1, 0, 255, 0) + 16;
                //blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].avgBnd, MAX_BLOCK_ADAPT - 1, 0, 255, 0) + 16;
          //      blk_detail = Int_FitRange_Float(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].avgBnd, 64.0F, 0.0F, (float)FrameInfoArgs->adapt_a, (float)FrameInfoArgs->quant_a);
        //        blk_detail = Int_FitRange_Int(blk_detail, 31, 1, 255, 0) + 16;
        //        blk_detail = Int_FitRange_Int(blk_detail, (float)FrameInfoArgs->adapt_a, (float)FrameInfoArgs->quant_a, 255, 0) + 16;
            //    blk_detail = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].avgBnd;

        blk_detail = Int_FitRange_Float(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].avgBnd, (float)FrameInfoArgs->adapt_a, (float)FrameInfoArgs->quant_a, 255.0F, 0.0F) + 16;
        break;


      case 67:
        blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].blkAdaptSmooth, MAX_BLOCK_ADAPT, 0, 255, 0) + 16;

        //        blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->DctLoopArgs[0].blkAdaptExpandAmt[blkNum], MAX_BLOCK_ADAPT - 1, 0, 255, 0) + 16;

        //blk_detail = FrameInfoArgs->MemoryArgs->DctLoopArgs[0].blkAdaptExpandAmt[blkNum];
        break;

      case 68:
        //blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].blkAdaptExpand, MAX_BLOCK_ADAPT - 1, 0, 255, 0) + 16;
        //blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->DctLoopArgs[0].blkAdaptExpandAmt[blkNum], MAX_BLOCK_ADAPT - 1, 0, 255, 0); // + 16;
        //        blk_detail = FrameInfoArgs->MemoryArgs->DctLoopArgs[0].blkAdaptExpand[blkNum];
        //        blk_detail = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].blkAdaptExpandAmt;
        //        blk_detail = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].AvgLuma;

        // THE FOLLOWING 2 ARE DIFFERENT FROM 23
        //        blk_detail = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].blkAdaptExpand;  //   = avgLumaVal; //(uint8_t)(avgLuma + 0.5F);
        //blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].blkAdaptExpandAmt, MAX_BLOCK_ADAPT - 1, 0, 255, 0) + 16;      //   = avgLumaVal; //(uint8_t)(avgLuma + 0.5F);
        //blk_detail = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].blkAdaptExpand;      //   = avgLumaVal; //(uint8_t)(avgLuma + 0.5F);  adapt_a quant_a
        //blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].blkAdaptExpand, 128, 0, FrameInfoArgs->adapt_a, FrameInfoArgs->quant_a);
        //blk_detail = Int_FitRange_Int(blk_detail, 32, 0, 255, 0) + 16;
        //blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].blkAdaptExpand, 32, 0, 0, 255) + 16;

        //blk_detail = FrameInfoArgs->MemoryArgs->DctLoopArgs[0].blkAdaptExpandAmt[blkNum]; // = avgLumaVal;  //(uint8_t)(avgLuma + 0.5F);
        blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->DctLoopArgs[0].blkAdaptExpandAmt[blkNum], 31, 0, 255, 32) + 0;

        //blk_detail = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].AvgLuma; // THIS IS THE SAME AS mask=23

        break;

      case 69:
        blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].blkAdaptSharp, MAX_BLOCK_ADAPT - 1, 0, 255, 0) + 16;
        break;



      case 70:
        blk_detail = Int_FitRange_Int((FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sadIncrease << 1) +
          (FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].blkAdaptSmooth << 0) +
          //(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sad3x3avg << 2), MAX_BLOCK_ADAPT - 1, 0, 255, 0) + 16;
          (FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sad3x3avg << 2), (MAX_BLOCK_ADAPT << 3) - 1, 0, 255, 0) + 16;
        break;

      case 71:
        blk_detail_temp = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].coeff8_3x3avg;
        //blk_detail = Int_FitRange_Int(blk_detail_temp, MAX_BLOCK_ADAPT - 8, 0, 255, 0) + 16;
        blk_detail = Int_FitRange_Int(blk_detail_temp, 512, 0, 255, 32);
        //blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sad3x3avg, MAX_BLOCK_ADAPT - 1, 0, 255, 0) + 16;
        //blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sad3x3avg, 9, 0, 255, 0) + 16;
        break;

      case 72:
        blk_detail_temp = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].AvgBound;
        blk_detail = Int_FitRange_Int(blk_detail_temp, 255, 0, 255, 0) + 16;
        //blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sad3x3avg, MAX_BLOCK_ADAPT - 1, 0, 255, 0) + 16;
        //blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sad3x3avg, 9, 0, 255, 0) + 16;
        break;

      case 73:
        blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sad3x3avg, MAX_BLOCK_ADAPT - 1, 0, 255, 0) + 16;
        //blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sad3x3avg, MAX_BLOCK_ADAPT - 1, 0, 255, 0) + 16;
        //blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sad3x3avg, 9, 0, 255, 0) + 16;
        break;




        //      case 80:
        //        blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].detH2, 200000, 0, 255, 0) + 16;
        //        break;

      case 82:
        blk_detail = Int_FitRange_Int((int)FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sadBMedf, 2028, 0, 255, 0) + 16;
        break;

      case 83:
        blk_detail = Int_FitRange_Int((int)FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sadCMedf, 2028, 0, 255, 0) + 16;
        break;

      case 84:
        blk_detail = Int_FitRange_Int((int)FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sadEMedf, 2028, 0, 255, 0) + 16;
        break;


      case 85:
        blk_detail = Int_FitRange_Int(abs((int)(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sadCH2Medf - FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sadCMedf + .5)), 32, 0, 255, 0) + 32;
        break;


        //      case 86:
        //        blk_detail = Int_FitRange_Int(abs((int)(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sadCH2Medf - FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sadCMedf + .5)), 32, 0, 255, 0) + 32;
        //        break;

         // /*

      case 90:// range is 1-15
        blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].detDifEC0, 255, 0, 255, 16);  // sad divided by range;
        //blk_detail = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].MedianLumaC; //Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].MedianLumaC) //, 15, 0, 255, 0);  // sad divided by range;
        //blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].detDifECF, MAX_BLOCK_ADAPT - 1, 0, 255, 0) + 16;  // sad divided by range;
        break;

      case 91:// range is 1-15
        blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].detDifEC1, 255, 0, 255, 16);  // sad divided by range;
        //blk_detail = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].MedianLumaC1; //Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].MedianLumaC) //, 15, 0, 255, 0);  // sad divided by range;
        //blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].detDifEC, MAX_BLOCK_ADAPT - 1, 0, 255, 0) + 16;  // sad divided by range;
        break;

      case 92:// range is 1-15
        blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].detDifEC2, 31, 0, 255, 0);  // sad divided by range;
        //blk_detail = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].MedianLumaE; //Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].MedianLumaC) //, 15, 0, 255, 0);  // sad divided by range;
        //blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].detDifEC, MAX_BLOCK_ADAPT - 1, 0, 255, 0) + 16;  // sad divided by range;
        break;

      case 93:// range is 1-15
        //blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].detDifEC, 8, 0, 255, 0);  // sad divided by range;
        //blk_detail = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].detDifEC3; //Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].MedianLumaC) //, 15, 0, 255, 0);  // sad divided by range;
        blk_detail = Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].detDifEC3, 255, 0, 255, 32);  // sad divided by range;
        break;
        ///*


                //blk_detail = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].AvgLuma;

      case 94:
        blk_detailF = (float)FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].RS_Mean; //Int_FitRange_Int(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].MedianLumaC) //, 15, 0, 255, 0);  // sad divided by range;
        //blk_detailF = Float_FitRange_Float(blk_detailF, 0, 64, 32, 255);
        blk_detail = ROUND_TOINT(blk_detailF);
        break;

      case 95:
        blk_detailF = (float)FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].RS_Variance;
        blk_detail = Int_FitRange_Float(blk_detailF, 12000.0, 0.0, 255.0, 32.0);
        break;

      case 96:
        blk_detailF = (float)FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].RS_StDeviation;
        blk_detail = Int_FitRange_Float(blk_detailF, 140.0, 0.0, 255.0, 16.0);
        break;

      case 97:  // if (detail > 80) detail = 255;
        blk_detail_temp = ROUND_TOINT(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].RS_Mean);
        blk_detailF = (float)FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].RS_Skewness;
        //        blk_detail = Int_FitRange_Float(blk_detailF, 3.0, 0.0, 255.0,  16.0);
        //        blk_detail = Int_FitRange_Float(blk_detailF, 516.0, 0.0, 255.0,  16.0);
        blk_detail = Int_FitRange_Float(blk_detailF, 516.0, 0.0, 1.0, 254.0);
        if (blk_detail < 200) blk_detail = 0; //blk_detail_temp; //ROUND_TOINT(FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].RS_Mean);
        //if (blk_detail < 200) blk_detail = 128;
        break;
        //*/
      case 98:
        blk_detailF = (float)FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].RS_Kurtosis;
        //        blk_detail = Int_FitRange_Float(blk_detailF, 130.0, 56.0, 255.0,  16.0);
        //        blk_detail = Int_FitRange_Float(blk_detailF, 600.0, 56.0, 255.0,  16.0);
        //        blk_detail = Int_FitRange_Float(blk_detailF, 4000.0, 64.0, 255.0,  16.0);
        blk_detail = Int_FitRange_Float(blk_detailF, 4000.0, 64.0, 0.0, 255.0);
        break;




        //      case 98:
        //        blk_detail = Int_FitRange_Int((int)FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].blkAdaptExpand, 31, 0, 255,16);  // sad divided by range;
        //        break;

      case 99:
        blk_detail = Int_FitRange_Int((int)FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sadZeroE1Detf, 4000, 0, 255, 0);  // sad divided by range;
        //blk_detail = (int)((FrameInfoArgs->MemoryArgs->LumaPerBlockArgs[blkNum].sadZeroE1Detf / 64.0F) + .5);
        break;
        // */
      }

      // */
      if (blk_detail > 255) blk_detail = 255;
      if (blk_detail < 0)   blk_detail = 0;
      //      psrcP[h*src_width + x] = (uint8_t)detail;
      psrcP[h * src_width + x] = (uint8_t)blk_detail;
    }
  }

  copyFrameToDst(FrameInfoArgs, 4);

  return(0);
}













/////////////////////////////////////////////////////////////////////////
// NOTE the following code is from Tom Barry's Avisynth undot filter.
//////////////////////////////////////////////////////////////////////////
// Copyright (c) 2002 Tom Barry.  All rights reserved.
//      trbarry@trbarry.com
// Requires Avisynth source code to compile for Avisynth
// Avisynth Copyright 2000 Ben Rudiak-Gould.
//      http://www.math.berkeley.edu/~benrg/avisynth.html
/////////////////////////////////////////////////////////////////////////////
//
//  This file is subject to the terms of the GNU General Public License as
//  published by the Free Software Foundation.  A copy of this license is
//  included with this software distribution in the file COPYING.  If you
//  do not have a copy, you may obtain a copy by writing to the Free
//  Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
//
//  This software is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details
//
//  Also, this program is "Philanthropy-Ware".  That is, if you like it and
//  feel the need to reward or inspire the author then please feel free (but
//  not obligated) to consider joining or donating to the Electronic Frontier
//  Foundation. This will help keep cyber space free of barbed wire and bullsh*t.
//
/////////////////////////////////////////////////////////////////////////////
// Change Log
//
// Date          Developer             Changes
//
// 16 Jan 2003   Tom Barry           Use AvisynthPluginInit2

// 02 Nov 2002   Tom Barry           Create UnDot V 0.0.1.0
//
/////////////////////////////////////////////////////////////////////////////

// YV12 has 1 bytes per pixel
#undef  BPP
#define BPP 1

#ifdef USE_NEW_INTRINSICS
// This macro gets the high and low values of the 8 surrounding pixels. Register esi
// is assumed to point one line above the target pixel and reg ebx is assumed to
// contain the source pitch. Defined value BPP must be set to bytes per pixel,
// either 1 for YV12 or 2 for YUY2. Answer returned in mm0, mm1
#define GetMinMax(esi, ebx, mm0, mm1) \
{ \
    __m128i above_left = _mm_loadl_epi64((__m128i*)(esi - BPP)); \
    __m128i above_center = _mm_loadl_epi64((__m128i*)(esi)); \
    __m128i above_right = _mm_loadl_epi64((__m128i*)(esi + BPP)); \
    __m128i left = _mm_loadl_epi64((__m128i*)(esi + ebx - BPP)); \
    __m128i right = _mm_loadl_epi64((__m128i*)(esi + ebx + BPP)); \
    __m128i bottom_left = _mm_loadl_epi64((__m128i*)(esi + 2 * ebx - BPP)); \
    __m128i bottom_center = _mm_loadl_epi64((__m128i*)(esi + 2 * ebx)); \
    __m128i bottom_right = _mm_loadl_epi64((__m128i*)(esi + 2 * ebx + BPP)); \
    mm0 = _mm_min_epu8(above_left, above_center); \
    mm1 = _mm_max_epu8(above_left, above_center); \
    mm0 = _mm_min_epu8(mm0, above_right); \
    mm1 = _mm_max_epu8(mm1, above_right); \
    mm0 = _mm_min_epu8(mm0, left); \
    mm1 = _mm_max_epu8(mm1, left); \
    mm0 = _mm_min_epu8(mm0, right); \
    mm1 = _mm_max_epu8(mm1, right); \
    mm0 = _mm_min_epu8(mm0, bottom_left); \
    mm1 = _mm_max_epu8(mm1, bottom_left); \
    mm0 = _mm_min_epu8(mm0, bottom_center); \
    mm1 = _mm_max_epu8(mm1, bottom_center); \
    mm0 = _mm_min_epu8(mm0, bottom_right); \
    mm1 = _mm_max_epu8(mm1, bottom_right); \
}

void UnDot(int src_pit, int dst_pit, int row_size, const uint8_t* srcp, uint8_t* dstp, int FldHeight) {
  const uint8_t* pSrc = srcp;
  uint8_t* pDest = dstp;
  int y;

  memcpy(pDest, pSrc, row_size);    // copy first line
  memcpy(pDest + dst_pit * (FldHeight - 1), pSrc + src_pit * (FldHeight - 1), row_size); // and last
  pDest += dst_pit;

  for (y = 1; y <= FldHeight - 2; y++) {
    const uint8_t* esi = pSrc;
    uint8_t* edi = pDest;
    int ecx = row_size >> 3; // do 8 bytes at a time

    // do first  qword
    __m128i mm0 = _mm_loadl_epi64((__m128i*)(esi + src_pit));
    _mm_storel_epi64((__m128i*)edi, mm0);
    esi += BPP; // skip over first pixel

    __m128i mm1;
    GetMinMax(esi, src_pit, mm0, mm1);
    mm0 = _mm_max_epu8(mm0, _mm_loadl_epi64((__m128i*)(esi + src_pit)));
    mm0 = _mm_min_epu8(mm0, mm1);
    _mm_storel_epi64((__m128i*)(edi + BPP), mm0);

    // do last qword
    esi += row_size - 8 - BPP; // point at last qword
    mm0 = _mm_loadl_epi64((__m128i*)(esi + src_pit));
    _mm_storel_epi64((__m128i*)(edi + row_size - 8), mm0);
    esi -= BPP; // back up one pixel

    GetMinMax(esi, src_pit, mm0, mm1);
    mm0 = _mm_max_epu8(mm0, _mm_loadl_epi64((__m128i*)(esi + src_pit)));
    mm0 = _mm_min_epu8(mm0, mm1);
    _mm_storel_epi64((__m128i*)(edi + row_size - BPP - 8), mm0);

    esi = pSrc; // restore esi
    ecx -= 1; // but both ends done manually
    if (ecx == 0) goto AllDoneYV12; // too short, don't bother

    while (ecx--) {
      esi += 8; // bump to next qword
      edi += 8;

      GetMinMax(esi, src_pit, mm0, mm1);
      mm0 = _mm_max_epu8(mm0, _mm_loadl_epi64((__m128i*)(esi + src_pit)));
      mm0 = _mm_min_epu8(mm0, mm1);
      _mm_storel_epi64((__m128i*)edi, mm0); // and store it
    }

  AllDoneYV12:
    // adjust for next line
    pSrc += src_pit;
    pDest += dst_pit;
  }

}

#else
// This macro gets the high and low values of the 8 surrounding pixels. Register esi
// is assumed to point one line above the target pixel and reg ebx is assumbed to
// contain the source pitch. Defined value BPP must be set to bytes per pixel,
// either 1 for YV12 or 2 for YUY2. Answer returned in mm0, mm1
#define GetMinMax \
__asm { \
    __asm movq  mm0, qword ptr[esi-BPP]      /* Above left, mm0 will hold min */ \
    __asm movq  mm1, mm0            /* Above left, mm1 will hold max */ \
    __asm pminub mm0, qword ptr[esi]      /* above center */ \
    __asm pmaxub mm1, qword ptr[esi]      /* above center */ \
    __asm pminub mm0, qword ptr[esi+BPP]    /* above right */ \
    __asm pmaxub mm1, qword ptr[esi+BPP]    /* above right */ \
    __asm pminub mm0, qword ptr[esi+ebx-BPP]  /* left */ \
    __asm pmaxub mm1, qword ptr[esi+ebx-BPP]  /* left */ \
    __asm pminub mm0, qword ptr[esi+ebx+BPP]  /* right */ \
    __asm pmaxub mm1, qword ptr[esi+ebx+BPP]  /* right */ \
    __asm pminub mm0, qword ptr[esi+2*ebx-BPP]  /* bottom left */ \
    __asm pmaxub mm1, qword ptr[esi+2*ebx-BPP]  /* bottom left */ \
    __asm pminub mm0, qword ptr[esi+2*ebx]    /* bottom center */ \
    __asm pmaxub mm1, qword ptr[esi+2*ebx]    /* bottom center */ \
    __asm pminub mm0, qword ptr[esi+2*ebx+BPP]  /* bottom right */ \
    __asm pmaxub mm1, qword ptr[esi+2*ebx+BPP]  /* bottom right */ \
}


void UnDot(int src_pit, int dst_pit, int row_size, const uint8_t* srcp, uint8_t* dstp, int FldHeight)
{

  const uint8_t* pSrc = srcp;
  uint8_t* pDest = dstp;
  int y;

  memcpy(pDest, pSrc, row_size);    // copy first line
  memcpy(pDest + dst_pit * (FldHeight - 1),
    pSrc + src_pit * (FldHeight - 1), row_size);  // and last
  pDest += dst_pit;

  for (y = 1; y <= FldHeight - 2; y++)
  {
    __asm
    {
      // Loop general reg usage
      //
      // ecx - count
      // edi - dest pixels
      // esi - src pixels, 1 line up

      mov    esi, pSrc
      mov    ebx, src_pit
      mov    edi, pDest
      mov    eax, row_size
      mov    ecx, eax
      shr    ecx, 3            // do 8 bytes at a time

      // do first  qword
      movq  mm0, qword ptr[esi + ebx]    // move 1st 4 pixels
      movq  qword ptr[edi], mm0
      add    esi, BPP          // skip over first pixel
      GetMinMax
      pmaxub  mm0, qword ptr[esi + ebx]    // allow no lower than min
      pminub  mm0, mm1          // allow no higher than max
      movq  qword ptr[edi + BPP], mm0

      // do last qword
      lea    esi, [esi + eax - 8 - BPP]    // point at last qword
      movq  mm0, qword ptr[esi + ebx]    // move 1st 4 pixels
      movq  qword ptr[edi + eax - 8], mm0
      sub    esi, BPP          // back up one pixel
      //GetMinMax
      pmaxub  mm0, qword ptr[esi + ebx]    // allow no lower than min
      pminub  mm0, mm1          // allow no higher than max
      movq  qword ptr[edi + eax - BPP - 8], mm0

      mov    esi, pSrc          // restore esi
      sub    ecx, 1            // but both ends done manually
      jz    AllDoneYV12          // too short, don't bother

      LoopYV12 :
      add    esi, 8            // bump to next qword
        add    edi, 8

        GetMinMax
        pmaxub  mm0, qword ptr[esi + ebx]    // allow no lower than min
        pminub  mm0, mm1          // allow no higher than max
        movntq  qword ptr[edi], mm0      // and store it
        loop  LoopYV12

        AllDoneYV12 :
    }

    // adjust for next line
    pSrc += src_pit;
    pDest += dst_pit;
  }


  return;
}

#endif






// This code was taken and slightly modified from decomb filter package by
// Donald Graft which has the following msg:
//
//      Borrowed from the author of IT.dll, whose name I
//      could not determine. Modified for YV12 by Donald Graft.
// example call DrawYV12(dst, 0, 0, buf);
// font chars are in 8 wide by 10 pixel high blocks.
// pass in dst_height and dst_width
// x1 and y1 specify bit position in frame to start printing the characters onto the frame.
// s is the string to print onto the frame.
void DrawYV12(unsigned char* dst, unsigned int dst_height, unsigned int dst_width, int x1, int y1, const char* s)
{
  int x, y = y1 * 20, num, tx, ty, xx;
  unsigned char* dpY;

  if (y + 20 >= (int)(dst_height)) return;
  for (xx = 0; *s; ++s, ++xx)
  {
    x = (x1 + xx) * 10;
    if (x + 10 >= (int)(dst_width)) return;
    num = *s - ' ';
    for (tx = 0; tx < 10; tx++)
    {
      for (ty = 0; ty < 20; ty++)
      {
        dpY = &dst[(x + tx) + (y + ty) * dst_width];
        if (font[num][ty] & (1 << (15 - tx))) *dpY = 255;
        else *dpY = (unsigned char)(*dpY >> 1);
      }
    }
  }
}

void Draw16x16Block(unsigned char* dst, unsigned int dst_height, unsigned int dst_width, int x1, int y1, const char val)
{
  int x, y = y1 * 20 + 4, tx, ty;

  if (y + 10 >= (int)(dst_height)) return;

  x = x1 * 10;
  if (x + 11 >= (int)(dst_width)) return;
  for (tx = 0; tx < 11; tx++) {
    for (ty = 0; ty < 11; ty++) {
      dst[(x + tx) + (y + ty) * dst_width] = val;
    }
  }
}







