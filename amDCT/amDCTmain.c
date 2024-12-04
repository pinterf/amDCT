

#include <math.h>
#include <stdio.h>

#include "amDCT.h"
#include "Memory.h"
#include "amDCTtypedefs.h"  // includes portab.h

#include "amFilters.h"

#include "DispatchLoop.h"
#include "FrameInfo.h"
#include "avgDctLoop.h"
#include "Adapt.h"
#include "Matrix.h"

#include "Utilities.h"
#include "Font.h"

#include <malloc.h>
#include "emmintrin.h"

#ifdef ARCH_IS_IA32
#define FREE_AND_RETURN_0            \
  free_data(&FrameInfoArgs);       \
  _aligned_free(pictSrcBufFull);   \
  _mm_empty(); \
  return(0);
#else
#define FREE_AND_RETURN_0            \
  free_data(&FrameInfoArgs);       \
  _aligned_free(pictSrcBufFull);   \
  return(0);
#endif


// THESE ARE GLOBAL DEBUGGING VARIABLES
//    THIS IS WHERE THE MEMORY IS CREATED AND INTIALIZED
int max_debug3 = 0;
int min_debug3 = 9999;


int max_debug4 = 0;
int min_debug4 = 9999;


int max_debug5 = 0;
int min_debug5 = 9999;


int max_debug6 = 0;
int min_debug6 = 9999;

int max_debug7 = 0;
int min_debug7 = 9999;

int T2Global = 0;

int MaxDCT0 = 0;
int MinDCT0 = 64000;

//int t2_GLOABAL = 0;


