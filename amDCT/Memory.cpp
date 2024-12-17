#include "avs/config.h"
#if (defined(GCC) || defined(CLANG)) && !defined(_WIN32)
#include <stdlib.h>
#define _aligned_malloc(size, alignment) aligned_alloc(alignment, size)
#define _aligned_free(ptr) free(ptr)
#endif

#include "Memory.h"
#include "amDCT.h"      // needed for BLKSIZE
#include "amDCTtypedefs.h"

//  Changing INIT_FRAME_VALUE is a good QA test since there should be no diftference.
#define INIT_FRAME_VALUE 16     // All pixels in a frame are initialized to this value when malloc'ed.
//#define INIT_FRAME_VALUE 128  // All pixels in a frame are initialized to this value when malloc'ed.

// A matrix has 64 entries and the xvid routines may need up to 8 copies of it each one modified for xvids use.
// #define MATRIX_SIZE sizeof(uint16_t) * 64 * 8 This is defined in amDCTtypedefs.h

// There are 5 groups of data that need to be alloced and freed
// Frames
// Arrays
// Multi threaded data
// Block data
// Matrix data

int  malloc_Frames(Memory_args* args, int frameSize);
void   free_Frames(Memory_args* args);

int  malloc_Arrays(Memory_args* args);
void   free_Arrays(Memory_args* args);

int  malloc_MultiThread(Memory_args* args, int numBlocksWork, int blocksSize, int ncpu);
void   free_MultiThread(Memory_args* args, int ncpu);

int  malloc_blocksFrame(Memory_args* args, int blocksSize, int ncpu);
void   free_blocksFrame(Memory_args* args, int ncpu);

int  malloc_matrix_data(Memory_args* args, int ncpu);
void   free_matrix_data(Memory_args* args, int ncpu);



////  uint8_t  *frame_ImpulseNoise   = FrameInfoArgs->frame_ImpulseNoise;
//    uint8_t  *frame_3x3mean        = FrameInfoArgs->frame_3x3mean;
////  uint8_t  *frame_3x3sadm        = FrameInfoArgs->frame_3x3sadm;
//    uint8_t  *frame_boundaries     = FrameInfoArgs->frame_boundaries;
//    uint8_t  *frame_grainMask      = FrameInfoArgs->frame_grainMask;
////  uint8_t  *frame_localMax9x9    = FrameInfoArgs->frame_localMax9x9;
////  uint8_t  *frame_localMin9x9    = FrameInfoArgs->frame_localMin9x9;

//  frameSize;      The size of the src frame, including the 8 bit boarder on each side.
//  numBlocksY      The total number of 8x8 blocks in the source frame. It is equal to numBlocks_high*numBlocks_wide.
//  sizeBlocksWork  The size of the src frame, including a 16 bit boarder on each side. Size of work buffers in DctLoop.c
//  numBlocksWork   The total number of blocks in the work buffer.
int
malloc_data(FrameInfo_args* FrameInfoArgs) {
  int   frameSize = FrameInfoArgs->sizeY;          // The size of the frame in pixels.
  int   sizeBlocksWork = FrameInfoArgs->sizeBlocksWork; // The size of the work buffer in pixels.
  int   numBlocksWork = FrameInfoArgs->numBlocksWork;  // The size of the work buffer in blocks.
  int   LumaBlockInfo_Size = numBlocksWork * sizeof(LumaPerBlock_args);


  Memory_args* args = (Memory_args*)_aligned_malloc(sizeof(Memory_args), ALIGN_16);
  if (args == 0) return(1);
  MY_MEMSET(args, 0, sizeof(Memory_args));
  FrameInfoArgs->MemoryArgs = args;


  if (malloc_Frames(args, frameSize)) {
    _aligned_free(args);
    return(1);
  }

  if (malloc_Arrays(args)) {
    free_Frames(args);
    _aligned_free(args);
    return(1);
  }

  // LumaPerBlockArgs
  args->LumaPerBlockArgs = (LumaPerBlock_args*)_aligned_malloc(LumaBlockInfo_Size, ALIGN_16);
  if (!args->LumaPerBlockArgs) {
    free_Arrays(args);
    free_Frames(args);
    _aligned_free(args);
    return(1);
  }
  MY_MEMSET(args->LumaPerBlockArgs, 0, LumaBlockInfo_Size);

  if (malloc_MultiThread(args, numBlocksWork, sizeBlocksWork, FrameInfoArgs->ncpu)) {
    _aligned_free(args->LumaPerBlockArgs);
    free_Arrays(args);
    free_Frames(args);
    _aligned_free(args);
    return(1);
  }

  //  if (malloc_blocksFrame(args, sizeBlocksWork, ncpu) {
  //    free_MultiThread(args);
  //    _aligned_free(args->LumaPerBlockArgs);
  //    free_Arrays(args);
  //    free_Frames(args);
  //    _aligned_free(args);
  //    return(1);
  //  }
  //  if (malloc_matrix_data(args, sizeBlocksWork, ncpu) {
  //    free_blocksFrame(args);
  //    free_MultiThread(args);
  //    _aligned_free(args->LumaPerBlockArgs);
  //    free_Arrays(args);
  //    free_Frames(args);
  //    _aligned_free(args);
  //    return(1);
  //  }

  return(0);
}

