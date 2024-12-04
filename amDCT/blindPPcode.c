#include  <math.h>
#include <emmintrin.h> // Header for SSE2 intrinsics
#include <tmmintrin.h> // Header for SSSE3 intrinsics
#include <smmintrin.h> // SSE4.2

#include "blindPPcode.h"
#include "amDCTPortab.h"  // needed for uint32_t etc.  
#include "Utilities.h"

#define true  1
#define false 0



/***************************
 * The code that follows in this file is a copy of several routines from blindPP.
 * The routines are only slightly modified from the original.
 * Most changes are in the arguments to make it easier to call from within amDCT - Jim Conklin 1-20-2013
 *
 * I found the following names in PostProcess.cpp, which is were blindPP is found, and History.text in the dgdecode158scr
 *
 *        Thank You All and the others I don't know about for this great piece of code.
 *
 "tritical"
 Donald A. Graft
 MarcFD
 Vlad59
 Tom Barry
 These source-files are changes to the mpeg2dec_dll.zip file from www.davetech.org/software2.htm

liaor@iname.com
http://members.tripod.com/~liaor
 ***************************
 */




 //  This routine is called from FrameInfo.c to help control halos that have been introduced by sharpening and expansion.
 //  This routine runs dering() on FrameInfoArgs->psrc 
void doDering(FrameInfo_args* FrameInfoArgs) {

  uint8_t* psrc = FrameInfoArgs->psrc;
  //  uint8_t   *psrcMem    = FrameInfoArgs->MemoryArgs->DctLoopArgs[0].BF_srcP;
  uint8_t   sharpWAmt = FrameInfoArgs->sharpWAmt;
  uint8_t   sharpTAmt = FrameInfoArgs->sharpTAmt;
  uint16_t  src_height = FrameInfoArgs->src_height;
  uint16_t  src_width = FrameInfoArgs->src_width;


  if (sharpWAmt >= 21 || sharpTAmt >= 21) {
    dering(psrc, src_height, src_width, 3);
    //    MY_MEMCPY(psrcMem, psrc, FrameInfoArgs->sizeY);    
    return;
  }

  if (sharpWAmt >= 15 || sharpTAmt >= 15) {
    dering(psrc, src_height, src_width, 2);
    //    MY_MEMCPY(psrcMem, psrc, FrameInfoArgs->sizeY);    
    return;
  }

  if (sharpWAmt > 9 || sharpTAmt > 9) {
    dering(psrc, src_height, src_width, 1);
    //    MY_MEMCPY(psrcMem, psrc, FrameInfoArgs->sizeY);    
    return;
  }


  return;
}








int  deblock_horiz_DC_on(uint8_t* v, int stride, int QP);
int  deblock_horiz_default_filter(uint8_t* v, int stride, int QP);
int  deblock_horiz_useDC(uint8_t* v, int stride, int moderate_h);

void deblock_vert_default_filter(uint8_t* v, int stride, int QP);

int deblock_vert_DC_on(uint8_t* v, int stride, int QP);
void deblock_vert_default_filter_c(uint8_t* v, int stride, int QP);
void deblock_vert_default_filter_ORIGINAL(uint8_t* v, int stride, int QP);

const static uint64_t mm_fours = 0x0004000400040004;

int deblock_vert_useDC(uint8_t* v, int stride, int moderate_v);

void deblock_vert_choose_p1p2(uint8_t* v, int stride, uint64_t* p1p2, int QP);
void deblock_vert_copy_and_unpack(int stride, uint8_t* source, uint64_t* dest, int n);

#define ABS(X)        (((X)>0)?(X) : 0-(X))
#define ABS_MACRO(X)  (((X)>0)?(X) : 0-(X))









/*
 * This code is a horizontal deblocking filter - i.e. it will smooth _vertical_ block edges
 * which means that if you do a diff before and after running the filter you will see
 * the changes as vertical lines.
 */
void deblock_horiz(uint8_t* image, int height, int width, int stride, int quant) {
  int x, y;
  uint8_t* v;
  int useDC, DC_on;
  int moderate_h = quant; // 11 or 20 are used in original
  //long int amt = 0;  

  if (quant < 1 || quant > 30) return;

  y = 0;

  /* loop over image's pixel rows , four at a time */
  for (y = 0; y < height; y += 4)
  {

    /* loop over every block boundary in that row */
    for (x = 8; x < width; x += 8)
    {

      /* v points to pixel v0, in the left-hand block */
      v = &(image[y * stride + x]) - 5;

      //      if (image[y*stride + x]   > 192 || image[y*stride + x]   < 32) continue;
      //      if (image[y*stride + x+1] > 192 || image[y*stride + x+1] < 32) continue;
      //      if (image[y*stride + x+2] > 192 || image[y*stride + x+2] < 32) continue;
      //      if (image[y*stride + x+3] > 192 || image[y*stride + x+3] < 32) continue;
      //      if (image[y*stride + x+4] > 192 || image[y*stride + x+4] < 32) continue;
      //      if (image[y*stride + x+5] > 192 || image[y*stride + x+5] < 32) continue;
      //      if (image[y*stride + x+6] > 192 || image[y*stride + x+6] < 32) continue;
      //      if (image[y*stride + x+7] > 192 || image[y*stride + x+7] < 32) continue;

            /* first decide whether to use default or DC offset mode */
      useDC = deblock_horiz_useDC(v, stride, moderate_h);   // OLD WORKING
      useDC = 0; //deblock_horiz_useDC(v, stride, moderate_h);

      if (useDC) /* use DC offset mode */
      {
        //continue;
        DC_on = deblock_horiz_DC_on(v, stride, quant);

        if (DC_on)
        {
          deblock_horiz_lpf9(v, stride, quant);
        }
      }
      else  /* use default mode */
      {
        deblock_horiz_default_filter(v, stride, quant);
      }
      //amt += deblock_horiz_default_filter(v, stride, quant);
      //deblock_horiz_lpf9(v, stride, quant); 
    }
  }
#ifdef ARCH_IS_IA32
  _mm_empty();
#endif

  return;
}



/*
 * This code is a horizontal deblocking filter - i.e. it will smooth _vertical_ block edges
 * which means that if you do a diff before and after running the filter you will see
 * the changes as vertical lines.
 */
void deblock_horiz_DoDC(uint8_t* image, int height, int width, int stride, int quant) {
  int x, y;
  uint8_t* v;
  int useDC, DC_on;
  int moderate_h = quant; // 11 or 20 are used in original
  //long int amt = 0;  

  if (quant < 1 || quant > 30) return;

  y = 0;

  /* loop over image's pixel rows , four at a time */
  for (y = 0; y < height; y += 4)
  {

    /* loop over every block boundary in that row */
    for (x = 8; x < width; x += 8)
    {

      /* v points to pixel v0, in the left-hand block */
      v = &(image[y * stride + x]) - 5;

      //      if (image[y*stride + x]   > 192 || image[y*stride + x]   < 32) continue;
      //      if (image[y*stride + x+1] > 192 || image[y*stride + x+1] < 32) continue;
      //      if (image[y*stride + x+2] > 192 || image[y*stride + x+2] < 32) continue;
      //      if (image[y*stride + x+3] > 192 || image[y*stride + x+3] < 32) continue;
      //      if (image[y*stride + x+4] > 192 || image[y*stride + x+4] < 32) continue;
      //      if (image[y*stride + x+5] > 192 || image[y*stride + x+5] < 32) continue;
      //      if (image[y*stride + x+6] > 192 || image[y*stride + x+6] < 32) continue;
      //      if (image[y*stride + x+7] > 192 || image[y*stride + x+7] < 32) continue;

            /* first decide whether to use default or DC offet mode */
      useDC = deblock_horiz_useDC(v, stride, moderate_h);   // OLD WORKING
      //useDC = 0; //deblock_horiz_useDC(v, stride, moderate_h);

      if (useDC) /* use DC offset mode */
      {
        //continue;
        DC_on = deblock_horiz_DC_on(v, stride, quant);

        if (DC_on)
        {
          deblock_horiz_lpf9(v, stride, quant);
        }
      }
      else  /* use default mode */
      {
        deblock_horiz_default_filter(v, stride, quant);
      }
      //amt += deblock_horiz_default_filter(v, stride, quant);
      //deblock_horiz_lpf9(v, stride, quant); 
    }
  }
#ifdef ARCH_IS_IA32
  _mm_empty();
#endif

  return;
}



/* this is a vertical deblocking filter - i.e. it will smooth _horizontal_ block edges */
// ORIGINAL void deblock_horiz(uint8_t *image,             int width, int stride,                  QP_STORE_T *QP_store, int QP_stride, int chromaFlag, int moderate_h)
// ORIGINAL void deblock_vert( uint8_t *image,             int width, int stride,  int moderate_v, QP_STORE_T *QP_store, int QP_stride, int chromaFlag) 
//      NEW void deblock_vert( uint8_t *image, int height, int width, int width,   int quant)   chromaFlag=0 
// chromaFlag set to NO CHROMA
// moderate_v becomes quant
//void deblock_vert( uint8_t *image, int height, int width, int stride, QP_STORE_T *QP_store, int QP_stride, int chromaFlag, int quant) 
//void deblock_vert( uint8_t *image, int height, int width, int stride, int quant, int T2) 
void deblock_vert_DoDC(uint8_t* image, int height, int width, int stride, int quant)
{
  uint64_t v_local[20];
  uint64_t p1p2[4];
  int Bx, x, y;
  int QP; // , QPx16;
  uint8_t* v;
  int useDC, DC_on;
  int moderate_v = quant;
  //  int dx;
  QP = quant;
  //  #ifdef PREFETCH_AHEAD_V
  //  void *prefetch_addr;
  //  #endif

  if (quant < 1 || quant > 30) return;

  y = 0;

  /* loop over image's block boundary rows */
  for (y = 8; y < height; y += 8) {

    /* loop over all blocks, left to right */
    for (Bx = 0; Bx < width; Bx += 8)
    {
      //      QP = chromaFlag == 1 ? QP_store[y/8*QP_stride+Bx/8]
      //                  : chromaFlag == 0 ? QP_store[y/16*QP_stride+Bx/16]
      //            : QP_store[y/16*QP_stride+Bx/8];  
      //      QPx16 = 16 * QP;
      v = &(image[y * stride + Bx]) - 5 * stride;

      //      #ifdef PREFETCH_AHEAD_V
      //      /* try a prefetch PREFETCH_AHEAD_V bytes ahead on all eight rows... experimental */
      //      prefetch_addr = v + PREFETCH_AHEAD_V;
      //      __asm 
      //      {
      //        push eax
      //        push ebx
      //        mov eax, prefetch_addr
      //        mov ebx, stride
      //        add      eax , ebx        /* prefetch_addr+= stride */
      //        prefetcht0 [eax]           
      //        add      eax , ebx        /* prefetch_addr+= stride */
      //        prefetcht0 [eax]           
      //        add      eax , ebx        /* prefetch_addr+= stride */
      //        prefetcht0 [eax]           
      //        add      eax , ebx        /* prefetch_addr+= stride */
      //        prefetcht0 [eax]           
      //        add      eax , ebx        /* prefetch_addr+= stride */
      //        prefetcht0 [eax]           
      //        add      eax , ebx        /* prefetch_addr+= stride */
      //        prefetcht0 [eax]           
      //        add      eax , ebx        /* prefetch_addr+= stride */
      //        prefetcht0 [eax]           
      //        add      eax , ebx        /* prefetch_addr+= stride */
      //        prefetcht0 [eax]           
      //        pop ebx
      //        pop eax
      //      };
      //      #endif

            /* decide whether to use DC mode on a block-by-block basis */
      useDC = deblock_vert_useDC(v, stride, moderate_v);

      //if (T2==141) useDC=0;
      //if (T2==142) useDC=1;

      if (useDC)
      {
        /* we are in DC mode for this block.  But we only want to filter low-energy areas */

        /* decide whether the filter should be on or off for this block */
        DC_on = deblock_vert_DC_on(v, stride, QP);
        //DC_on = 1; //deblock_vert_DC_on(v, stride, QP);
//if (T2==143) DC_on=1;

        /* use DC offset mode */
        if (DC_on)
        {
          v = &(image[y * stride + Bx]) - 5 * stride;

          /* copy the block we're working on and unpack to 16-bit values */
          //deblock_vert_copy_and_unpack_c(stride, &(v[stride]), &(v_local[2]), 8); // test
          deblock_vert_copy_and_unpack(stride, &(v[stride]), &(v_local[2]), 8);

          deblock_vert_choose_p1p2(v, stride, p1p2, QP);

          deblock_vert_lpf9(v_local, p1p2, v, stride);
        }
      }

      if (!useDC) /* use the default filter */
      {

        ///* loop over every column of pixels crossing that horizontal boundary */
//        for (dx=0; dx<8; dx++) {

        x = Bx;// + dx;
        //x = Bx + dx;
        v = &(image[y * stride + x]) - 5 * stride;

        deblock_vert_default_filter(v, stride, QP);

        //        }
      }
    }
  }
#ifdef ARCH_IS_IA32
  _mm_empty();
#endif
}

///* NEW VERSION
///* this is a vertical deblocking filter - i.e. it will smooth _horizontal_ block edges */
//void deblock_vert( uint8_t *image, int height, int width, int stride, int quant) {
//  int Bx, x, y, dx;
//  uint8_t *v;
//  uint64_t v_local[20];
//  uint64_t p1p2[4];
//  y = 0;
//  
//  /* loop over image's block boundary rows */
//  for (y=8; y<height; y+=8) {  
//    
//    /* loop over all blocks, left to right */
//    for (Bx=0; Bx<width; Bx+=8) 
//    {
//      v = &(image[y*stride + Bx]) - 5*stride;
//
//        ///* loop over every column of pixels crossing that horizontal boundary */
//        for (dx=0; dx<8; dx++) {
//    
//          //x = Bx;// + dx;
//          x = Bx + dx;
//          v = &(image[y*stride + x])- 5*stride;
//      
//          //deblock_vert_default_filter(v, stride, quant);
//
//deblock_vert_lpf9(v, p1p2, v, stride); 
////deblock_vert_lpf9(uint64_t *v_local, uint64_t *p1p2, uint8_t *v, int stride);
//
//        }
//      }
//    } 
//  _mm_empty()
//
//}


// THIS IS THE WORKING COPY !!!!!!!!!!!!!!!!!!!!!!!!!!!
/* this is a vertical deblocking filter - i.e. it will smooth _horizontal_ block edges */
//
void deblock_vert(uint8_t* image, int height, int width, int stride, int quant) {
  //void deblock_vert_old( uint8_t *image, int height, int width, int stride, int quant) {
    //int Bx, x, y; //, dx;
  int Bx, x, y; // , dx;
  uint8_t* v;

  if (quant < 1 || quant > 30) return;

  y = 0;

  /* loop over image's block boundary rows */
  for (y = 8; y < height; y += 8) {

    /* loop over all blocks, left to right */
    for (Bx = 0; Bx < width; Bx += 8)
    {
      v = &(image[y * stride + Bx]) - 5 * stride;

      ///* loop over every column of pixels crossing that horizontal boundary */
//        for (dx=0; dx<8; dx++) {

      x = Bx;// + dx;
      //          x = Bx + dx;
      v = &(image[y * stride + x]) - 5 * stride;

      deblock_vert_default_filter(v, stride, quant);

      //        }
    }
  }
#ifdef ARCH_IS_IA32
  _mm_empty();
#endif

}


/* decide whether the DC filter should be turned on according to MAX_abs_DctBlkVal */
int deblock_horiz_DC_on(uint8_t* v, int stride, int QP)
{
  int i;
  /* 99% of the time, this test turns out the same as the |max-min| strategy in the standard */
  for (i = 0; i < 4; ++i)
  {
    if (ABS_MACRO(v[0] - v[5]) >= 2 * QP) return false;
    if (ABS_MACRO(v[1] - v[8]) >= 2 * QP) return false;
    if (ABS_MACRO(v[1] - v[4]) >= 2 * QP) return false;
    if (ABS_MACRO(v[2] - v[7]) >= 2 * QP) return false;
    if (ABS_MACRO(v[3] - v[6]) >= 2 * QP) return false;
    v += stride;
  }
  return true;
}


/* horizontal deblocking filter used in default (non-DC) mode */
// QP is strength
int deblock_horiz_default_filter(uint8_t* v, int stride, int QP)
{
  int a3_0, a3_1, a3_2, d;
  int q1, q;
  int y;
  //  int maxPix = 160;
  //  int minPix = 64;
  //  long int amt = 0;  
  //  long int amt1 = 0;  
  //  long int amt2 = 0;  

  for (y = 0; y < 4; y++) {

    //if (v[1] > maxPix || v[2] > maxPix || v[3] > maxPix || v[4] > maxPix || v[5] > maxPix || v[6] > maxPix || v[7] > maxPix || v[8] > maxPix) {
    //    v += stride;
    //    continue;
    //}
    //if (v[1] < minPix || v[2] < minPix || v[3] < minPix || v[4] < minPix || v[5] < minPix || v[6] < minPix || v[7] < minPix || v[8] < minPix) {
    //    v += stride;
    //    continue;
    //}
    q1 = v[4] - v[5];
    q = q1 / 2;
    if (q) {

      a3_0 = q1;
      a3_0 += a3_0 << 2;
      a3_0 = 2 * (v[3] - v[6]) - a3_0;

      /* apply the 'delta' function first and check there is a difference to avoid wasting time */
      if (ABS_MACRO(a3_0) < 8 * QP) {
        a3_1 = v[3] - v[2];
        a3_2 = v[7] - v[8];
        a3_1 += a3_1 << 2;
        a3_2 += a3_2 << 2;
        a3_1 += (v[1] - v[4]) << 1;
        a3_2 += (v[5] - v[8]) << 1;
        d = ABS_MACRO(a3_0) - MIN(ABS_MACRO(a3_1), ABS_MACRO(a3_2));

        //amt1 += abs(d);    
        if (d > 0) { /* energy across boundary is greater than in one or both of the blocks */
          d += d << 2;
          d = (d + 32) >> 6;

          //amt2 += abs(d);
          if (d > 0) {
            //amt2 += abs(d);  
            //d *= SIGN(-a3_0);

            /* clip d in the range 0 ... q */
            if (q > 0) {
              if (a3_0 < 0) {
                //d = d<0 ? 0 : d;
                d = d > q ? q : d;
                //amt2 += abs(d);
                v[4] -= (uint8_t)d;
                v[5] += (uint8_t)d;
              }
            }
            else {
              if (a3_0 > 0) {
                //d = d>0 ? 0 : d;
                d = (-d) < q ? q : (-d);
                //amt2 += abs(d);                
                v[4] -= (uint8_t)d;
                v[5] += (uint8_t)d;
              }
            }
          }
        }
      }
    }

    v += stride;
  }

  //if (amt1 > amt2) amt = amt1 - amt2;
  //if (amt1 < amt2) amt = amt2 - amt1;
  //amt = abs(amt1 - amt2);
  return(1);
}

#ifndef ARCH_IS_X86_64

const static uint64_t mm64_0008 = 0x0008000800080008;
const static uint64_t mm64_0101 = 0x0101010101010101;
static uint64_t mm64_temp;
const static uint64_t mm64_coefs[18] = {
  0x0001000200040006, /* p1 left */ 0x0000000000000001, /* v1 right */
  0x0001000200020004, /* v1 left */ 0x0000000000010001, /* v2 right */
  0x0002000200040002, /* v2 left */ 0x0000000100010002, /* v3 right */
  0x0002000400020002, /* v3 left */ 0x0001000100020002, /* v4 right */
  0x0004000200020001, /* v4 left */ 0x0001000200020004, /* v5 right */
  0x0002000200010001, /* v5 left */ 0x0002000200040002, /* v6 right */
  0x0002000100010000, /* v6 left */ 0x0002000400020002, /* v7 right */
  0x0001000100000000, /* v7 left */ 0x0004000200020001, /* v8 right */
  0x0001000000000000, /* v8 left */ 0x0006000400020001  /* p2 right */
};
static uint32_t mm32_p1p2;