int   amDCTmain(
  const uint8_t* pExternSrc,
  //uint8_t *pf1,                // The previous motion compensated frame or is NULL
  //uint8_t *pb1,                // The previous motion compensated frame or is NULL
  uint8_t* pdst,
  unsigned int  src_height_a, // src includes the 8 bit boarder on each side.
  unsigned int  src_width_a,
  unsigned int  src_pitch_a,
  unsigned int  dst_height_a, // dst is the size of the output frame without any boarder.
  unsigned int  dst_width_a,
  unsigned int    dst_pitch_a,
  int        quant_a,
  int        adapt_a,
  int        shift_a,
  int        matrix_a,
  int        qtype_a,
  int          expand_a,
  int          sharpWPos_a,
  int          sharpWAmt_a,
  int        sharpTPos_a,
  int        sharpTAmt_a,
  int           quality_a,
  int        brightStart_a,
  int          brightAmt_a,
  int        darkStart_a,
  int          darkAmt_a,
  int          showMask_a,
  int          T2_a,
  int        ncpu_a) {



  // These args should all work to 31.  quant_a, adapt_a, expand_a, sharpWAmt_a, sharpTAmt_a, brightAmt_a, darkAmt_a



    // Convert arguments to internal size representation.
  uint16_t  src_height = (uint16_t)src_height_a;
  uint16_t  src_width = (uint16_t)src_width_a;
  uint16_t  src_pitch = (uint16_t)src_pitch_a;
  uint16_t  dst_height = (uint16_t)dst_height_a;
  uint16_t  dst_width = (uint16_t)dst_width_a;
  uint16_t  dst_pitch = (uint16_t)dst_pitch_a;
  uint8_t    quant = (uint8_t)quant_a;
  uint8_t    adapt = (uint8_t)adapt_a;
  uint8_t    shift = (uint8_t)shift_a;
  uint8_t    matrix = (uint8_t)matrix_a;
  uint8_t    qtype = (uint8_t)qtype_a;
  uint8_t    expand = (uint8_t)expand_a;
  uint8_t    sharpWPos = (uint8_t)sharpWPos_a;
  uint8_t    sharpWAmt = (uint8_t)sharpWAmt_a;
  uint8_t    sharpTPos = (uint8_t)sharpTPos_a;
  uint8_t    sharpTAmt = (uint8_t)sharpTAmt_a;
  uint8_t   quality = (uint8_t)quality_a;
  uint8_t    brightStart = (uint8_t)brightStart_a;
  uint8_t    brightAmt = (uint8_t)brightAmt_a;
  uint8_t    darkStart = (uint8_t)darkStart_a;
  uint8_t    darkAmt = (uint8_t)darkAmt_a;
  uint8_t    showMask = (uint8_t)showMask_a;
  uint8_t    T2 = (uint8_t)T2_a;
  uint8_t    ncpu = (uint8_t)ncpu_a;


  uint8_t* externSrcp = (uint8_t*)pExternSrc;
  //  uint8_t   *psrcP    = psrc;
  //  uint8_t   *pf1P      = pf1;
  //  uint8_t   *pb1P      = pb1;
  uint8_t* pdstP = pdst;

  uint8_t    use_adapt = adapt;
  uint8_t    FrameQuant = quant;
  uint8_t     doSharpFlag = 0;
  uint8_t     doSmoothFlag = 0;
  uint8_t     doExpandFlag = 0;
  uint8_t     doBrightFlag = 0;
  uint8_t     doDarkFlag = 0;

  uint8_t    boarder = 8;  // number of pixels to add on each side of the frame.
  uint16_t   height = src_height + 2 * boarder;
  uint16_t   width = src_width + 2 * boarder;
  uint32_t   frameSize = width * height * sizeof(uint8_t);  // frameSize includes the boarder.

  uint16_t   h, w;
  //  char       msgBuf[255];              // Debug text buffer when printing messages on the screen.

  if (frameSize < 256) frameSize = 256;    // This is only to keep Microsoft lint happy.

  // Check for errors that should never occur.
  if (src_height < 16 || src_width < 16) return(2);

  // Check that  width and height are evenly divisible by 8 since the DCT routines operate on 8x8 blocks of pixels.
  if (src_height_a % 8) return(3);
  if (src_width_a % 8) return(3);
  if (src_pitch_a % 8) return(3);
  if (dst_height_a % 8) return(3);
  if (dst_width_a % 8) return(3);
  if (dst_pitch_a % 8) return(3);


  // Bring out of spec arguments into normal range.  The calling program should have done this.
  if (quant > 31) quant = 31;
  if (adapt > 31) adapt = 31;
  if (shift > 5) shift = 5;  // shift = 5 means 64 overlapping passes, the maximum possible with an 8x8 block, through the DCT loop.  Each pass is shifted to a new overlapping position.
  if (matrix > 255) matrix = 255;
  if (qtype > 31) qtype = 31;
  if (expand > 31) expand = 31;
  if (quality > 6) quality = 6;
  if (brightStart > 255) brightStart = 255;
  if (brightAmt > 31) brightAmt = 31;
  if (darkStart > 255) darkStart = 255;
  if (darkAmt > 31) darkAmt = 31;
  if (ncpu > 4) ncpu = 4;
  if (T2 > 255) T2 = 255;
  if (showMask > 255) showMask = 255;

  T2Global = T2;  // GLOBAL DEBUGGING VARIABLE


  // FrameInfoArgs and msgBuf are created on the stack so their memory will automatically be freed when this filter returns.
  // FrameInfoArgs contains all of the info used during processing including pointers to all of the malloced memory.
  // To make sure the malloced memory is freed the FREE_AND_RETURN_0 macro, defined at the top of this file.
  FrameInfo_args FrameInfoArgs;          // We create FrameInfoArgs on the stack
  char           msgBuf[255];             // Debug text buffer when printing messages on the screen.
  MY_MEMSET(&FrameInfoArgs, 0, sizeof(FrameInfoArgs));
  MY_MEMSET(msgBuf, 0, sizeof(msgBuf));

  // To make sure the malloced memory is freed the FREE_AND_RETURN_0 macro, defined at the top of this file.

  //  if (quality > 4) quality = 4;  // UNTILL HIGHER QUALITIES COMPLEATED !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

  // Set defaults.
  if (shift == 0)                        ncpu = 1;  // Shift == 0 means only 1 pass through the DCT Loop
  if (quant >= 1)                        doSmoothFlag = 1;
  if (expand >= 1)                        doExpandFlag = 1;
  if (quality == 0)                        quality = 2;
  if (sharpWAmt >= 1 || sharpTAmt >= 1)            doSharpFlag = 1;
  if (brightAmt >= 1 && brightStart <= 254)          doBrightFlag = 1;  // THE START VALUES SHOULD BE REASONABLE 235? Possibly base upon the amounts?  WHAT IF WE ARE SMOOTHING MASKS????
  if (darkAmt >= 1 && darkStart >= 1 && doSmoothFlag == 1)  doDarkFlag = 1;  // Dark Protect from quant smoothing and bright smoothing  THE START VALUES SHOULD BE REASONABLE  32?
  //  if (doExpandFlag == 1 || doSharpFlag == 1)            doDarkFlag   = 0;  // Dark Protect is not needed if doing expansion or sharpening which tend to protect dark detail.



  //    sprintf_s(msgBuf, sizeof(msgBuf), "src_height=%5d  src_width=%5d", (int32_t)src_height_a, (int32_t)src_width_a);
  //    DrawYV12(pdst, dst_height_a, dst_pitch_a, 5, 5, msgBuf);
  //    DrawYV12(pdst, dst_height_a, dst_pitch_a, 5, 5, "TEST");
  //    return(0);





    // If there is no work to do copy the source frame to the destination frame and return.
  if (doSmoothFlag == 0 &&
    doExpandFlag == 0 &&
    doSharpFlag == 0 &&
    doBrightFlag == 0 &&
    doDarkFlag == 0 &&
    adapt == 0 &&
    quality < 6 &&    // quality > 5 pre-process the edges.    JUST OUTPUT THE FRAME WITH EDGES DONE   THIS SHOULD BE A MASK NUMBER
    showMask == 0) {

    for (h = 0; h < src_height; h++) {
      memcpy(pdst, externSrcp, src_width);
      pdst += dst_pitch;
      externSrcp += src_pitch;
    }

    return(0);   // No memory allocated yet, so just return;
  }

  if (brightStart >= 255) brightAmt = 0;
  if (brightAmt <= 0)   brightStart = 255;  // If we comment this out we can use this to provide hints for extra smoothing. This would need to be changed in amDCT.cpp as well.


  if (darkStart <= 0)   darkAmt = 0;
  if (darkAmt <= 0)   darkStart = 0;


  // -- !! The same parameter checking and conversion is done both in filter constructor and the actual main processing

  // Set the default sharpWPos and sharpTPos if only sharpWAmt or sharpTAmt is specified.
  if (sharpWPos == 255 && sharpTPos != 255) sharpWPos = sharpTPos;
  if (sharpTPos == 255 && sharpWPos != 255) sharpTPos = sharpWPos;

  // If neither sharpWPos or sharpTPos have been set then use their defaults.
  if (sharpWPos == 255 && sharpTPos == 255 && sharpWAmt != 255)  sharpWPos = 5;
  if (sharpWPos == 255 && sharpTPos == 255 && sharpTAmt != 255)  sharpTPos = 7;

  if (sharpWAmt == 255 && sharpTAmt != 255) sharpWAmt = sharpTAmt;
  if (sharpTAmt == 255 && sharpWAmt != 255) sharpTAmt = sharpWAmt;

  if ((sharpWAmt == 255 && sharpTAmt == 255) ||
    (sharpWPos == 255 && sharpTPos == 255)) {

    sharpWAmt = 0;
    sharpTAmt = 0;
    sharpWPos = 0;
    sharpTPos = 0;
  }

  if (sharpTPos == 255 && sharpWPos != 255) {
    sharpTPos = sharpWPos;
    sharpTAmt = sharpWAmt;
  }

  if (sharpTPos != 255 && sharpWPos == 255) {
    sharpWPos = sharpTPos;
    sharpWAmt = sharpTAmt;
  }
  // --- !! End of similarity


  // Currently sharpWAmt and sharpTAmt need to be <= 30.     BUG SHOULD WORK AT <= 31  !!!!!!
  if (sharpWAmt >= 31) sharpWAmt = 30;
  if (sharpTAmt >= 31) sharpTAmt = 30;




  // NOTE NEED TO RECHECK THESE !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // Workarounds for limitations in the quant dequant routines.
  if ((qtype == 1 || qtype == 11) && quant == 31) quant = 30;
  if (qtype == 3 && ncpu > 1) ncpu = 1;  // Workaround for bug in quant_mpeg_inter_mmx() and dequant_mpeg_inter_mmx() routines.
  if (adapt_a == 31) { adapt = 30; adapt_a = 30; }



  // In order to reduce edge of frame artifacts we copy the src frame into the center of a frame that
  // has an 8 pixel wide boarder on all 4 sides.  That boarder is filled with the mirror of the 8 pixel wide edge of the original frame.

  // malloc a buffer that will contain the source frame we will actually use
  uint8_t* pictSrcBufFull = (uint8_t*)_aligned_malloc(frameSize, ALIGN_16);
  if (pictSrcBufFull == 0) // If out of memory return failure.
    return(1);

  //MY_MEMSET(pictSrcBufFull, 0, frameSize);
  MY_MEMSET(pictSrcBufFull, 128, frameSize);

  uint8_t* pictSrcBuf = pictSrcBufFull;
  uint8_t* pictSrc = pictSrcBuf;
  uint8_t* psrc = pictSrcBuf;



  /*    //  This is experimental code for using extra Motion Compensated frames which would be
      //  summed at the output of the DCT loop and then sent to the averaging routines.
      //  Probably need a GPU version because it gets really slow!!!
      uint8_t       *pf1Buf  = NULL;
      uint8_t       *pf1Bufp = NULL;
      uint8_t       *ppf1    = NULL;

      uint8_t       *bf1Buf  = NULL;
      uint8_t       *bf1Bufp = NULL;
      uint8_t       *pbf1    = NULL;
      if (pf1 != NULL) {
        pf1Buf  = (uint8_t *)_aligned_malloc(frameSize, ALIGN);
        pf1Bufp = pf1Buf;
        ppf1    = pf1Bufp;
        memset(pf1Buf,  0, frameSize);
      }

      if (bf1 != NULL) {
        bf1Buf  = (uint8_t *)_aligned_malloc(frameSize, ALIGN);
        bf1Bufp = bf1Buf;
        pbf1    = bf1Bufp;
        memset(bf1Buf,  0, frameSize);
      }
  */




  // First we deal with the Y Plane
  externSrcp = (uint8_t*)pExternSrc;
  psrc = pictSrc + width * boarder + boarder;  // Center the source frame in the bordered frame which starts 8 down (width*boarder) and 8 in from left side.
  for (h = 0; h < src_height; h++) {           // Loop from top line to bottom line (Sames as YUY2.
    memcpy(psrc, externSrcp, src_width);
    externSrcp += src_pitch;         // Add the src pitch of the line (in bytes) to the source image.
    psrc += src_width + boarder * 2;
  }



  // Now that the src is centered in the work buffer
  // We need to mirror 8 pixel in from the edge of the source
  // to the boarder pixels in the work buffer.
  // We do each side individually.

/*
  // THIS CODE IF DOING PREVIOUS AND NEXT FRAMES TEST IN FUTURE.
  // mirror top
  for (h=8; h < 16; h++) {
    for (w = 8; w < width-8; w++){
      int offset1 = h*width + w;
      int offset2 = (15-h)*width + w;
      pictSrcBuf[offset2] = pictSrcBuf[offset1];
//          if (pf1 != NULL) pf1Buf[offset2] = pf1Buf[offset1];
//          if (bf1 != NULL) bf1Buf[offset2] = bf1Buf[offset1];
    }
  }
*/

// mirror top
//int offset1 = 0; //h*width + w;
//int offset2 = 0; //(15-h)*width + w;
  for (h = 8; h < 16; h++) {
    int offset1 = h * width;
    int offset2 = (15 - h) * width;
    for (w = 8; w < width - 8; w++) {
      offset1++;
      offset2++;
      pictSrcBuf[offset2] = pictSrcBuf[offset1];
      //          if (pf1 != NULL) pf1Buf[offset2] = pf1Buf[offset1];
      //          if (bf1 != NULL) bf1Buf[offset2] = bf1Buf[offset1];
    }
  }

  // mirror bottom
  int hstart = height - 8;
  int d = hstart - 1;
  int u = hstart;
  for (int i = 0; i < 8; i++) {
    int offset1 = d * width + 8; //+ w;
    int offset2 = u * width + 8; // + w;
    for (w = 8; w < width - 8; w++) {
      offset1++;
      offset2++;
      pictSrcBuf[offset2] = pictSrcBuf[offset1];
    }
    d--;
    u++;
  }


  // mirror left
  for (h = 0; h < height; h++) {
    for (w = 8; w < 16; w++) {
      int offset1 = h * width + w;
      int offset2 = h * width + 15 - w;
      pictSrcBuf[offset2] = pictSrcBuf[offset1];
    }
  }


  // mirror right
  int wstart = width - 8;
  int l = wstart - 1;
  int r = wstart;
  for (int i = 0; i < 8; i++) {
    for (h = 0; h < height; h++) {
      int offset1 = h * width + l;
      int offset2 = h * width + r;
      pictSrcBuf[offset2] = pictSrcBuf[offset1];
    }
    l--;
    r++;
  }



  // Next smooth the edge pixels
  // smooth top
  for (h = 7; h > 1; h--) {
    for (w = 1; w < width - 1; w++) {
      int offset1 = h * width + w;
      pictSrcBuf[offset1] = (pictSrcBuf[offset1 - 1] + pictSrcBuf[offset1] + pictSrcBuf[offset1 + 1] + pictSrcBuf[offset1 - width]) >> 2;
    }
  }

  // smooth bottom
  for (h = height - 7; h < height - 1; h++) {
    for (w = 1; w < width - 1; w++) {
      int offset1 = h * width + w;
      pictSrcBuf[offset1] = (pictSrcBuf[offset1 - 1] + pictSrcBuf[offset1] + pictSrcBuf[offset1 + 1] + pictSrcBuf[offset1 + width]) >> 2;
    }
  }

  // smooth left
  for (h = 1; h < height - 1; h++) {
    for (w = 7; w > 1; w--) {
      int offset1 = h * width + w;
      pictSrcBuf[offset1 + 1] = (pictSrcBuf[offset1] + pictSrcBuf[offset1 + 1] + pictSrcBuf[offset1 + 1] + pictSrcBuf[offset1 + 1]) >> 2;
    }
  }


  // smooth right
  for (h = 1; h < height - 1; h++) {
    for (w = width - 7; w < width - 1; w++) {
      int offset1 = h * width + w;
      pictSrcBuf[offset1 - 1] = (pictSrcBuf[offset1] + pictSrcBuf[offset1 - 1] + pictSrcBuf[offset1 - 1] + pictSrcBuf[offset1 - 1]) >> 2;
    }
  }


  // FOR FUTURE IF DOING PREVEOUS AND NEXT FRAMES
  // The buffers for any optional clips have already been filled with 0.
  // If the optional clip was supplied then copy the clip data to the work buffer.
//    if (pf1 != NULL) {
//      ppf1 = pf1Bufp + width*boarder + boarder;   // copy to center which is 8 down (width*boarder) and 8 in from side (+ boarder)
//          for (h=0; h < src_height; h++) {           // Loop from top line to bottom line (Same as YUY2).
//        memcpy(ppf1, pf1Srcp, src_width);
//            pf1Srcp += pf1_pitch;
//            ppf1    += src_width + boarder*2;
//          }
//        }




  // NOTE  create_frame_info() in "FrameInfo.c" will call malloc_data() in "Memory.c"  to allocate all of the memory used by amDCT
  //        If malloc_data() fails any frames tables work buffers etc. will have been freed before it returns.

  if (create_frame_info(&FrameInfoArgs, pictSrc, pdstP, quant, adapt, width, src_pitch, height, dst_width, dst_pitch, dst_height, brightStart, brightAmt, darkStart, darkAmt, doSmoothFlag, doSharpFlag, doExpandFlag, doBrightFlag, doDarkFlag, ncpu, quality, T2) > 0) {
    //    sprintf_s(msgBuf, sizeof(msgBuf), "Out Of Memory in amDCT  amDCTmain.c");
    //    DrawYV12(pdst, dst_height, dst_width, 0, 1, msgBuf);

    _aligned_free(pictSrcBufFull);
#ifdef ARCH_IS_IA32
    _mm_empty();
#endif
    return(1); // Out of Memory
  }


  //    sprintf_s(msgBuf, sizeof(msgBuf), "FrameInfoArgs->sizeY=%3d", (int32_t)frameSize);
  //    DrawYV12(pdst, FrameInfoArgs.dst_height, FrameInfoArgs.dst_width, 1, 1, msgBuf);
  //  FREE_AND_RETURN_0;  // defined at top of this file.


    //   Load user args into newly malloced FrameInfoArgs.
    //  These args are needed for any DCT loop processing.
    //  The args may change while processing a frame depending
    //  upon what DCT processing is being done.
  doFrameInfo(FrameQuant,
    quant,
    qtype,
    adapt,
    shift,
    matrix,

    expand,
    sharpWPos,
    sharpWAmt,
    sharpTPos,
    sharpTAmt,

    &FrameInfoArgs,

    showMask,
    T2);


  if (showMask == SHOW_REF_SRC) {   // Basic system interface check.  Just return the unmodified source.
    copySrcToDst(&FrameInfoArgs);
    FREE_AND_RETURN_0;            // Defined at top of this file.
  }




  if (doAdapt(&FrameInfoArgs) != 0) {  // If doAdapt() returns 1 it has written debugging info onto the pdst frame.
    FREE_AND_RETURN_0;  // Defined at top of this file.
  }


  if (showMask == SHOW_ADAPT_SRC) {
    copyFrameToDst(&FrameInfoArgs, showMask);
    FREE_AND_RETURN_0;  // Defined at top of this file.
  }


  //
  //    sprintf_s(msgBuf, sizeof(msgBuf), "FrameInfoArgs->sizeY=%3d", (int32_t)frameSize);
  //    DrawYV12(pdst, FrameInfoArgs.dst_height, FrameInfoArgs.dst_width, 1, 1, msgBuf);
  //  FREE_AND_RETURN_0;  // Defined at top of this file.


    // NOTES:
    //
    // psrc is the src         frame passed into amDCT()  It is renamed to RefSrc and should not be modified.
    // pdst is the destination frame passed into amDCT()  It is the output frame. It is smaller, no frame boarder, than psrc.  It is modified just before amDCT() returns.

    // pworkSrc is the frame that is operated upon by various routines.
    // padapt   is a copy of psrc after it has been operated upon by adapt(), doBrightSmoothed() and preProcessEdges(). It is not changed after that.


    // LumaFrameInfo() is needed by LumaBlockInfo() to do expand per-block adapt.
    //  if (quality >= 3) LumaFrameInfo(FrameInfoArgs);




    // quant specifies the minimum amount of smoothing that will be done on the frame.
    // FrameQuant, computed below, specifies the actual amount of smoothing that will be done on the frame.
    //
    // If adapt is larger than quant then the minimum amount of smoothing for the frame
    // will be increased up to the value of adapt.  The amount of increase is
    // determined by the overall blockiness of the frame which is computed by AdaptQuant().
    //
    // AdaptQuant() returns a value between 0-31
    // AdaptQuant() determines an estimate of the overall blocking of the frame by looking
    // at how much change the blindPP code makes to the frame and, if quality >= 3, the per-block blockiness
    // returned by LumaBlockInfo().
    //
    // If ANY level of adaptation is specified then the image will retain the very mild deblocking, equivalent to adapt=3,
    // done by the blindPP code.
  if (FrameInfoArgs.adapt > 0) {

    if (quality > 3) {
      //    if (quality >= 3) {  // Test if this quality == 3 causes the problem !!!!!!!!!
      buildMasks(&FrameInfoArgs);  // doAdapt() uses info from LumaBlockInfo() to help determine overall frame blockiness.
    }
    if (doAdapt(&FrameInfoArgs) != 0) {  // If doAdapt() returns 1 it has written debugging info onto the pdst frame.
      FREE_AND_RETURN_0;  // Defined at top of this file.
    }

    // Fast exit to user if they ONLY want to do adaptive deblock.
    if (showMask == SHOW_ADAPT_SRC || (doSmoothFlag == 0 &&
      doExpandFlag == 0 &&
      doSharpFlag == 0 &&
      doBrightFlag == 0 &&
      showMask == 0 &&
      quality < 5)) {   // if quality >= 5 we need to do preProcessEdges() before we return.  /// THIS NEEDS TO BE TESTED !!!!!!!!!!!!!!!!!!!!!!!

      copySrcToDst(&FrameInfoArgs);
      //goto test2;
      FREE_AND_RETURN_0;  // Defined at top of this file.
    }
    //}
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
    //      FREE_AND_RETURN_0;  // Defined at top of this file.
    ////      return(0);
    //    }


    //    MY_MEMCPY(&FrameInfoArgs.MemoryArgs->DctLoopArgs[0].BF_srcP, psrc, FrameInfoArgs.sizeY);
    //    MY_MEMCPY(&FrameInfoArgs.psrc,                               psrc, FrameInfoArgs.sizeY);  // The copy from psrc is the adapt frame.
    //    MY_MEMCPY(&FrameInfoArgs.MemoryArgs->frame_workSource,       psrc, FrameInfoArgs.sizeY);
    //
    //    copySrcToDst(&FrameInfoArgs);
    //    FREE_AND_RETURN_0;  // Defined at top of this file.
    //    }

    use_adapt = FrameInfoArgs.adapt;
    FrameQuant = FrameInfoArgs.quant;

    if (quant > 0 && quant < FrameInfoArgs.FrameQuant) {
      FrameQuant = FrameInfoArgs.FrameQuant;
    }

    if (quant_a == 0) {
      FrameQuant = 0;
      quant = 0;
    }
  }  //  END  adapt > 0

//FrameInfoArgs->sizeY
//    sprintf_s(msgBuf, sizeof(msgBuf), "FrameInfoArgs->sizeY=%3d", (int32_t)FrameInfoArgs.sizeY);
//    sprintf_s(msgBuf, sizeof(msgBuf), "FrameInfoArgs->sizeY=%3d", (int32_t)frameSize);
//    DrawYV12(pdst, FrameInfoArgs.dst_height, FrameInfoArgs.dst_width, 1, 1, msgBuf);
//    DrawYV12(pdst, FrameInfoArgs.dst_height, FrameInfoArgs.dst_width, 1, 1, msgBuf);

//frameSize
  //  The user just wants the very smoothed frame.
//  if (FrameInfoArgs.showMask == SHOW_SMOOTHED) {
  if (showMask == SHOW_SMOOTHED) {
    buildMasks(&FrameInfoArgs);
    copyFrameToDst(&FrameInfoArgs, showMask);
    //goto test2;
    //    FREE_AND_RETURN_0;
  }

  //  if (showMask == SHOW_SMOOTHED) {
  //    buildMasks(&FrameInfoArgs);
  //    copyFrameToDst(&FrameInfoArgs, showMask);
  //    FREE_AND_RETURN_0;
  //  }



  //      Debug adaptWeight
  //    sprintf_s(msgBuf, sizeof(msgBuf), "adaptWeight=%4d", FrameInfoArgs.adaptWeight);
  //    DrawYV12(pdst, dst_height, dst_width, 1, 3, msgBuf);
  //    FREE_AND_RETURN_0;  // Defined at top of this file.




    // NOTE: showMask == 18 just does the DCT and skips the quant-dequant as well as the IDCT
    //       showMask == 19 does the quant-dequant processing, skips the IDCT processing, and then returns.
    //
    // Show what the DCT transformed image looks like.
    // The only user arguments that has an effect are
    //   adapt which will have done it's work above.
    //   and shift which is used here.
    //   Normally shift will be 0.
  if (showMask == 18 || showMask == 19) {
    uint8_t use_shift_arg = shift;
    uint8_t use_qtype_arg = 1;
    uint8_t use_expand_arg = 0;
    uint8_t use_matrix_arg = MATRIX_FLAT3;  // All matrix values for MATRIX_FLAT3 are 3.
    uint8_t use_sharpWAmt_arg = 0;
    uint8_t use_sharpTAmt_arg = 0;
    uint8_t use_quant_arg = 1;

    doFrameInfo(FrameQuant,
      use_quant_arg,
      use_qtype_arg,
      use_adapt,
      use_shift_arg,
      use_matrix_arg,

      use_expand_arg,
      sharpWPos,
      use_sharpWAmt_arg,
      sharpTPos,
      use_sharpTAmt_arg,

      &FrameInfoArgs,

      showMask,
      T2);


    set_matrix(&FrameInfoArgs, use_matrix_arg, use_qtype_arg, use_quant_arg, use_expand_arg);

    dup_cpu_data(&FrameInfoArgs);
    DispatchLoop(&FrameInfoArgs);

    // We don't want the DCT transformed image to have any bright or dark post processing done on it.
    FrameInfoArgs.doDarkFlag = 0;
    FrameInfoArgs.darkStart = 0;
    FrameInfoArgs.darkAmt = 0;

    FrameInfoArgs.doBrightFlag = 0;
    FrameInfoArgs.brightStart = 255;
    FrameInfoArgs.brightAmt = 0;

    avgDctLoopAccum(&FrameInfoArgs);

    // The value in dct[0] represents the average brightness of a block being processed.
    // The following 2 lines are used to find the max and min dct[0] values.
    // This allows dct[0] to be calibrated to the range 0 to 255.

    //sprintf_s(msgBuf, sizeof(msgBuf), "MaxDCT0=%5d MinDCT0=%5d", (int32_t)MaxDCT0, (int32_t)MinDCT0);
    //DrawYV12(pdst, FrameInfoArgs.dst_height, FrameInfoArgs.dst_width, 0, 0, msgBuf);

    copySrcToDst(&FrameInfoArgs);
    FREE_AND_RETURN_0;  // Defined at top of this file.
  }


  // NOT WORKING YET !!!!!
  //   PRE PROCESS THE EDGES
  //   Reduces halos and edge noise.
  //   This should be done after any smoothing of the bright noisy parts is done.
  if (quality >= 6) {
    buildMasks(&FrameInfoArgs);
    //    preProcessEdges(&FrameInfoArgs);

        // Fast exit to user if they just want the mask.
        // If there is no more work to do copy the source frame to to the destination frame and return.
    if ((doSmoothFlag == 0 &&
      doExpandFlag == 0 &&
      doSharpFlag == 0 &&
      doBrightFlag == 0 &&
      doDarkFlag == 0 &&
      adapt == 0) ||
      showMask == 22) {

      if (showMask == 0) showMask = 22;
      copyFrameToDst(&FrameInfoArgs, showMask);

      FREE_AND_RETURN_0;  // Defined at top of this file.
    }
  }



  // Now do the main processing.
  amFilters(&FrameInfoArgs);

  FREE_AND_RETURN_0;  // Defined at top of this file.




  /////////////////////////////////////////////////////////////////////////////////
  //
  // THE FOLLOWING CODE WILL PRINT DEBBUGING INFORMATION ON THE PROCESED FRAME  pdst
  //
  // To enable the debug code put a    goto test2;
  // before one of the                FREE_AND_RETURN_0;
  // statements above
  //
  // Recompile and run the filter as usual.


  //test2:

  // DEBUGGING INTERNAL INFORMATION    PRINTS INFO ON THE OUTPUT FRAME

  //static int max_debug3=0;
  //static int min_debug3=9999;
  //  these are global static variables used to collect info across many frames
  //static int max_debug7=0;
  //static int min_debug7=9999;



  //sprintf_s(msgBuf, sizeof(msgBuf), "frame_SmoothMax=%3d  frame_SmoothMin=%3d", (int32_t)FrameInfoArgs.frame_SmoothMax, (int32_t)FrameInfoArgs.frame_SmoothMin);
  //DrawYV12(pdst, FrameInfoArgs.dst_height, FrameInfoArgs.dst_width, 1, 2, msgBuf);

  /*
  sprintf_s(msgBuf, sizeof(msgBuf), "SMmax  =%4d SMmin  =%4d", (int32_t)FrameInfoArgs.frame_SmoothMax, (int32_t)FrameInfoArgs.frame_SmoothMin);
  DrawYV12(pdst, FrameInfoArgs.dst_height, FrameInfoArgs.dst_width, 0, 1, msgBuf);

  sprintf_s(msgBuf, sizeof(msgBuf), "SrcMax =%4d SrcMin =%4d", (int32_t)FrameInfoArgs.frame_Max,       (int32_t)FrameInfoArgs.frame_Min);
  DrawYV12(pdst, FrameInfoArgs.dst_height, FrameInfoArgs.dst_width, 0, 2, msgBuf);




  sprintf_s(msgBuf, sizeof(msgBuf), "minSmoothCutoff    =%5d", (int32_t)FrameInfoArgs.minSmoothCutoff);
  DrawYV12(pdst, FrameInfoArgs.dst_height, FrameInfoArgs.dst_width, 0, 3, msgBuf);

  sprintf_s(msgBuf, sizeof(msgBuf), "maxNumBrightPix    =%5d", (int32_t)FrameInfoArgs.maxNumBrightPix);
  DrawYV12(pdst, FrameInfoArgs.dst_height, FrameInfoArgs.dst_width, 0, 4, msgBuf);

  sprintf_s(msgBuf, sizeof(msgBuf), "minMaxSmooth       =%5d", (int32_t)FrameInfoArgs.minMaxSmooth);
  DrawYV12(pdst, FrameInfoArgs.dst_height, FrameInfoArgs.dst_width, 0, 5, msgBuf);

  sprintf_s(msgBuf, sizeof(msgBuf), "frameSmoothMax     =%5d", (int32_t)FrameInfoArgs.frame_SmoothMax);
  DrawYV12(pdst, FrameInfoArgs.dst_height, FrameInfoArgs.dst_width, 0, 6, msgBuf);

  sprintf_s(msgBuf, sizeof(msgBuf), "frame_SmoothAvgLuma=%5d", (int32_t)FrameInfoArgs.frame_SmoothAvgLuma);
  DrawYV12(pdst, FrameInfoArgs.dst_height, FrameInfoArgs.dst_width, 0, 7, msgBuf);

  sprintf_s(msgBuf, sizeof(msgBuf), "frame_SrcAvgLuma   =%5d", (int32_t)FrameInfoArgs.frame_Avg);
  DrawYV12(pdst, FrameInfoArgs.dst_height, FrameInfoArgs.dst_width, 0, 8, msgBuf);

  sprintf_s(msgBuf, sizeof(msgBuf), "totalBrightSmooth  =%5d", (int32_t)FrameInfoArgs.totalBrightSmooth);
  DrawYV12(pdst, FrameInfoArgs.dst_height, FrameInfoArgs.dst_width, 0, 9, msgBuf);

  sprintf_s(msgBuf, sizeof(msgBuf), "totalBrightSrc     =%5d", (int32_t)FrameInfoArgs.totalBrightSrc);
  DrawYV12(pdst, FrameInfoArgs.dst_height, FrameInfoArgs.dst_width, 0, 10, msgBuf);

  sprintf_s(msgBuf, sizeof(msgBuf), "totalBrightSmooth  =%5d", (int32_t)FrameInfoArgs.frame_avgBrightSmooth);
  DrawYV12(pdst, FrameInfoArgs.dst_height, FrameInfoArgs.dst_width, 0, 11, msgBuf);

  sprintf_s(msgBuf, sizeof(msgBuf), "totalBrightSrc     =%5d", (int32_t)FrameInfoArgs.frame_avgBrightSrc);
  DrawYV12(pdst, FrameInfoArgs.dst_height, FrameInfoArgs.dst_width, 0, 12, msgBuf);
  */

  //  FrameInfoArgs->frame_SmoothMax      = maxSmooth;
  //  FrameInfoArgs->frame_SmoothMin      = minSmooth;
  //  FrameInfoArgs->frame_Max            = maxSrc;
  //  FrameInfoArgs->frame_Min            = minSrc;
  //  FrameInfoArgs->frame_SmoothAvgLuma = avgSmooth;
  //  FrameInfoArgs->frame_Avg           = avgSrc;
  //  FrameInfoArgs->totalBrightSrc      = totalBrightSrc;
  //  FrameInfoArgs->totalBrightSmooth   = totalBrightSmooth;

  //  FREE_AND_RETURN_0;
}
//     DrawYV12(              pdst, FrameInfoArgs.dst_height, FrameInfoArgs.dst_width,      1,      2,             msgBuf);
//void DrawYV12(unsigned char *dst,  unsigned int dst_height,  unsigned int dst_width, int x1, int y1, const char *s)