// //////////////////
//   MALLOC THE VARIOUS FRAMES
//
int
malloc_Frames(Memory_args* args, int frameSize) {
  // These 12 buffers are only used outside of the multi threading code so we only need 1 copy.

  //  FRAME_3X3SADM
  args->frame_3x3sadm = (uint8_t*)_aligned_malloc(sizeof(uint8_t) * frameSize, ALIGN_16);
  if (!args->frame_3x3sadm) {
    return(1);
  }
  MY_MEMSET(args->frame_3x3sadm, 0, sizeof(uint8_t) * frameSize);


  //  FRAME_IMPULSENOISE
  args->frame_ImpulseNoise = (uint8_t*)_aligned_malloc(sizeof(uint8_t) * frameSize, ALIGN_16);
  if (!args->frame_ImpulseNoise) {
    _aligned_free(args->frame_3x3sadm);
    return(1);
  }
  MY_MEMSET(args->frame_ImpulseNoise, 0, sizeof(uint8_t) * frameSize);


  //  FRAME_SMOOTHED
  args->frame_smoothed = (uint8_t*)_aligned_malloc(sizeof(uint8_t) * frameSize, ALIGN_16);
  if (!args->frame_smoothed) {
    _aligned_free(args->frame_ImpulseNoise);
    _aligned_free(args->frame_3x3sadm);
    return(1);
  }
  MY_MEMSET(args->frame_smoothed, INIT_FRAME_VALUE, sizeof(uint8_t) * frameSize);


  //  FRAME_BOUNDARIES
  args->frame_boundaries = (uint8_t*)_aligned_malloc(sizeof(uint8_t) * frameSize, ALIGN_16);
  if (!args->frame_boundaries) {
    _aligned_free(args->frame_smoothed);
    _aligned_free(args->frame_ImpulseNoise);
    _aligned_free(args->frame_3x3sadm);
    return(1);
  }
  MY_MEMSET(args->frame_boundaries, 0, sizeof(uint8_t) * frameSize);


  //  FRAME_DERINGMASK
  args->frame_deringMask = (uint8_t*)_aligned_malloc(sizeof(uint8_t) * frameSize, ALIGN_16);
  if (!args->frame_deringMask) {
    _aligned_free(args->frame_boundaries);

    _aligned_free(args->frame_smoothed);
    _aligned_free(args->frame_ImpulseNoise);
    _aligned_free(args->frame_3x3sadm);
    return(1);
  }
  MY_MEMSET(args->frame_deringMask, 0, sizeof(uint8_t) * frameSize);


  //  FRAME_SHARP
  args->frame_sharp = (uint8_t*)_aligned_malloc(sizeof(uint8_t) * frameSize, ALIGN_16);
  if (!args->frame_sharp) {
    _aligned_free(args->frame_deringMask);
    _aligned_free(args->frame_boundaries);

    _aligned_free(args->frame_smoothed);
    _aligned_free(args->frame_ImpulseNoise);
    _aligned_free(args->frame_3x3sadm);
    return(1);
  }
  MY_MEMSET(args->frame_sharp, INIT_FRAME_VALUE, sizeof(uint8_t) * frameSize);


  //  FRAME_GRAINMASK
  args->frame_grainMask = (uint8_t*)_aligned_malloc(sizeof(uint8_t) * frameSize, ALIGN_16);
  if (!args->frame_grainMask) {
    _aligned_free(args->frame_sharp);
    _aligned_free(args->frame_deringMask);
    _aligned_free(args->frame_boundaries);

    _aligned_free(args->frame_smoothed);
    _aligned_free(args->frame_ImpulseNoise);
    _aligned_free(args->frame_3x3sadm);
    return(1);
  }
  MY_MEMSET(args->frame_grainMask, 0, sizeof(uint8_t) * frameSize);


  //  FRAME_LOCALMAX9X9
  args->frame_localMax9x9 = (uint8_t*)_aligned_malloc(sizeof(uint8_t) * frameSize, ALIGN_16);
  if (!args->frame_localMax9x9) {
    _aligned_free(args->frame_grainMask);

    _aligned_free(args->frame_sharp);
    _aligned_free(args->frame_deringMask);
    _aligned_free(args->frame_boundaries);

    _aligned_free(args->frame_smoothed);
    _aligned_free(args->frame_ImpulseNoise);
    _aligned_free(args->frame_3x3sadm);
    return(1);
  }
  MY_MEMSET(args->frame_localMax9x9, INIT_FRAME_VALUE, sizeof(uint8_t) * frameSize);



  //  FRAME_LOCALMIN9X9
  args->frame_localMin9x9 = (uint8_t*)_aligned_malloc(sizeof(uint8_t) * frameSize, ALIGN_16);
  if (!args->frame_localMin9x9) {
    _aligned_free(args->frame_localMax9x9);
    _aligned_free(args->frame_grainMask);

    _aligned_free(args->frame_sharp);
    _aligned_free(args->frame_deringMask);
    _aligned_free(args->frame_boundaries);

    _aligned_free(args->frame_smoothed);
    _aligned_free(args->frame_ImpulseNoise);
    _aligned_free(args->frame_3x3sadm);
    return(1);
  }
  MY_MEMSET(args->frame_localMin9x9, INIT_FRAME_VALUE, sizeof(uint8_t) * frameSize);



  // FRAME_WORKSOURCE
  args->frame_workSource = (uint8_t*)_aligned_malloc(sizeof(uint8_t) * frameSize, ALIGN_16);
  if (!args->frame_workSource) {
    _aligned_free(args->frame_localMin9x9);
    _aligned_free(args->frame_localMax9x9);
    _aligned_free(args->frame_grainMask);

    _aligned_free(args->frame_sharp);
    _aligned_free(args->frame_deringMask);
    _aligned_free(args->frame_boundaries);

    _aligned_free(args->frame_smoothed);
    _aligned_free(args->frame_ImpulseNoise);
    _aligned_free(args->frame_3x3sadm);
    return(1);
  }
  MY_MEMSET(args->frame_workSource, INIT_FRAME_VALUE, sizeof(uint8_t) * frameSize);



  // FRAME_3X3MEAN
  args->frame_3x3mean = (uint8_t*)_aligned_malloc(sizeof(uint8_t) * frameSize, ALIGN_16);
  if (!args->frame_3x3mean) {
    _aligned_free(args->frame_workSource);

    _aligned_free(args->frame_localMin9x9);
    _aligned_free(args->frame_localMax9x9);
    _aligned_free(args->frame_grainMask);

    _aligned_free(args->frame_sharp);
    _aligned_free(args->frame_deringMask);
    _aligned_free(args->frame_boundaries);

    _aligned_free(args->frame_smoothed);
    _aligned_free(args->frame_ImpulseNoise);
    _aligned_free(args->frame_3x3sadm);
    return(1);
  }
  MY_MEMSET(args->frame_3x3mean, 0, sizeof(uint8_t) * frameSize);








  // FRAME_SHARPSMOOTHDIF
  args->frame_sharpSmoothDif = (uint8_t*)_aligned_malloc(sizeof(uint8_t) * frameSize, ALIGN_16);
  if (!args->frame_sharpSmoothDif) {
    _aligned_free(args->frame_3x3mean);
    _aligned_free(args->frame_workSource);

    _aligned_free(args->frame_localMin9x9);
    _aligned_free(args->frame_localMax9x9);
    _aligned_free(args->frame_grainMask);

    _aligned_free(args->frame_sharp);
    _aligned_free(args->frame_deringMask);
    _aligned_free(args->frame_boundaries);

    _aligned_free(args->frame_smoothed);
    _aligned_free(args->frame_ImpulseNoise);
    _aligned_free(args->frame_3x3sadm);
    return(1);
  }
  MY_MEMSET(args->frame_sharpSmoothDif, 0, sizeof(uint8_t) * frameSize);




  // FRAME_REFSOURCE
  args->frame_refSource = (uint8_t*)_aligned_malloc(sizeof(uint8_t) * frameSize, ALIGN_16);
  if (!args->frame_refSource) {
    _aligned_free(args->frame_sharpSmoothDif);
    _aligned_free(args->frame_3x3mean);
    _aligned_free(args->frame_workSource);

    _aligned_free(args->frame_localMin9x9);
    _aligned_free(args->frame_localMax9x9);
    _aligned_free(args->frame_grainMask);

    _aligned_free(args->frame_sharp);
    _aligned_free(args->frame_deringMask);
    _aligned_free(args->frame_boundaries);

    _aligned_free(args->frame_smoothed);
    _aligned_free(args->frame_ImpulseNoise);
    _aligned_free(args->frame_3x3sadm);
    return(1);
  }
  MY_MEMSET(args->frame_refSource, INIT_FRAME_VALUE, sizeof(uint8_t) * frameSize);




  // FRAME_ADAPTSOURCE
  args->frame_adaptSource = (uint8_t*)_aligned_malloc(sizeof(uint8_t) * frameSize, ALIGN_16);
  if (!args->frame_adaptSource) {
    _aligned_free(args->frame_refSource);

    _aligned_free(args->frame_sharpSmoothDif);
    _aligned_free(args->frame_3x3mean);
    _aligned_free(args->frame_workSource);

    _aligned_free(args->frame_localMin9x9);
    _aligned_free(args->frame_localMax9x9);
    _aligned_free(args->frame_grainMask);

    _aligned_free(args->frame_sharp);
    _aligned_free(args->frame_deringMask);
    _aligned_free(args->frame_boundaries);

    _aligned_free(args->frame_smoothed);
    _aligned_free(args->frame_ImpulseNoise);
    _aligned_free(args->frame_3x3sadm);
    return(1);
  }
  MY_MEMSET(args->frame_adaptSource, INIT_FRAME_VALUE, sizeof(uint8_t) * frameSize);

  return(0);
}


//
//   END MALLOC THE VARIOUS FRAME BUFFERS
// //////////////////