/* The 9-tap low pass filter used in "DC" regions */
/* I'm not sure that I like this implementation any more...! */
void deblock_horiz_lpf9_mmx(uint8_t* v, int stride, int QP)
{
  int y, p1, p2;
  uint64_t* pmm1;  // NOTE THIS HAD BEEN A GLOBAL !!!

#ifdef PP_SELF_CHECK
  uint8_t selfcheck[9];
  int psum;
  uint8_t* vv;
  int i;
#endif

  for (y = 0; y < 4; y++)
  {
    p1 = (ABS_MACRO(v[0 + y * stride] - v[1 + y * stride]) < QP) ? v[0 + y * stride] : v[1 + y * stride];
    p2 = (ABS_MACRO(v[8 + y * stride] - v[9 + y * stride]) < QP) ? v[9 + y * stride] : v[8 + y * stride];

    mm32_p1p2 = 0x0101 * ((p2 << 16) + p1);

#ifdef PP_SELF_CHECK
    /* generate a self-check version of the filter result in selfcheck[9] */
    /* low pass filtering (LPF9: 1 1 2 2 4 2 2 1 1) */
    vv = &(v[y * stride]);
    psum = p1 + p1 + p1 + vv[1] + vv[2] + vv[3] + vv[4] + 4;
    selfcheck[1] = (((psum + vv[1]) << 1) - (vv[4] - vv[5])) >> 4;
    psum += vv[5] - p1;
    selfcheck[2] = (((psum + vv[2]) << 1) - (vv[5] - vv[6])) >> 4;
    psum += vv[6] - p1;
    selfcheck[3] = (((psum + vv[3]) << 1) - (vv[6] - vv[7])) >> 4;
    psum += vv[7] - p1;
    selfcheck[4] = (((psum + vv[4]) << 1) + p1 - vv[1] - (vv[7] - vv[8])) >> 4;
    psum += vv[8] - vv[1];
    selfcheck[5] = (((psum + vv[5]) << 1) + (vv[1] - vv[2]) - vv[8] + p2) >> 4;
    psum += p2 - vv[2];
    selfcheck[6] = (((psum + vv[6]) << 1) + (vv[2] - vv[3])) >> 4;
    psum += p2 - vv[3];
    selfcheck[7] = (((psum + vv[7]) << 1) + (vv[3] - vv[4])) >> 4;
    psum += p2 - vv[4];
    selfcheck[8] = (((psum + vv[8]) << 1) + (vv[4] - vv[5])) >> 4;
#endif

    pmm1 = (uint64_t*)(&(v[y * stride - 3])); /* this is 64-aligned */


    /* mm7 = 0, mm6 is left hand accumulator, mm5 is right hand acc */
    __asm
    {
      push eax
      push ebx
      mov eax, pmm1
      lea ebx, mm64_coefs

#ifdef PREFETCH_ENABLE
      prefetcht0 32[ebx]
#endif

      movd   mm0, mm32_p1p2            /* mm0 = ________p2p2p1p1    0w1 2 3 4 5 6 7    */
      punpcklbw mm0, mm0                 /* mm0 = p2p2p2p2p1p1p1p1    0m1 2 3 4 5 6 7    */

      movq    mm2, qword ptr[eax]       /* mm2 = v4v3v2v1xxxxxxxx    0 1 2w3 4 5 6 7    */
      pxor    mm7, mm7                   /* mm7 = 0000000000000000    0 1 2 3 4 5 6 7w   */

      movq     mm6, mm64_0008            /* mm6 = 0008000800080008    0 1 2 3 4 5 6w7    */
      punpckhbw mm2, mm2                 /* mm2 = v4__v3__v2__v1__    0 1 2m3 4 5 6 7    */

      movq     mm64_temp, mm0            /*temp = p2p2p2p2p1p1p1p1    0r1 2 3 4 5 6 7    */

      punpcklbw mm0, mm7                 /* mm0 = __p1__p1__p1__p1    0m1 2 3 4 5 6 7    */
      movq      mm5, mm6                 /* mm5 = 0008000800080008    0 1 2 3 4 5w6r7    */

      pmullw    mm0, [ebx]               /* mm0 *= mm64_coefs[0]      0m1 2 3 4 5 6 7    */

      movq      mm1, mm2                 /* mm1 = v4v4v3v3v2v2v1v1    0 1w2r3 4 5 6 7    */
      punpcklbw mm2, mm2                 /* mm2 = v2v2v2v2v1v1v1v1    0 1 2m3 4 5 6 7    */

      punpckhbw mm1, mm1                 /* mm1 = v4v4v4v4v3v3v3v3    0 1m2 3 4 5 6 7    */

#ifdef PREFETCH_ENABLE
      prefetcht0 32[ebx]
#endif

      movq      mm3, mm2                /* mm3 = v2v2v2v2v1v1v1v1    0 1 2r3w4 5 6 7    */
      punpcklbw mm2, mm7                /* mm2 = __v1__v1__v1__v1    0 1 2m3 4 5 6 7    */

      punpckhbw mm3, mm7                /* mm3 = __v2__v2__v2__v2    0 1 2 3m4 5 6 7    */
      paddw     mm6, mm0                /* mm6 += mm0                0r1 2 3 4 5 6m7    */

      movq      mm0, mm2                /* mm0 = __v1__v1__v1__v1    0w1 2r3 4 5 6 7    */

      pmullw    mm0, 8[ebx]             /* mm2 *= mm64_coefs[1]      0m1 2 3 4 5 6 7    */
      movq      mm4, mm3                /* mm4 = __v2__v2__v2__v2    0 1 2 3r4w5 6 7    */

      pmullw    mm2, 16[ebx]            /* mm2 *= mm64_coefs[2]      0 1 2m3 4 5 6 7    */

      pmullw    mm3, 32[ebx]            /* mm3 *= mm64_coefs[4]      0 1 2 3m4 5 6 7    */

      pmullw    mm4, 24[ebx]            /* mm3 *= mm64_coefs[3]      0 1 2 3 4m5 6 7    */
      paddw     mm5, mm0                /* mm5 += mm0                0r1 2 3 4 5m6 7    */

      paddw     mm6, mm2                /* mm6 += mm2                0 1 2r3 4 5 6m7    */
      movq      mm2, mm1                /* mm2 = v4v4v4v4v3v3v3v3    0 1 2 3 4 5 6 7    */

      punpckhbw mm2, mm7                /* mm2 = __v4__v4__v4__v4    0 1 2m3 4 5 6 7r   */
      paddw     mm5, mm4                /* mm5 += mm4                0 1 2 3 4r5m6 7    */

      punpcklbw mm1, mm7                /* mm1 = __v3__v3__v3__v3    0 1m2 3 4 5 6 7r   */
      paddw     mm6, mm3                /* mm6 += mm3                0 1 2 3r4 5 6m7    */

#ifdef PREFETCH_ENABLE
      prefetcht0 64[ebx]
#endif
      movq      mm0, mm1                /* mm0 = __v3__v3__v3__v3    0w1 2 3 4 5 6 7    */

      pmullw    mm1, 48[ebx]            /* mm1 *= mm64_coefs[6]      0 1m2 3 4 5 6 7    */

      pmullw    mm0, 40[ebx]            /* mm0 *= mm64_coefs[5]      0m1 2 3 4 5 6 7    */
      movq      mm4, mm2                /* mm4 = __v4__v4__v4__v4    0 1 2r3 4w5 6 7    */

      pmullw    mm2, 64[ebx]            /* mm2 *= mm64_coefs[8]      0 1 2 3 4 5 6 7    */
      paddw     mm6, mm1                /* mm6 += mm1                0 1 2 3 4 5 6 7    */

      pmullw    mm4, 56[ebx]            /* mm4 *= mm64_coefs[7]      0 1 2 3 4m5 6 7    */
      pxor      mm3, mm3                /* mm3 = 0000000000000000    0 1 2 3w4 5 6 7    */

      movq      mm1, 8[eax]             /* mm1 = xxxxxxxxv8v7v6v5    0 1w2 3 4 5 6 7    */
      paddw     mm5, mm0                /* mm5 += mm0                0r1 2 3 4 5 6 7    */

      punpcklbw mm1, mm1                /* mm1 = v8v8v7v7v6v6v5v5    0 1m2 3m4 5 6 7    */
      paddw     mm6, mm2                /* mm6 += mm2                0 1 2r3 4 5 6 7    */

#ifdef PREFETCH_ENABLE
      prefetcht0 96[ebx]
#endif

      movq      mm2, mm1                /* mm2 = v8v8v7v7v6v6v5v5    0 1r2w3 4 5 6 7    */
      paddw     mm5, mm4                /* mm5 += mm4                0 1 2 3 4r5 6 7    */

      punpcklbw mm2, mm2                /* mm2 = v6v6v6v6v5v5v5v5    0 1 2m3 4 5 6 7    */
      punpckhbw mm1, mm1                /* mm1 = v8v8v8v8v7v7v7v7    0 1m2 3 4 5 6 7    */

      movq      mm3, mm2                /* mm3 = v6v6v6v6v5v5v5v5    0 1 2r3w4 5 6 7    */
      punpcklbw mm2, mm7                /* mm2 = __v5__v5__v5__v5    0 1 2m3 4 5 6 7r   */

      punpckhbw mm3, mm7                /* mm3 = __v6__v6__v6__v6    0 1 2 3m4 5 6 7r   */
      movq      mm0, mm2                /* mm0 = __v5__v5__v5__v5    0w1 2b3 4 5 6 7    */

      pmullw    mm0, 72[ebx]            /* mm0 *= mm64_coefs[9]      0m1 2 3 4 5 6 7    */
      movq      mm4, mm3                /* mm4 = __v6__v6__v6__v6    0 1 2 3 4w5 6 7    */

      pmullw    mm2, 80[ebx]            /* mm2 *= mm64_coefs[10]     0 1 2m3 4 5 6 7    */

      pmullw    mm3, 96[ebx]            /* mm3 *= mm64_coefs[12]     0 1 2 3m4 5 6 7    */

      pmullw    mm4, 88[ebx]            /* mm4 *= mm64_coefs[11]     0 1 2 3 4m5 6 7    */
      paddw     mm5, mm0                /* mm5 += mm0                0r1 2 3 4 5 6 7    */

      paddw     mm6, mm2                /* mm6 += mm2                0 1 2r3 4 5 6 7    */
      movq      mm2, mm1                /* mm2 = v8v8v8v8v7v7v7v7    0 1r2w3 4 5 6 7    */

      paddw     mm6, mm3                /* mm6 += mm3                0 1 2 3r4 5 6 7    */
      punpcklbw mm1, mm7                /* mm1 = __v7__v7__v7__v7    0 1m2 3 4 5 6 7r   */

      paddw     mm5, mm4                /* mm5 += mm4                0 1 2 3 4r5 6 7    */
      punpckhbw mm2, mm7                /* mm2 = __v8__v8__v8__v8    0 1 2m3 4 5 6 7    */

#ifdef PREFETCH_ENABLE
      prefetcht0 128[ebx]
#endif

      movq      mm3, mm64_temp          /* mm0 = p2p2p2p2p1p1p1p1    0 1 2 3w4 5 6 7    */
      movq      mm0, mm1                /* mm0 = __v7__v7__v7__v7    0w1r2 3 4 5 6 7    */

      pmullw    mm0, 104[ebx]           /* mm0 *= mm64_coefs[13]     0m1b2 3 4 5 6 7    */
      movq      mm4, mm2                /* mm4 = __v8__v8__v8__v8    0 1 2r3 4w5 6 7    */

      pmullw    mm1, 112[ebx]           /* mm1 *= mm64_coefs[14]     0 1w2 3 4 5 6 7    */
      punpckhbw mm3, mm7                /* mm0 = __p2__p2__p2__p2    0 1 2 3 4 5 6 7    */

      pmullw    mm2, 128[ebx]           /* mm2 *= mm64_coefs[16]     0 1b2m3 4 5 6 7    */

      pmullw    mm4, 120[ebx]           /* mm4 *= mm64_coefs[15]     0 1b2 3 4m5 6 7    */
      paddw     mm5, mm0                /* mm5 += mm0                0r1 2 3 4 5m6 7    */

      pmullw    mm3, 136[ebx]           /* mm0 *= mm64_coefs[17]     0 1 2 3m4 5 6 7    */
      paddw     mm6, mm1                /* mm6 += mm1                0 1w2 3 4 5 6m7    */

      paddw     mm6, mm2                /* mm6 += mm2                0 1 2r3 4 5 6m7    */

      paddw     mm5, mm4                /* mm5 += mm4                0 1 2 3 4r5m6 7    */
      psrlw     mm6, 4                  /* mm6 /= 16                 0 1 2 3 4 5 6m7    */

      paddw     mm5, mm3                /* mm6 += mm0                0 1 2 3r4 5m6 7    */

      psrlw     mm5, 4                  /* mm5 /= 16                 0 1 2 3 4 5m6 7    */

      packuswb  mm6, mm5                /* pack result into mm6      0 1 2 3 4 5r6m7    */

      movq      4[eax], mm6             /* v[] = mm6                 0 1 2 3 4 5 6r7    */

      pop ebx
      pop eax
    };

  }
  __asm emms
}
#endif

void deblock_horiz_lpf9(uint8_t* v, int stride, int QP) {
#ifdef USE_NEW_INTRINSICS
  deblock_horiz_lpf9_ssse3(v, stride, QP);
#else
  deblock_horiz_lpf9_mmx(v, stride, QP);
#endif
  // deblock_horiz_lpf9_c(v, stride, QP);
}

/* The 9-tap low pass filter used in "DC" regions */
/* I'm not sure that I like this implementation any more...! */
void deblock_horiz_lpf9_c(uint8_t* v, int stride, int QP) {
  uint8_t temp[16];
  for (int y = 0; y < 4; y++) {
    // Calculate p1 and p2 based on QP threshold
    int p1 = (abs(v[0 + y * stride] - v[1 + y * stride]) < QP) ?
      v[0 + y * stride] : v[1 + y * stride];

    int p2 = (abs(v[8 + y * stride] - v[9 + y * stride]) < QP) ?
      v[9 + y * stride] : v[8 + y * stride];

    uint8_t selfcheck[9];
    int psum;
    uint8_t* vv;
    int i;
    /* put the result into temp buffer in selfcheck[9], 1-8 indexes go back to v[1..8] */
    /* low pass filtering (LPF9: 1 1 2 2 4 2 2 1 1) */
    vv = &(v[y * stride]);

    // used inputs: v[0] to v[9]
    // p1 is decided by v[0] and v[1]
    // p2 is decided by v[8] and v[9]
    /*
    Used from by weights above
      v[1] calculated from:  p1, p1, p1, p1, v1, v2, v3, v4, v5
      v[2] calculated from:  p1, p1, p1, v1, v2, v3, v4, v5, v6
      v[3] calculated from:  p1, p1, v1, v2, v3, v4, v5, v6, v7
      v[4] calculated from:  p1, v1, v2, v3, v4, v5, v6, v7, v8
      v[5] calculated from:  v1, v2, v3, v4, v5, v6, v7, v8, p2
      v[6] calculated from:  v2, v3, v4, v5, v6, v7, v8, p2, p2
      v[7] calculated from:  v3, v4, v5, v6, v7, v8, p2, p2, p2
      v[8] calculated from:  v4, v5, v6, v7, v8, p2, p2, p2, p2
    */
    // When index would go before v[1] then p1 is used there.
    // When index would go after v[8] then p2 is used there.
    // Weights for LowPassFilter9: 1 1 2 2 4 2 2 1 1

    psum = p1 + p1 + p1 + vv[1] + vv[2] + vv[3] + vv[4] + 4;
    selfcheck[1] = (((psum + vv[1]) << 1) - (vv[4] - vv[5])) >> 4; // 1*p1 + 1*p1 + 2*p1 + 2*p1 + 4*v[1] + 2*v[2] + 2*v[3] + 1*v[4] + 1*v[4]
    // moving window. Add next on the right: vv[5], subtract one filler from the left: p1, modify the weights by adding or subtracting the appropriate v[] element
    // and so on...
    psum += vv[5] - p1;
    selfcheck[2] = (((psum + vv[2]) << 1) - (vv[5] - vv[6])) >> 4;
    psum += vv[6] - p1;
    selfcheck[3] = (((psum + vv[3]) << 1) - (vv[6] - vv[7])) >> 4;
    psum += vv[7] - p1;
    selfcheck[4] = (((psum + vv[4]) << 1) + p1 - vv[1] - (vv[7] - vv[8])) >> 4;
    psum += vv[8] - vv[1];
    selfcheck[5] = (((psum + vv[5]) << 1) + (vv[1] - vv[2]) - vv[8] + p2) >> 4;
    psum += p2 - vv[2];
    selfcheck[6] = (((psum + vv[6]) << 1) + (vv[2] - vv[3])) >> 4;
    psum += p2 - vv[3];
    selfcheck[7] = (((psum + vv[7]) << 1) + (vv[3] - vv[4])) >> 4;
    psum += p2 - vv[4];
    selfcheck[8] = (((psum + vv[8]) << 1) + (vv[4] - vv[5])) >> 4;

    for (int x = 1; x < 9; x++) {
      v[x + y * stride] = selfcheck[x];
    }
  }
}

/* The 9-tap low pass filter used in "DC" regions */
// intrinsic rewrite by pinterf
// FIXME: not very optimal do it like in C and in deblock_vert_lpf9
void deblock_horiz_lpf9_ssse3(uint8_t* v, int stride, int QP) {
  for (int y = 0; y < 4; y++) {
    // Calculate p1 and p2 based on QP threshold
    int p1 = (abs(v[0 + y * stride] - v[1 + y * stride]) < QP) ? v[0 + y * stride] : v[1 + y * stride];
    int p2 = (abs(v[8 + y * stride] - v[9 + y * stride]) < QP) ? v[9 + y * stride] : v[8 + y * stride];

    uint8_t* vv = &(v[y * stride]);

    // Create extended pixel vector: p1,p1,p1,p1, v1-v8, p2,p2,p2,p2
    __m128i pixels = _mm_set_epi8(
      p2, p2, p2, p2,                   // Last 4 bytes
      vv[8], vv[7], vv[6], vv[5],       // Second half of input
      vv[4], vv[3], vv[2], vv[1],       // First half of input
      p1, p1, p1, p1                    // First 4 bytes
    );

    // Fixed weights vector: 1,1,2,2,4,2,2,1,1 (padded with zeros)
    const __m128i weights = _mm_set_epi8(
      0, 0, 0, 0, 0, 0, 0,
      1,                    // Weight for last position
      1, 2, 2, 4, 2, 2, 1, 1  // Weights for 9-point filter
    );

    // Results array for all 8 positions
    uint8_t results[8];

    // Process all 8 positions in a loop
    for (int x = 0; x < 8; x++) {
      // ssse3 unsigned * signed bytes to 16 bit signed, pre hadd-ed
      __m128i mult = _mm_maddubs_epi16(pixels, weights); // SSSE3
      // S7 S6 S5 S4 S3 S2 S1 S0
      //  0  0  0 S4 S3 S2 S1 S0 
      __m128i hadd = _mm_hadd_epi16(mult, mult); // SSSE3
      hadd = _mm_hadd_epi16(hadd, hadd);
      hadd = _mm_hadd_epi16(hadd, hadd);
      results[x] = (uint8_t)((_mm_extract_epi16(hadd, 0) + 8) >> 4); // round and /= 16
      pixels = _mm_srli_si128(pixels, 1);
    }

    // Store results back to memory
    for (int x = 1; x < 9; x++) {
      v[x + y * stride] = results[x - 1];
    }
  }
}



#define ABS_MACRO(X)  (((X)>0)?(X) : 0-(X))

// PF converted to C from self check
/* decide DC mode or default mode for the horizontal filter */
int deblock_horiz_useDC_c(uint8_t* v, int stride, int moderate_h)
{
  int useDC;

  int eq_cnt2, j, k;

  eq_cnt2 = 0;
  for (k = 0; k < 4; k++)
  {
    for (j = 1; j <= 7; j++)
    {
      if (ABS_MACRO(v[j + k * stride] - v[1 + j + k * stride]) <= 1) eq_cnt2++;
    }
  }

  useDC = eq_cnt2 >= moderate_h;

  return useDC;
}

#ifdef USE_NEW_INTRINSICS
// written based on C (sse4.2)
int deblock_horiz_useDC(uint8_t* v, int stride, int moderate_h) {
  int useDC;
  int eq_cnt2 = 0;

  __m128i one = _mm_set1_epi8(1);
  __m128i zero = _mm_setzero_si128();

  for (int k = 0; k < 4; k++) {
    for (int j = 1; j <= 7; j++) {
      __m128i current = _mm_loadu_si128((__m128i*) & v[j + k * stride]);
      __m128i next = _mm_loadu_si128((__m128i*) & v[1 + j + k * stride]);

      __m128i diff = _mm_subs_epu8(current, next);
      __m128i abs_diff = _mm_max_epu8(diff, _mm_subs_epu8(zero, diff));

      __m128i cmp = _mm_cmpeq_epi8(_mm_min_epu8(abs_diff, one), abs_diff);
      eq_cnt2 += _mm_popcnt_u32(_mm_movemask_epi8(cmp));
    }
  }

  useDC = eq_cnt2 >= moderate_h;

  return useDC;
}
#else
// FIXME PF converted to intrinsics
/* decide DC mode or default mode for the horizontal filter */
int deblock_horiz_useDC(uint8_t* v, int stride, int moderate_h)
{
  const uint64_t mm64_mask = 0x00fefefefefefefe;
  uint32_t mm32_result;
  uint64_t* pmm1;
  int eq_cnt, useDC;

#ifdef PP_SELF_CHECK
  int eq_cnt2, j, k;
#endif

  pmm1 = (uint64_t*)(&(v[1])); /* this is a 32-bit aligned pointer, not 64-aligned */

  __asm
  {
    push eax
    mov eax, pmm1

    /* first load some constants into mm4, mm5, mm6, mm7 */
    movq mm6, mm64_mask          /*mm6 = 0x00fefefefefefefe       */
    pxor mm4, mm4                /*mm4 = 0x0000000000000000       */

    movq mm1, qword ptr[eax]    /* mm1 = *pmm            0 1 2 3 4 5 6 7    */
    add eax, stride              /* eax += stride/8      0 1 2 3 4 5 6 7    */

    movq mm5, mm1                /* mm5 = mm1             0 1 2 3 4 5 6 7    */
    psrlq mm1, 8                 /* mm1 >>= 8             0 1 2 3 4 5 6 7    */

    movq mm2, mm5                /* mm2 = mm5             0 1 2 3 4 5 6 7    */
    psubusb mm5, mm1             /* mm5 -= mm1            0 1 2 3 4 5 6 7    */

    movq mm3, qword ptr[eax]    /* mm3 = *pmm            0 1 2 3 4 5 6 7    */
    psubusb mm1, mm2             /* mm1 -= mm2            0 1 2 3 4 5 6 7    */

    add eax, stride              /* eax += stride/8      0 1 2 3 4 5 6 7    */
    por mm5, mm1                 /* mm5 |= mm1            0 1 2 3 4 5 6 7    */

    movq mm0, mm3                /* mm0 = mm3             0 1 2 3 4 5 6 7    */
    pand mm5, mm6                /* mm5 &= 0xfefefefefefefefe     */

    pxor mm7, mm7                /*mm7 = 0x0000000000000000       */
    pcmpeqb mm5, mm4             /* are the bytes of mm5 == 0 ?   */

    movq mm1, qword ptr[eax]    /* mm3 = *pmm            0 1 2 3 4 5 6 7    */
    psubb mm7, mm5               /* mm7 has running total of eqcnts */

    psrlq mm3, 8                 /* mm3 >>= 8             0 1 2 3 4 5 6 7    */
    movq mm5, mm0                /* mm5 = mm0             0 1 2 3 4 5 6 7    */

    psubusb mm0, mm3             /* mm0 -= mm3            0 1 2 3 4 5 6 7    */

    add eax, stride              /* eax += stride/8      0 1 2 3 4 5 6 7    */
    psubusb mm3, mm5             /* mm3 -= mm5            0 1 2 3 4 5 6 7    */

    movq mm5, qword ptr[eax]    /* mm5 = *pmm            0 1 2 3 4 5 6 7    */
    por mm0, mm3                 /* mm0 |= mm3            0 1 2 3 4 5 6 7    */

    movq mm3, mm1                /* mm3 = mm1             0 1 2 3 4 5 6 7    */
    pand mm0, mm6                /* mm0 &= 0xfefefefefefefefe     */

    psrlq   mm1, 8               /* mm1 >>= 8             0 1 2 3 4 5 6 7    */
    pcmpeqb mm0, mm4             /* are the bytes of mm0 == 0 ?   */

    movq mm2, mm3                /* mm2 = mm3             0 1 2 3 4 5 6 7    */
    psubb mm7, mm0               /* mm7 has running total of eqcnts */

    psubusb mm3, mm1             /* mm3 -= mm1            0 1 2 3 4 5 6 7    */

    psubusb mm1, mm2             /* mm1 -= mm2            0 1 2 3 4 5 6 7    */

    por mm3, mm1                 /* mm3 |= mm1            0 1 2 3 4 5 6 7    */
    movq mm1, mm5                /* mm1 = mm5             0 1 2 3 4 5 6 7    */

    pand    mm3, mm6             /* mm3 &= 0xfefefefefefefefe     */
    psrlq   mm5, 8               /* mm5 >>= 8             0 1 2 3 4 5 6 7    */

    pcmpeqb mm3, mm4             /* are the bytes of mm3 == 0 ?   */
    movq    mm0, mm1             /* mm0 = mm1             0 1 2 3 4 5 6 7    */

    psubb   mm7, mm3             /* mm7 has running total of eqcnts */
    psubusb mm1, mm5             /* mm1 -= mm5            0 1 2 3 4 5 6 7    */

    psubusb mm5, mm0             /* mm5 -= mm0            0 1 2 3 4 5 6 7    */
    por     mm1, mm5             /* mm1 |= mm5            0 1 2 3 4 5 6 7    */

    pand    mm1, mm6             /* mm1 &= 0xfefefefefefefefe     */

    pcmpeqb mm1, mm4             /* are the bytes of mm1 == 0 ?   */

    psubb   mm7, mm1             /* mm7 has running total of eqcnts */

    movq    mm1, mm7             /* mm1 = mm7             0 1w2 3 4 5 6 7r   */
    psllq   mm7, 8               /* mm7 >>= 24            0 1 2 3 4 5 6 7m   */

    psrlq   mm1, 24              /* mm7 >>= 24            0 1 2 3 4 5 6 7m   */

    paddb   mm7, mm1             /* mm7 has running total of eqcnts */

    movq mm1, mm7                /* mm1 = mm7             0 1w2 3 4 5 6 7r   */
    psrlq mm7, 16                /* mm7 >>= 16            0 1 2 3 4 5 6 7m   */

    paddb   mm7, mm1             /* mm7 has running total of eqcnts */

    movq mm1, mm7                /* mm1 = mm7             0 1w2 3 4 5 6 7r   */
    psrlq   mm7, 8               /* mm7 >>= 8             0 1 2 3 4 5 6 7m   */

    paddb mm7, mm1               /* mm7 has running total of eqcnts */

    movd mm32_result, mm7

    pop eax

  };

  __asm  emms

  eq_cnt = mm32_result & 0xff;

#ifdef PP_SELF_CHECK
  eq_cnt2 = 0;
  for (k = 0; k < 4; k++)
  {
    for (j = 1; j <= 7; j++)
    {
      if (ABS_MACRO(v[j + k * stride] - v[1 + j + k * stride]) <= 1) eq_cnt2++;
    }
  }
  if (eq_cnt2 != eq_cnt)
    dprintf("ERROR: MMX version of useDC is incorrect\n");
#endif

  useDC = eq_cnt >= moderate_h;

  return useDC;
}