//    sprintf_s(msgBuf, sizeof(msgBuf), "dst_height=%4d  dst_width=%4d", (int32_t)dst_height_a, (int32_t)dst_width_a);
//    DrawYV12(pdst, dst_height, dst_width, 1, 2, msgBuf);


//    sprintf_s(msgBuf, sizeof(msgBuf), "MAX RS_Kurtosis=%3d", (int32_t)max_debug4);
//    DrawYV12(pdst, FrameInfoArgs.dst_height, FrameInfoArgs.dst_width, 1, 1, msgBuf);
//
//    sprintf_s(msgBuf, sizeof(msgBuf), "MIN RS_Kurtosis=%3d", (int32_t)min_debug4);
//    DrawYV12(pdst, dst_height, dst_width, 1, 2, msgBuf);
//
//    sprintf_s(msgBuf, sizeof(msgBuf), "MAX RS_Skewness=%3d", (int32_t)max_debug5);
//    DrawYV12(pdst, FrameInfoArgs.dst_height, FrameInfoArgs.dst_width, 1, 3, msgBuf);
//
//    sprintf_s(msgBuf, sizeof(msgBuf), "MAX RS_Skewness=%3d", (int32_t)min_debug5);
//    DrawYV12(pdst, dst_height, dst_width, 1, 4, msgBuf);
//
//sprintf_s(msgBuf, sizeof(msgBuf), "percentBlksUsed=%3d", (int32_t)FrameInfoArgs.percentBlksUsed); //, (int32_t)FrameInfoArgs.frame_SmoothMin);
//DrawYV12(pdst, FrameInfoArgs.dst_height, FrameInfoArgs.dst_width, 2, 6, msgBuf);
//
//sprintf_s(msgBuf, sizeof(msgBuf), "usedBlks=%3d", (int32_t)FrameInfoArgs.usedBlks); //, (int32_t)FrameInfoArgs.frame_SmoothMin);
//DrawYV12(pdst, FrameInfoArgs.dst_height, FrameInfoArgs.dst_width, 2, 8, msgBuf);
//
//sprintf_s(msgBuf, sizeof(msgBuf), "usedBlks=%3d", (int32_t)FrameInfoArgs.MemoryArgs->usedBlks); //, (int32_t)FrameInfoArgs.frame_SmoothMin);
//DrawYV12(pdst, FrameInfoArgs.dst_height, FrameInfoArgs.dst_width, 2, 9, msgBuf);
//
//sprintf_s(msgBuf, sizeof(msgBuf), "frameAvgDetE=%3d", (int32_t)FrameInfoArgs.frameAvgDetE); //, (int32_t)FrameInfoArgs.frame_SmoothMin);
//DrawYV12(pdst, FrameInfoArgs.dst_height, FrameInfoArgs.dst_width, 2, 10, msgBuf);