// //////////////////
//   FREE ALL OF THE FRAME BUFFERS
void
free_Frames(Memory_args* args) {

  _aligned_free(args->frame_adaptSource);
  _aligned_free(args->frame_refSource);

  _aligned_free(args->frame_sharpSmoothDif);
  _aligned_free(args->frame_3x3mean);
  _aligned_free(args->frame_workSource);

  _aligned_free(args->frame_localMin9x9);
  _aligned_free(args->frame_localMax9x9);
  _aligned_free(args->frame_grainMask);

  _aligned_free(args->frame_sharp);
  _aligned_free(args->frame_deringMask);
  _aligned_free(args->frame_boundaries);

  _aligned_free(args->frame_smoothed);
  _aligned_free(args->frame_ImpulseNoise);
  _aligned_free(args->frame_3x3sadm);

  return;
}
//
//   END FREE THE VARIOUS FRAME BUFFERS
// //////////////////







/////////////////////////
//  //// MALLOC ARRAYS
/////////////////////////
// /*
int
malloc_Arrays(Memory_args* args) {

  // AccumLimitArr[srcval][diff]
  args->AccumLimitArr = (uint8_t*)_aligned_malloc(256 * MAX_ACCUM_LIMIT_DIFF * sizeof(uint8_t), ALIGN_16);
  if (!args->AccumLimitArr) {
    return(1);
  }
  MY_MEMSET(args->AccumLimitArr, 0, 256 * MAX_ACCUM_LIMIT_DIFF);


  //  // AccumLimitArr[srcval][diff]
  //  args->AccumLimitArr = (uint8_t *) _aligned_malloc(256 * MAX_ACCUM_LIMIT_SHARP_DIFF * sizeof(uint8_t), ALIGN_16);
  //  if (!args->AccumLimitArr) {
  //    _aligned_free(args->AccumLimitArr);
  //    return(1);
  //  }
  //  MY_MEMSET(args->AccumLimitArr, 0, 256 * MAX_ACCUM_LIMIT_SHARP_DIFF);


    // AccumLimitBoundArr[srcval][diff]
  args->AccumLimitBoundArr = (uint8_t*)_aligned_malloc(256 * MAX_ACCUM_LIMIT_BOUND_DIFF * sizeof(uint8_t), ALIGN_16);
  if (!args->AccumLimitBoundArr) {
    _aligned_free(args->AccumLimitArr);
    return(1);
  }
  MY_MEMSET(args->AccumLimitArr, 0, 256 * MAX_ACCUM_LIMIT_BOUND_DIFF);



  // AccumLimitSmoothBoundArr[srcval][diff]
  args->AccumLimitSmoothBoundArr = (uint8_t*)_aligned_malloc(256 * MAX_ACCUM_LIMIT_SMOOTH_BOUND_DIFF * sizeof(uint8_t), ALIGN_16);
  if (!args->AccumLimitSmoothBoundArr) {
    _aligned_free(args->AccumLimitBoundArr);
    _aligned_free(args->AccumLimitArr);
    return(1);
  }
  MY_MEMSET(args->AccumLimitSmoothBoundArr, 0, 256 * MAX_ACCUM_LIMIT_SMOOTH_BOUND_DIFF);


  // DarkProtectDifArr[srcval][diff]
  args->DarkProtectDifArr = (uint8_t*)_aligned_malloc(256 * MAX_DARK_PROTECT_DIFF * sizeof(uint8_t), ALIGN_16);
  if (!args->DarkProtectDifArr) {
    _aligned_free(args->AccumLimitSmoothBoundArr);
    _aligned_free(args->AccumLimitBoundArr);
    _aligned_free(args->AccumLimitArr);
    return(1);
  }
  MY_MEMSET(args->DarkProtectDifArr, 0, 256 * MAX_DARK_PROTECT_DIFF);


  // BrightProtectDifArr[srcval][diff]
  args->BrightProtectDifArr = (uint8_t*)_aligned_malloc(256 * MAX_BRIGHT_SMOOTH_DIFF * sizeof(uint8_t), ALIGN_16);
  if (!args->BrightProtectDifArr) {
    _aligned_free(args->DarkProtectDifArr);
    _aligned_free(args->AccumLimitSmoothBoundArr);
    _aligned_free(args->AccumLimitBoundArr);
    _aligned_free(args->AccumLimitArr);
    return(1);
  }
  MY_MEMSET(args->BrightProtectDifArr, 0, 256 * MAX_BRIGHT_SMOOTH_DIFF);







  // AvgLumaBlockProtectArr[srcval][diff]
  args->AvgLumaBlockProtectArr = (uint8_t*)_aligned_malloc(256 * MAX_AVGLUMA_BLOCK_PROTECT * sizeof(uint8_t), ALIGN_16);
  if (!args->AvgLumaBlockProtectArr) {
    _aligned_free(args->BrightProtectDifArr);
    _aligned_free(args->DarkProtectDifArr);
    _aligned_free(args->AccumLimitSmoothBoundArr);
    _aligned_free(args->AccumLimitBoundArr);
    _aligned_free(args->AccumLimitArr);
    return(1);
  }
  MY_MEMSET(args->AvgLumaBlockProtectArr, 0, 256 * MAX_AVGLUMA_BLOCK_PROTECT);


  // SmoothDeHighlightArr[srcval]
  args->SmoothDeHighlightArr = (uint8_t*)_aligned_malloc(256 * sizeof(uint8_t), ALIGN_16);
  if (!args->SmoothDeHighlightArr) {
    _aligned_free(args->AvgLumaBlockProtectArr);
    _aligned_free(args->BrightProtectDifArr);
    _aligned_free(args->DarkProtectDifArr);
    _aligned_free(args->AccumLimitSmoothBoundArr);
    _aligned_free(args->AccumLimitBoundArr);
    _aligned_free(args->AccumLimitArr);
    return(1);
  }
  MY_MEMSET(args->SmoothDeHighlightArr, 0, 256);


  // BoundaryStrengthArr[srcval]
  args->BoundaryStrengthArr = (uint8_t*)_aligned_malloc(256 * sizeof(uint8_t), ALIGN_16);
  if (!args->BoundaryStrengthArr) {
    _aligned_free(args->SmoothDeHighlightArr);
    _aligned_free(args->AvgLumaBlockProtectArr);
    _aligned_free(args->BrightProtectDifArr);
    _aligned_free(args->DarkProtectDifArr);
    _aligned_free(args->AccumLimitSmoothBoundArr);
    _aligned_free(args->AccumLimitBoundArr);
    _aligned_free(args->AccumLimitArr);
    return(1);
  }
  MY_MEMSET(args->BoundaryStrengthArr, 0, 256);



  return(0);
}



// ////////////////////////////////
//   FREE Arrays
//
void
free_Arrays(Memory_args* args) {

  _aligned_free(args->BoundaryStrengthArr);
  _aligned_free(args->SmoothDeHighlightArr);
  _aligned_free(args->AvgLumaBlockProtectArr);
  _aligned_free(args->BrightProtectDifArr);
  _aligned_free(args->DarkProtectDifArr);
  _aligned_free(args->AccumLimitSmoothBoundArr);
  _aligned_free(args->AccumLimitBoundArr);
  _aligned_free(args->AccumLimitArr);
  return;
}

//  END FREE Arrays
//










// ///////////////////////////////////////
//
//  All of the following are used inside the multi-threading code,
//  which means we need a copy of the data for each thread.
//  The number of threads is specified by the user argument ncpu.
//
////////////////////////////////////////