#endif
///////////////////////////////////////////////////////////////////////////////////////
//////////                        DEBLOCK VERTICAL FILTER CODE
///////////////////////////////////////////////////////////////////////////////////////

#ifdef USE_NEW_INTRINSICS
// converted by PF to intrinsics
void deblock_vert_default_filter(uint8_t* v, int stride, int QP) {

  __m128i mm_0020 = _mm_set1_epi16(0x0020);
  __m128i mm_8_x_QP = _mm_set1_epi16(0x0008 * QP);
  __m128i mm7 = _mm_setzero_si128();
  __m128i mm6, mm1;
  __m128i mm0, mm2, mm3, mm4, mm5;
  int i;

  /* working in 4-pixel wide columns, left to right */
  /*i=0 in left, i=1 in right */
  for (i = 0; i < 2; i++) {
    /* v should be 64-bit aligned here */
    uint8_t* pmm1 = &(v[4 * i]);
    /* pmm1 will be 32-bit aligned but this doesn't matter as we'll use movd not movq */

    pmm1 += stride;
    mm0 = _mm_cvtsi32_si128(*(int*)pmm1);
    mm0 = _mm_unpacklo_epi8(mm0, mm7);

    pmm1 += stride;
    mm1 = _mm_cvtsi32_si128(*(int*)pmm1);
    mm1 = _mm_unpacklo_epi8(mm1, mm7);

    pmm1 += stride;
    mm2 = _mm_cvtsi32_si128(*(int*)pmm1);
    mm2 = _mm_unpacklo_epi8(mm2, mm7);

    pmm1 += stride;
    mm3 = _mm_cvtsi32_si128(*(int*)pmm1);
    mm3 = _mm_unpacklo_epi8(mm3, mm7);

    mm1 = _mm_sub_epi16(mm1, mm2);
    mm4 = mm1;
    mm1 = _mm_slli_epi16(mm1, 2);
    mm1 = _mm_add_epi16(mm1, mm4);
    mm0 = _mm_sub_epi16(mm0, mm3);
    mm0 = _mm_slli_epi16(mm0, 1);
    mm0 = _mm_sub_epi16(mm0, mm1);

    mm1 = _mm_setzero_si128();
    mm1 = _mm_cmpgt_epi16(mm1, mm0);
    mm0 = _mm_xor_si128(mm0, mm1);
    mm0 = _mm_sub_epi16(mm0, mm1);

    pmm1 += stride;
    mm1 = _mm_cvtsi32_si128(*(int*)pmm1);
    mm1 = _mm_unpacklo_epi8(mm1, mm7);

    mm3 = _mm_sub_epi16(mm3, mm1);

    pmm1 += stride;
    mm4 = _mm_cvtsi32_si128(*(int*)pmm1);
    mm4 = _mm_unpacklo_epi8(mm4, mm7);

    pmm1 += stride;
    mm5 = _mm_cvtsi32_si128(*(int*)pmm1);
    mm5 = _mm_unpacklo_epi8(mm5, mm7);

    mm2 = _mm_sub_epi16(mm2, mm4);
    mm5 = _mm_sub_epi16(mm5, mm4);
    mm4 = mm5;
    mm4 = _mm_slli_epi16(mm4, 2);
    mm5 = _mm_add_epi16(mm5, mm4);

    pmm1 += stride;
    mm4 = _mm_cvtsi32_si128(*(int*)pmm1);
    mm4 = _mm_unpacklo_epi8(mm4, mm7);

    mm1 = _mm_sub_epi16(mm1, mm4);
    mm1 = _mm_slli_epi16(mm1, 1);
    mm1 = _mm_add_epi16(mm1, mm5);

    mm4 = _mm_setzero_si128();
    mm4 = _mm_cmpgt_epi16(mm4, mm1);
    mm1 = _mm_xor_si128(mm1, mm4);
    mm1 = _mm_sub_epi16(mm1, mm4);
    /* at this point, mm0 = ABS(a3_1), mm1 = ABS(a3_2), mm2 = v3 - v6, mm3 = v4 - v5 */
    mm4 = mm1;
    mm1 = _mm_cmpgt_epi16(mm1, mm0);
    mm0 = _mm_and_si128(mm0, mm1);
    mm1 = _mm_andnot_si128(mm1, mm4);
    mm0 = _mm_or_si128(mm0, mm1);
    /* at this point, mm0 = MIN( ABS(a3_1), ABS(a3_2), mm2 = v3 - v6, mm3 = v4 - v5 */
    mm1 = mm3;
    mm3 = _mm_slli_epi16(mm3, 2);
    mm3 = _mm_add_epi16(mm3, mm1);
    mm2 = _mm_slli_epi16(mm2, 1);
    mm2 = _mm_sub_epi16(mm2, mm3);
    /* at this point, mm0 = MIN( ABS(a3_1), ABS(a3_2), mm1 = v4 - v5, mm2 = a3_0 */
    mm4 = mm2;
    mm3 = _mm_setzero_si128();
    mm3 = _mm_cmpgt_epi16(mm3, mm2);
    mm4 = _mm_xor_si128(mm4, mm3);
    mm4 = _mm_sub_epi16(mm4, mm3);
    /* compare a3_0 against 8*QP */
    mm2 = mm_8_x_QP;
    mm2 = _mm_cmpgt_epi16(mm2, mm4);
    mm2 = _mm_and_si128(mm2, mm4);

    mm4 = mm2;
    /* at this point, mm0 = MIN( ABS(a3_1), ABS(a3_2), mm1 = v4 - v5, mm2 = a3_0 , mm3 = SGN(a3_0), mm4 = ABS(a3_0) */
    mm4 = _mm_subs_epu16(mm4, mm0);
    mm0 = mm4;
    mm0 = _mm_slli_epi16(mm0, 2);
    mm0 = _mm_add_epi16(mm0, mm4);
    mm0 = _mm_add_epi16(mm0, mm_0020);
    mm0 = _mm_srai_epi16(mm0, 6);
    /* at this point, mm0 = ABS(d), mm1 = v4 - v5, mm3 = SGN(a3_0) */
    mm2 = _mm_setzero_si128();
    mm2 = _mm_cmpgt_epi16(mm2, mm1);
    mm1 = _mm_xor_si128(mm1, mm2);
    mm1 = _mm_sub_epi16(mm1, mm2);
    mm1 = _mm_srai_epi16(mm1, 1);
    /* OK at this point, mm0 = ABS(d), mm1 = ABS(q), mm2 = SGN(q), mm3 = SGN(-d) */
    mm4 = mm2;
    mm4 = _mm_xor_si128(mm4, mm3);
    mm5 = mm0;
    mm5 = _mm_cmpgt_epi16(mm5, mm1);
    mm1 = _mm_and_si128(mm1, mm5);
    mm5 = _mm_andnot_si128(mm5, mm0);
    mm1 = _mm_or_si128(mm1, mm5);
    mm1 = _mm_and_si128(mm1, mm4);
    mm1 = _mm_xor_si128(mm1, mm2);
    mm1 = _mm_sub_epi16(mm1, mm2);
    /* at this point we have d in mm1 */
    if (i == 0) {
      mm6 = mm1;
    }
  }
  /* add d to rows l4 and l5 in memory... */
  uint8_t* pmm1 = &(v[4 * stride]);
  mm6 = _mm_packs_epi16(mm6, mm1);
  // Shuffle the packed result to place the second argument's packed values in the correct position
  // since the original MMX case packed the second argument's 0-63 into the target's 32-63 bits
  mm6 = _mm_shuffle_epi32(mm6, _MM_SHUFFLE(3, 1, 2, 0));
  mm0 = _mm_loadl_epi64((__m128i*)pmm1);
  mm0 = _mm_sub_epi8(mm0, mm6);
  _mm_storel_epi64((__m128i*)pmm1, mm0);

  pmm1 += stride;
  mm6 = _mm_add_epi8(mm6, _mm_loadl_epi64((__m128i*)pmm1));
  _mm_storel_epi64((__m128i*)pmm1, mm6);

}
#else
/* Vertical deblocking filter for use in non-flat picture regions */
void deblock_vert_default_filter(uint8_t* v, int stride, int QP)
{
  uint64_t* pmm1;
  const uint64_t mm_0020 = 0x0020002000200020;
  uint64_t mm_8_x_QP;
  int i;



  ((uint32_t*)&mm_8_x_QP)[0] =
    ((uint32_t*)&mm_8_x_QP)[1] = 0x00080008 * QP;

  /* working in 4-pixel wide columns, left to right */
  /*i=0 in left, i=1 in right */
  for (i = 0; i < 2; i++)
  {
    /* v should be 64-bit aligned here */
    pmm1 = (uint64_t*)(&(v[4 * i]));
    /* pmm1 will be 32-bit aligned but this doesn't matter as we'll use movd not movq */

    __asm
    {
      push ecx
      mov ecx, pmm1

      pxor      mm7, mm7               /* mm7 = 0000000000000000    0 1 2 3 4 5 6 7w   */
      add      ecx, stride           /* %0 += stride              0 1 2 3 4 5 6 7    */

      movd      mm0, [ecx]            /* mm0 = v1v1v1v1v1v1v1v1    0w1 2 3 4 5 6 7    */
      punpcklbw mm0, mm7               /* mm0 = __v1__v1__v1__v1 L  0m1 2 3 4 5 6 7r   */

      add      ecx, stride           /* ecx += stride              0 1 2 3 4 5 6 7    */
      movd      mm1, [ecx]            /* mm1 = v2v2v2v2v2v2v2v2    0 1w2 3 4 5 6 7    */

      add      ecx, stride           /* ecx += stride              0 1 2 3 4 5 6 7    */
      punpcklbw mm1, mm7               /* mm1 = __v2__v2__v2__v2 L  0 1m2 3 4 5 6 7r   */

      movd      mm2, [ecx]            /* mm2 = v3v3v3v3v3v3v3v3    0 1 2w3 4 5 6 7    */
      add      ecx, stride           /* ecx += stride              0 1 2 3 4 5 6 7    */

      punpcklbw mm2, mm7               /* mm2 = __v3__v3__v3__v3 L  0 1 2m3 4 5 6 7r   */

      movd      mm3, [ecx]            /* mm3 = v4v4v4v4v4v4v4v4    0 1 2 3w4 5 6 7    */

      punpcklbw mm3, mm7               /* mm3 = __v4__v4__v4__v4 L  0 1 2 3m4 5 6 7r   */

      psubw     mm1, mm2               /* mm1 = v2 - v3          L  0 1m2r3 4 5 6 7    */

      movq      mm4, mm1               /* mm4 = v2 - v3          L  0 1r2 3 4w5 6 7    */
      psllw     mm1, 2                 /* mm1 = 4 * (v2 - v3)    L  0 1m2 3 4 5 6 7    */

      paddw     mm1, mm4               /* mm1 = 5 * (v2 - v3)    L  0 1m2 3 4r5 6 7    */
      psubw     mm0, mm3               /* mm0 = v1 - v4          L  0m1 2 3r4 5 6 7    */

      psllw     mm0, 1                 /* mm0 = 2 * (v1 - v4)    L  0m1 2 3 4 5 6 7    */

      psubw     mm0, mm1               /* mm0 = a3_1             L  0m1r2 3 4 5 6 7    */

      pxor      mm1, mm1               /* mm1 = 0000000000000000    0 1w2 3 4 5 6 7    */

      pcmpgtw   mm1, mm0               /* is 0 > a3_1 ?          L  0r1m2 3 4 5 6 7    */

      add      ecx, stride           /* ecx += stride              0 1 2 3 4 5 6 7    */
      pxor      mm0, mm1               /* mm0 = ABS(a3_1) step 1 L  0m1r2 3 4 5 6 7    */

      psubw     mm0, mm1               /* mm0 = ABS(a3_1) step 2 L  0m1r2 3 4 5 6 7    */

      movd      mm1, [ecx]            /* mm1 = v5v5v5v5v5v5v5v5    0 1w2 3 4 5 6 7    */

      punpcklbw mm1, mm7               /* mm1 = __v5__v5__v5__v5 L  0 1m2 3 4 5 6 7r   */


      add      ecx, stride           /* ecx += stride              0 1 2 3 4 5 6 7    */
      psubw     mm3, mm1               /* mm3 = v4 - v5          L  0 1r2 3m4 5 6 7    */

      movd      mm4, [ecx]            /* mm4 = v6v6v6v6v6v6v6v6    0 1 2 3 4w5 6 7    */

      punpcklbw mm4, mm7               /* mm4 = __v6__v6__v6__v6 L  0 1 2 3 4m5 6 7r   */

      add      ecx, stride           /* ecx += stride              0 1 2 3 4 5 6 7    */

      movd      mm5, [ecx]            /* mm5 = v7v7v7v7v7v7v7v7    0 1 2 3 4 5w6 7    */
      psubw     mm2, mm4               /* mm2 = v3 - v6          L  0 1 2m3 4r5 6 7    */

      punpcklbw mm5, mm7               /* mm5 = __v7__v7__v7__v7 L  0 1 2 3 4 5m6 7r   */

      add      ecx, stride           /* ecx += stride              0 1 2 3 4 5 6 7    */
      psubw     mm5, mm4               /* mm5 = v7 - v6          L  0 1 2 3 4r5m6 7    */

      movq      mm4, mm5               /* mm4 = v7 - v6          L  0 1 2 3 4w5r6 7    */

      psllw     mm4, 2                 /* mm4 = 4 * (v7 - v6)    L  0 1 2 3 4 5m6 7    */

      paddw     mm5, mm4               /* mm5 = 5 * (v7 - v6)    L  0 1 2 3 4r5m6 7    */

      movd      mm4, [ecx]            /* mm4 = v8v8v8v8v8v8v8v8    0 1 2 3 4w5 6 7    */

      punpcklbw mm4, mm7               /* mm4 = __v8__v8__v8__v8 L  0 1 2 3 4m5 6 7r   */

      psubw     mm1, mm4               /* mm1 = v5 - v8          L  0 1m2 3 4r5 6 7    */

      pxor      mm4, mm4               /* mm4 = 0000000000000000    0 1 2 3 4w5 6 7    */
      psllw     mm1, 1                 /* mm1 = 2 * (v5 - v8)    L  0 1m2 3 4 5 6 7    */

      paddw     mm1, mm5               /* mm1 = a3_2             L  0 1m2 3 4 5r6 7    */

      pcmpgtw   mm4, mm1               /* is 0 > a3_2 ?          L  0 1r2 3 4m5 6 7    */

      pxor      mm1, mm4               /* mm1 = ABS(a3_2) step 1 L  0 1m2 3 4r5 6 7    */

      psubw     mm1, mm4               /* mm1 = ABS(a3_2) step 2 L  0 1m2 3 4r5 6 7    */

      /* at this point, mm0 = ABS(a3_1), mm1 = ABS(a3_2), mm2 = v3 - v6, mm3 = v4 - v5 */
      movq      mm4, mm1               /* mm4 = ABS(a3_2)        L  0 1r2 3 4w5 6 7    */

      pcmpgtw   mm1, mm0               /* is ABS(a3_2) > ABS(a3_1)  0r1m2 3 4 5 6 7    */

      pand      mm0, mm1               /* min() step 1           L  0m1r2 3 4 5 6 7    */

      pandn     mm1, mm4               /* min() step 2           L  0 1m2 3 4r5 6 7    */

      por       mm0, mm1               /* min() step 3           L  0m1r2 3 4 5 6 7    */

      /* at this point, mm0 = MIN( ABS(a3_1), ABS(a3_2), mm2 = v3 - v6, mm3 = v4 - v5 */
      movq      mm1, mm3               /* mm1 = v4 - v5          L  0 1w2 3r4 5 6 7    */
      psllw     mm3, 2                 /* mm3 = 4 * (v4 - v5)    L  0 1 2 3m4 5 6 7    */

      paddw     mm3, mm1               /* mm3 = 5 * (v4 - v5)    L  0 1r2 3m4 5 6 7    */
      psllw     mm2, 1                 /* mm2 = 2 * (v3 - v6)    L  0 1 2m3 4 5 6 7    */

      psubw     mm2, mm3               /* mm2 = a3_0             L  0 1 2m3r4 5 6 7    */

      /* at this point, mm0 = MIN( ABS(a3_1), ABS(a3_2), mm1 = v4 - v5, mm2 = a3_0 */
      movq      mm4, mm2               /* mm4 = a3_0             L  0 1 2r3 4w5 6 7    */
      pxor      mm3, mm3               /* mm3 = 0000000000000000    0 1 2 3w4 5 6 7    */

      pcmpgtw   mm3, mm2               /* is 0 > a3_0 ?          L  0 1 2r3m4 5 6 7    */

      movq      mm2, mm_8_x_QP         /* mm4 = 8*QP                0 1 2w3 4 5 6 7    */
      pxor      mm4, mm3               /* mm4 = ABS(a3_0) step 1 L  0 1 2 3r4m5 6 7    */

      psubw     mm4, mm3               /* mm4 = ABS(a3_0) step 2 L  0 1 2 3r4m5 6 7    */

      /* compare a3_0 against 8*QP */
      pcmpgtw   mm2, mm4               /* is 8*QP > ABS(d) ?     L  0 1 2m3 4r5 6 7    */

      pand      mm2, mm4               /* if no, d = 0           L  0 1 2m3 4r5 6 7    */

      movq      mm4, mm2               /* mm2 = a3_0             L  0 1 2r3 4w5 6 7    */

      /* at this point, mm0 = MIN( ABS(a3_1), ABS(a3_2), mm1 = v4 - v5, mm2 = a3_0 , mm3 = SGN(a3_0), mm4 = ABS(a3_0) */
      psubusw   mm4, mm0               /* mm0 = (A3_0 - a3_0)    L  0r1 2 3 4m5 6 7    */

      movq      mm0, mm4               /* mm0=ABS(d)             L  0w1 2 3 4r5 6 7    */

      psllw     mm0, 2                 /* mm0 = 4 * (A3_0-a3_0)  L  0m1 2 3 4 5 6 7    */

      paddw     mm0, mm4               /* mm0 = 5 * (A3_0-a3_0)  L  0m1 2 3 4r5 6 7    */

      paddw     mm0, mm_0020           /* mm0 += 32              L  0m1 2 3 4 5 6 7    */

      psraw     mm0, 6                 /* mm0 >>= 6              L  0m1 2 3 4 5 6 7    */

      /* at this point, mm0 = ABS(d), mm1 = v4 - v5, mm3 = SGN(a3_0) */
      pxor      mm2, mm2               /* mm2 = 0000000000000000    0 1 2w3 4 5 6 7    */

      pcmpgtw   mm2, mm1               /* is 0 > (v4 - v5) ?     L  0 1r2m3 4 5 6 7    */

      pxor      mm1, mm2               /* mm1 = ABS(mm1) step 1  L  0 1m2r3 4 5 6 7    */

      psubw     mm1, mm2               /* mm1 = ABS(mm1) step 2  L  0 1m2r3 4 5 6 7    */

      psraw     mm1, 1                 /* mm1 >>= 2              L  0 1m2 3 4 5 6 7    */

      /* OK at this point, mm0 = ABS(d), mm1 = ABS(q), mm2 = SGN(q), mm3 = SGN(-d) */
      movq      mm4, mm2               /* mm4 = SGN(q)           L  0 1 2r3 4w5 6 7    */

      pxor      mm4, mm3               /* mm4 = SGN(q) ^ SGN(-d) L  0 1 2 3r4m5 6 7    */

      movq      mm5, mm0               /* mm5 = ABS(d)           L  0r1 2 3 4 5w6 7    */

      pcmpgtw   mm5, mm1               /* is ABS(d) > ABS(q) ?   L  0 1r2 3 4 5m6 7    */

      pand      mm1, mm5               /* min() step 1           L  0m1 2 3 4 5r6 7    */

      pandn     mm5, mm0               /* min() step 2           L  0 1r2 3 4 5m6 7    */

      por       mm1, mm5               /* min() step 3           L  0m1 2 3 4 5r6 7    */

      pand      mm1, mm4               /* if signs differ, set 0 L  0m1 2 3 4r5 6 7    */

      pxor      mm1, mm2               /* Apply sign step 1      L  0m1 2r3 4 5 6 7    */

      psubw     mm1, mm2               /* Apply sign step 2      L  0m1 2r3 4 5 6 7    */

      /* at this point we have d in mm1 */
      pop ecx
    };

    if (i == 0)
    {
      __asm movq mm6, mm1;
    }
  }

  /* add d to rows l4 and l5 in memory... */
  pmm1 = (uint64_t*)(&(v[4 * stride]));
  __asm
  {
    push ecx
    mov ecx, pmm1
    packsswb  mm6, mm1
    movq      mm0, [ecx]
    psubb     mm0, mm6
    movq[ecx], mm0
    add       ecx, stride                    /* %0 += stride              0 1 2 3 4 5 6 7    */
    paddb     mm6, [ecx]
    movq[ecx], mm6
    pop ecx
  };

  __asm  emms
}
#endif


///////////////////////////////////////////////////////// NEW VERTICAL  IS BELOW ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

/* This function chooses the "endstops" for the vertical LPF9 filter: p1 and p2 */
/* We also convert these to 16-bit values here */
void deblock_vert_choose_p1p2_c(uint8_t* v, int stride, uint64_t* p1p2, int QP)
{
  int i;

  /* check p1 and p2 have been calculated correctly */
  /* p2 */
  for (i = 0; i < 8; i++)
  {
    ((uint16_t*)(&(p1p2[2])))[i] = ((ABS(v[9 * stride + i] - v[8 * stride + i]) - QP > 0) ? v[8 * stride + i] : v[9 * stride + i]);
    /*if (((ABS(v[9 * stride + i] - v[8 * stride + i]) - QP > 0) ? v[8 * stride + i] : v[9 * stride + i])
      != ((uint16_t*)(&(p1p2[2])))[i])
    {
      dprintf("ERROR: problem with P2\n");
    }
    */
  }

  /* p1 */
  for (i = 0; i < 8; i++)
  {
    ((uint16_t*)(&(p1p2[0])))[i] = ((ABS(v[0 * stride + i] - v[1 * stride + i]) - QP > 0) ? v[1 * stride + i] : v[0 * stride + i]);
    /*
    if (((ABS(v[0 * stride + i] - v[1 * stride + i]) - QP > 0) ? v[1 * stride + i] : v[0 * stride + i])
      != ((uint16_t*)(&(p1p2[0])))[i])
    {
      dprintf("ERROR: problem with P1\n");
    }
    */
  }

}