//sprintf_s(msgBuf, sizeof(msgBuf), "pctBri= %3d SMmin =%5d", (int32_t)FrameInfoArgs.percentBright, (int32_t)FrameInfoArgs.frame_SmoothMin);
//DrawYV12(pdst, FrameInfoArgs.dst_height, FrameInfoArgs.dst_width, 0, 0, msgBuf);

//sprintf_s(msgBuf, sizeof(msgBuf), " SMmax=%4d SMmin =%5d", (int32_t)FrameInfoArgs.frame_SmoothMax, (int32_t)FrameInfoArgs.frame_SmoothMin);
//DrawYV12(pdst, FrameInfoArgs.dst_height, FrameInfoArgs.dst_width, 0, 1, msgBuf);
//sprintf_s(msgBuf, sizeof(msgBuf), "SrcMax =%3d SrcMin=%5d", (int32_t)FrameInfoArgs.frame_Max, (int32_t)FrameInfoArgs.frame_Min);
//DrawYV12(pdst, FrameInfoArgs.dst_height, FrameInfoArgs.dst_width, 0, 2, msgBuf);
//
//sprintf_s(msgBuf, sizeof(msgBuf), "minSmoothCutoff   =%5d", (int32_t)FrameInfoArgs.minSmoothCutoff);
//DrawYV12(pdst, FrameInfoArgs.dst_height, FrameInfoArgs.dst_width, 0, 3, msgBuf);
//
//sprintf_s(msgBuf, sizeof(msgBuf), "maxNumBrightPix   =%5d", (int32_t)FrameInfoArgs.maxNumBrightPix);
//DrawYV12(pdst, FrameInfoArgs.dst_height, FrameInfoArgs.dst_width, 0, 4, msgBuf);
//
//sprintf_s(msgBuf, sizeof(msgBuf), "minMaxSmooth      =%5d", (int32_t)FrameInfoArgs.minMaxSmooth);
//DrawYV12(pdst, FrameInfoArgs.dst_height, FrameInfoArgs.dst_width, 0, 5, msgBuf);