//   MALLOC MULTI THREAD
int  malloc_MultiThread(Memory_args* args, int numBlocksWork, int sizeBlocksWork, int ncpu) {
  int i = 0;
  int fail = -1;
  uint8_t* tempPtr;

  // malloc_blocksFrame
  fail = -1;
  for (i = 0; i < ncpu; i++) {
    if (malloc_blocksFrame(args, sizeBlocksWork, i) != 0) {
      fail = i;
      break;
    }
  }

  // If malloc_blocksFrame() fails it will have cleaned up from any mallocs for the cpu number.
  // This allows us to treat malloc_blocksFrame() and free_blocksFrame()
  // just like malloc() and free(), as far as error recovery when running out of memory.
  // If malloc failed then fail will contain the number of mallocs that completed.
  if (fail != -1) {
    for (i = 0; i < fail; i++) {    // Free those that completed "i < fail"
      free_blocksFrame(args, i);
    }

    _aligned_free(args->LumaPerBlockArgs);
    free_Arrays(args);
    free_Frames(args);
    return(1);
  }

  //  malloc_matrix_data
  fail = -1;
  for (i = 0; i < ncpu; i++) {
    if (malloc_matrix_data(args, i) != 0) {
      fail = i;
      break;
    }
  }

  if (fail != -1) {
    for (i = 0; i < fail; i++) {    // Free those that completed "i < fail"
      free_matrix_data(args, i);
    }
    for (i = 0; i < ncpu; i++) {    // Free all previous mallocs for multi-threaded data that completed.
      free_blocksFrame(args, i);
    }

    // Free all non multi-threaded data.
    _aligned_free(args->LumaPerBlockArgs);
    free_Arrays(args);
    free_Frames(args);
    return(1);
  }

  //  blkAdaptSmoothAmt
  fail = -1;
  for (i = 0; i < ncpu; i++) {
    tempPtr = (uint8_t *)_aligned_malloc(numBlocksWork * 4, ALIGN_16);
    if (tempPtr == 0) {
      fail = i;
      break;
    }
    MY_MEMSET(tempPtr, 0, numBlocksWork);
    args->DctLoopArgs[i].blkAdaptSmoothAmt = tempPtr;
  }

  if (fail != -1) {
    for (i = 0; i < fail; i++) { // If we failed on the first malloc then fail == 0 and we won't try to free it.
      _aligned_free(args->DctLoopArgs[i].blkAdaptSmoothAmt);
    }
    for (i = 0; i < ncpu; i++) {
      free_blocksFrame(args, i);
      free_matrix_data(args, i);
    }
    _aligned_free(args->LumaPerBlockArgs);
    free_Arrays(args);
    free_Frames(args);
    return(1);
  }

  //  blkAdaptExpandAmt
  fail = -1;
  for (i = 0; i < ncpu; i++) {
    tempPtr = (uint8_t *)_aligned_malloc(numBlocksWork * 4, ALIGN_16);
    if (tempPtr == 0) {
      fail = i;
      break;
    }
    MY_MEMSET(tempPtr, 0, numBlocksWork);
    args->DctLoopArgs[i].blkAdaptExpandAmt = tempPtr;
  }

  if (fail != -1) {
    for (i = 0; i < fail; i++) { // If we failed on the first malloc then fail == 0 and we won't try to free it.
      _aligned_free(args->DctLoopArgs[i].blkAdaptExpandAmt);
    }
    for (i = 0; i < ncpu; i++) {
      _aligned_free(args->DctLoopArgs[i].blkAdaptSmoothAmt);
      free_blocksFrame(args, i);
      free_matrix_data(args, i);
    }

    _aligned_free(args->LumaPerBlockArgs);
    free_Arrays(args);
    free_Frames(args);
    return(1);
  }

  //  blkAdaptSharpAmt
  fail = -1;
  for (i = 0; i < ncpu; i++) {
    tempPtr = (uint8_t*)_aligned_malloc(numBlocksWork * 4, ALIGN_16);
    if (tempPtr == 0) {
      fail = i;
      break;
    }
    MY_MEMSET(tempPtr, 0, numBlocksWork);
    args->DctLoopArgs[i].blkAdaptSharpAmt = tempPtr;
  }

  if (fail != -1) {
    for (i = 0; i < fail; i++) { // If we failed on the first malloc then fail == 0 and we won't try to free it.
      _aligned_free(args->DctLoopArgs[i].blkAdaptSharpAmt);
    }

    for (i = 0; i < ncpu; i++) {
      _aligned_free(args->DctLoopArgs[i].blkAdaptExpandAmt);
      _aligned_free(args->DctLoopArgs[i].blkAdaptSmoothAmt);
      free_blocksFrame(args, i);
      free_matrix_data(args, i);
    }
    _aligned_free(args->LumaPerBlockArgs);
    free_Arrays(args);
    free_Frames(args);
    return(1);
  }

  return(0);
}



int
malloc_blocksFrame(Memory_args* args, int sizeBlocksWork, int ncpu) {

  // BF_accum
  args->DctLoopArgs[ncpu].BF_accumP = (uint16_t*)_aligned_malloc(sizeof(uint16_t) * sizeBlocksWork, ALIGN_16);
  if (!args->DctLoopArgs[ncpu].BF_accumP) {
    return(1);
  }
  MY_MEMSET(args->DctLoopArgs[ncpu].BF_accumP, 0, sizeof(uint16_t) * sizeBlocksWork);

  // BF_work
  args->DctLoopArgs[ncpu].BF_workP = (uint8_t*)_aligned_malloc(sizeof(uint8_t) * sizeBlocksWork, ALIGN_16);
  if (!args->DctLoopArgs[ncpu].BF_workP) {
    _aligned_free(args->DctLoopArgs[ncpu].BF_accumP);
    return(1);
  }
  MY_MEMSET(args->DctLoopArgs[ncpu].BF_workP, 0, sizeof(uint8_t) * sizeBlocksWork);

  // BF_src
  args->DctLoopArgs[ncpu].BF_srcP = (uint8_t*)_aligned_malloc(sizeof(uint8_t) * sizeBlocksWork, ALIGN_16);
  if (!args->DctLoopArgs[ncpu].BF_srcP) {
    _aligned_free(args->DctLoopArgs[ncpu].BF_accumP);
    _aligned_free(args->DctLoopArgs[ncpu].BF_workP);
    return(1);
  }
  MY_MEMSET(args->DctLoopArgs[ncpu].BF_srcP, 0, sizeof(uint8_t) * sizeBlocksWork);

  // BF_tmp_16
  args->DctLoopArgs[ncpu].BF_tmp_16P = (int16_t*)_aligned_malloc(sizeof(int16_t) * sizeBlocksWork, ALIGN_16);
  if (!args->DctLoopArgs[ncpu].BF_tmp_16P) {
    _aligned_free(args->DctLoopArgs[ncpu].BF_accumP);
    _aligned_free(args->DctLoopArgs[ncpu].BF_workP);
    _aligned_free(args->DctLoopArgs[ncpu].BF_srcP);
    return(1);
  }
  MY_MEMSET(args->DctLoopArgs[ncpu].BF_tmp_16P, 0, sizeof(int16_t) * sizeBlocksWork);

  //BF_tmp_8
  args->DctLoopArgs[ncpu].BF_tmp_8P = (uint8_t*)_aligned_malloc(sizeof(uint8_t) * sizeBlocksWork, ALIGN_16);
  if (!args->DctLoopArgs[ncpu].BF_tmp_8P) {
    _aligned_free(args->DctLoopArgs[ncpu].BF_accumP);
    _aligned_free(args->DctLoopArgs[ncpu].BF_workP);
    _aligned_free(args->DctLoopArgs[ncpu].BF_srcP);
    _aligned_free(args->DctLoopArgs[ncpu].BF_tmp_16P);
    return(1);
  }
  MY_MEMSET(args->DctLoopArgs[ncpu].BF_tmp_8P, INIT_FRAME_VALUE, sizeof(uint8_t) * sizeBlocksWork);

  return(0);
}