#ifdef USE_NEW_INTRINSICS
// converted to intrinsics by PF
/* This function chooses the "endstops" for the vertical LPF9 filter: p1 and p2 */
/* We also convert these to 16-bit values here */
void deblock_vert_choose_p1p2(uint8_t* v, int stride, uint64_t* p1p2, int QP) {
  __m128i mm_b_qp = _mm_set1_epi8(QP);

#ifdef PP_SELF_CHECK
  int i;
#endif

  uint8_t* pmm1 = &(v[0 * stride]);
  uint8_t* pmm2 = &(v[8 * stride]);

  // p1
  __m128i mm7 = _mm_setzero_si128(); // mm7 = 0
  __m128i mm0 = _mm_loadl_epi64((__m128i*)pmm1); // mm0 = *pmm1 = v[l0]
  __m128i mm2 = mm0; // mm2 = mm0 = v[l0]
  pmm1 += stride;
  __m128i mm1 = _mm_loadl_epi64((__m128i*)pmm1); // mm1 = *pmm1 = v[l1]
  __m128i mm3 = mm1; // mm3 = mm1 = v[l1]
  mm0 = _mm_subs_epu8(mm0, mm1); // mm0 -= mm1
  mm1 = _mm_subs_epu8(mm1, mm2); // mm1 -= mm2
  mm0 = _mm_or_si128(mm0, mm1); // mm0 |= mm1
  mm0 = _mm_subs_epu8(mm0, mm_b_qp); // mm0 -= QP

  // now a zero byte in mm0 indicates use v0 else use v1
  mm0 = _mm_cmpeq_epi8(mm0, mm7); // zero bytes to ff others to 00
  mm1 = mm0; // make a copy of mm0

  // now ff byte in mm0 indicates use v0 else use v1
  mm0 = _mm_andnot_si128(mm0, mm3); // mask v1 into 00 bytes in mm0
  mm1 = _mm_and_si128(mm1, mm2); // mask v0 into ff bytes in mm0
  mm0 = _mm_or_si128(mm0, mm1); // mm0 |= mm1
  mm1 = mm0; // make a copy of mm0

  // Now we have our result, p1, in mm0. Next, unpack.
  mm0 = _mm_unpacklo_epi8(mm0, mm7); // low bytes to mm0
  mm1 = _mm_unpackhi_epi8(mm1, mm7); // high bytes to mm1

  // Store p1 in memory
  _mm_store_si128((__m128i*) & p1p2[0], mm0); // low words to p1p2[0]
  _mm_store_si128((__m128i*) & p1p2[1], mm1); // high words to p1p2[1]

  // p2
  mm1 = _mm_loadl_epi64((__m128i*)pmm2); // mm1 = *pmm2 = v[l8]
  mm3 = mm1; // mm3 = mm1 = v[l8]
  pmm2 += stride;
  mm0 = _mm_loadl_epi64((__m128i*)pmm2); // mm0 = *pmm2 = v[l9]
  mm2 = mm0; // mm2 = mm0 = v[l9]
  mm0 = _mm_subs_epu8(mm0, mm1); // mm0 -= mm1
  mm1 = _mm_subs_epu8(mm1, mm2); // mm1 -= mm2
  mm0 = _mm_or_si128(mm0, mm1); // mm0 |= mm1
  mm0 = _mm_subs_epu8(mm0, mm_b_qp); // mm0 -= QP

  // now a zero byte in mm0 indicates use v0 else use v1
  mm0 = _mm_cmpeq_epi8(mm0, mm7); // zero bytes to ff others to 00
  mm1 = mm0; // make a copy of mm0

  // now ff byte in mm0 indicates use v0 else use v1
  mm0 = _mm_andnot_si128(mm0, mm3); // mask v1 into 00 bytes in mm0
  mm1 = _mm_and_si128(mm1, mm2); // mask v0 into ff bytes in mm0
  mm0 = _mm_or_si128(mm0, mm1); // mm0 |= mm1
  mm1 = mm0; // make a copy of mm0

  // Now we have our result, p2, in mm0. Next, unpack.
  mm0 = _mm_unpacklo_epi8(mm0, mm7); // low bytes to mm0
  mm1 = _mm_unpackhi_epi8(mm1, mm7); // high bytes to mm1

  // Store p2 in memory
  _mm_store_si128((__m128i*) & p1p2[2], mm0); // low words to p1p2[2]
  _mm_store_si128((__m128i*) & p1p2[3], mm1); // high words to p1p2[3]

#ifdef PP_SELF_CHECK
  /* check p1 and p2 have been calculated correctly */
  /* p2 */
  for (i = 0; i < 8; i++) {
    if (((abs(v[9 * stride + i] - v[8 * stride + i]) - QP > 0) ? v[8 * stride + i] : v[9 * stride + i])
      != ((uint16_t*)(&(p1p2[2])))[i]) {
      dprintf("ERROR: problem with P2\n");
    }
  }

  /* p1 */
  for (i = 0; i < 8; i++) {
    if (((abs(v[0 * stride + i] - v[1 * stride + i]) - QP > 0) ? v[1 * stride + i] : v[0 * stride + i])
      != ((uint16_t*)(&(p1p2[0])))[i]) {
      dprintf("ERROR: problem with P1\n");
    }
  }
#endif
}

#else
/* This function chooses the "endstops" for the vertical LPF9 filter: p1 and p2 */
/* We also convert these to 16-bit values here */
void deblock_vert_choose_p1p2(uint8_t* v, int stride, uint64_t* p1p2, int QP)
{
  uint64_t* pmm1, * pmm2;
  uint64_t mm_b_qp;

#ifdef PP_SELF_CHECK
  int i;
#endif

  /* load QP into every one of the 8 bytes in mm_b_qp */
  ((uint32_t*)&mm_b_qp)[0] =
    ((uint32_t*)&mm_b_qp)[1] = 0x01010101 * QP;

  pmm1 = (uint64_t*)(&(v[0 * stride]));
  pmm2 = (uint64_t*)(&(v[8 * stride]));

  __asm
  {
    push eax
    push ebx
    push ecx

    mov eax, pmm1
    mov ebx, pmm2
    mov ecx, p1p2

    /* p1 */
    pxor     mm7, mm7             /* mm7 = 0                       */
    movq     mm0, [eax]           /* mm0 = *pmm1 = v[l0]           */
    movq     mm2, mm0             /* mm2 = mm0 = v[l0]             */
    add      eax, stride          /* pmm1 += stride                */
    movq     mm1, [eax]           /* mm1 = *pmm1 = v[l1]           */
    movq     mm3, mm1             /* mm3 = mm1 = v[l1]             */
    psubusb  mm0, mm1             /* mm0 -= mm1                    */
    psubusb  mm1, mm2             /* mm1 -= mm2                    */
    por      mm0, mm1             /* mm0 |= mm1                    */
    psubusb  mm0, mm_b_qp         /* mm0 -= QP                     */

    /* now a zero byte in mm0 indicates use v0 else use v1         */
    pcmpeqb  mm0, mm7             /* zero bytes to ff others to 00 */
    movq     mm1, mm0             /* make a copy of mm0            */

    /* now ff byte in mm0 indicates use v0 else use v1             */
    pandn    mm0, mm3             /* mask v1 into 00 bytes in mm0  */
    pand     mm1, mm2             /* mask v0 into ff bytes in mm0  */
    por      mm0, mm1             /* mm0 |= mm1                    */
    movq     mm1, mm0             /* make a copy of mm0            */

    /* Now we have our result, p1, in mm0.  Next, unpack.          */
    punpcklbw mm0, mm7            /* low bytes to mm0              */
    punpckhbw mm1, mm7            /* high bytes to mm1             */

    /* Store p1 in memory                                          */
    movq[ecx], mm0           /* low words to p1p2[0]          */
    movq     8[ecx], mm1          /* high words to p1p2[1]         */

    /* p2 */
    movq     mm1, [ebx]           /* mm1 = *pmm2 = v[l8]           */
    movq     mm3, mm1             /* mm3 = mm1 = v[l8]             */
    add      ebx, stride          /* pmm2 += stride                */
    movq     mm0, [ebx]           /* mm0 = *pmm2 = v[l9]           */
    movq     mm2, mm0             /* mm2 = mm0 = v[l9]             */
    psubusb  mm0, mm1             /* mm0 -= mm1                    */
    psubusb  mm1, mm2             /* mm1 -= mm2                    */
    por      mm0, mm1             /* mm0 |= mm1                    */
    psubusb  mm0, mm_b_qp         /* mm0 -= QP                     */

    /* now a zero byte in mm0 indicates use v0 else use v1              */
    pcmpeqb  mm0, mm7             /* zero bytes to ff others to 00 */
    movq     mm1, mm0             /* make a copy of mm0            */

    /* now ff byte in mm0 indicates use v0 else use v1                  */
    pandn    mm0, mm3             /* mask v1 into 00 bytes in mm0  */
    pand     mm1, mm2             /* mask v0 into ff bytes in mm0  */
    por      mm0, mm1             /* mm0 |= mm1                    */
    movq     mm1, mm0             /* make a copy of mm0            */

    /* Now we have our result, p2, in mm0.  Next, unpack.               */
    punpcklbw mm0, mm7            /* low bytes to mm0              */
    punpckhbw mm1, mm7            /* high bytes to mm1             */

    /* Store p2 in memory                                               */
    movq     16[ecx], mm0         /* low words to p1p2[2]          */
    movq     24[ecx], mm1         /* high words to p1p2[3]         */

    pop ecx
    pop ebx
    pop eax
  };

#ifdef PP_SELF_CHECK
  /* check p1 and p2 have been calculated correctly */
  /* p2 */
  for (i = 0; i < 8; i++)
  {
    if (((ABS(v[9 * stride + i] - v[8 * stride + i]) - QP > 0) ? v[8 * stride + i] : v[9 * stride + i])
      != ((uint16_t*)(&(p1p2[2])))[i])
    {
      dprintf("ERROR: problem with P2\n");
    }
  }

  /* p1 */
  for (i = 0; i < 8; i++)
  {
    if (((ABS(v[0 * stride + i] - v[1 * stride + i]) - QP > 0) ? v[1 * stride + i] : v[0 * stride + i])
      != ((uint16_t*)(&(p1p2[0])))[i])
    {
      dprintf("ERROR: problem with P1\n");
    }
  }
#endif

#ifdef ARCH_IS_IA32
  _mm_empty();
#endif

}
#endif

// like the checker code below
void deblock_vert_copy_and_unpack_c(int stride, uint8_t* source, uint64_t* dest, int n) {
  int j, k;
  uint64_t* pmm1 = (uint64_t*)source;
  uint64_t* pmm2 = (uint64_t*)dest;
  int i = -n / 2;
  for (k = 0; k < n; k++)
  {
    for (j = 0; j < 8; j++)
    {
      ((uint16_t*)dest)[k * 8 + j] = source[k * stride + j];
    }
  }

}

#ifdef USE_NEW_INTRINSICS
/* function using MMX to copy an 8-pixel wide column and unpack to 16-bit values */
/* n is the number of rows to copy - this must be even */
void deblock_vert_copy_and_unpack(int stride, uint8_t* source, uint64_t* dest, int n)
{
  uint64_t* pmm1 = (uint64_t*)source;
  uint64_t* pmm2 = (uint64_t*)dest;
  int i = -n / 2;

  for (; i < 0; ++i) {
    __m128i xmm0 = _mm_loadl_epi64((__m128i*)source); // Load 8 bytes from source
    source += stride; // Move to the next row

    __m128i xmm1 = _mm_unpacklo_epi8(xmm0, _mm_setzero_si128()); // Unpack low bytes to 16-bit values
    _mm_store_si128((__m128i*)dest, xmm1); // Store the result in dest
    dest += 2; // Move to the next 16 bytes in dest

    xmm0 = _mm_loadl_epi64((__m128i*)source); // Load next 8 bytes from source
    source += stride; // Move to the next row

    xmm1 = _mm_unpacklo_epi8(xmm0, _mm_setzero_si128()); // Unpack low bytes to 16-bit values
    _mm_store_si128((__m128i*)dest, xmm1); // Store the result in dest
    dest += 2; // Move to the next 16 bytes in dest
  }
#ifdef PP_SELF_CHECK
  int j, k;
#endif

#ifdef PP_SELF_CHECK
  /* check that MMX copy has worked correctly */
  for (k = 0; k < n; k++)
  {
    for (j = 0; j < 8; j++)
    {
      if (((uint16_t*)dest)[k * 8 + j] != source[k * stride + j])
      {
        dprintf("ERROR: MMX copy block is flawed at (%d, %d)\n", j, k);
      }
    }
  }
#endif


}
#else
/* function using MMX to copy an 8-pixel wide column and unpack to 16-bit values */
/* n is the number of rows to copy - this must be even */
void deblock_vert_copy_and_unpack(int stride, uint8_t* source, uint64_t* dest, int n)
{
  uint64_t* pmm1 = (uint64_t*)source;
  uint64_t* pmm2 = (uint64_t*)dest;
  int i = -n / 2;

#ifdef PP_SELF_CHECK
  int j, k;
#endif

  /* copy block to local store whilst unpacking to 16-bit values */
  __asm
  {
    push edi
    push eax
    push ebx

    mov edi, i
    mov eax, pmm1
    mov ebx, pmm2

    pxor   mm7, mm7                        /* set mm7 = 0                     */
    deblock_v_L1 :                             /* now p1 is in mm1                */
    movq   mm0, [eax]                     /* mm0 = v[0*stride]               */

#ifdef PREFETCH_ENABLE
      prefetcht0 0[ebx]
#endif

      add   eax, stride                    /* p_data += stride                */
      movq   mm1, mm0                        /* mm1 = v[0*stride]               */
      punpcklbw mm0, mm7                     /* unpack low bytes (left hand 4)  */

      movq   mm2, [eax]                     /* mm2 = v[0*stride]               */
      punpckhbw mm1, mm7                     /* unpack high bytes (right hand 4)*/

      movq   mm3, mm2                        /* mm3 = v[0*stride]               */
      punpcklbw mm2, mm7                     /* unpack low bytes (left hand 4)  */

      movq[ebx], mm0                     /* v_local[n] = mm0 (left)         */
      add   eax, stride                    /* p_data += stride                */

      movq   8[ebx], mm1                    /* v_local[n+8] = mm1 (right)      */
      punpckhbw mm3, mm7                     /* unpack high bytes (right hand 4)*/

      movq   16[ebx], mm2                   /* v_local[n+16] = mm2 (left)      */

      movq   24[ebx], mm3                   /* v_local[n+24] = mm3 (right)     */

      add   ebx, 32                        /* p_data2 += 8                    */

      add   i, 1                            /* increment loop counter          */
      jne    deblock_v_L1

      pop ebx
      pop eax
      pop edi
  };

#ifdef PP_SELF_CHECK
  /* check that MMX copy has worked correctly */
  for (k = 0; k < n; k++)
  {
    for (j = 0; j < 8; j++)
    {
      if (((uint16_t*)dest)[k * 8 + j] != source[k * stride + j])
      {
        dprintf("ERROR: MMX copy block is flawed at (%d, %d)\n", j, k);
      }
    }
  }
#endif

#ifdef ARCH_IS_IA32
  _mm_empty();
#endif

}
#endif

/* decide whether the DC filter should be turned on according to QP */
int deblock_vert_DC_on(uint8_t* v, int stride, int QP)
{
  for (int i = 0; i < 8; ++i)
  {
    if (ABS(v[i + 0 * stride] - v[i + 5 * stride]) >= 2 * QP) return false;
    if (ABS(v[i + 1 * stride] - v[i + 4 * stride]) >= 2 * QP) return false;
    if (ABS(v[i + 1 * stride] - v[i + 8 * stride]) >= 2 * QP) return false;
    if (ABS(v[i + 2 * stride] - v[i + 7 * stride]) >= 2 * QP) return false;
    if (ABS(v[i + 3 * stride] - v[i + 6 * stride]) >= 2 * QP) return false;
  }
  return true;
}

#if 0
// This older method produces artifacts.

int deblock_vert_DC_on(uint8_t* v, int stride, int QP)
{
  uint64_t QP_x_2;
  uint8_t* ptr1;
  uint8_t* ptr2;
  int DC_on;

#ifdef PP_SELF_CHECK
  int i, DC_on2;
#endif

  ptr1 = &(v[1 * stride]);
  ptr2 = &(v[8 * stride]);

#ifdef PP_SELF_CHECK
  DC_on2 = 1;
  for (i = 0; i < 8; i++)
  {
    if (ABS(v[i + 1 * stride] - v[i + 8 * stride]) > 2 * QP) DC_on2 = 0;
  }
#endif

  /* load 2*QP into every byte in QP_x_2 */
  ((uint32_t*)(&QP_x_2))[0] =
    ((uint32_t*)(&QP_x_2))[1] = 0x02020202 * QP;

  /* MMX assembler to compute DC_on */
  __asm
  {
    push eax
    push ebx

    mov eax, ptr1
    mov ebx, ptr2

    movq     mm0, [eax]               /* mm0 = v[l1]                   */
    movq     mm1, mm0                 /* mm1 = v[l1]                   */
    movq     mm2, [ebx]               /* mm2 = v[l8]                   */
    psubusb  mm0, mm2                 /* mm0 -= mm2                    */
    psubusb  mm2, mm1                 /* mm2 -= mm1                    */
    por      mm0, mm2                 /* mm0 |= mm2                    */
    psubusb  mm0, QP_x_2              /* mm0 -= 2 * QP                 */
    movq     mm1, mm0                 /* mm1 = mm0                     */
    psrlq    mm0, 32                  /* shift mm0 right 32 bits       */
    por      mm0, mm1                 /*                               */
    movd     DC_on, mm0               /* this is actually !DC_on       */

    pop ebx
    pop eax
  };

  DC_on = !DC_on; /* the above assembler computes the opposite sense! */

#ifdef PP_SELF_CHECK
  if (DC_on != DC_on2)
  {
    dprintf("ERROR: MMX version of DC_on is incorrect\n");
  }
#endif

  return DC_on;
}
#endif

/* Vertical deblocking filter for use in non-flat picture regions */
// obtained from _ORIGINAL, self_check case
// plus define SIGN macro
#define SIGN(X)   (((X)>0)?1:-1)

void deblock_vert_default_filter_c(uint8_t* v, int stride, int QP)
{
  //  int i;

    /* define semi-constants to enable us to move up and down the picture easily... */
  int l1 = 1 * stride;
  int l2 = 2 * stride;
  int l3 = 3 * stride;
  int l4 = 4 * stride;
  int l5 = 5 * stride;
  int l6 = 6 * stride;
  int l7 = 7 * stride;
  int l8 = 8 * stride;
  int x, y, a3_0_SC, a3_1_SC, a3_2_SC, d_SC, q_SC;
  //uint8_t selfcheck[8][2];

    /* compute selfcheck matrix for later comparison */
  for (x = 0; x < 8; x++)
  {
    a3_0_SC = 2 * v[l3 + x] - 5 * v[l4 + x] + 5 * v[l5 + x] - 2 * v[l6 + x];
    a3_1_SC = 2 * v[l1 + x] - 5 * v[l2 + x] + 5 * v[l3 + x] - 2 * v[l4 + x];
    a3_2_SC = 2 * v[l5 + x] - 5 * v[l6 + x] + 5 * v[l7 + x] - 2 * v[l8 + x];
    q_SC = (v[l4 + x] - v[l5 + x]) / 2;

    if (ABS(a3_0_SC) < 8 * QP)
    {

      d_SC = ABS(a3_0_SC) - MIN(ABS(a3_1_SC), ABS(a3_2_SC));
      if (d_SC < 0) d_SC = 0;

      d_SC = (5 * d_SC + 32) >> 6;
      d_SC *= SIGN(-a3_0_SC);

      //printf("d_SC[%d] preclip=%d\n", x, d_SC);
      /* clip d in the range 0 ... q */
      if (q_SC > 0)
      {
        d_SC = d_SC < 0 ? 0 : d_SC;
        d_SC = d_SC > q_SC ? q_SC : d_SC;
      }
      else
      {
        d_SC = d_SC > 0 ? 0 : d_SC;
        d_SC = d_SC < q_SC ? q_SC : d_SC;
      }
    }
    else
    {
      d_SC = 0;
    }

    v[l4 + x + 0 * stride] = v[l4 + x] - d_SC;
    v[l4 + x + 1 * stride] = v[l5 + x] + d_SC;
    //selfcheck[x][0] = v[l4 + x] - d_SC;
    //selfcheck[x][1] = v[l5 + x] + d_SC;
  }

  /* do selfcheck */
  /*
  for (x = 0; x < 8; x++)
  {
    for (y = 0; y < 2; y++)
    {
      v[l4 + x + y * stride] = selfcheck[x][y];
    }
  }
  */
}