//sprintf_s(msgBuf, sizeof(msgBuf), "frameSmoothMax    =%5d", (int32_t)FrameInfoArgs.frame_SmoothMax);
//DrawYV12(pdst, FrameInfoArgs.dst_height, FrameInfoArgs.dst_width, 0, 6, msgBuf);
//
//sprintf_s(msgBuf, sizeof(msgBuf), "maxSmooth         =%5d", (int32_t)FrameInfoArgs.maxSmooth);
//DrawYV12(pdst, FrameInfoArgs.dst_height, FrameInfoArgs.dst_width, 0, 7, msgBuf);
//sprintf_s(msgBuf, sizeof(msgBuf), "detDifEC2Max=%3d  detDifEC2Min=%3d", (int32_t)FrameInfoArgs.detDifEC2Max, (int32_t)FrameInfoArgs.detDifEC2Min);
//DrawYV12(pdst, FrameInfoArgs.dst_height, FrameInfoArgs.dst_width, 0, 0, msgBuf);
//
//sprintf_s(msgBuf, sizeof(msgBuf), "minMaxSmooth=%5d", (int32_t)FrameInfoArgs.minMaxSmooth); //, (int32_t)FrameInfoArgs.frame_SmoothMax);
//DrawYV12(pdst, FrameInfoArgs.dst_height, FrameInfoArgs.dst_width, 0, 1, msgBuf);



//  FREE_AND_RETURN_0;  // Defined at top of this file.