void free_blocksFrame(Memory_args* args, int ncpu) {

  _aligned_free(args->DctLoopArgs[ncpu].BF_accumP);
  _aligned_free(args->DctLoopArgs[ncpu].BF_workP);
  _aligned_free(args->DctLoopArgs[ncpu].BF_srcP);
  _aligned_free(args->DctLoopArgs[ncpu].BF_tmp_16P);
  _aligned_free(args->DctLoopArgs[ncpu].BF_tmp_8P);

  ////    _aligned_free(args->DctLoopArgs[ncpu].BF_pf1P);  // FUTURE: previous and next frame.
  ////    _aligned_free(args->DctLoopArgs[ncpu].BF_nf1P);
  return;
}

///////  FUTURE EXPERIMENTAL CODE !!!
//  //BF_tmp2_8
//  args->DctLoopArgs[ncpu].BF_tmp2_8P = (uint8_t*) _aligned_malloc(sizeof(uint8_t) * sizeBlocksWork, ALIGN_16);
//  if (!args->DctLoopArgs[ncpu].BF_tmp2_8P) {
//    _aligned_free(args->DctLoopArgs[ncpu].BF_accumP);
//    _aligned_free(args->DctLoopArgs[ncpu].BF_workP);
////    _aligned_free(args->DctLoopArgs[ncpu].BF_pf1P);
////    _aligned_free(args->DctLoopArgs[ncpu].BF_nf1P);
//    _aligned_free(args->DctLoopArgs[ncpu].BF_srcP);
//    _aligned_free(args->DctLoopArgs[ncpu].BF_tmp_16P);
//    _aligned_free(args->DctLoopArgs[ncpu].BF_tmp_8P);
//    return(1);
//  }
//  MY_MEMSET(args->DctLoopArgs[ncpu].BF_tmp2_8P, 0, sizeof(uint8_t) * sizeBlocksWork);
//
//
//  //BF_tmp_32
//  args->DctLoopArgs[ncpu].BF_tmp_32P = (int32_t*) _aligned_malloc(sizeof(int32_t) * sizeBlocksWork, ALIGN_16);
//  if (!args->DctLoopArgs[ncpu].BF_tmp_32P) {
//    _aligned_free(args->DctLoopArgs[ncpu].BF_accumP);
//    _aligned_free(args->DctLoopArgs[ncpu].BF_workP);
//    _aligned_free(args->DctLoopArgs[ncpu].BF_pf1P);
//    _aligned_free(args->DctLoopArgs[ncpu].BF_nf1P);
//    _aligned_free(args->DctLoopArgs[ncpu].BF_srcP);
//    _aligned_free(args->DctLoopArgs[ncpu].BF_tmp_16P);
//    _aligned_free(args->DctLoopArgs[ncpu].BF_tmp_8P);
//    _aligned_free(args->DctLoopArgs[ncpu].BF_tmp2_8P);
//    return(1);
//  }
//  MY_MEMSET(args->DctLoopArgs[ncpu].BF_tmp_32P, 0, sizeof(int32_t) * sizeBlocksWork);
  // BF_pf1
//  args->DctLoopArgs[ncpu].BF_pf1P = (uint8_t*) _aligned_malloc(sizeof(uint8_t) * sizeBlocksWork, ALIGN_16);
//  if (!args->DctLoopArgs[ncpu].BF_pf1P) {
//    _aligned_free(args->DctLoopArgs[ncpu].BF_accumP);
//    _aligned_free(args->DctLoopArgs[ncpu].BF_workP);
//    _aligned_free(args->DctLoopArgs[ncpu].BF_srcP);
//    return(1);
//  }
//  MY_MEMSET(args->DctLoopArgs[ncpu].BF_pf1P, 0, sizeof(uint8_t) * sizeBlocksWork);
//
//
//
//  // BF_nf1
//  args->DctLoopArgs[ncpu].BF_nf1P = (uint8_t*) _aligned_malloc(sizeof(uint8_t) * sizeBlocksWork, ALIGN_16);
//  if (!args->DctLoopArgs[ncpu].BF_nf1P) {
//    _aligned_free(args->DctLoopArgs[ncpu].BF_pf1P);
//    _aligned_free(args->DctLoopArgs[ncpu].BF_accumP);
//    _aligned_free(args->DctLoopArgs[ncpu].BF_workP);
//    _aligned_free(args->DctLoopArgs[ncpu].BF_srcP);
//    return(1);
//  }
//  MY_MEMSET(args->DctLoopArgs[ncpu].BF_nf1P, 0, sizeof(uint8_t) * sizeBlocksWork);