#if 0
void deblock_vert_default_filter_ORIGINAL(uint8_t* v, int stride, int QP)
{
  uint64_t* pmm1;
  const uint64_t mm_0020 = 0x0020002000200020;
  uint64_t mm_8_x_QP;
  int i;

#ifdef PP_SELF_CHECK
  /* define semi-constants to enable us to move up and down the picture easily... */
  int l1 = 1 * stride;
  int l2 = 2 * stride;
  int l3 = 3 * stride;
  int l4 = 4 * stride;
  int l5 = 5 * stride;
  int l6 = 6 * stride;
  int l7 = 7 * stride;
  int l8 = 8 * stride;
  int x, y, a3_0_SC, a3_1_SC, a3_2_SC, d_SC, q_SC;
  uint8_t selfcheck[8][2];
#endif

#ifdef PP_SELF_CHECK
  /* compute selfcheck matrix for later comparison */
  for (x = 0; x < 8; x++)
  {
    a3_0_SC = 2 * v[l3 + x] - 5 * v[l4 + x] + 5 * v[l5 + x] - 2 * v[l6 + x];
    a3_1_SC = 2 * v[l1 + x] - 5 * v[l2 + x] + 5 * v[l3 + x] - 2 * v[l4 + x];
    a3_2_SC = 2 * v[l5 + x] - 5 * v[l6 + x] + 5 * v[l7 + x] - 2 * v[l8 + x];
    q_SC = (v[l4 + x] - v[l5 + x]) / 2;

    if (ABS(a3_0_SC) < 8 * QP)
    {

      d_SC = ABS(a3_0_SC) - MIN(ABS(a3_1_SC), ABS(a3_2_SC));
      if (d_SC < 0) d_SC = 0;

      d_SC = (5 * d_SC + 32) >> 6;
      d_SC *= SIGN(-a3_0_SC);

      //printf("d_SC[%d] preclip=%d\n", x, d_SC);
      /* clip d in the range 0 ... q */
      if (q_SC > 0)
      {
        d_SC = d_SC < 0 ? 0 : d_SC;
        d_SC = d_SC > q_SC ? q_SC : d_SC;
      }
      else
      {
        d_SC = d_SC > 0 ? 0 : d_SC;
        d_SC = d_SC < q_SC ? q_SC : d_SC;
      }
    }
    else
    {
      d_SC = 0;
    }
    selfcheck[x][0] = v[l4 + x] - d_SC;
    selfcheck[x][1] = v[l5 + x] + d_SC;
  }
#endif

  ((uint32_t*)&mm_8_x_QP)[0] =
    ((uint32_t*)&mm_8_x_QP)[1] = 0x00080008 * QP;

  /* working in 4-pixel wide columns, left to right */
  /*i=0 in left, i=1 in right */
  for (i = 0; i < 2; i++)
  {
    /* v should be 64-bit aligned here */
    pmm1 = (uint64_t*)(&(v[4 * i]));
    /* pmm1 will be 32-bit aligned but this doesn't matter as we'll use movd not movq */

    __asm
    {
      push ecx
      mov ecx, pmm1

      pxor      mm7, mm7               /* mm7 = 0000000000000000    0 1 2 3 4 5 6 7w   */
      add      ecx, stride           /* %0 += stride              0 1 2 3 4 5 6 7    */

      movd      mm0, [ecx]            /* mm0 = v1v1v1v1v1v1v1v1    0w1 2 3 4 5 6 7    */
      punpcklbw mm0, mm7               /* mm0 = __v1__v1__v1__v1 L  0m1 2 3 4 5 6 7r   */

      add      ecx, stride           /* ecx += stride              0 1 2 3 4 5 6 7    */
      movd      mm1, [ecx]            /* mm1 = v2v2v2v2v2v2v2v2    0 1w2 3 4 5 6 7    */

      add      ecx, stride           /* ecx += stride              0 1 2 3 4 5 6 7    */
      punpcklbw mm1, mm7               /* mm1 = __v2__v2__v2__v2 L  0 1m2 3 4 5 6 7r   */

      movd      mm2, [ecx]            /* mm2 = v3v3v3v3v3v3v3v3    0 1 2w3 4 5 6 7    */
      add      ecx, stride           /* ecx += stride              0 1 2 3 4 5 6 7    */

      punpcklbw mm2, mm7               /* mm2 = __v3__v3__v3__v3 L  0 1 2m3 4 5 6 7r   */

      movd      mm3, [ecx]            /* mm3 = v4v4v4v4v4v4v4v4    0 1 2 3w4 5 6 7    */

      punpcklbw mm3, mm7               /* mm3 = __v4__v4__v4__v4 L  0 1 2 3m4 5 6 7r   */

      psubw     mm1, mm2               /* mm1 = v2 - v3          L  0 1m2r3 4 5 6 7    */

      movq      mm4, mm1               /* mm4 = v2 - v3          L  0 1r2 3 4w5 6 7    */
      psllw     mm1, 2                 /* mm1 = 4 * (v2 - v3)    L  0 1m2 3 4 5 6 7    */

      paddw     mm1, mm4               /* mm1 = 5 * (v2 - v3)    L  0 1m2 3 4r5 6 7    */
      psubw     mm0, mm3               /* mm0 = v1 - v4          L  0m1 2 3r4 5 6 7    */

      psllw     mm0, 1                 /* mm0 = 2 * (v1 - v4)    L  0m1 2 3 4 5 6 7    */

      psubw     mm0, mm1               /* mm0 = a3_1             L  0m1r2 3 4 5 6 7    */

      pxor      mm1, mm1               /* mm1 = 0000000000000000    0 1w2 3 4 5 6 7    */

      pcmpgtw   mm1, mm0               /* is 0 > a3_1 ?          L  0r1m2 3 4 5 6 7    */

      add      ecx, stride           /* ecx += stride              0 1 2 3 4 5 6 7    */
      pxor      mm0, mm1               /* mm0 = ABS(a3_1) step 1 L  0m1r2 3 4 5 6 7    */

      psubw     mm0, mm1               /* mm0 = ABS(a3_1) step 2 L  0m1r2 3 4 5 6 7    */

      movd      mm1, [ecx]            /* mm1 = v5v5v5v5v5v5v5v5    0 1w2 3 4 5 6 7    */

      punpcklbw mm1, mm7               /* mm1 = __v5__v5__v5__v5 L  0 1m2 3 4 5 6 7r   */


      add      ecx, stride           /* ecx += stride              0 1 2 3 4 5 6 7    */
      psubw     mm3, mm1               /* mm3 = v4 - v5          L  0 1r2 3m4 5 6 7    */

      movd      mm4, [ecx]            /* mm4 = v6v6v6v6v6v6v6v6    0 1 2 3 4w5 6 7    */

      punpcklbw mm4, mm7               /* mm4 = __v6__v6__v6__v6 L  0 1 2 3 4m5 6 7r   */

      add      ecx, stride           /* ecx += stride              0 1 2 3 4 5 6 7    */

      movd      mm5, [ecx]            /* mm5 = v7v7v7v7v7v7v7v7    0 1 2 3 4 5w6 7    */
      psubw     mm2, mm4               /* mm2 = v3 - v6          L  0 1 2m3 4r5 6 7    */

      punpcklbw mm5, mm7               /* mm5 = __v7__v7__v7__v7 L  0 1 2 3 4 5m6 7r   */

      add      ecx, stride           /* ecx += stride              0 1 2 3 4 5 6 7    */
      psubw     mm5, mm4               /* mm5 = v7 - v6          L  0 1 2 3 4r5m6 7    */

      movq      mm4, mm5               /* mm4 = v7 - v6          L  0 1 2 3 4w5r6 7    */

      psllw     mm4, 2                 /* mm4 = 4 * (v7 - v6)    L  0 1 2 3 4 5m6 7    */

      paddw     mm5, mm4               /* mm5 = 5 * (v7 - v6)    L  0 1 2 3 4r5m6 7    */

      movd      mm4, [ecx]            /* mm4 = v8v8v8v8v8v8v8v8    0 1 2 3 4w5 6 7    */

      punpcklbw mm4, mm7               /* mm4 = __v8__v8__v8__v8 L  0 1 2 3 4m5 6 7r   */

      psubw     mm1, mm4               /* mm1 = v5 - v8          L  0 1m2 3 4r5 6 7    */

      pxor      mm4, mm4               /* mm4 = 0000000000000000    0 1 2 3 4w5 6 7    */
      psllw     mm1, 1                 /* mm1 = 2 * (v5 - v8)    L  0 1m2 3 4 5 6 7    */

      paddw     mm1, mm5               /* mm1 = a3_2             L  0 1m2 3 4 5r6 7    */

      pcmpgtw   mm4, mm1               /* is 0 > a3_2 ?          L  0 1r2 3 4m5 6 7    */

      pxor      mm1, mm4               /* mm1 = ABS(a3_2) step 1 L  0 1m2 3 4r5 6 7    */

      psubw     mm1, mm4               /* mm1 = ABS(a3_2) step 2 L  0 1m2 3 4r5 6 7    */

      /* at this point, mm0 = ABS(a3_1), mm1 = ABS(a3_2), mm2 = v3 - v6, mm3 = v4 - v5 */
      movq      mm4, mm1               /* mm4 = ABS(a3_2)        L  0 1r2 3 4w5 6 7    */

      pcmpgtw   mm1, mm0               /* is ABS(a3_2) > ABS(a3_1)  0r1m2 3 4 5 6 7    */

      pand      mm0, mm1               /* min() step 1           L  0m1r2 3 4 5 6 7    */

      pandn     mm1, mm4               /* min() step 2           L  0 1m2 3 4r5 6 7    */

      por       mm0, mm1               /* min() step 3           L  0m1r2 3 4 5 6 7    */

      /* at this point, mm0 = MIN( ABS(a3_1), ABS(a3_2), mm2 = v3 - v6, mm3 = v4 - v5 */
      movq      mm1, mm3               /* mm1 = v4 - v5          L  0 1w2 3r4 5 6 7    */
      psllw     mm3, 2                 /* mm3 = 4 * (v4 - v5)    L  0 1 2 3m4 5 6 7    */

      paddw     mm3, mm1               /* mm3 = 5 * (v4 - v5)    L  0 1r2 3m4 5 6 7    */
      psllw     mm2, 1                 /* mm2 = 2 * (v3 - v6)    L  0 1 2m3 4 5 6 7    */

      psubw     mm2, mm3               /* mm2 = a3_0             L  0 1 2m3r4 5 6 7    */

      /* at this point, mm0 = MIN( ABS(a3_1), ABS(a3_2), mm1 = v4 - v5, mm2 = a3_0 */
      movq      mm4, mm2               /* mm4 = a3_0             L  0 1 2r3 4w5 6 7    */
      pxor      mm3, mm3               /* mm3 = 0000000000000000    0 1 2 3w4 5 6 7    */

      pcmpgtw   mm3, mm2               /* is 0 > a3_0 ?          L  0 1 2r3m4 5 6 7    */

      movq      mm2, mm_8_x_QP         /* mm4 = 8*QP                0 1 2w3 4 5 6 7    */
      pxor      mm4, mm3               /* mm4 = ABS(a3_0) step 1 L  0 1 2 3r4m5 6 7    */

      psubw     mm4, mm3               /* mm4 = ABS(a3_0) step 2 L  0 1 2 3r4m5 6 7    */

      /* compare a3_0 against 8*QP */
      pcmpgtw   mm2, mm4               /* is 8*QP > ABS(d) ?     L  0 1 2m3 4r5 6 7    */

      pand      mm2, mm4               /* if no, d = 0           L  0 1 2m3 4r5 6 7    */

      movq      mm4, mm2               /* mm2 = a3_0             L  0 1 2r3 4w5 6 7    */

      /* at this point, mm0 = MIN( ABS(a3_1), ABS(a3_2), mm1 = v4 - v5, mm2 = a3_0 , mm3 = SGN(a3_0), mm4 = ABS(a3_0) */
      psubusw   mm4, mm0               /* mm0 = (A3_0 - a3_0)    L  0r1 2 3 4m5 6 7    */

      movq      mm0, mm4               /* mm0=ABS(d)             L  0w1 2 3 4r5 6 7    */

      psllw     mm0, 2                 /* mm0 = 4 * (A3_0-a3_0)  L  0m1 2 3 4 5 6 7    */

      paddw     mm0, mm4               /* mm0 = 5 * (A3_0-a3_0)  L  0m1 2 3 4r5 6 7    */

      paddw     mm0, mm_0020           /* mm0 += 32              L  0m1 2 3 4 5 6 7    */

      psraw     mm0, 6                 /* mm0 >>= 6              L  0m1 2 3 4 5 6 7    */

      /* at this point, mm0 = ABS(d), mm1 = v4 - v5, mm3 = SGN(a3_0) */
      pxor      mm2, mm2               /* mm2 = 0000000000000000    0 1 2w3 4 5 6 7    */

      pcmpgtw   mm2, mm1               /* is 0 > (v4 - v5) ?     L  0 1r2m3 4 5 6 7    */

      pxor      mm1, mm2               /* mm1 = ABS(mm1) step 1  L  0 1m2r3 4 5 6 7    */

      psubw     mm1, mm2               /* mm1 = ABS(mm1) step 2  L  0 1m2r3 4 5 6 7    */

      psraw     mm1, 1                 /* mm1 >>= 2              L  0 1m2 3 4 5 6 7    */

      /* OK at this point, mm0 = ABS(d), mm1 = ABS(q), mm2 = SGN(q), mm3 = SGN(-d) */
      movq      mm4, mm2               /* mm4 = SGN(q)           L  0 1 2r3 4w5 6 7    */

      pxor      mm4, mm3               /* mm4 = SGN(q) ^ SGN(-d) L  0 1 2 3r4m5 6 7    */

      movq      mm5, mm0               /* mm5 = ABS(d)           L  0r1 2 3 4 5w6 7    */

      pcmpgtw   mm5, mm1               /* is ABS(d) > ABS(q) ?   L  0 1r2 3 4 5m6 7    */

      pand      mm1, mm5               /* min() step 1           L  0m1 2 3 4 5r6 7    */

      pandn     mm5, mm0               /* min() step 2           L  0 1r2 3 4 5m6 7    */

      por       mm1, mm5               /* min() step 3           L  0m1 2 3 4 5r6 7    */

      pand      mm1, mm4               /* if signs differ, set 0 L  0m1 2 3 4r5 6 7    */

      pxor      mm1, mm2               /* Apply sign step 1      L  0m1 2r3 4 5 6 7    */

      psubw     mm1, mm2               /* Apply sign step 2      L  0m1 2r3 4 5 6 7    */

      /* at this point we have d in mm1 */
      pop ecx
    };

    if (i == 0)
    {
      __asm movq mm6, mm1;
    }

  }

  /* add d to rows l4 and l5 in memory... */
  pmm1 = (uint64_t*)(&(v[4 * stride]));
  __asm
  {
    push ecx
    mov ecx, pmm1
    packsswb  mm6, mm1
    movq      mm0, [ecx]
    psubb     mm0, mm6
    movq[ecx], mm0
    add       ecx, stride                    /* %0 += stride              0 1 2 3 4 5 6 7    */
    paddb     mm6, [ecx]
    movq[ecx], mm6
    pop ecx
  };

#ifdef PP_SELF_CHECK
  /* do selfcheck */
  for (x = 0; x < 8; x++)
  {
    for (y = 0; y < 2; y++)
    {
      if (selfcheck[x][y] != v[l4 + x + y * stride])
      {
        dprintf("ERROR: problem with vertical default filter in col %d, row %d\n", x, y);
        dprintf("%d should be %d\n", v[l4 + x + y * stride], selfcheck[x][y]);

      }
    }
  }
#endif
}
#endif