////////////////
/// MALLOC MATRIX DATA
////////////////
int malloc_matrix_data(Memory_args* args, int ncpu) {
  int i;
  int fail;

  // quant_intra_matrix
  args->DctLoopArgs[ncpu].quant_intra_matrix = (uint16_t*)_aligned_malloc(MATRIX_SIZE, ALIGN_16);
  if (!args->DctLoopArgs[ncpu].quant_intra_matrix) {
    return(1);
  }
  MY_MEMSET(args->DctLoopArgs[ncpu].quant_intra_matrix, 0, MATRIX_SIZE);

  // quant_inter_matrix
  args->DctLoopArgs[ncpu].quant_inter_matrix = (uint16_t*)_aligned_malloc(MATRIX_SIZE, ALIGN_16);
  if (!args->DctLoopArgs[ncpu].quant_inter_matrix) {
    _aligned_free(args->DctLoopArgs[ncpu].quant_intra_matrix);
    return(1);
  }
  MY_MEMSET(args->DctLoopArgs[ncpu].quant_inter_matrix, 0, MATRIX_SIZE);

  if (ncpu == 0) {  // Not part of threading so only allocate 1 copy.
    // quant_intra_matrix_sharp
    args->quant_intra_matrix_sharp = (uint16_t*)_aligned_malloc(MATRIX_SIZE, ALIGN_16);
    if (!args->quant_intra_matrix_sharp) {
      _aligned_free(args->DctLoopArgs[ncpu].quant_intra_matrix);
      _aligned_free(args->DctLoopArgs[ncpu].quant_inter_matrix);
      return(1);
    }
    MY_MEMSET(args->quant_intra_matrix_sharp, 0, MATRIX_SIZE);

    // quant_inter_matrix_sharp
    args->quant_inter_matrix_sharp = (uint16_t*)_aligned_malloc(MATRIX_SIZE, ALIGN_16);
    if (!args->quant_inter_matrix_sharp) {
      _aligned_free(args->quant_intra_matrix_sharp);
      _aligned_free(args->DctLoopArgs[ncpu].quant_intra_matrix);
      _aligned_free(args->DctLoopArgs[ncpu].quant_inter_matrix);
      return(1);
    }
    MY_MEMSET(args->quant_inter_matrix_sharp, 0, MATRIX_SIZE);
  } // END ncpu == 0

  // The following maticies come in 31 "MAX_BLOCK_ADAPT" different "strengths".
  //     One for each possible adapt strength value.
  // qtype1_matrix_quant
  fail = -1;
  for (i = 0; i < MAX_BLOCK_ADAPT; i++) {
    args->DctLoopArgs[ncpu].qtype1_matrix_quant[i] = (uint16_t*)_aligned_malloc(MATRIX_SIZE, ALIGN_16);

    if (!args->DctLoopArgs[ncpu].qtype1_matrix_quant[i]) {
      fail = i;
      break;
    }
    MY_MEMSET(args->DctLoopArgs[ncpu].qtype1_matrix_quant[i], 0, MATRIX_SIZE);
  }

  if (fail != -1) {
    for (i = 0; i < fail; i++) {
      _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix_quant[i]);
    }
    _aligned_free(args->DctLoopArgs[ncpu].quant_intra_matrix);
    _aligned_free(args->DctLoopArgs[ncpu].quant_inter_matrix);
    _aligned_free(args->quant_intra_matrix_sharp);
    _aligned_free(args->quant_inter_matrix_sharp);
    return(1);
  }


  // qtype1_matrix
  fail = -1;
  for (i = 0; i < MAX_BLOCK_ADAPT; i++) {
    args->DctLoopArgs[ncpu].qtype1_matrix[i] = (uint16_t*)_aligned_malloc(MATRIX_SIZE, ALIGN_16);
    if (!args->DctLoopArgs[ncpu].qtype1_matrix[i]) {
      fail = i;
      break;
    }
    MY_MEMSET(args->DctLoopArgs[ncpu].qtype1_matrix[i], 0, MATRIX_SIZE);
  }

  if (fail != -1) {
    for (i = 0; i < fail; i++) {
      _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix[i]);
    }
    for (i = 0; i < MAX_BLOCK_ADAPT; i++) {
      _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix_quant[i]);
    }
    _aligned_free(args->DctLoopArgs[ncpu].quant_intra_matrix);
    _aligned_free(args->DctLoopArgs[ncpu].quant_inter_matrix);
    _aligned_free(args->quant_intra_matrix_sharp);
    _aligned_free(args->quant_inter_matrix_sharp);
    return(1);
  }

  // qtype1_matrix_range
  fail = -1;
  for (i = 0; i < MAX_BLOCK_ADAPT; i++) {
    args->DctLoopArgs[ncpu].qtype1_matrix_range[i] = (uint16_t*)_aligned_malloc(MATRIX_SIZE, ALIGN_16);
    if (!args->DctLoopArgs[ncpu].qtype1_matrix_range[i]) {
      fail = i;
      break;
    }
    MY_MEMSET(args->DctLoopArgs[ncpu].qtype1_matrix_range[i], 0, MATRIX_SIZE);
  }
  if (fail != -1) {
    for (i = 0; i < fail; i++) {
      _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix_range[i]);
    }
    for (i = 0; i < MAX_BLOCK_ADAPT; i++) {
      _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix_quant[i]);
      _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix[i]);
    }
    _aligned_free(args->DctLoopArgs[ncpu].quant_intra_matrix);
    _aligned_free(args->DctLoopArgs[ncpu].quant_inter_matrix);
    _aligned_free(args->quant_intra_matrix_sharp);
    _aligned_free(args->quant_inter_matrix_sharp);
    return(1);
  }

  // qtype1_matrix_quant_range
  fail = -1;
  for (i = 0; i < MAX_BLOCK_ADAPT; i++) {
    args->DctLoopArgs[ncpu].qtype1_matrix_quant_range[i] = (uint16_t*)_aligned_malloc(MATRIX_SIZE, ALIGN_16);
    if (!args->DctLoopArgs[ncpu].qtype1_matrix_quant_range[i]) {
      fail = i;
      break;
    }
    MY_MEMSET(args->DctLoopArgs[ncpu].qtype1_matrix_quant_range[i], 0, MATRIX_SIZE);
  }
  if (fail != -1) {
    for (i = 0; i < fail; i++) {
      _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix_quant_range[i]);
    }
    for (i = 0; i < MAX_BLOCK_ADAPT; i++) {
      _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix_range[i]);
      _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix_quant[i]);
      _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix[i]);
    }
    _aligned_free(args->DctLoopArgs[ncpu].quant_intra_matrix);
    _aligned_free(args->DctLoopArgs[ncpu].quant_inter_matrix);
    _aligned_free(args->quant_intra_matrix_sharp);
    _aligned_free(args->quant_inter_matrix_sharp);
    return(1);
  }

  // qtype1_matrix_sharp
  fail = -1;
  for (i = 0; i < MAX_BLOCK_ADAPT; i++) {
    args->DctLoopArgs[ncpu].qtype1_matrix_sharp[i] = (uint16_t*)_aligned_malloc(MATRIX_SIZE, ALIGN_16);
    if (!args->DctLoopArgs[ncpu].qtype1_matrix_sharp[i]) {
      fail = i;
      break;
    }
    MY_MEMSET(args->DctLoopArgs[ncpu].qtype1_matrix_sharp[i], 0, MATRIX_SIZE);
  }

  if (fail != -1) {
    for (i = 0; i < fail; i++) {
      _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix_sharp[i]);
    }
    for (i = 0; i < MAX_BLOCK_ADAPT; i++) {
      _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix_quant_range[i]);
      _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix_range[i]);
      _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix_quant[i]);
      _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix[i]);
    }
    _aligned_free(args->DctLoopArgs[ncpu].quant_intra_matrix);
    _aligned_free(args->DctLoopArgs[ncpu].quant_inter_matrix);
    _aligned_free(args->quant_intra_matrix_sharp);
    _aligned_free(args->quant_inter_matrix_sharp);
    return(1);
  }


  // qtype1_matrix_quant_sharp
  fail = -1;
  for (i = 0; i < MAX_BLOCK_ADAPT; i++) {
    args->DctLoopArgs[ncpu].qtype1_matrix_quant_sharp[i] = (uint16_t*)_aligned_malloc(MATRIX_SIZE, ALIGN_16);

    if (!args->DctLoopArgs[ncpu].qtype1_matrix_quant_sharp[i]) {
      fail = i;
      break;
    }
    MY_MEMSET(args->DctLoopArgs[ncpu].qtype1_matrix_quant_sharp[i], 0, MATRIX_SIZE);
  }

  if (fail != -1) {
    for (i = 0; i < fail; i++) {
      _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix_quant_sharp[i]);
    }
    for (i = 0; i < MAX_BLOCK_ADAPT; i++) {
      _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix_sharp[i]);
      _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix_quant_range[i]);
      _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix_range[i]);
      _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix_quant[i]);
      _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix[i]);
    }
    _aligned_free(args->DctLoopArgs[ncpu].quant_intra_matrix);
    _aligned_free(args->DctLoopArgs[ncpu].quant_inter_matrix);
    _aligned_free(args->quant_intra_matrix_sharp);
    _aligned_free(args->quant_inter_matrix_sharp);
    return(1);
  }


  // qtype11_matrix_quant
  fail = -1;
  for (i = 0; i < MAX_BLOCK_ADAPT; i++) {
    args->DctLoopArgs[ncpu].qtype11_matrix_quant[i] = (uint16_t*)_aligned_malloc(MATRIX_SIZE, ALIGN_16);

    if (!args->DctLoopArgs[ncpu].qtype11_matrix_quant[i]) {
      fail = i;
      break;
    }
    MY_MEMSET(args->DctLoopArgs[ncpu].qtype11_matrix_quant[i], 0, MATRIX_SIZE);
  }

  if (fail != -1) {
    for (i = 0; i < fail; i++) {
      _aligned_free(args->DctLoopArgs[ncpu].qtype11_matrix_quant[i]);
    }
    for (i = 0; i < MAX_BLOCK_ADAPT; i++) {
      _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix_quant_sharp[i]);
      _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix_sharp[i]);
      _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix_quant_range[i]);
      _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix_range[i]);
      _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix_quant[i]);
      _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix[i]);
    }
    _aligned_free(args->DctLoopArgs[ncpu].quant_intra_matrix);
    _aligned_free(args->DctLoopArgs[ncpu].quant_inter_matrix);
    _aligned_free(args->quant_intra_matrix_sharp);
    _aligned_free(args->quant_inter_matrix_sharp);
    return(1);
  }

  // qtype11_matrix
  fail = -1;
  for (i = 0; i < MAX_BLOCK_ADAPT; i++) {
    args->DctLoopArgs[ncpu].qtype11_matrix[i] = (uint16_t*)_aligned_malloc(MATRIX_SIZE, ALIGN_16);

    if (!args->DctLoopArgs[ncpu].qtype11_matrix[i]) {
      fail = i;
      break;
    }
    MY_MEMSET(args->DctLoopArgs[ncpu].qtype11_matrix[i], 0, MATRIX_SIZE);
  }

  if (fail != -1) {
    for (i = 0; i < fail; i++) {
      _aligned_free(args->DctLoopArgs[ncpu].qtype11_matrix[i]);
    }
    for (i = 0; i < MAX_BLOCK_ADAPT; i++) {
      _aligned_free(args->DctLoopArgs[ncpu].qtype11_matrix_quant[i]);
      _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix_quant_sharp[i]);
      _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix_sharp[i]);
      _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix_quant_range[i]);
      _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix_range[i]);
      _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix_quant[i]);
      _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix[i]);
    }
    _aligned_free(args->DctLoopArgs[ncpu].quant_intra_matrix);
    _aligned_free(args->DctLoopArgs[ncpu].quant_inter_matrix);
    _aligned_free(args->quant_intra_matrix_sharp);
    _aligned_free(args->quant_inter_matrix_sharp);
    return(1);
  }

  // 64 is the 64 pixels in an 8x8 block.
  // MAX_LEVEL_IDX are the 70 resulting 8 bit values for each position in the 8x8 block.
  // The values are obtained by precomputing the new DCT values, done in sharp.c.
  // They are accessed as a 64xMAX_LEVEL_IDX array the first index is the pixel location
  // the second is the resulting values.

  // quant_sharp_preComp
  fail = -1;
  for (i = 0; i < MAX_BLOCK_ADAPT; i++) {
    args->DctLoopArgs[ncpu].quant_sharp_preComp[i] = (uint8_t*)_aligned_malloc(sizeof(uint8_t) * 64 * MAX_LEVEL_IDX, ALIGN_16);

    if (!args->DctLoopArgs[ncpu].quant_sharp_preComp[i]) {
      fail = i;
      break;
    }
    MY_MEMSET(args->DctLoopArgs[ncpu].quant_sharp_preComp[i], 0, sizeof(uint8_t) * 64 * MAX_LEVEL_IDX);
  }

  if (fail != -1) {
    for (i = 0; i < fail; i++) {
      _aligned_free(args->DctLoopArgs[ncpu].quant_sharp_preComp[i]);
    }
    for (i = 0; i < MAX_BLOCK_ADAPT; i++) {
      _aligned_free(args->DctLoopArgs[ncpu].qtype11_matrix[i]);
      _aligned_free(args->DctLoopArgs[ncpu].qtype11_matrix_quant[i]);
      _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix_quant_sharp[i]);
      _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix_sharp[i]);
      _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix_quant_range[i]);
      _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix_range[i]);
      _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix_quant[i]);
      _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix[i]);
    }
    _aligned_free(args->DctLoopArgs[ncpu].quant_intra_matrix);
    _aligned_free(args->DctLoopArgs[ncpu].quant_inter_matrix);
    _aligned_free(args->quant_intra_matrix_sharp);
    _aligned_free(args->quant_inter_matrix_sharp);
    return(1);
  }
  return(0);
}