#ifndef ARCH_IS_X86_64
void deblock_vert_lpf9_mmx(uint64_t* v_local, uint64_t* p1p2, uint8_t* v, int stride)
{
#ifdef PP_SELF_CHECK
  int j, k;
  uint8_t selfcheck[64];
  uint16_t* vv;
  int p1, p2, psum;
  /* define semi-constants to enable us to move up and down the picture easily... */
  int local_stride = 8; // for v_local
  int l1 = 1 * local_stride;
  int l2 = 2 * local_stride;
  int l3 = 3 * local_stride;
  int l4 = 4 * local_stride;
  int l5 = 5 * local_stride;
  int l6 = 6 * local_stride;
  int l7 = 7 * local_stride;
  int l8 = 8 * local_stride;

#endif


#ifdef PP_SELF_CHECK
  /* generate a self-check version of the filter result in selfcheck[64] */
  /* loop left->right */
  for (j = 0; j < 8; j++)
  {
    vv = &((uint16_t*)(v_local))[j]; // 2*8 uint16_t
    p1 = ((uint16_t*)(&(p1p2[0 + j / 4])))[j % 4]; /* yuck! */
    p2 = ((uint16_t*)(&(p1p2[2 + j / 4])))[j % 4]; /* yuck! */
    /* the above may well be endian-fussy */
    psum = p1 + p1 + p1 + vv[l1] + vv[l2] + vv[l3] + vv[l4] + 4;
    // 1*p1 + 1*p1 + 2*p1 + 2*p1 + 4*vv[l1] + 2*vv[l2] + 2*vv[l3] + 1*vv[l4] + 1*vv[l4]
    selfcheck[j + 8 * 0] = (((psum + vv[l1]) << 1) - (vv[l4] - vv[l5])) >> 4;
    // moving window. Add next on the right: vv[l5], subtract one filler from the left: p1, modify the weights by adding or subtracting the appropriate vv[] element
    // and so on...
    psum += vv[l5] - p1;
    selfcheck[j + 8 * 1] = (((psum + vv[l2]) << 1) - (vv[l5] - vv[l6])) >> 4;
    psum += vv[l6] - p1;
    selfcheck[j + 8 * 2] = (((psum + vv[l3]) << 1) - (vv[l6] - vv[l7])) >> 4;
    psum += vv[l7] - p1;
    selfcheck[j + 8 * 3] = (((psum + vv[l4]) << 1) + p1 - vv[l1] - (vv[l7] - vv[l8])) >> 4;
    psum += vv[l8] - vv[l1];
    selfcheck[j + 8 * 4] = (((psum + vv[l5]) << 1) + (vv[l1] - vv[l2]) - vv[l8] + p2) >> 4;
    psum += p2 - vv[l2];
    selfcheck[j + 8 * 5] = (((psum + vv[l6]) << 1) + (vv[l2] - vv[l3])) >> 4;
    psum += p2 - vv[l3];
    selfcheck[j + 8 * 6] = (((psum + vv[l7]) << 1) + (vv[l3] - vv[l4])) >> 4;
    psum += p2 - vv[l4];
    selfcheck[j + 8 * 7] = (((psum + vv[l8]) << 1) + (vv[l4] - vv[l5])) >> 4;
  }
#endif

  /* vertical DC filter in MMX
    mm2 - p1/2 left
    mm3 - p1/2 right
    mm4 - psum left
    mm5 - psum right */
    /* alternate between using mm0/mm1 and mm6/mm7 to accumulate left/right */

  __asm
  {
    push eax
    push ebx
    push ecx

    mov eax, p1p2
    mov ebx, v_local
    mov ecx, v

    /* load p1 left into mm2 and p1 right into mm3 */
    movq   mm2, [eax]                  /* mm2 = p1p2[0]               0 1 2w3 4 5 6 7    */
    add   ecx, stride                  /* ecx points at v[1*stride]   0 1 2 3 4 5 6 7    */

    movq   mm3, 8[eax]                 /* mm3 = p1p2[1]               0 1 2 3w4 5 6 7    */

    movq   mm4, mm_fours               /* mm4 = 0x0004000400040004    0 1 2 3 4w5 6 7    */

    /* psum = p1 + p1 + p1 + vv[1] + vv[2] + vv[3] + vv[4] + 4 */
    /* psum left will be in mm4, right in mm5          */

    movq   mm5, mm4                     /* mm5 = 0x0004000400040004    0 1 2 3 4 5w6 7    */

    paddsw mm4, 16[ebx]                 /* mm4 += vv[1] left           0 1 2 3 4m5 6 7    */
    paddw  mm5, mm3                     /* mm5 += p2 left              0 1 2 3r4 5m6 7    */

    paddsw mm4, 32[ebx]                 /* mm4 += vv[2] left           0 1 2 3 4m5 6 7    */
    paddw  mm5, mm3                     /* mm5 += p2 left              0 1 2 3r4 5m6 7    */

    paddsw mm4, 48[ebx]                 /* mm4 += vv[3] left           0 1 2 3 4m5 6 7    */
    paddw  mm5, mm3                     /* mm5 += p2 left              0 1 2 3r4 5m6 7    */

    paddsw mm5, 24[ebx]                 /* mm5 += vv[1] right          0 1 2 3 4 5m6 7    */
    paddw  mm4, mm2                     /* mm4 += p1 left              0 1 2r3 4m5 6 7    */

    paddsw mm5, 40[ebx]                 /* mm5 += vv[2] right          0 1 2 3 4 5m6 7    */
    paddw  mm4, mm2                     /* mm4 += p1 left              0 1 2r3 4m5 6 7    */

    paddsw mm5, 56[ebx]                 /* mm5 += vv[3] right          0 1 2 3 4 5m6 7    */
    paddw  mm4, mm2                     /* mm4 += p1 left              0 1 2r3 4m5 6 7    */

    paddsw mm4, 64[ebx]                 /* mm4 += vv[4] left           0 1 2 3 4m5 6 7    */

    paddsw mm5, 72[ebx]                 /* mm5 += vv[4] right          0 1 2 3 4 5m6 7    */

    /* v[1] = (((psum + vv[1]) << 1) - (vv[4] - vv[5])) >> 4 */
    /* compute this in mm0 (left) and mm1 (right)   */

    movq   mm0, mm4                     /* mm0 = psum left             0w1 2 3 4 5 6 7    */

    paddsw mm0, 16[ebx]                 /* mm0 += vv[1] left           0m1 2 3 4 5 6 7    */
    movq   mm1, mm5                     /* mm1 = psum right            0 1w2 3 4 5r6 7    */

    paddsw mm1, 24[ebx]                 /* mm1 += vv[1] right          0 1 2 3 4 5 6 7    */
    psllw  mm0, 1                       /* mm0 <<= 1                   0m1 2 3 4 5 6 7    */

    psubsw mm0, 64[ebx]                 /* mm0 -= vv[4] left           0m1 2 3 4 5 6 7    */
    psllw  mm1, 1                       /* mm1 <<= 1                   0 1 2 3 4 5 6 7    */

    psubsw mm1, 72[ebx]                 /* mm1 -= vv[4] right          0 1m2 3 4 5 6 7    */

    paddsw mm0, 80[ebx]                 /* mm0 += vv[5] left           0m1 2 3 4 5 6 7    */

    paddsw mm1, 88[ebx]                 /* mm1 += vv[5] right          0 1m2 3 4 5 6 7    */
    psrlw  mm0, 4                       /* mm0 >>= 4                   0m1 2 3 4 5 6 7    */

    /* psum += vv[5] - p1 */
    paddsw mm4, 80[ebx]                 /* mm4 += vv[5] left           0 1 2 3 4m5 6 7    */
    psrlw  mm1, 4                       /* mm1 >>= 4                   0 1m2 3 4 5 6 7    */

    paddsw mm5, 88[ebx]                 /* mm5 += vv[5] right          0 1 2 3 4 5 6 7    */
    psubsw mm4, [eax]                  /* mm4 -= p1 left              0 1 2 3 4 5 6 7    */

    packuswb mm0, mm1                   /* pack mm1, mm0 to mm0        0m1 2 3 4 5 6 7    */
    psubsw mm5, 8[eax]                 /* mm5 -= p1 right             0 1 2 3 4 5 6 7    */

    /* v[2] = (((psum + vv[2]) << 1) - (vv[5] - vv[6])) >> 4 */
    /* compute this in mm6 (left) and mm7 (right)   */
    movq   mm6, mm4                     /* mm6 = psum left             0 1 2 3 4 5 6 7    */

    paddsw mm6, 32[ebx]                 /* mm6 += vv[2] left           0 1 2 3 4 5 6 7    */
    movq   mm7, mm5                     /* mm7 = psum right            0 1 2 3 4 5 6 7    */

    paddsw mm7, 40[ebx]                 /* mm7 += vv[2] right          0 1 2 3 4 5 6 7    */
    psllw  mm6, 1                       /* mm6 <<= 1                   0 1 2 3 4 5 6 7    */

    psubsw mm6, 80[ebx]                 /* mm6 -= vv[5] left           0 1 2 3 4 5 6 7    */
    psllw  mm7, 1                       /* mm7 <<= 1                   0 1 2 3 4 5 6 7    */

    psubsw mm7, 88[ebx]                 /* mm7 -= vv[5] right          0 1 2 3 4 5 6 7    */

    movq[ecx], mm0                     /* v[1*stride] = mm0           0 1 2 3 4 5 6 7    */

    paddsw mm6, 96[ebx]                 /* mm6 += vv[6] left           0 1 2 3 4 5 6 7    */
    add   ecx, stride                    /* ecx points at v[2*stride]   0 1 2 3 4 5 6 7    */

    paddsw mm7, 104[ebx]                /* mm7 += vv[6] right          0 1 2 3 4 5 6 7    */

    /* psum += vv[6] - p1 */

    paddsw mm4, 96[ebx]                 /* mm4 += vv[6] left           0 1 2 3 4 5 6 7    */
    psrlw  mm6, 4                       /* mm6 >>= 4                   0 1 2 3 4 5 6 7    */

    paddsw mm5, 104[ebx]                /* mm5 += vv[6] right          0 1 2 3 4 5 6 7    */
    psrlw  mm7, 4                       /* mm7 >>= 4                   0 1 2 3 4 5 6 7    */

    psubsw mm4, [eax]                   /* mm4 -= p1 left              0 1 2 3 4 5 6 7    */
    packuswb mm6, mm7                   /* pack mm7, mm6 to mm6        0 1 2 3 4 5 6 7    */

    psubsw mm5, 8[eax]                  /* mm5 -= p1 right             0 1 2 3 4 5 6 7    */

    /* v[3] = (((psum + vv[3]) << 1) - (vv[6] - vv[7])) >> 4 */
    /* compute this in mm0 (left) and mm1 (right)    */

    movq   mm0, mm4                     /* mm0 = psum left             0 1 2 3 4 5 6 7    */

    paddsw mm0, 48[ebx]                 /* mm0 += vv[3] left           0 1 2 3 4 5 6 7    */
    movq   mm1, mm5                     /* mm1 = psum right            0 1 2 3 4 5 6 7    */

    paddsw mm1, 56[ebx]                 /* mm1 += vv[3] right          0 1 2 3 4 5 6 7    */
    psllw  mm0, 1                       /* mm0 <<= 1                   0 1 2 3 4 5 6 7    */

    psubsw mm0, 96[ebx]                 /* mm0 -= vv[6] left           0 1 2 3 4 5 6 7    */
    psllw  mm1, 1                       /* mm1 <<= 1                   0 1 2 3 4 5 6 7    */

    psubsw mm1, 104[ebx]                /* mm1 -= vv[6] right          0 1 2 3 4 5 6 7    */

    movq[ecx], mm6                   /* v[2*stride] = mm6           0 1 2 3 4 5 6 7    */
    paddsw mm0, 112[ebx]                /* mm0 += vv[7] left           0 1 2 3 4 5 6 7    */

    paddsw mm1, 120[ebx]                /* mm1 += vv[7] right          0 1 2 3 4 5 6 7    */
    add   ecx, stride                   /* ecx points at v[3*stride]   0 1 2 3 4 5 6 7    */

    /* psum += vv[7] - p1 */
    paddsw mm4, 112[ebx]                /* mm4 += vv[5] left           0 1 2 3 4 5 6 7    */
    psrlw  mm0, 4                       /* mm0 >>= 4                   0 1 2 3 4 5 6 7    */

    paddsw mm5, 120[ebx]                /* mm5 += vv[5] right          0 1 2 3 4 5 6 7    */
    psrlw  mm1, 4                       /* mm1 >>= 4                   0 1 2 3 4 5 6 7    */

    psubsw mm4, [eax]                   /* mm4 -= p1 left              0 1 2 3 4 5 6 7    */
    packuswb mm0, mm1                   /* pack mm1, mm0 to mm0        0 1 2 3 4 5 6 7    */

    psubsw mm5, 8[eax]                  /* mm5 -= p1 right             0 1 2 3 4 5 6 7    */

    /* v[4] = (((psum + vv[4]) << 1) + p1 - vv[1] - (vv[7] - vv[8])) >> 4 */
    /* compute this in mm6 (left) and mm7 (right)    */
    movq[ecx], mm0                   /* v[3*stride] = mm0           0 1 2 3 4 5 6 7    */
    movq   mm6, mm4                     /* mm6 = psum left             0 1 2 3 4 5 6 7    */

    paddsw mm6, 64[ebx]                 /* mm6 += vv[4] left           0 1 2 3 4 5 6 7    */
    movq   mm7, mm5                     /* mm7 = psum right            0 1 2 3 4 5 6 7    */

    paddsw mm7, 72[ebx]                 /* mm7 += vv[4] right          0 1 2 3 4 5 6 7    */
    psllw  mm6, 1                       /* mm6 <<= 1                   0 1 2 3 4 5 6 7    */

    paddsw mm6, [eax]                   /* mm6 += p1 left              0 1 2 3 4 5 6 7    */
    psllw  mm7, 1                       /* mm7 <<= 1                   0 1 2 3 4 5 6 7    */

    paddsw mm7, 8[eax]                  /* mm7 += p1 right             0 1 2 3 4 5 6 7    */

    psubsw mm6, 16[ebx]                 /* mm6 -= vv[1] left           0 1 2 3 4 5 6 7    */

    psubsw mm7, 24[ebx]                 /* mm7 -= vv[1] right          0 1 2 3 4 5 6 7    */

    psubsw mm6, 112[ebx]                /* mm6 -= vv[7] left           0 1 2 3 4 5 6 7    */

    psubsw mm7, 120[ebx]                /* mm7 -= vv[7] right          0 1 2 3 4 5 6 7    */

    paddsw mm6, 128[ebx]                /* mm6 += vv[8] left           0 1 2 3 4 5 6 7    */
    add   ecx, stride                   /* ecx points at v[4*stride]   0 1 2 3 4 5 6 7    */

    paddsw mm7, 136[ebx]                /* mm7 += vv[8] right          0 1 2 3 4 5 6 7    */
    /* psum += vv[8] - vv[1] */

    paddsw mm4, 128[ebx]                /* mm4 += vv[5] left           0 1 2 3 4 5 6 7    */
    psrlw  mm6, 4                       /* mm6 >>= 4                   0 1 2 3 4 5 6 7    */

    paddsw mm5, 136[ebx]                /* mm5 += vv[5] right          0 1 2 3 4 5 6 7    */
    psrlw  mm7, 4                       /* mm7 >>= 4                   0 1 2 3 4 5 6 7    */

    psubsw mm4, 16[ebx]                 /* mm4 -= vv[1] left           0 1 2 3 4 5 6 7    */
    packuswb mm6, mm7                   /* pack mm7, mm6 to mm6        0 1 2 3 4 5 6 7    */

    psubsw mm5, 24[ebx]                 /* mm5 -= vv[1] right          0 1 2 3 4 5 6 7    */

    /* v[5] = (((psum + vv[5]) << 1) + (vv[1] - vv[2]) - vv[8] + p2) >> 4 */
    /* compute this in mm0 (left) and mm1 (right)    */
    movq   mm0, mm4                     /* mm0 = psum left             0 1 2 3 4 5 6 7    */

    paddsw mm0, 80[ebx]                 /* mm0 += vv[5] left           0 1 2 3 4 5 6 7    */
    movq   mm1, mm5                     /* mm1 = psum right            0 1 2 3 4 5 6 7    */

    paddsw mm1, 88[ebx]                 /* mm1 += vv[5] right          0 1 2 3 4 5 6 7    */
    psllw  mm0, 1                       /* mm0 <<= 1                   0 1 2 3 4 5 6 7    */

    paddsw mm0, 16[eax]                 /* mm0 += p2 left              0 1 2 3 4 5 6 7    */
    psllw  mm1, 1                       /* mm1 <<= 1                   0 1 2 3 4 5 6 7    */

    paddsw mm1, 24[eax]                 /* mm1 += p2 right             0 1 2 3 4 5 6 7    */

    paddsw mm0, 16[ebx]                 /* mm0 += vv[1] left           0 1 2 3 4 5 6 7    */
    movq[ecx], mm6                   /* v[4*stride] = mm6           0 1 2 3 4 5 6 7    */

    paddsw mm1, 24[ebx]                 /* mm1 += vv[1] right          0 1 2 3 4 5 6 7    */

    psubsw mm0, 32[ebx]                 /* mm0 -= vv[2] left           0 1 2 3 4 5 6 7    */

    psubsw mm1, 40[ebx]                 /* mm1 -= vv[2] right          0 1 2 3 4 5 6 7    */

    psubsw mm0, 128[ebx]                /* mm0 -= vv[8] left           0 1 2 3 4 5 6 7    */

    psubsw mm1, 136[ebx]                /* mm1 -= vv[8] right          0 1 2 3 4 5 6 7    */

    /* psum += p2 - vv[2] */
    paddsw mm4, 16[eax]                 /* mm4 += p2 left              0 1 2 3 4 5 6 7    */
    add   ecx, stride                   /* ecx points at v[5*stride]   0 1 2 3 4 5 6 7    */

    paddsw mm5, 24[eax]                 /* mm5 += p2 right             0 1 2 3 4 5 6 7    */

    psubsw mm4, 32[ebx]                 /* mm4 -= vv[2] left           0 1 2 3 4 5 6 7    */

    psubsw mm5, 40[ebx]                 /* mm5 -= vv[2] right          0 1 2 3 4 5 6 7    */

    /* v[6] = (((psum + vv[6]) << 1) + (vv[2] - vv[3])) >> 4 */
    /* compute this in mm6 (left) and mm7 (right)    */

    movq   mm6, mm4                     /* mm6 = psum left             0 1 2 3 4 5 6 7    */

    paddsw mm6, 96[ebx]                 /* mm6 += vv[6] left           0 1 2 3 4 5 6 7    */
    movq   mm7, mm5                     /* mm7 = psum right            0 1 2 3 4 5 6 7    */

    paddsw mm7, 104[ebx]                /* mm7 += vv[6] right          0 1 2 3 4 5 6 7    */
    psllw  mm6, 1                       /* mm6 <<= 1                   0 1 2 3 4 5 6 7    */

    paddsw mm6, 32[ebx]                 /* mm6 += vv[2] left           0 1 2 3 4 5 6 7    */
    psllw  mm7, 1                       /* mm7 <<= 1                   0 1 2 3 4 5 6 7    */

    paddsw mm7, 40[ebx]                 /* mm7 += vv[2] right          0 1 2 3 4 5 6 7    */
    psrlw  mm0, 4                       /* mm0 >>= 4                   0 1 2 3 4 5 6 7    */

    psubsw mm6, 48[ebx]                 /* mm6 -= vv[3] left           0 1 2 3 4 5 6 7    */
    psrlw  mm1, 4                       /* mm1 >>= 4                   0 1 2 3 4 5 6 7    */

    psubsw mm7, 56[ebx]                 /* mm7 -= vv[3] right          0 1 2 3 4 5 6 7    */
    packuswb mm0, mm1                   /* pack mm1, mm0 to mm0        0 1 2 3 4 5 6 7    */

    movq[ecx], mm0                   /* v[5*stride] = mm0           0 1 2 3 4 5 6 7    */

    /* psum += p2 - vv[3] */

    paddsw mm4, 16[eax]                 /* mm4 += p2 left              0 1 2 3 4 5 6 7    */
    psrlw  mm6, 4                       /* mm6 >>= 4                   0 1 2 3 4 5 6 7    */

    paddsw mm5, 24[eax]                 /* mm5 += p2 right             0 1 2 3 4 5 6 7    */
    psrlw  mm7, 4                       /* mm7 >>= 4                   0 1 2 3 4 5 6 7    */

    psubsw mm4, 48[ebx]                 /* mm4 -= vv[3] left           0 1 2 3 4 5 6 7    */
    add   ecx, stride                   /* ecx points at v[6*stride]   0 1 2 3 4 5 6 7    */

    psubsw mm5, 56[ebx]                 /* mm5 -= vv[3] right           0 1 2 3 4 5 6 7    */

    /* v[7] = (((psum + vv[7]) << 1) + (vv[3] - vv[4])) >> 4 */
    /* compute this in mm0 (left) and mm1 (right)     */

    movq   mm0, mm4                     /* mm0 = psum left             0 1 2 3 4 5 6 7    */

    paddsw mm0, 112[ebx]                /* mm0 += vv[7] left           0 1 2 3 4 5 6 7    */
    movq   mm1, mm5                     /* mm1 = psum right            0 1 2 3 4 5 6 7    */

    paddsw mm1, 120[ebx]                /* mm1 += vv[7] right          0 1 2 3 4 5 6 7    */
    psllw  mm0, 1                       /* mm0 <<= 1                   0 1 2 3 4 5 6 7    */

    paddsw mm0, 48[ebx]                 /* mm0 += vv[3] left           0 1 2 3 4 5 6 7    */
    psllw  mm1, 1                       /* mm1 <<= 1                   0 1 2 3 4 5 6 7    */

    paddsw mm1, 56[ebx]                 /* mm1 += vv[3] right          0 1 2 3 4 5 6 7    */
    packuswb mm6, mm7                   /* pack mm7, mm6 to mm6        0 1 2 3 4 5 6 7    */

    psubsw mm0, 64[ebx]                 /* mm0 -= vv[4] left           0 1 2 3 4 5 6 7    */

    psubsw mm1, 72[ebx]                 /* mm1 -= vv[4] right          0 1 2 3 4 5 6 7    */
    psrlw  mm0, 4                       /* mm0 >>= 4                   0 1 2 3 4 5 6 7    */

    movq[ecx], mm6                   /* v[6*stride] = mm6           0 1 2 3 4 5 6 7    */

    /* psum += p2 - vv[4] */

    paddsw mm4, 16[eax]                 /* mm4 += p2 left               0 1 2 3 4 5 6 7    */

    paddsw mm5, 24[eax]                 /* mm5 += p2 right              0 1 2 3 4 5 6 7    */
    add    ecx, stride                  /* ecx points at v[7*stride]   0 1 2 3 4 5 6 7    */

    psubsw mm4, 64[ebx]                 /* mm4 -= vv[4] left            0 1 2 3 4 5 6 7    */
    psrlw  mm1, 4                       /* mm1 >>= 4                   0 1 2 3 4 5 6 7    */

    psubsw mm5, 72[ebx]                 /* mm5 -= vv[4] right 0 1 2 3 4 5 6 7    */

    /* v[8] = (((psum + vv[8]) << 1) + (vv[4] - vv[5])) >> 4 */
    /* compute this in mm6 (left) and mm7 (right)     */

    movq   mm6, mm4                     /* mm6 = psum left             0 1 2 3 4 5 6 7    */

    paddsw mm6, 128[ebx]                /* mm6 += vv[8] left           0 1 2 3 4 5 6 7    */
    movq   mm7, mm5                     /* mm7 = psum right            0 1 2 3 4 5 6 7    */

    paddsw mm7, 136[ebx]                /* mm7 += vv[8] right          0 1 2 3 4 5 6 7    */
    psllw  mm6, 1                       /* mm6 <<= 1                   0 1 2 3 4 5 6 7    */

    paddsw mm6, 64[ebx]                 /* mm6 += vv[4] left           0 1 2 3 4 5 6 7    */
    psllw  mm7, 1                       /* mm7 <<= 1                   0 1 2 3 4 5 6 7    */

    paddsw mm7, 72[ebx]                 /* mm7 += vv[4] right          0 1 2 3 4 5 6 7    */
    packuswb mm0, mm1                   /* pack mm1, mm0 to mm0        0 1 2 3 4 5 6 7    */

    psubsw mm6, 80[ebx]                 /* mm6 -= vv[5] left           0 1 2 3 4 5 6 7    */

    psubsw mm7, 88[ebx]                 /* mm7 -= vv[5] right          0 1 2 3 4 5 6 7    */
    psrlw  mm6, 4                       /* mm6 >>= 4                   0 1 2 3 4 5 6 7    */

    movq[ecx], mm0                   /* v[7*stride] = mm0           0 1 2 3 4 5 6 7    */
    psrlw  mm7, 4                       /* mm7 >>= 4                   0 1 2 3 4 5 6 7    */

    packuswb mm6, mm7                   /* pack mm7, mm6 to mm6        0 1 2 3 4 5 6 7    */

    add   ecx, stride                   /* ecx points at v[8*stride]   0 1 2 3 4 5 6 7    */

    nop                                 /*                             0 1 2 3 4 5 6 7    */

    movq[ecx], mm6                   /* v[8*stride] = mm6           0 1 2 3 4 5 6 7    */

    pop ecx
    pop ebx
    pop eax
  };

#ifdef PP_SELF_CHECK
  /* use the self-check version of the filter result in selfcheck[64] to verify the filter output */
  /* loop top->bottom */
  for (k = 0; k < 8; k++)
  {   /* loop left->right */
    for (j = 0; j < 8; j++)
    {
      uint8_t* target_check = &(v[(k + 1) * stride + j]);
      if (*target_check != selfcheck[j + k * 8])
      {
        dprintf("ERROR: problem with vertical LPF9 filter in row %d\n", k + 1);
      }
    }
  }
#endif
#ifdef ARCH_IS_IA32
  _mm_empty();
#endif
}
#endif

void deblock_vert_lpf9(uint64_t* v_local, uint64_t* p1p2, uint8_t* v, int stride) {
#ifdef USE_NEW_INTRINSICS
  deblock_vert_lpf9_ssse3(v_local, p1p2, v, stride);
#else
  deblock_vert_lpf9_mmx(v_local, p1p2, v, stride);
#endif
  // deblock_vert_lpf9_c(v_local, p1p2, v, stride);
}

/* Vertical 9-tap low-pass filter, weights 1-1-2-2-4-2-2-1-1 */
// v_local is a 9x8 uint_16 array (9*16 bytes), serves as input. Value 0-255 byte pixel values are already.
// extended to uint16_t size. The 0th eight uint16_t elements are not used, v_local contains valid data (8 * 8 uint16_t) from there.
// p1p2: an uint64_t pointer, but it really stores a 2*8 element uint16_t array: 8 different p1 values, then 8 p2 values; actual p1 and p2 are different for each j
// "uint8_t *v" this pointer goes into ecx in the mmx version. Results are stored as 8 bytes to pointers v + 1*stride to v + 8*stride
// For storing the vertical step between loops is "int stride"
// v[1] calculated from : p1, p1, p1, p1, v1, v2, v3, v4, v5
// v[2] calculated from : p1, p1, p1, v1, v2, v3, v4, v5, v6
// v[3] calculated from : p1, p1, v1, v2, v3, v4, v5, v6, v7
// v[4] calculated from : p1, v1, v2, v3, v4, v5, v6, v7, v8
// v[5] calculated from : v1, v2, v3, v4, v5, v6, v7, v8, p2
// v[6] calculated from : v2, v3, v4, v5, v6, v7, v8, p2, p2
// v[7] calculated from : v3, v4, v5, v6, v7, v8, p2, p2, p2
// v[8] calculated from : v4, v5, v6, v7, v8, p2, p2, p2, p2
// When index would go before v[1] then p1 is used there.
// When index would go after v[8] then p2 is used there.
// Weights for LowPassFilter9: 1 1 2 2 4 2 2 1 1

void deblock_vert_lpf9_c(uint64_t* v_local, uint64_t* p1p2, uint8_t* v, int stride)
{
  int j, k;
  uint8_t selfcheck[64];
  uint16_t* vv;
  int p1, p2, psum;
  /* define semi-constants to enable us to move up and down the picture easily... */
  int local_stride = 8; // for v_local
  int l1 = 1 * local_stride;
  int l2 = 2 * local_stride;
  int l3 = 3 * local_stride;
  int l4 = 4 * local_stride;
  int l5 = 5 * local_stride;
  int l6 = 6 * local_stride;
  int l7 = 7 * local_stride;
  int l8 = 8 * local_stride;

  /* generate a self-check version of the filter result in selfcheck[64] */
  /* loop left->right */
  for (j = 0; j < 8; j++)
  {
    vv = &((uint16_t*)(v_local))[j]; // 2*8 uint16_t
    p1 = ((uint16_t*)(&(p1p2[0 + j / 4])))[j % 4]; /* yuck! */
    p2 = ((uint16_t*)(&(p1p2[2 + j / 4])))[j % 4]; /* yuck! */
    /* the above may well be endian-fussy */
    psum = p1 + p1 + p1 + vv[l1] + vv[l2] + vv[l3] + vv[l4] + 4;
    // 1*p1 + 1*p1 + 2*p1 + 2*p1 + 4*vv[l1] + 2*vv[l2] + 2*vv[l3] + 1*vv[l4] + 1*vv[l4]
    selfcheck[j + 8 * 0] = (((psum + vv[l1]) << 1) - (vv[l4] - vv[l5])) >> 4;
    // moving window. Add next on the right: vv[l5], subtract one filler from the left: p1, modify the weights by adding or subtracting the appropriate vv[] element
    // and so on...
    psum += vv[l5] - p1;
    selfcheck[j + 8 * 1] = (((psum + vv[l2]) << 1) - (vv[l5] - vv[l6])) >> 4;
    psum += vv[l6] - p1;
    selfcheck[j + 8 * 2] = (((psum + vv[l3]) << 1) - (vv[l6] - vv[l7])) >> 4;
    psum += vv[l7] - p1;
    selfcheck[j + 8 * 3] = (((psum + vv[l4]) << 1) + p1 - vv[l1] - (vv[l7] - vv[l8])) >> 4;
    psum += vv[l8] - vv[l1];
    selfcheck[j + 8 * 4] = (((psum + vv[l5]) << 1) + (vv[l1] - vv[l2]) - vv[l8] + p2) >> 4;
    psum += p2 - vv[l2];
    selfcheck[j + 8 * 5] = (((psum + vv[l6]) << 1) + (vv[l2] - vv[l3])) >> 4;
    psum += p2 - vv[l3];
    selfcheck[j + 8 * 6] = (((psum + vv[l7]) << 1) + (vv[l3] - vv[l4])) >> 4;
    psum += p2 - vv[l4];
    selfcheck[j + 8 * 7] = (((psum + vv[l8]) << 1) + (vv[l4] - vv[l5])) >> 4;
  }
  /* use the self-check version of the filter result in selfcheck[64] to verify the filter output */
  /* loop top->bottom */
  for (k = 0; k < 8; k++)
  {   /* loop left->right */
    for (j = 0; j < 8; j++) {
      v[(k + 1) * stride + j] = selfcheck[j + k * 8];
    }
  }
}

#if 0
// similar to the horizontal version, but input structure is a bit different
// rather like following the brute logic of summing data*weights
// we have better version
void deblock_vert_lpf9_ssse3_b(uint64_t* v_local, uint64_t* p1p2, uint8_t* v, int stride) {
  int p1, p2;
  int local_stride = 8;
  /* define semi-constants to enable us to move up and down the picture easily... */
  int l1 = 1 * local_stride;
  int l2 = 2 * local_stride;
  int l3 = 3 * local_stride;
  int l4 = 4 * local_stride;
  int l5 = 5 * local_stride;
  int l6 = 6 * local_stride;
  int l7 = 7 * local_stride;
  int l8 = 8 * local_stride;

  /* The 9-tap low pass filter used in "DC" regions */
  /* loop left->right */
  for (int j = 0; j < 8; j++) {
    p1 = ((uint16_t*)(&(p1p2[0 + j / 4])))[j % 4]; /* yuck! */
    p2 = ((uint16_t*)(&(p1p2[2 + j / 4])))[j % 4]; /* yuck! */

    uint16_t* vv = &((uint16_t*)(v_local))[j]; // 2*8 uint16_t with only 8 bit valid data inside

    // Create extended pixel vector: p1,p1,p1,p1, v1-v8, p2,p2,p2,p2
    __m128i pixels = _mm_set_epi8(
      p2, p2, p2, p2,                   // Last 4 bytes
      vv[l8], vv[l7], vv[l6], vv[l5],       // Second half of input
      vv[l4], vv[l3], vv[l2], vv[l1],       // First half of input
      p1, p1, p1, p1                    // First 4 bytes
    );

    // Fixed weights vector: 1,1,2,2,4,2,2,1,1 (padded with zeros)
    const __m128i weights = _mm_set_epi8(
      0, 0, 0, 0, 0, 0, 0,
      1,                    // Weight for last position
      1, 2, 2, 4, 2, 2, 1, 1  // Weights for 9-point filter
    );

    // Results array for all 8 positions
    uint8_t results[8];

    // Process all 8 positions in a loop
    for (int x = 0; x < 8; x++) {
      // ssse3 unsigned * signed bytes to 16 bit signed, pre hadd-ed
      __m128i mult = _mm_maddubs_epi16(pixels, weights); // SSSE3
      // S7 S6 S5 S4 S3 S2 S1 S0
      //  0  0  0 S4 S3 S2 S1 S0 
      __m128i hadd = _mm_hadd_epi16(mult, mult); // SSSE3
      hadd = _mm_hadd_epi16(hadd, hadd);
      hadd = _mm_hadd_epi16(hadd, hadd);
      results[x] = (uint8_t)((_mm_extract_epi16(hadd, 0) + 8) >> 4); // round and /= 16
      pixels = _mm_srli_si128(pixels, 1);
    }

    // Store results back to memory
    for (int x = 0; x < 8; x++) {
      //v[x + (j+1) * stride] = results[x];
      v[(x + 1) * stride + j] = results[x];
    }
  }
}
#endif

// based on deblock_horiz_lpf9_c
// doing only smart add and subtracts from a previous temporary result
void deblock_vert_lpf9_ssse3(uint64_t* v_local, uint64_t* p1p2, uint8_t* v, int stride) {
  __m128i* vv_ptr = (__m128i*)v_local;
  __m128i* p1p2_ptr = (__m128i*)p1p2;

  // Load p1 and p2 values (each contains 8 uint16_t values)
  __m128i p1_vec = _mm_load_si128(p1p2_ptr);     // First 8 values (p1)
  __m128i p2_vec = _mm_load_si128(p1p2_ptr + 1); // Next 8 values (p2)

  // Load input rows (v1 through v8)
  __m128i v1 = _mm_load_si128(vv_ptr + 1);
  __m128i v2 = _mm_load_si128(vv_ptr + 2);
  __m128i v3 = _mm_load_si128(vv_ptr + 3);
  __m128i v4 = _mm_load_si128(vv_ptr + 4);
  __m128i v5 = _mm_load_si128(vv_ptr + 5);
  __m128i v6 = _mm_load_si128(vv_ptr + 6);
  __m128i v7 = _mm_load_si128(vv_ptr + 7);
  __m128i v8 = _mm_load_si128(vv_ptr + 8);

  // Constants
  __m128i four = _mm_set1_epi16(4);

  // Pre-calculate common terms for the sliding window
  __m128i p1_x3 = _mm_add_epi16(p1_vec, _mm_add_epi16(p1_vec, p1_vec)); // p1 * 3

  // Initial sum for sliding window
  __m128i base_sum = _mm_add_epi16(p1_x3, v1);
  base_sum = _mm_add_epi16(base_sum, v2);
  base_sum = _mm_add_epi16(base_sum, v3);
  base_sum = _mm_add_epi16(base_sum, v4);
  base_sum = _mm_add_epi16(base_sum, four); // Add rounding factor, factor is 8 but will get multiplied by 2 soon

  // Process each output row using sliding window approach
  __m128i result;
  __m128i window_sum;

  // selfcheck[1]
  // v[1] = (p1 + p1 + 2*p1 + 2*p1 + 4*v1 + v2*2 + v3*2 + v4 + v5) >> 4
  window_sum = _mm_add_epi16(base_sum, v1);
  window_sum = _mm_add_epi16(window_sum, window_sum);
  window_sum = _mm_add_epi16(window_sum, v5);
  window_sum = _mm_sub_epi16(window_sum, v4);
  result = _mm_srli_epi16(window_sum, 4);
  result = _mm_packus_epi16(result, result);
  _mm_storel_epi64((__m128i*)(v + 1 * stride), result);

  // selfcheck[2]
  // Slide window: subtract one p1, add v6 and others, see C implementation
  base_sum = _mm_sub_epi16(base_sum, p1_vec);
  base_sum = _mm_add_epi16(base_sum, v5);
  window_sum = _mm_add_epi16(base_sum, v2);
  window_sum = _mm_add_epi16(window_sum, window_sum);
  window_sum = _mm_add_epi16(window_sum, v6);
  window_sum = _mm_sub_epi16(window_sum, v5);
  result = _mm_srli_epi16(window_sum, 4);
  result = _mm_packus_epi16(result, result);
  _mm_storel_epi64((__m128i*)(v + 2 * stride), result);

  // selfcheck[3]
  // Slide window: subtract one p1, add v7 and others, see C implementation
  base_sum = _mm_sub_epi16(base_sum, p1_vec);
  base_sum = _mm_add_epi16(base_sum, v6);
  window_sum = _mm_add_epi16(base_sum, v3);
  window_sum = _mm_add_epi16(window_sum, window_sum);
  window_sum = _mm_add_epi16(window_sum, v7);
  window_sum = _mm_sub_epi16(window_sum, v6);
  result = _mm_srli_epi16(window_sum, 4);
  result = _mm_packus_epi16(result, result);
  _mm_storel_epi64((__m128i*)(v + 3 * stride), result);

  // selfcheck[4]
  // Slide window: subtract p1, add v8, adjust weights and others, see C implementation
  base_sum = _mm_sub_epi16(base_sum, p1_vec);
  base_sum = _mm_add_epi16(base_sum, v7);
  window_sum = _mm_add_epi16(base_sum, v4);
  window_sum = _mm_add_epi16(window_sum, window_sum);
  window_sum = _mm_add_epi16(window_sum, p1_vec);
  window_sum = _mm_sub_epi16(window_sum, v1);
  window_sum = _mm_add_epi16(window_sum, v8);
  window_sum = _mm_sub_epi16(window_sum, v7);
  result = _mm_srli_epi16(window_sum, 4);
  result = _mm_packus_epi16(result, result);
  _mm_storel_epi64((__m128i*)(v + 4 * stride), result);

  // selfcheck[5]
  // Middle point: transition from p1 to p2 and others, see C implementation
  base_sum = _mm_sub_epi16(base_sum, v1);
  base_sum = _mm_add_epi16(base_sum, v8);
  window_sum = _mm_add_epi16(base_sum, v5);
  window_sum = _mm_add_epi16(window_sum, window_sum);
  window_sum = _mm_add_epi16(window_sum, v1);
  window_sum = _mm_sub_epi16(window_sum, v2);
  window_sum = _mm_add_epi16(window_sum, p2_vec);
  window_sum = _mm_sub_epi16(window_sum, v8);
  result = _mm_srli_epi16(window_sum, 4);
  result = _mm_packus_epi16(result, result);
  _mm_storel_epi64((__m128i*)(v + 5 * stride), result);

  // selfcheck[6]
  // Slide window: replace v2 with p2 and others, see C implementation
  base_sum = _mm_sub_epi16(base_sum, v2);
  base_sum = _mm_add_epi16(base_sum, p2_vec);
  window_sum = _mm_add_epi16(base_sum, v6);
  window_sum = _mm_add_epi16(window_sum, window_sum);
  window_sum = _mm_add_epi16(window_sum, v2);
  window_sum = _mm_sub_epi16(window_sum, v3);
  result = _mm_srli_epi16(window_sum, 4);
  result = _mm_packus_epi16(result, result);
  _mm_storel_epi64((__m128i*)(v + 6 * stride), result);

  // selfcheck[7]
  // Slide window: replace v3 with p2 and others, see C implementation
  base_sum = _mm_sub_epi16(base_sum, v3);
  base_sum = _mm_add_epi16(base_sum, p2_vec);
  window_sum = _mm_add_epi16(base_sum, v7);
  window_sum = _mm_add_epi16(window_sum, window_sum);
  window_sum = _mm_add_epi16(window_sum, v3);
  window_sum = _mm_sub_epi16(window_sum, v4);
  result = _mm_srli_epi16(window_sum, 4);
  result = _mm_packus_epi16(result, result);
  _mm_storel_epi64((__m128i*)(v + 7 * stride), result);

  // selfcheck[8]
  // Final row: replace v4 with p2 and others, see C implementation
  base_sum = _mm_sub_epi16(base_sum, v4);
  base_sum = _mm_add_epi16(base_sum, p2_vec);
  window_sum = _mm_add_epi16(base_sum, v8);
  window_sum = _mm_add_epi16(window_sum, window_sum);
  window_sum = _mm_add_epi16(window_sum, v4);
  window_sum = _mm_sub_epi16(window_sum, v5);
  result = _mm_srli_epi16(window_sum, 4);
  result = _mm_packus_epi16(result, result);
  _mm_storel_epi64((__m128i*)(v + 8 * stride), result);

}

/* decide DC mode or default mode in assembler */
// C conversion by PF
int deblock_vert_useDC_c(uint8_t* v, int stride, int moderate_v)
{
  int eq_cnt, useDC;
  int useDC2, i, j;

  /* C-code version for testing */
  eq_cnt = 0;
  for (j = 1; j < 8; j++)
  {
    for (i = 0; i < 8; i++)
    {
      if (ABS(v[j * stride + i] - v[(j + 1) * stride + i]) <= 1) eq_cnt++;
    }
  }
  useDC2 = (eq_cnt > moderate_v);

  return useDC2;
}

// new helper function SSE2 compat
int my_mm_popcnt_epi8(__m128i x) {
  // Count the number of set bits in each byte
  x = _mm_sub_epi8(x, _mm_and_si128(_mm_srli_epi16(x, 1), _mm_set1_epi8(0x55)));
  x = _mm_add_epi8(_mm_and_si128(x, _mm_set1_epi8(0x33)), _mm_and_si128(_mm_srli_epi16(x, 2), _mm_set1_epi8(0x33)));
  x = _mm_add_epi8(x, _mm_srli_epi16(x, 4));
  x = _mm_and_si128(x, _mm_set1_epi8(0x0F));

  // Sum the counts in each byte
  __m128i sum1 = _mm_sad_epu8(x, _mm_setzero_si128());
  return _mm_extract_epi16(sum1, 0) + _mm_extract_epi16(sum1, 4);
}

#ifdef USE_NEW_INTRINSICS
// PF converted to intrinsics, generated from C original
int deblock_vert_useDC(uint8_t* v, int stride, int moderate_v) {
  int eq_cnt = 0;

  __m128i one = _mm_set1_epi8(1);

  for (int j = 1; j < 8; j++) {
    for (int i = 0; i < 8; i += 16) {
      __m128i curr_row = _mm_loadu_si128((__m128i*) & v[j * stride + i]);
      __m128i next_row = _mm_loadu_si128((__m128i*) & v[(j + 1) * stride + i]);

      __m128i diff1 = _mm_subs_epu8(curr_row, next_row);
      __m128i diff2 = _mm_subs_epu8(next_row, curr_row);
      __m128i abs_diff = _mm_max_epu8(diff1, diff2);

      __m128i cmp = _mm_cmpeq_epi8(abs_diff, one);
      eq_cnt += my_mm_popcnt_epi8(cmp);
    }
  }

  int useDC2 = (eq_cnt > moderate_v);
  return useDC2;
}

#else
/* decide DC mode or default mode in assembler */
int deblock_vert_useDC(uint8_t* v, int stride, int moderate_v)
{
  const uint64_t mask = 0xfefefefefefefefe;
  uint32_t mm_data1;
  uint64_t* pmm1;
  int eq_cnt, useDC;
#ifdef PP_SELF_CHECK
  int useDC2, i, j;
#endif

#ifdef PP_SELF_CHECK
  /* C-code version for testing */
  eq_cnt = 0;
  for (j = 1; j < 8; j++)
  {
    for (i = 0; i < 8; i++)
    {
      if (ABS(v[j * stride + i] - v[(j + 1) * stride + i]) <= 1) eq_cnt++;
    }
  }
  useDC2 = (eq_cnt > moderate_v);
#endif

  /* starting pointer is at v[stride] == v1 in mpeg4 notation */
  pmm1 = (uint64_t*)(&(v[stride]));

  /* first load some constants into mm4, mm6, mm7 */
  __asm
  {
    push eax
    mov eax, pmm1

    movq mm6, mask               /*mm6 = 0xfefefefefefefefe       */
    pxor mm7, mm7                /*mm7 = 0x0000000000000000       */

    movq mm2, [eax]              /* mm2 = *p_data                 */
    pxor mm4, mm4                /*mm4 = 0x0000000000000000       */

    add   eax, stride            /* p_data += stride              */
    movq   mm3, mm2              /* mm3 = *p_data                 */
  };

  __asm
  {
    movq   mm2, [eax]           /* mm2 = *p_data                 */
    movq   mm0, mm3             /* mm0 = mm3                     */

    movq   mm3, mm2             /* mm3 = *p_data                 */
    movq   mm1, mm0             /* mm1 = mm0                     */

    psubusb mm0, mm2            /* mm0 -= mm2                    */
    add   eax, stride           /* p_data += stride              */

    psubusb mm2, mm1            /* mm2 -= mm1                    */
    por    mm0, mm2             /* mm0 |= mm2                    */

    pand   mm0, mm6             /* mm0 &= 0xfefefefefefefefe     */
    pcmpeqb mm0, mm4            /* is mm0 == 0 ?                 */

    movq   mm2, [eax]           /* mm2 = *p_data                 */
    psubb  mm7, mm0             /* mm7 has running total of eqcnts */

    movq   mm5, mm3             /* mm5 = mm3                     */
    movq   mm3, mm2             /* mm3 = *p_data                 */

    movq   mm1, mm5             /* mm1 = mm5                     */
    psubusb mm5, mm2            /* mm5 -= mm2                    */

    psubusb mm2, mm1            /* mm2 -= mm1                    */
    por    mm5, mm2             /* mm5 |= mm2                    */

    add   eax, stride           /* p_data += stride              */
    pand   mm5, mm6             /* mm5 &= 0xfefefefefefefefe     */

    pcmpeqb mm5, mm4            /* is mm0 == 0 ?                 */
    psubb  mm7, mm5             /* mm7 has running total of eqcnts */

    movq   mm2, [eax]           /* mm2 = *p_data                 */
    movq   mm0, mm3             /* mm0 = mm3                     */

    movq   mm3, mm2             /* mm3 = *p_data                 */
    movq   mm1, mm0             /* mm1 = mm0                     */

    psubusb mm0, mm2            /* mm0 -= mm2                    */
    add   eax, stride           /* p_data += stride              */

    psubusb mm2, mm1            /* mm2 -= mm1                    */
    por    mm0, mm2             /* mm0 |= mm2                    */

    pand   mm0, mm6             /* mm0 &= 0xfefefefefefefefe     */
    pcmpeqb mm0, mm4            /* is mm0 == 0 ?                 */

    movq   mm2, [eax]           /* mm2 = *p_data                 */
    psubb  mm7, mm0             /* mm7 has running total of eqcnts */

    movq   mm5, mm3             /* mm5 = mm3                     */
    movq   mm3, mm2             /* mm3 = *p_data                 */

    movq   mm1, mm5             /* mm1 = mm5                     */
    psubusb mm5, mm2            /* mm5 -= mm2                    */

    psubusb mm2, mm1            /* mm2 -= mm1                    */
    por    mm5, mm2             /* mm5 |= mm2                    */

    add   eax, stride           /* p_data += stride              */
    pand   mm5, mm6             /* mm5 &= 0xfefefefefefefefe     */

    pcmpeqb mm5, mm4            /* is mm0 == 0 ?                 */
    psubb  mm7, mm5             /* mm7 has running total of eqcnts */

    movq   mm2, [eax]           /* mm2 = *p_data                 */
    movq   mm0, mm3             /* mm0 = mm3                     */

    movq   mm3, mm2             /* mm3 = *p_data                 */
    movq   mm1, mm0             /* mm1 = mm0                     */

    psubusb mm0, mm2            /* mm0 -= mm2                    */
    add   eax, stride           /* p_data += stride              */

    psubusb mm2, mm1            /* mm2 -= mm1                    */
    por    mm0, mm2             /* mm0 |= mm2                    */

    pand   mm0, mm6             /* mm0 &= 0xfefefefefefefefe     */
    pcmpeqb mm0, mm4            /* is mm0 == 0 ?                 */

    movq   mm2, [eax]           /* mm2 = *p_data                 */
    psubb  mm7, mm0             /* mm7 has running total of eqcnts */

    movq   mm5, mm3             /* mm5 = mm3                     */
    movq   mm3, mm2             /* mm3 = *p_data                 */

    movq   mm1, mm5             /* mm1 = mm5                     */
    psubusb mm5, mm2            /* mm5 -= mm2                    */

    psubusb mm2, mm1            /* mm2 -= mm1                    */
    por    mm5, mm2             /* mm5 |= mm2                    */

    add   eax, stride           /* p_data += stride              */
    pand   mm5, mm6             /* mm5 &= 0xfefefefefefefefe     */

    pcmpeqb mm5, mm4            /* is mm0 == 0 ?                 */
    psubb  mm7, mm5             /* mm7 has running total of eqcnts */

    movq   mm2, [eax]           /* mm2 = *p_data                 */
    movq   mm0, mm3             /* mm0 = mm3                     */

    movq   mm3, mm2             /* mm3 = *p_data                 */
    movq   mm1, mm0             /* mm1 = mm0                     */

    psubusb mm0, mm2            /* mm0 -= mm2                    */
    add   eax, stride           /* p_data += stride              */

    psubusb mm2, mm1            /* mm2 -= mm1                    */
    por    mm0, mm2             /* mm0 |= mm2                    */

    pand   mm0, mm6             /* mm0 &= 0xfefefefefefefefe     */
    pcmpeqb mm0, mm4            /* is mm0 == 0 ?                 */

    psubb  mm7, mm0             /* mm7 has running total of eqcnts */

    pop eax
  };

  /* now mm7 contains negative eq_cnt for all 8-columns */
  /* copy this to mm_data1                              */
  /* sum all 8 bytes in mm7 */
  __asm
  {
    movq    mm1, mm7            /* mm1 = mm7             0 1w2 3 4 5 6 7r   */
    psrlq   mm7, 32             /* mm7 >>= 32            0 1 2 3 4 5 6 7m   */

    paddb   mm7, mm1            /* mm7 has running total of eqcnts */

    movq mm1, mm7               /* mm1 = mm7             0 1w2 3 4 5 6 7r   */
    psrlq   mm7, 16             /* mm7 >>= 16            0 1 2 3 4 5 6 7m   */

    paddb   mm1, mm7            /* mm7 has running total of eqcnts */

    movq mm7, mm1               /* mm1 = mm7             0 1w2 3 4 5 6 7r   */
    psrlq   mm7, 8              /* mm7 >>= 8             0 1 2 3 4 5 6 7m   */

    paddb   mm7, mm1            /* mm7 has running total of eqcnts */

    movd mm_data1, mm7          /* mm_data1 = mm7       */
  };

  eq_cnt = mm_data1 & 0xff;

  useDC = (eq_cnt > moderate_v);

#ifdef PP_SELF_CHECK
  if (useDC != useDC2) dprintf("ERROR: MMX version of useDC is incorrect\n");
#endif

#ifdef ARCH_IS_IA32
  _mm_empty();
#endif
  return useDC;
}
#endif

#ifdef USE_NEW_INTRINSICS
// PF converted to intrinsics sse4.2 or maybe less
void dering(uint8_t* image, int height, int width, int quant) {
  int stride = width;
  int x, y;
  uint8_t* b8x8, * b10x10;
  uint8_t b8x8filtered[64];
  int max_diff;
  uint8_t min, max, thr, range;
  uint8_t max_diffq[8];  // a qword value of max_diff

  // loop over all the 8x8 blocks in the image...
  // don't process outer row of blocks for the time being.
  for (y = 8; y < height - 8; y += 8) {
    for (x = 8; x < width - 8; x += 8) {
      // pointer to the top left pixel in 8x8 block
      b8x8 = &(image[stride * y + x]);

      // pointer to the top left pixel in 10x10 block
      b10x10 = &(image[stride * (y - 1) + (x - 1)]);

      {
        // Threshold determination - find min and max grey levels in the block 
        __m128i min_val = _mm_loadl_epi64((__m128i*)b8x8);
        __m128i max_val = min_val;

        for (int i = 1; i < 8; ++i) {
          __m128i row = _mm_loadl_epi64((__m128i*)(b8x8 + i * stride));
          min_val = _mm_min_epu8(min_val, row);
          max_val = _mm_max_epu8(max_val, row);
        }

        // get min of 8 bytes in min_val
        min_val = _mm_min_epu8(min_val, _mm_srli_si128(min_val, 4));
        min_val = _mm_min_epu8(min_val, _mm_srli_si128(min_val, 2));
        min_val = _mm_min_epu8(min_val, _mm_srli_si128(min_val, 1));
        min = _mm_extract_epi8(min_val, 0);

        // get max of 8 bytes in max_val
        max_val = _mm_max_epu8(max_val, _mm_srli_si128(max_val, 4));
        max_val = _mm_max_epu8(max_val, _mm_srli_si128(max_val, 2));
        max_val = _mm_max_epu8(max_val, _mm_srli_si128(max_val, 1));
        max = _mm_extract_epi8(max_val, 0);
      }
      // Threshold determination - compute threshold and dynamic range
      thr = (max + min + 1) >> 1;
      range = max - min;

      max_diff = quant >> 1;
      for (int i = 0; i < 8; ++i) {
        max_diffq[i] = (uint8_t)max_diff;
      }

      // Fill in b10x10 array completely with  2 dim center weighted average
      // This section now also includes the clipping step.
      // (Implementation of this part would follow similarly using SSE4.2 intrinsics)

      // 2nd asm block
// Threshold determination - find min and max grey levels in the block
      {
        __m128i min_val = _mm_loadl_epi64((__m128i*)b8x8);
        __m128i max_val = min_val;

        for (int i = 1; i < 8; ++i) {
          __m128i row = _mm_loadl_epi64((__m128i*)(b8x8 + i * stride));
          min_val = _mm_min_epu8(min_val, row);
          max_val = _mm_max_epu8(max_val, row);
        }

        // get min of 8 bytes in min_val
        min_val = _mm_min_epu8(min_val, _mm_srli_si128(min_val, 4));
        min_val = _mm_min_epu8(min_val, _mm_srli_si128(min_val, 2));
        min_val = _mm_min_epu8(min_val, _mm_srli_si128(min_val, 1));
        min = _mm_extract_epi8(min_val, 0);

        // get max of 8 bytes in max_val
        max_val = _mm_max_epu8(max_val, _mm_srli_si128(max_val, 4));
        max_val = _mm_max_epu8(max_val, _mm_srli_si128(max_val, 2));
        max_val = _mm_max_epu8(max_val, _mm_srli_si128(max_val, 1));
        max = _mm_extract_epi8(max_val, 0);

        // Threshold determination - compute threshold and dynamic range
        thr = (max + min + 1) >> 1;
        range = max - min;

        max_diff = quant >> 1;
        for (int i = 0; i < 8; ++i) {
          max_diffq[i] = (uint8_t)max_diff;
        }

        // Fill in b10x10 array completely with 2 dim center weighted average
        // This section now also includes the clipping step.
        __m128i max_diff_vec = _mm_set1_epi8((char)max_diff);

        for (int i = 0; i < 8; ++i) {
          __m128i line0 = _mm_loadl_epi64((__m128i*)(b10x10 + i * stride));
          __m128i line1 = _mm_loadl_epi64((__m128i*)(b10x10 + (i + 1) * stride));
          __m128i line2 = _mm_loadl_epi64((__m128i*)(b10x10 + (i + 2) * stride));

          __m128i avg1 = _mm_avg_epu8(line0, line2);
          __m128i avg2 = _mm_avg_epu8(avg1, line1);

          __m128i min_clip = _mm_subs_epu8(line1, max_diff_vec);
          __m128i max_clip = _mm_adds_epu8(line1, max_diff_vec);

          avg2 = _mm_max_epu8(avg2, min_clip);
          avg2 = _mm_min_epu8(avg2, max_clip);

          _mm_storel_epi64((__m128i*)(b8x8filtered + i * 8), avg2);
        }
      }

      // 3rd asm block
      {
        __m128i thr_vec = _mm_set1_epi8(thr);
        __m128i zero = _mm_setzero_si128();

        for (int i = 0; i < 8; ++i) {
          __m128i line_m1 = _mm_loadl_epi64((__m128i*)(b10x10 + (i - 1) * stride + 1));
          __m128i line0 = _mm_loadl_epi64((__m128i*)(b10x10 + i * stride + 1));
          __m128i line1 = _mm_loadl_epi64((__m128i*)(b10x10 + (i + 1) * stride + 1));

          __m128i cmp_m1 = _mm_cmpeq_epi8(_mm_subs_epu8(thr_vec, line_m1), zero);
          __m128i cmp0 = _mm_cmpeq_epi8(_mm_subs_epu8(thr_vec, line0), zero);
          __m128i cmp1 = _mm_cmpeq_epi8(_mm_subs_epu8(thr_vec, line1), zero);

          __m128i avg_m1 = _mm_avg_epu8(line_m1, line0);
          __m128i avg0 = _mm_avg_epu8(line0, line1);
          __m128i avg1 = _mm_avg_epu8(line1, line_m1);

          __m128i combined_avg = _mm_avg_epu8(_mm_avg_epu8(avg_m1, avg0), avg1);

          __m128i filtered = _mm_loadl_epi64((__m128i*)(b8x8filtered + i * 8));
          __m128i result = _mm_or_si128(_mm_and_si128(cmp0, line0), _mm_andnot_si128(cmp0, filtered));

          _mm_storel_epi64((__m128i*)(b8x8 + i * stride), result);
        }
      }

    }
  }
}