void free_matrix_data(Memory_args* args, int ncpu) {
  unsigned int i;

  if (ncpu == 0) {
    _aligned_free(args->quant_intra_matrix_sharp);
    _aligned_free(args->quant_inter_matrix_sharp);
  }
  for (i = 0; i < MAX_BLOCK_ADAPT; i++) {
    _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix[i]);
    _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix_quant[i]);
    _aligned_free(args->DctLoopArgs[ncpu].qtype11_matrix[i]);
    _aligned_free(args->DctLoopArgs[ncpu].qtype11_matrix_quant[i]);
    _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix_range[i]);
    _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix_quant_range[i]);
    _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix_sharp[i]);
    _aligned_free(args->DctLoopArgs[ncpu].qtype1_matrix_quant_sharp[i]);
    _aligned_free(args->DctLoopArgs[ncpu].quant_sharp_preComp[i]);
  }
  _aligned_free(args->DctLoopArgs[ncpu].quant_intra_matrix);
  _aligned_free(args->DctLoopArgs[ncpu].quant_inter_matrix);

  return;
}



void free_data(FrameInfo_args* FrameInfoArgs) {
  Memory_args* args = FrameInfoArgs->MemoryArgs;
  unsigned int i;

  for (i = 0; i < FrameInfoArgs->ncpu; i++) {
    free_blocksFrame(args, i);
    free_matrix_data(args, i);

    _aligned_free(args->DctLoopArgs[i].blkAdaptSharpAmt);
    _aligned_free(args->DctLoopArgs[i].blkAdaptExpandAmt);
    _aligned_free(args->DctLoopArgs[i].blkAdaptSmoothAmt);
  }

  _aligned_free(args->LumaPerBlockArgs);
  free_Arrays(args);
  free_Frames(args);

  _aligned_free(FrameInfoArgs->MemoryArgs);
  return;

  //  _aligned_free(args->PixCountArgs);
  //  _aligned_free(args->AvgLumaBlockProtectArr);
  //  _aligned_free(args->BrightProtectDifArr);
  //  _aligned_free(args->DarkProtectDifArr);
  //  _aligned_free(args->AccumLimitSmoothBoundArr);
  //  _aligned_free(args->AccumLimitBoundArr);
  //  _aligned_free(args->AccumLimitArr);
  //  _aligned_free(args->LumaPerBlockArgs);


  /*
    _aligned_free(args->frame_adaptSource);
    _aligned_free(args->frame_refSource);
    _aligned_free(args->frame_sharpSmoothDif);
    _aligned_free(args->frame_3x3mean);
    _aligned_free(args->frame_workSource);
  //  _aligned_free(args->frame_localMin9x9);
  //  _aligned_free(args->frame_localMax9x9);
    _aligned_free(args->frame_grainMask);
    _aligned_free(args->frame_sharp);
    _aligned_free(args->frame_deringMask);
    _aligned_free(args->frame_boundaries);
    _aligned_free(args->frame_smoothed);
    _aligned_free(args->frame_ImpulseNoise);
    _aligned_free(args->frame_3x3sadm);
  */

}




void clear_data_work_buffers(FrameInfo_args* FrameInfoArgs) {
  Memory_args* memargs = FrameInfoArgs->MemoryArgs;
  int          sizeBlocksWork = FrameInfoArgs->sizeBlocksWork;
  int          ncpu = FrameInfoArgs->ncpu;
  int          i;

  for (i = 0; i < ncpu; i++) {
    MY_MEMSET(memargs->DctLoopArgs[i].BF_accumP, 0, sizeof(uint16_t) * sizeBlocksWork);
    MY_MEMSET(memargs->DctLoopArgs[i].BF_workP, 0, sizeof(uint8_t) * sizeBlocksWork);
    MY_MEMSET(memargs->DctLoopArgs[i].BF_tmp_16P, 0, sizeof(int16_t) * sizeBlocksWork);  // Used in Adapt
    //    MY_MEMSET(memargs->DctLoopArgs[i].BF_tmp_8P,  0, sizeof(int8_t)   * sizeBlocksWork);  // ONLY Used OUTSIDE OF DCTLOOP
    //    MY_MEMSET(memargs->DctLoopArgs[i].BF_tmp_32P, 0, sizeof(int32_t)  * sizeBlocksWork);  // Not Currently Used
    memargs->DctLoopArgs[i].cntShift = 0;
  }
  //  MY_MEMSET(memargs->DctLoopArgs[0].BF_tmp_8P,  0, sizeof(int8_t)   * sizeBlocksWork);  // ONLY Used OUTSIDE OF DCTLOOP
  return;
}