#else
/* this is the de_ringing filter */
// new MMXSSE version - trbarry 3/15/2002
// modified calling sequence for smoothD2() - Jim Conklin 1-20-2013
void dering(uint8_t* image, int height, int width, int quant) {
  int stride = width;
  int x, y;
  uint8_t* b8x8, * b10x10;
  uint8_t b8x8filtered[64];
  int max_diff;
  uint8_t min, max, thr, range;
  uint8_t max_diffq[8];  // a qword value of max_diff


  /* loop over all the 8x8 blocks in the image... */
  /* don't process outer row of blocks for the time being. */
  for (y = 8; y < height - 8; y += 8)
  {
    for (x = 8; x < width - 8; x += 8)
    {

      /* pointer to the top left pixel in 8x8   block */
      b8x8 = &(image[stride * y + x]);

      /* pointer to the top left pixel in 10x10 block */
      b10x10 = &(image[stride * (y - 1) + (x - 1)]);

      // Threshold determination - find min and max grey levels in the block 
      // but remove loop through 64 bytes.  trbarry 03/13/2002

      __asm
      {
        mov    esi, b8x8    // the block addr
        mov    eax, stride    // offset to next qword in block

        movq  mm0, qword ptr[esi] // row 0, 1st qword is our min
        movq    mm1, mm0      // .. and our max

        pminub  mm0, qword ptr[esi + eax]    // row 1
        pmaxub  mm1, qword ptr[esi + eax]

        lea    esi, [esi + 2 * eax]      // bump for next 2
        pminub  mm0, qword ptr[esi]      // row 2
        pmaxub  mm1, qword ptr[esi]

        pminub  mm0, qword ptr[esi + eax]    // row 3
        pmaxub  mm1, qword ptr[esi + eax]

        lea    esi, [esi + 2 * eax]      // bump for next 2
        pminub  mm0, qword ptr[esi]      // row 4
        pmaxub  mm1, qword ptr[esi]

        pminub  mm0, qword ptr[esi + eax]    // row 5
        pmaxub  mm1, qword ptr[esi + eax]

        lea    esi, [esi + 2 * eax]      // bump for next 2
        pminub  mm0, qword ptr[esi]      // row 6
        pmaxub  mm1, qword ptr[esi]

        pminub  mm0, qword ptr[esi + eax]    // row 7
        pmaxub  mm1, qword ptr[esi + eax]

        // get min of 8 bytes in mm0
        pshufw  mm2, mm0, (3 << 2) + 2      // words 3,2 into low words of mm2
        pminub  mm0, mm2            // now 4 min bytes in low half of mm0
        pshufw  mm2, mm0, 1               // get word 1 of mm0 in low word of mm2
        pminub  mm0, mm2            // got it down to 2 bytes
        movq  mm2, mm0
        psrlq  mm2, 8              // byte 1 to low byte
        pminub  mm0, mm2            // our answer in low order byte
        movd  eax, mm0
        mov    min, al              // save answer

        // get max of 8 bytes in mm1
        pshufw  mm2, mm1, (3 << 2) + 2      // words 3,2 into low words of mm2
        pmaxub  mm1, mm2            // now 4 max bytes in low half of mm1
        pshufw  mm2, mm1, 1               // get word 1 of mm1 in low word of mm2
        pmaxub  mm1, mm2            // got it down to 2 bytes
        movq  mm2, mm1
        psrlq  mm2, 8              // byte 1 to low byte
        pmaxub  mm1, mm2            // our answer in low order byte
        movd  eax, mm1
        mov    max, al              // save answer

        emms;
      }

      /* Threshold determination - compute threshold and dynamic range */
      thr = (max + min + 1) >> 1; // Nick / 2 changed to >> 1
      range = max - min;

      max_diff = quant >> 1;
      max_diffq[0] = max_diffq[1] = max_diffq[2] = max_diffq[3]
        = max_diffq[4] = max_diffq[5] = max_diffq[6] = max_diffq[7]
        = (uint8_t)max_diff;

      /* To optimize in MMX it's better to always fill in the b8x8filtered[] array

      b8x8filtered[8*v + h] = ( 8
        + 1 * b10x10[stride*(v+0) + (h+0)] + 2 * b10x10[stride*(v+0) + (h+1)]
                            + 1 * b10x10[stride*(v+0) + (h+2)]
        + 2 * b10x10[stride*(v+1) + (h+0)] + 4 * b10x10[stride*(v+1) + (h+1)]
                            + 2 * b10x10[stride*(v+1) + (h+2)]
        + 1 * b10x10[stride*(v+2) + (h+0)] + 2 * b10x10[stride*(v+2) + (h+1)]
                            + 1 * b10x10[stride*(v+2) + (h+2)]
           >> 4;  // Nick / 16 changed to >> 4

      Note - As near as I can see, (a + 2*b + c)/4 = avg( avg(a,c), b) and likewise vertical. So since
      there is a nice pavgb MMX instruction that gives a rounded vector average we may as well use it.
      */
      // Fill in b10x10 array completely with  2 dim center weighted average
      // This section now also includes the clipping step.
      // trbarry 03/14/2002

      _asm
      {
        mov   esi, b10x10          // ptr to 10x10 source array
        lea   edi, b8x8filtered      // ptr to 8x8 output array
        mov   eax, stride          // amt to bump source ptr for next row

        movq  mm7, qword ptr[esi]      // this is really line -1
        pavgb  mm7, qword ptr[esi + 2]    // avg( b[v,h], b[v,h+2] } 
        pavgb   mm7, qword ptr[esi + 1]       // center weighted avg of 3 pixels  w=1,2,1

        lea     esi, [esi + eax]        // bump src ptr to point at line 0
        movq  mm0, qword ptr[esi]      // really line 0
        pavgb  mm0, qword ptr[esi + 2]
        movq    mm2, qword ptr[esi + 1]    // save orig line 0 vals for later clip
        pavgb   mm0, mm2

        movq  mm1, qword ptr[esi + eax]    // get line 1
        pavgb  mm1, qword ptr[esi + eax + 2]
        movq    mm3, qword ptr[esi + eax + 1]  // save orig line 1 vals for later clip
        pavgb   mm1, mm3

        // 0
        pavgb   mm7, mm1          // avg lines surrounding line 0
        pavgb   mm7, mm0          // center weighted avg of lines -1,0,1

        movq    mm4, mm2          // value for clip min
        psubusb mm4, max_diffq        // min
        pmaxub  mm7, mm4          // must be at least min
        paddusb mm2, max_diffq        // max
        pminub  mm7, mm2          // but no greater than max

        movq    qword ptr[edi], mm7

        lea    esi, [esi + 2 * eax]      // bump source ptr 2 lines 
        movq  mm2, qword ptr[esi]      // get line 2
        pavgb  mm2, qword ptr[esi + 2]
        movq    mm4, qword ptr[esi + 1]    // save orig line 2 vals for later clip
        pavgb   mm2, mm4

        // 1
        pavgb   mm0, mm2          // avg lines surrounding line 1
        pavgb   mm0, mm1          // center weighted avg of lines 0,1,2

        movq    mm5, mm3          // value for clip min
        psubusb mm5, max_diffq        // min
        pmaxub  mm0, mm5          // must be at least min
        paddusb mm3, max_diffq        // max
        pminub  mm0, mm3          // but no greater than max

        movq    qword ptr[edi + 8], mm0

        movq  mm3, qword ptr[esi + eax]    // get line 3
        pavgb  mm3, qword ptr[esi + eax + 2]
        movq    mm5, qword ptr[esi + eax + 1]  // save orig line 3 vals for later clip
        pavgb   mm3, mm5

        // 2
        pavgb   mm1, mm3          // avg lines surrounding line 2
        pavgb   mm1, mm2          // center weighted avg of lines 1,2,3

        movq    mm6, mm4          // value for clip min
        psubusb mm6, max_diffq        // min
        pmaxub  mm1, mm6          // must be at least min
        paddusb mm4, max_diffq        // max
        pminub  mm1, mm4          // but no greater than max

        movq    qword ptr[edi + 16], mm1

        lea    esi, [esi + 2 * eax]      // bump source ptr 2 lines
        movq  mm4, qword ptr[esi]      // get line 4
        pavgb  mm4, qword ptr[esi + 2]
        movq    mm6, qword ptr[esi + 1]    // save orig line 4 vals for later clip
        pavgb   mm4, mm6

        // 3
        pavgb   mm2, mm4          // avg lines surrounding line 3
        pavgb   mm2, mm3          // center weighted avg of lines 2,3,4

        movq    mm7, mm5          // save value for clip min
        psubusb mm7, max_diffq        // min
        pmaxub  mm2, mm7          // must be at least min
        paddusb mm5, max_diffq        // max
        pminub  mm2, mm5          // but no greater than max

        movq    qword ptr[edi + 24], mm2

        movq  mm5, qword ptr[esi + eax]    // get line 5
        pavgb  mm5, qword ptr[esi + eax + 2]
        movq    mm7, qword ptr[esi + eax + 1]  // save orig line 5 vals for later clip
        pavgb   mm5, mm7

        // 4
        pavgb   mm3, mm5          // avg lines surrounding line 4
        pavgb   mm3, mm4          // center weighted avg of lines 3,4,5

        movq    mm0, mm6          // save value for clip min
        psubusb mm0, max_diffq        // min
        pmaxub  mm3, mm0          // must be at least min
        paddusb mm6, max_diffq        // max
        pminub  mm3, mm6          // but no greater than max

        movq    qword ptr[edi + 32], mm3

        lea    esi, [esi + 2 * eax]      // bump source ptr 2 lines
        movq  mm6, qword ptr[esi]      // get line 6
        pavgb  mm6, qword ptr[esi + 2]
        movq    mm0, qword ptr[esi + 1]    // save orig line 6 vals for later clip
        pavgb   mm6, mm0

        // 5
        pavgb   mm4, mm6          // avg lines surrounding line 5
        pavgb   mm4, mm5          // center weighted avg of lines 4,5,6

        movq    mm1, mm7          // save value for clip min
        psubusb mm1, max_diffq        // min
        pmaxub  mm4, mm1          // must be at least min
        paddusb mm7, max_diffq        // max
        pminub  mm4, mm7          // but no greater than max

        movq    qword ptr[edi + 40], mm4

        movq  mm7, qword ptr[esi + eax]    // get line 7
        pavgb  mm7, qword ptr[esi + eax + 2]
        movq    mm1, qword ptr[esi + eax + 1]  // save orig line 7 vals for later clip
        pavgb   mm7, mm1

        // 6
        pavgb   mm5, mm7          // avg lines surrounding line 6
        pavgb   mm5, mm6          // center weighted avg of lines 5,6,7

        movq    mm2, mm0          // save value for clip min
        psubusb mm2, max_diffq        // min
        pmaxub  mm5, mm2          // must be at least min
        paddusb mm0, max_diffq        // max
        pminub  mm5, mm0          // but no greater than max

        movq    qword ptr[edi + 48], mm5

        movq  mm0, qword ptr[esi + 2 * eax]  // get line 8
        pavgb  mm0, qword ptr[esi + 2 * eax + 2]
        pavgb   mm0, qword ptr[esi + 2 * eax + 1]

        // 7
        pavgb   mm6, mm0          // avg lines surrounding line 7
        pavgb   mm6, mm7          // center weighted avg of lines 6,7,8

        movq    mm3, mm1          // save value for clip min
        psubusb mm3, max_diffq        // min
        pmaxub  mm6, mm3          // must be at least min
        paddusb mm1, max_diffq        // max
        pminub  mm6, mm1          // but no greater than max

        movq    qword ptr[edi + 56], mm6

        emms
      }

      // trbarry 03-15-2002
      // If I (hopefully) understand all this correctly then we are supposed to only use the filtered
      // pixels created above in the case where the orig pixel and all its 8 surrounding neighbors
      // are either all above or all below the threshold.  Since a mmx compare sets the mmx reg
      // bytes to either 00 or ff we will just check a lopsided average of all 9 of these to see
      // if it is still 00 or ff.
      // register notes:
      // esi  b10x10
      // edi  b8x8filtered   
      // eax  stride
      // mm0   line 0, 3, or 6    compare average
      // mm1  line 1, 4, or 7      "
      // mm2  line 2, 5, 8, or -1    "
      // mm3  odd line original pixel value
      // mm4  even line original pixel value
      // mm5
      // mm6  8 00's
      // mm7  8 copies of threshold

      _asm
      {
        mov   esi, b10x10          // ptr to 10x10 source array
        lea   edi, b8x8filtered      // ptr to 8x8 output array
        mov   eax, stride          // amt to bump source ptr for next row
        mov    bh, thr
        mov   bl, thr
        psubusb mm6, mm6          // all 00
        movd   mm7, ebx
        pshufw  mm7, mm7, 0          // low word to all words

        // get compare avg of row -1
        movq  mm3, qword ptr[esi + 1]    // save center pixels of line -1

        movq  mm2, mm7
        psubusb  mm2, qword ptr[esi]      // left pixels >= threshold?
        pcmpeqb mm2, mm6          // ff if >= else 00

        movq  mm5, mm7
        psubusb mm5, mm3          // center pixels >= threshold?
        pcmpeqb mm5, mm6
        pavgb   mm2, mm5          // combine answers

        movq  mm5, mm7
        psubusb mm5, qword ptr[esi + 2]       // right pixels >= threshold?
        pcmpeqb mm5, mm6
        pavgb   mm2, mm5          // combine answers

        lea    esi, [esi + eax]        // &row 0

        // get compare avg of row 0
        movq  mm4, qword ptr[esi + 1]    // save center pixels 

        movq  mm0, mm7
        psubusb  mm0, qword ptr[esi]      // left pixels >= threshold?
        pcmpeqb mm0, mm6          // ff if >= else 00

        movq  mm5, mm7
        psubusb mm5, mm4          // center pixels >= threshold?
        pcmpeqb mm5, mm6
        pavgb   mm0, mm5          // combine answers

        movq  mm5, mm7
        psubusb mm5, qword ptr[esi + 2]       // right pixels >= threshold?
        pcmpeqb mm5, mm6
        pavgb   mm0, mm5          // combine answers

        // get compare avg of row 1

        movq  mm3, qword ptr[esi + eax + 1]  // save center pixels 

        movq  mm1, mm7
        psubusb  mm1, qword ptr[esi + eax]    // left pixels >= threshold?
        pcmpeqb mm1, mm6          // ff if >= else 00

        movq  mm5, mm7
        psubusb mm5, mm3          // center pixels >= threshold?
        pcmpeqb mm5, mm6
        pavgb   mm1, mm5          // combine answers

        movq  mm5, mm7
        psubusb mm5, qword ptr[esi + eax + 2]   // right pixels >= threshold?
        pcmpeqb mm5, mm6
        pavgb   mm1, mm5          // combine answers

        // decide whether to move filtered or orig pixels to row 0, move them

        pavgb  mm2, mm0
        pavgb   mm2, mm1
        pcmpeqb mm5, mm5          // all ff
        pcmpeqb mm5, mm2          // 00 or ff
        pcmpeqb mm5, mm2          // mm2 = ff or 00 ? ff : 00
        psubusb mm2, mm2          // 00
        pcmpeqb mm2, mm5          // opposite of mm5
        pand  mm2, mm4          // use orig vales if thresh's diff, else 00         
        pand  mm5, qword ptr[edi]         // use filtered vales if thresh's same, else 00  
        por    mm5, mm2          // one of them has a value
        movq    qword ptr[esi + 1], mm5       // and store 8 pixels

        lea    esi, [esi + eax]        // &row 1

        // get compare avg of row 2

        movq  mm4, qword ptr[esi + eax + 1]  // save center pixels 

        movq  mm2, mm7
        psubusb  mm2, qword ptr[esi + eax]    // left pixels >= threshold?
        pcmpeqb mm2, mm6          // ff if >= else 00

        movq  mm5, mm7
        psubusb mm5, mm4          // center pixels >= threshold?
        pcmpeqb mm5, mm6
        pavgb   mm2, mm5          // combine answers

        movq  mm5, mm7
        psubusb mm5, qword ptr[esi + eax + 2]   // right pixels >= threshold?
        pcmpeqb mm5, mm6
        pavgb   mm2, mm5          // combine answers

        // decide whether to move filtered or orig pixels to row 1, move them

        pavgb  mm0, mm1
        pavgb   mm0, mm2
        pcmpeqb mm5, mm5          // all ff
        pcmpeqb mm5, mm0          // 00 or ff
        pcmpeqb mm5, mm0          // mm2 = ff or 00 ? ff : 00
        psubusb mm0, mm0          // 00
        pcmpeqb mm0, mm5          // opposite of mm5
        pand  mm0, mm3          // use orig vales if thresh's diff, else 00         
        pand  mm5, qword ptr[edi + 1 * 8]     // use filtered vales if thresh's same, else 00  
        por    mm5, mm0          // one of them has a value
        movq    qword ptr[esi + 1], mm5     // and store 8 pixels

        lea    esi, [esi + eax]        // &row 2

        // get compare avg of row 3

        movq  mm3, qword ptr[esi + eax + 1]  // save center pixels 

        movq  mm0, mm7
        psubusb  mm0, qword ptr[esi + eax]    // left pixels >= threshold?
        pcmpeqb mm0, mm6          // ff if >= else 00

        movq  mm5, mm7
        psubusb mm5, mm3          // center pixels >= threshold?
        pcmpeqb mm5, mm6
        pavgb   mm0, mm5          // combine answers

        movq  mm5, mm7
        psubusb mm5, qword ptr[esi + eax + 2]   // right pixels >= threshold?
        pcmpeqb mm5, mm6
        pavgb   mm0, mm5          // combine answers

        // decide whether to move filtered or orig pixels to row 2, move them

        pavgb  mm1, mm0
        pavgb   mm1, mm2
        pcmpeqb mm5, mm5          // all ff
        pcmpeqb mm5, mm1          // 00 or ff
        pcmpeqb mm5, mm1          // mm2 = ff or 00 ? ff : 00
        psubusb mm1, mm1          // 00
        pcmpeqb mm1, mm5          // opposite of mm5
        pand  mm1, mm4          // use orig vales if thresh's diff, else 00         
        pand  mm5, qword ptr[edi + 2 * 8]     // use filtered vales if thresh's same, else 00  
        por    mm5, mm1          // one of them has a value
        movq    qword ptr[esi + 1], mm5     // and store 8 pixels

        lea    esi, [esi + eax]        // &row 3

        // get compare avg of row 4

        movq  mm4, qword ptr[esi + eax + 1]  // save center pixels 

        movq  mm1, mm7
        psubusb  mm1, qword ptr[esi + eax]    // left pixels >= threshold?
        pcmpeqb mm1, mm6          // ff if >= else 00

        movq  mm5, mm7
        psubusb mm5, mm4          // center pixels >= threshold?
        pcmpeqb mm5, mm6
        pavgb   mm1, mm5          // combine answers

        movq  mm5, mm7
        psubusb mm5, qword ptr[esi + eax + 2]   // right pixels >= threshold?
        pcmpeqb mm5, mm6
        pavgb   mm1, mm5          // combine answers

        // decide whether to move filtered or orig pixels to row 3, move them

        pavgb  mm2, mm0
        pavgb   mm2, mm1
        pcmpeqb mm5, mm5          // all ff
        pcmpeqb mm5, mm2          // 00 or ff
        pcmpeqb mm5, mm2          // mm2 = ff or 00 ? ff : 00
        psubusb mm2, mm2          // 00
        pcmpeqb mm2, mm5          // opposite of mm5
        pand  mm2, mm3          // use orig vales if thresh's diff, else 00         
        pand  mm5, qword ptr[edi + 3 * 8]     // use filtered vales if thresh's same, else 00  
        por    mm5, mm2          // one of them has a value
        movq    qword ptr[esi + 1], mm5     // and store 8 pixels

        lea    esi, [esi + eax]        // &row 4

        // get compare avg of row 5

        movq  mm3, qword ptr[esi + eax + 1]  // save center pixels 

        movq  mm2, mm7
        psubusb  mm2, qword ptr[esi + eax]    // left pixels >= threshold?
        pcmpeqb mm2, mm6          // ff if >= else 00

        movq  mm5, mm7
        psubusb mm5, mm3          // center pixels >= threshold?
        pcmpeqb mm5, mm6
        pavgb   mm2, mm5          // combine answers

        movq  mm5, mm7
        psubusb mm5, qword ptr[esi + eax + 2]   // right pixels >= threshold?
        pcmpeqb mm5, mm6
        pavgb   mm2, mm5          // combine answers

        // decide whether to move filtered or orig pixels to row 4, move them

        pavgb  mm0, mm2
        pavgb   mm0, mm1
        pcmpeqb mm5, mm5          // all ff
        pcmpeqb mm5, mm0          // 00 or ff
        pcmpeqb mm5, mm0          // mm2 = ff or 00 ? ff : 00
        psubusb mm0, mm0          // 00
        pcmpeqb mm0, mm5          // opposite of mm5
        pand  mm0, mm4          // use orig vales if thresh's diff, else 00         
        pand  mm5, qword ptr[edi + 4 * 8]     // use filtered vales if thresh's same, else 00  
        por    mm5, mm0          // one of them has a value
        movq    qword ptr[esi + 1], mm5     // and store 8 pixels

        lea    esi, [esi + eax]        // &row 5

        // get compare avg of row 6

        movq  mm4, qword ptr[esi + eax + 1]  // save center pixels 

        movq  mm0, mm7
        psubusb  mm0, qword ptr[esi + eax]    // left pixels >= threshold?
        pcmpeqb mm0, mm6          // ff if >= else 00

        movq  mm5, mm7
        psubusb mm5, mm4          // center pixels >= threshold?
        pcmpeqb mm5, mm6
        pavgb   mm0, mm5          // combine answers

        movq  mm5, mm7
        psubusb mm5, qword ptr[esi + eax + 2]   // right pixels >= threshold?
        pcmpeqb mm5, mm6
        pavgb   mm0, mm5          // combine answers

        // decide whether to move filtered or orig pixels to row 5, move them

        pavgb  mm1, mm0
        pavgb   mm1, mm2
        pcmpeqb mm5, mm5          // all ff
        pcmpeqb mm5, mm1          // 00 or ff
        pcmpeqb mm5, mm1          // mm2 = ff or 00 ? ff : 00
        psubusb mm1, mm1          // 00
        pcmpeqb mm1, mm5          // opposite of mm5
        pand  mm1, mm3          // use orig vales if thresh's diff, else 00         
        pand  mm5, qword ptr[edi + 5 * 8]     // use filtered vales if thresh's same, else 00  
        por    mm5, mm1          // one of them has a value
        movq    qword ptr[esi + 1], mm5     // and store 8 pixels

        lea    esi, [esi + eax]        // &row 6

        // get compare avg of row 7

        movq  mm3, qword ptr[esi + eax + 1]  // save center pixels 

        movq  mm1, mm7
        psubusb  mm1, qword ptr[esi + eax]    // left pixels >= threshold?
        pcmpeqb mm1, mm6          // ff if >= else 00

        movq  mm5, mm7
        psubusb mm5, mm3          // center pixels >= threshold?
        pcmpeqb mm5, mm6
        pavgb   mm1, mm5          // combine answers

        movq  mm5, mm7
        psubusb mm5, qword ptr[esi + eax + 2]   // right pixels >= threshold?
        pcmpeqb mm5, mm6
        pavgb   mm1, mm5          // combine answers

        // decide whether to move filtered or orig pixels to row 6, move them

        pavgb  mm2, mm0
        pavgb   mm2, mm1
        pcmpeqb mm5, mm5          // all ff
        pcmpeqb mm5, mm2          // 00 or ff
        pcmpeqb mm5, mm2          // mm2 = ff or 00 ? ff : 00
        psubusb mm2, mm2          // 00
        pcmpeqb mm2, mm5          // opposite of mm5
        pand  mm2, mm4          // use orig vales if thresh's diff, else 00         
        pand  mm5, qword ptr[edi + 6 * 8]     // use filtered vales if thresh's same, else 00  
        por    mm5, mm2          // one of them has a value
        movq    qword ptr[esi + 1], mm5     // and store 8 pixels

        lea    esi, [esi + eax]        // &row 7

        // get compare avg of row 8

        movq  mm4, qword ptr[esi + eax + 1]  // save center pixels 

        movq  mm2, mm7
        psubusb  mm2, qword ptr[esi + eax]    // left pixels >= threshold?
        pcmpeqb mm2, mm6          // ff if >= else 00

        movq  mm5, mm7
        psubusb mm5, mm4          // center pixels >= threshold?
        pcmpeqb mm5, mm6
        pavgb   mm2, mm5          // combine answers

        movq  mm5, mm7
        psubusb mm5, qword ptr[esi + eax + 2]   // right pixels >= threshold?
        pcmpeqb mm5, mm6
        pavgb   mm2, mm5          // combine answers

        // decide whether to move filtered or orig pixels to row 7, move them

        pavgb  mm0, mm1
        pavgb   mm0, mm2
        pcmpeqb mm5, mm5          // all ff
        pcmpeqb mm5, mm0          // 00 or ff
        pcmpeqb mm5, mm0          // mm2 = ff or 00 ? ff : 00
        psubusb mm0, mm0          // 00
        pcmpeqb mm0, mm5          // opposite of mm5
        pand  mm0, mm3          // use orig vales if thresh's diff, else 00         
        pand  mm5, qword ptr[edi + 7 * 8]     // use filtered vales if thresh's same, else 00  
        por    mm5, mm0          // one of them has a value
        movq    qword ptr[esi + 1], mm5     // and store 8 pixels

        emms
      }
    }
  }
}
#endif