// This routine sets DctLoopArgs structure and then replicates it as necessary for multiple CPU's
// This routine replicates the data so that it can be used independently by each thread.
void dup_cpu_data(FrameInfo_args* FrameInfoArgs) {
  Memory_args* memargs = FrameInfoArgs->MemoryArgs;
  int          sizeBlocksWork = FrameInfoArgs->sizeBlocksWork;
  int          numBlocksWork = FrameInfoArgs->numBlocksWork;
  int          ncpu = FrameInfoArgs->ncpu;
  int i, j, k;

  memargs->DctLoopArgs[0].qtype = FrameInfoArgs->qtype;
  memargs->DctLoopArgs[0].quant = FrameInfoArgs->quant;
  memargs->DctLoopArgs[0].max_quant = FrameInfoArgs->max_quant;

  memargs->DctLoopArgs[0].ncpu = FrameInfoArgs->ncpu;
  memargs->DctLoopArgs[0].numBlocks_wide = FrameInfoArgs->numBlocks_wide; // Num of cols in blocks.
  memargs->DctLoopArgs[0].numBlocks_high = FrameInfoArgs->numBlocks_high; // Num of rows in blocks.
  memargs->DctLoopArgs[0].src_width = FrameInfoArgs->src_width;
  memargs->DctLoopArgs[0].src_height = FrameInfoArgs->src_height;
  memargs->DctLoopArgs[0].sharpWPos = FrameInfoArgs->sharpWPos;
  memargs->DctLoopArgs[0].sharpWAmt = FrameInfoArgs->sharpWAmt;
  memargs->DctLoopArgs[0].sharpTPos = FrameInfoArgs->sharpTPos;
  memargs->DctLoopArgs[0].sharpTAmt = FrameInfoArgs->sharpTAmt;
  memargs->DctLoopArgs[0].sharpMaxWT = FrameInfoArgs->sharpMaxWT;
  memargs->DctLoopArgs[0].expand = FrameInfoArgs->expand;
  memargs->DctLoopArgs[0].doDCTadapt = FrameInfoArgs->doDCTadapt;
  memargs->DctLoopArgs[0].cntShift = 0;
  memargs->DctLoopArgs[0].quality = FrameInfoArgs->quality;
  memargs->DctLoopArgs[0].showMask = FrameInfoArgs->showMask;
  memargs->DctLoopArgs[0].T2 = FrameInfoArgs->T2;

  for (i = 1; i < ncpu; i++) {
    memargs->DctLoopArgs[i].qtype = memargs->DctLoopArgs[0].qtype;
    memargs->DctLoopArgs[i].quant = memargs->DctLoopArgs[0].quant;
    memargs->DctLoopArgs[i].max_quant = memargs->DctLoopArgs[0].max_quant;

    memargs->DctLoopArgs[i].ncpu = memargs->DctLoopArgs[0].ncpu;
    memargs->DctLoopArgs[i].numBlocks_wide = memargs->DctLoopArgs[0].numBlocks_wide;//Num of cols in blks
    memargs->DctLoopArgs[i].numBlocks_high = memargs->DctLoopArgs[0].numBlocks_high;//Num of rows in blks
    memargs->DctLoopArgs[i].src_width = memargs->DctLoopArgs[0].src_width;
    memargs->DctLoopArgs[i].src_height = memargs->DctLoopArgs[0].src_height;
    memargs->DctLoopArgs[i].sharpWPos = memargs->DctLoopArgs[0].sharpWPos;
    memargs->DctLoopArgs[i].sharpWAmt = memargs->DctLoopArgs[0].sharpWAmt;
    memargs->DctLoopArgs[i].sharpTPos = memargs->DctLoopArgs[0].sharpTPos;
    memargs->DctLoopArgs[i].sharpTAmt = memargs->DctLoopArgs[0].sharpTAmt;
    memargs->DctLoopArgs[i].sharpMaxWT = memargs->DctLoopArgs[0].sharpMaxWT;
    memargs->DctLoopArgs[i].expand = memargs->DctLoopArgs[0].expand;
    memargs->DctLoopArgs[i].quality = memargs->DctLoopArgs[0].quality;
    memargs->DctLoopArgs[i].showMask = memargs->DctLoopArgs[0].showMask;
    memargs->DctLoopArgs[i].T2 = memargs->DctLoopArgs[0].T2;

    memargs->DctLoopArgs[i].doDCTadapt = memargs->DctLoopArgs[0].doDCTadapt;
    memargs->DctLoopArgs[i].cntShift = memargs->DctLoopArgs[0].cntShift;

    MY_MEMCPY(memargs->DctLoopArgs[i].blkAdaptSmoothAmt, memargs->DctLoopArgs[0].blkAdaptSmoothAmt, numBlocksWork * 1);
    MY_MEMCPY(memargs->DctLoopArgs[i].blkAdaptExpandAmt, memargs->DctLoopArgs[0].blkAdaptExpandAmt, numBlocksWork * 1);
    MY_MEMCPY(memargs->DctLoopArgs[i].blkAdaptSharpAmt, memargs->DctLoopArgs[0].blkAdaptSharpAmt, numBlocksWork * 1);

    MY_MEMCPY(memargs->DctLoopArgs[i].BF_srcP, memargs->DctLoopArgs[0].BF_srcP, sizeof(uint8_t) * sizeBlocksWork);

    for (k = 0; k < 64; k++) {
      memargs->DctLoopArgs[i].quant_intra_matrix[k] = memargs->DctLoopArgs[0].quant_intra_matrix[k];
      memargs->DctLoopArgs[i].quant_inter_matrix[k] = memargs->DctLoopArgs[0].quant_inter_matrix[k];
    }

    for (j = 0; j < MAX_BLOCK_ADAPT; j++) {
      for (k = 0; k < 64; k++) {
        memargs->DctLoopArgs[i].qtype1_matrix_quant[j][k] = memargs->DctLoopArgs[0].qtype1_matrix_quant[j][k];
        memargs->DctLoopArgs[i].qtype1_matrix[j][k] = memargs->DctLoopArgs[0].qtype1_matrix[j][k];
        memargs->DctLoopArgs[i].qtype11_matrix_quant[j][k] = memargs->DctLoopArgs[0].qtype11_matrix_quant[j][k];
        memargs->DctLoopArgs[i].qtype11_matrix[j][k] = memargs->DctLoopArgs[0].qtype11_matrix[j][k];
        memargs->DctLoopArgs[i].qtype1_matrix_range[j][k] = memargs->DctLoopArgs[0].qtype1_matrix_range[j][k];
        memargs->DctLoopArgs[i].qtype1_matrix_quant_range[j][k] = memargs->DctLoopArgs[0].qtype1_matrix_quant_range[j][k];
        memargs->DctLoopArgs[i].qtype1_matrix_sharp[j][k] = memargs->DctLoopArgs[0].qtype1_matrix_sharp[j][k];
        memargs->DctLoopArgs[i].qtype1_matrix_quant_sharp[j][k] = memargs->DctLoopArgs[0].qtype1_matrix_quant_sharp[j][k];
      }
      for (k = 0; k < 64 * MAX_LEVEL_IDX; k++) {
        memargs->DctLoopArgs[i].quant_sharp_preComp[j][k] = memargs->DctLoopArgs[0].quant_sharp_preComp[j][k];
      }
    }
  }
  return;
}
