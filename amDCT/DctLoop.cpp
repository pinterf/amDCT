#include "avs/config.h"
#include <math.h>

#include <stdint.h>
#include <string.h>

#include <emmintrin.h> // Header for SSE2 intrinsics

#include "amDCTtypedefs.h"

#include "DctLoop.h"

#include "LumaBlockInfo.h"
#include "Utilities.h"
#include "FitRange.h"
#include "transfer_add.h"

#include "Matrix.h"

#include "quant/quant.h"
#include "dct/idct.h"
#include "dct/fdct.h"
#include "quant/quant_matrix.h"

void init_intra_matrixF(uint16_t* mpeg_quant_matrices, float quant);

void quantDequant_sharp(int16_t* dct_block, const uint8_t* quant_sharp_preComp);

void quantDequant_expandRange(int16_t* dct_block,
  const uint16_t* qtype1_matrix_range,
  const uint16_t* qtype1_matrix_quant_range);



#define SCALEBITS 17

// There are 3 buffers that the primary smoothing algorithm uses.
// 1: The uint8_t source buffer BF_src.  The values in this are never changed.
//    The source buffer is copied into the shifted position of the work buffer.
// 2: The uint8_t work buffer BF_work. The primary smoothing algorithm outputs uint16_t
//    values which are added with the existing values in the accumulator buffer. 
// 3: The uint16_t accumulator buffer BF_accum. 
//
//uint8_t  BF_src      The source      buffer
//uint8_t  BF_work     The work        buffer
//uint16_t BF_accum    The accumulator buffer
//
//uint8_t  *BF_srcP    The source      buffer pointer
//uint8_t  *BF_workP   The work        buffer pointer
//uint16_t *BF_accumP  The accumulator buffer pointer


void DctLoop(int starti, int startj, DctLoop_args* args) {

  unsigned int h;
  int32_t offset;

  int x, y;
  int32_t  blkStride, rowStride;
  uint32_t blkRowSt, blkColSt;
  int16_t  dbzero;

  //int MaxDCT0 = 0;
  //int MinDCT0 = 0;

  //int16_t c1, c2, c3, c4; //corner vals of block
  //int16_t cn1, cn2, cn3, cn4, cn11, cn21, cn31, cn41; 
  //int16_t cnn1, cnn2, cnn3, cnn4, cnn11, cnn21, cnn31, cnn41; 
  //int16_t cd1, cd2, cd3, cd4; // 1 in toward center from corner vals of block

  alignas(16) int16_t    dct_block[64];
  alignas(16) int16_t    coeff_block[64];
  alignas(16) int16_t    dct_blockTemp[64];  // Used for displaying the DCT values.

  alignas(16) int16_t    dct_blockOrig[MATRIX_SIZE];



  uint32_t  numBlocks_wide = args->numBlocks_wide;
  uint32_t  numBlocks_high = args->numBlocks_high;
  uint8_t   qtype = args->qtype;
  uint8_t   quant = args->quant;
  uint32_t  src_width = args->src_width;
  uint32_t  src_height = args->src_height;

  uint8_t    doDCTadapt = args->doDCTadapt;    // Flag  if 1 then do range, expand, and sharp strength block adapt in the DCT loop, otherwise just do full strength.. 

  uint8_t    expand = args->expand;
  uint8_t    sharpWAmt = args->sharpWAmt;
  uint8_t    sharpTAmt = args->sharpTAmt;
  uint8_t   sharpMaxWT = args->sharpMaxWT;     // = Max(sharpWAmt, sharpTAmt);
  int       T2 = args->T2;
  int       showMask = args->showMask;


  uint16_t* quant_intra_matrix = args->quant_intra_matrix;
  uint16_t* quant_inter_matrix = args->quant_inter_matrix;
  uint16_t* qtype1_matrix = args->qtype1_matrix[0];
  uint16_t* qtype1_matrix_quant = args->qtype1_matrix_quant[0];
  uint16_t* qtype11_matrix = args->qtype11_matrix[0];
  uint16_t* qtype11_matrix_quant = args->qtype11_matrix_quant[0];
  uint16_t* qtype1_matrix_range = args->qtype1_matrix_range[0];
  uint16_t* qtype1_matrix_quant_range = args->qtype1_matrix_quant_range[0];
  uint8_t* quant_sharp_preComp = args->quant_sharp_preComp[0];


  uint8_t* BF_srcP = args->BF_srcP;
  uint8_t* BF_workP = args->BF_workP;
  int16_t* BF_tmp_16P = args->BF_tmp_16P;
  uint16_t* BF_accumP = args->BF_accumP;
  uint16_t* BF_accumStart = args->BF_accumP;


  //   uint32_t   currentBlockNum   = 0;
  uint8_t    blkQuantVal = args->quant;
  uint8_t    blkExpandVal = args->expand;
  int  blkDetailECMod;

  int blockSize = (sizeof(int16_t)) << 6;

  offset = startj * BLKSIZE * numBlocks_wide + starti;
  BF_workP += offset;



  // Using The Agner Fog library code (look in the CodeCopiedFrom_OriginalSources directory)
  // Gives a 2 percent speed increase when using shift=4 
  // 30.1 to 30.7 fps 720x480p. Just enough to be worth it.
  // Both BF_src and BF_work are aligned data.
  // We are copying from an aligned position in BF_src
  // We are copying to a nonaligned position in BF_workP so that the shifted blocks start at an aligned position. 
  for (h = 0; h < src_height; h++) {
    MY_MEMCPY(BF_workP, BF_srcP, src_width);
    BF_srcP += src_width;
    BF_workP += numBlocks_wide << 3; // * BLKSIZE
  }

  BF_workP = args->BF_workP;
  BF_accumP = BF_accumP - offset;
  BF_tmp_16P = BF_tmp_16P - offset;


  // blkRowSt and blkColSt give the start location of the macro-block being processed.
  rowStride = numBlocks_wide << 3;  // numBlocks_wide*8  or numBlocks_wide*BLKSIZE
  blkStride = numBlocks_wide << 6;  // numBlocks_wide*64 or numBlocks_wide*BLKSIZE*BLKSIZE


  // This is the core of the amDCT algorithm 
  // which is based directly on the SmoothD2 algorithm 
  // which is based directly on the SmoothD algorithm
  // which was based on the original "JPEG Post-Processing through Re-application of JPEG" 
  // by Aria Nosratinia.  For more info see "JPEG re-sampling paper.pdf" in the documentation folder. 
  for (blkRowSt = 0; blkRowSt < numBlocks_high; blkRowSt++) {
    for (blkColSt = 0; blkColSt < numBlocks_wide; blkColSt++) {
      int blkNum = (blkRowSt * numBlocks_wide) + blkColSt;
      if (BF_accumP < BF_accumStart) {
        BF_accumP += BLKSIZE;
        BF_workP += BLKSIZE;
        BF_tmp_16P += BLKSIZE;

        continue;
      }


        // NOTE: Most of the mmx/xmm/sse2 routines and C equivalents are from XVID.  Thank You XVID.
        //       I tested the available XVID routines for each section of the algorithm.
        //       The ones that worked I ranked by speed and commented out all but the fastest one.
        // NOTE2 by pinterf, the above mentioned multiple xvid-asm versions are no longer used.
        // The original asm codes were rewritten in SIMD intrinsics, which are even quicker/much quicker
        // than the originally used implementation. Anyway, these rewrites are giving bit identical results
        // compared to the original asm versions.


          /*
           *  1: COPY SHIFTED FRAME TO DCT_BLOCK
           */
      transfer_8to16copy(dct_block, BF_workP, rowStride);  // fastest

      qtype = args->qtype;



              /*
               *  2: DO DCT ON THE BLOCK.
               */
#ifdef INTEL_INTRINSICS
      fdct_sse2(dct_block);       // based on fdct_sse2_skal
#else
      fdct_int32(dct_block);       // C version for testing.
#endif

      if (showMask == 18 || showMask == 19) {
        memcpy(dct_blockOrig, dct_block, blockSize);
        if (showMask == 18) goto fdct_only;
      }





      // The various quant dequant routines implement the core DCT smoothing operation, the strength of which is controlled by the quant value 
      //     and by their respective matrices.
      // For qtype's 1 and 11 the quant dequant matrices, specified by amDCT argument matrix=val, 
      //     along with some of the quant dequant processing have been precomputed for each quant value as a speed optimization. 
      // qtype's 2 and 4 have their matrices coded directly into their quant dequant sse2 routines.


      // Get the quant value for the block we are processing.
      // 31 matrices, corresponding to 31 strengths, have been computed for each type of processing.
      // The 31 strengths implement 31 levels of smoothing having strengths 
      //     between the user argument quant, and the computed value frameQuant.
      //             frameQuant ranges from quant to adapt.

      blkQuantVal = args->blkAdaptSmoothAmt[blkNum];
      if (doDCTadapt == 0 || qtype == 5)   blkQuantVal = quant;    // doDCTadapt allows internal routines, in FrameInfo.c, to turn off block adapt.

      if (blkQuantVal > 30)  blkQuantVal = 30;


      qtype1_matrix_quant = args->qtype1_matrix_quant[blkQuantVal];
      qtype1_matrix = args->qtype1_matrix[blkQuantVal];

      qtype11_matrix_quant = args->qtype11_matrix_quant[blkQuantVal];
      qtype11_matrix = args->qtype11_matrix[blkQuantVal];

      if (blkQuantVal <= 5 && qtype == 1) qtype = 11; // qtype == 1 needs quant > 4 to work.

      //if (T2==88) {
      //      qtype = 11;   // WHEN I GET AROUND TO IMPLIMENTING PER BLOCK MATRICIES USING PATTERNS IN DCT VALUES DETERMINE THE SMOOTHING MATRIX TO USE. 
      ////      qtype11_matrix_quant = args->qtype11_matrix_quantPS[blkNum].qtype11_matrix_quantPS;
      ////      qtype11_matrix       = args->qtype11_matrixPS[blkNum].qtype11_matrixPS;
      //}




      // Get the matrices that specify the range expansion for this block.  
      // The range expansion matrices, created in matrix.c, where built with 31 entries with strengths running from 0 to expand.
      // The values in blkAdaptExpandAmt[blkNum], created in LumaBlockInfo.c, where built with 31 entries with strengths running from 0 to 31
      // and represent the strength of expansion of the current block being processed.     
      blkExpandVal = args->blkAdaptExpandAmt[blkNum];
      if (doDCTadapt == 0)  blkExpandVal = expand; // Allows internal routines, in FrameInfo.c, to turn off block adapt.


      // if (T2==83 || T2==85 || T2==86)  blkExpandVal = expand; // FOR TESTING TURN OFF EXPAND BLOCK ADAPT
      if (blkExpandVal <= 0)   blkExpandVal = 0;
      if (blkExpandVal >= 30) blkExpandVal = 30;

      qtype1_matrix_quant_range = args->qtype1_matrix_quant_range[blkExpandVal];
      qtype1_matrix_range = args->qtype1_matrix_range[blkExpandVal];






      // Get the matrices that specifies the sharpening for this block.  
      // BLOCK ADAPT FOR SHARP DOESN"T WORK WELL!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!    
      blkDetailECMod = args->blkAdaptSharpAmt[blkNum];    // This returns the adapt amount of sharpening that should be done on the block.                          
      blkDetailECMod = 31 - blkDetailECMod;
      if (blkDetailECMod < 0)   blkDetailECMod = 0;
      if (blkDetailECMod > 30)   blkDetailECMod = 30;

      if (doDCTadapt == 0)  blkDetailECMod = sharpMaxWT; // Allows internal routines, in FrameInfo.c, to turn off block adapt.

      // BLOCK ADAPT FOR SHARP DOESN"T WORK WELL  SO IT IS CURRENTLY TURNED OFF!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!    
      // if (T2==84 || T2==85 || T2==86)  blkDetailECMod = sharpWAmt; // FOR TESTING TURN OFF SHARP BLOCK ADAPT             
      blkDetailECMod = sharpMaxWT;

      quant_sharp_preComp = args->quant_sharp_preComp[blkDetailECMod];


      // doing sharp before range expand gives slightly better results than doing range expand first.
      if (qtype < 20 && (sharpWAmt > 0 || sharpTAmt > 0))  quantDequant_sharp(dct_block, quant_sharp_preComp);
      if (qtype < 20 && expand > 0)  quantDequant_expandRange(dct_block, qtype1_matrix_range, qtype1_matrix_quant_range);

      if (quant > 0 || qtype >= 20) {
        /*
         *     3: DO QUANT DEQUANT PROCESSING ON THE BLOCK
         *
         * The following are the quant dequant pairs.
         * The 4 pairs are ordered by relative strength from weakest to strongest
         *      mpeg_intra, h263_intra, mpeg_inter, h263_inter
         * The first set of cases are mmx versions.    (qtype  1-4)
         * The second set are C versions for testing.  (qtype 11-14)
         *
         * NOTE that qtype 21 is used to do sharpening instead of smoothing.
         */
        switch (qtype) {
        case 1:
          // I (original author) couldn't get quant_mpeg_intra_mmx() to work so I wrote a merged quant-dequant in xmm
          // An optimized c version qtype=11 is automatically used if the xmm version would overflow.
          dbzero = dct_block[0];
          quantDequant(dct_block, qtype1_matrix, qtype1_matrix_quant);
          // instead of quant_mpeg_intra_mmx dequant_mpeg_intra_mmx pair
          // FIXME: check if same, figure out how to reach here, qtype=1 in changed to qtype=11 if some conditions apply
          //quantDequant_c(dct_block, qtype11_matrix, qtype11_matrix_quant);
          dct_block[0] = dbzero;
          break;

        case 2:
#ifdef INTEL_INTRINSICS
          quant_h263_intra_sse2(coeff_block, dct_block, quant, 1, quant_intra_matrix);
          dequant_h263_intra_sse2(dct_block, coeff_block, quant, 1, quant_intra_matrix);
#else
          quant_h263_intra_c(coeff_block, dct_block, quant, 1, quant_intra_matrix);
          dequant_h263_intra_c(dct_block, coeff_block, quant, 1, quant_intra_matrix);
#endif
          break;

        case 3:
#ifdef INTEL_INTRINSICS
          quant_mpeg_inter_ssse3(coeff_block, dct_block, quant, quant_inter_matrix);
          dequant_mpeg_inter_sse41(dct_block, coeff_block, quant, quant_inter_matrix);
#else
          quant_mpeg_inter_c(coeff_block, dct_block, quant, quant_inter_matrix);
          dequant_mpeg_inter_c(dct_block, coeff_block, quant, quant_inter_matrix);
#endif
          break;

        case 4:
#ifdef INTEL_INTRINSICS
          quant_h263_inter_sse2(coeff_block, dct_block, quant, quant_intra_matrix);
          dequant_h263_inter_sse2(dct_block, coeff_block, quant, quant_intra_matrix);
#else
          quant_h263_inter_c(coeff_block, dct_block, quant, quant_intra_matrix);
          dequant_h263_inter_c(dct_block, coeff_block, quant, quant_intra_matrix);
#endif
          break;




          // These are the C versions which are included for testing.
          // NOTE case 11 is required to process qtype=1 for Matrices with 1's in them.      
        case 11:
          // with differently scaled shift and rounder
          quantDequant_shift14_c(dct_block, qtype11_matrix, qtype11_matrix_quant);

          break;

        case 12:
          quant_h263_intra_c(coeff_block, dct_block, quant, 1, quant_intra_matrix);
          dequant_h263_intra_c(dct_block, coeff_block, quant, 1, quant_intra_matrix);
          break;

        case 13:
          quant_mpeg_inter_c(coeff_block, dct_block, quant, quant_inter_matrix);
          dequant_mpeg_inter_c(dct_block, coeff_block, quant, quant_inter_matrix);
          break;

        case 14:
          quant_h263_inter_c(coeff_block, dct_block, quant, quant_intra_matrix);
          dequant_h263_inter_c(dct_block, coeff_block, quant, quant_intra_matrix);
          break;



          // This case is called to do Sharpening in its own pass.  
        case 21:
          quantDequant_sharp(dct_block, quant_sharp_preComp);
          break;


          // This case is called to do Range Expansion in its own pass.
          // It is only called if (expand > 0 && (quality >= 2 || quant == 0)
        case 22:
          quantDequant_expandRange(dct_block, qtype1_matrix_range, qtype1_matrix_quant_range);
          break;



          // This case is only called to do a combined range expand and sharpening in 1 step  
          // to see what pixels are going to over sharpen.  Those pixels will then be de-emphasized 
          // before the main processing takes place.  IT DOESN'T HELP
        case 23:
          quantDequant_sharp(dct_block, quant_sharp_preComp);
          quantDequant_expandRange(dct_block, qtype1_matrix_range, qtype1_matrix_quant_range);
          break;


        default:
#ifdef INTEL_INTRINSICS
          // like 2
          quant_h263_intra_sse2(coeff_block, dct_block, quant, 1, quant_intra_matrix);
          dequant_h263_intra_sse2(dct_block, coeff_block, quant, 1, quant_intra_matrix);
#else
          quant_h263_intra_c(coeff_block, dct_block, quant, 1, quant_intra_matrix);
          dequant_h263_intra_c(dct_block, coeff_block, quant, 1, quant_intra_matrix);
#endif
          break;

        } // end switch

      } // end (quant > 0)


      // To show the DCT we bypass the IDCT processing.
      // Both showMask == 18 or 19 display the DCT
      // showMask 18 shows it for the unprocessed image.
      // showMask 19 shows it after the quant dequant processing.
      if (showMask == 19)
        goto fdct_only;

      /*
       *  4: DO iDCT ON THE BLOCK
       */
#ifdef INTEL_INTRINSICS
      idct_sse2(dct_block);    // based on idct_sse2_skal
#else
       //idct_int32(dct_block);      // C version for testing.
       simple_idct_c(dct_block);   // C version for testing. (more precise)
#endif



// showMask == 18 jumps here from near the top of the main loop.
    fdct_only:
      //  NOTE IF WE USE THE DCT VALUES DIRECTLY IN LUMABLOCKINFO.C THEN THE FITRANGES NEED TO BE TURNED INTO TABLES!!!
      if (showMask == 18 || showMask == 19) {
        int16_t zval = 0;

        // This outputs a frame that shows the DCT values within each block.
        // idx2 is used to flip the output values for each 8x8 block around the diagonal running from top left to bottom right.
        // This allows the dct values representing horizontal and vertical lines to display as horizontal and vertical lines.
        for (x = 0; x < 64; x++) dct_blockTemp[x] = dct_block[x];

        for (x = 0; x < 8; x++) {
          for (y = 0; y < 8; y++) {
            uint8_t idx1 = (uint8_t)(x * 8 + y);
            uint8_t idx2 = (uint8_t)(y * 8 + x);
            int16_t temp = dct_blockTemp[idx1];
            float   gamma = 2.0;

            if (T2 == 25) gamma = 1.0;
            if (T2 == 26) gamma = 4.0;

            if (x == 0 && y == 0) {
              if (dct_block[0] > MaxDCT0) MaxDCT0 = dct_block[0];
              if (dct_block[0] < MinDCT0) MinDCT0 = dct_block[0];

              // THE FOLOWING IS USED TO CALIBRATE dct_block[0] WHICH CONTAINS THE AVERAGE LUMA OF THE BLOCK.               
              zval = (int16_t)Int_FitRange_Int(dct_block[0], 0, 2044, 0, 255);  // 11 -> 1     251 -> 251
              continue;
            }

            if (temp < 0)     dct_block[idx2] = (int16_t)(255 - Int_fitRangeGamma_Int(abs(temp), 0, 640, gamma, 128, 255));
            else              dct_block[idx2] = (int16_t)Int_fitRangeGamma_Int(temp, 0, 640, gamma, 128, 255);
          }
        }
        dct_block[0] = zval;
      }


              /*
               *     5: ADD BLOCK VALUES TO THOSE IN BF_workP
               * Note that the values in dct_block are not in 0-255 range.
               * They are clipped to 0-255 in copy_add_16to16_clpsrc()
               */

      copy_add_16to16_clpsrc(BF_accumP, dct_block, rowStride);

      BF_accumP += BLKSIZE;
      BF_workP += BLKSIZE;
      BF_tmp_16P += BLKSIZE;
    } // end for blkColSt

    BF_accumP += blkStride - rowStride; // - rowStride since we already did a rowStride in the blkColSt loop.
    BF_workP += blkStride - rowStride; // - rowStride since we already did a rowStride in the blkColSt loop.    
    BF_tmp_16P += blkStride - rowStride; // - rowStride since we already did a rowStride in the blkColSt loop.    
  } // end for blkRowSt

  return;
}




static const uint8_t sharpTestmatrix[64] = {
      1, 1, 1, 1, 1, 2, 2, 2,
      1, 1, 1, 1, 2, 2, 2, 1,
      1, 1, 1, 2, 2, 2, 1, 1,
      1, 1, 2, 2, 2, 1, 1, 1,
      1, 2, 2, 2, 1, 1, 1, 1,
      2, 2, 2, 1, 1, 1, 1, 1,
      2, 2, 1, 1, 1, 1, 1, 1,
      2, 1, 1, 1, 1, 1, 1, 1
};



//  When doing sharpening the values in quant_sharp_preComp
//  have been set so that the amount of sharpening wanted is done at each position within a block.
//  We use the "sharp" matrices because sharpening is a range expansion for a particular
//  set of positions within a block.    
void quantDequant_sharp(
  int16_t* dct_block,
  const uint8_t* quant_sharp_preComp) {

  int     i;
  int32_t level, absLevel, absLevelOrig;
  uint8_t row, r;
  uint8_t col;
  int   levelCutoff = MAX_LEVEL_IDX;

  for (row = 0; row < 7; row++) {
    r = row << 3;
    for (col = 0; col < 7; col++) {    // if row==0 col goes from 5-7 (5 to 5+2)   row==1 col goes from 4-6 (4 to 4+2)
      i = r + col;

      if (sharpTestmatrix[i] == 1) continue;

      level = dct_block[i];

      if (level == 0) continue;

      absLevelOrig = abs(level);
      if (absLevelOrig >= levelCutoff) continue;   // Since we are sharpening we know that the level will get larger therefor level needs to be < levelCutoff to have headroom for sharpening.
      absLevel = absLevelOrig;

      // 64 is the 64 pixels in an 8x8 block.  MAX_LEVEL_IDX are the 64 resulting 8 bit values for each position in the 8x8 block, which are obtained from precomputing the new DCT values. 
      // They are accessed as a 64x64 array with the first index being the pixel location and the second being the resulting values.
      // quant_sharp_preComp[0][j][i*64 + absLevel]    [0]=NCPU  [j]=BLOCK_ADAPT   [i*64 + absLevel]=  i= row*8 + col the DCT position     k=0...63 is the input DCT value, the cell contains the returned DCT value

       //absLevel = quant_sharp_preComp[i*64 + absLevel]; 
      absLevel = quant_sharp_preComp[(i << 6) + absLevel];

      if (absLevel == 0) continue;  // No change in value.

      if (row == 0 || col == 0) {  // This keeps us from over sharpening horizontal or vertical lines.
        //absLevel = (absLevel + absLevelOrig + 1)>>1;
        absLevel = (absLevel + absLevelOrig) >> 1;
      }

      if (level > 0)  level = absLevel;
      else            level = -absLevel;

      dct_block[i] = (int16_t)level;
    }
  }

  return;
}







void quantDequant_expandRange(
  int16_t* dct_block,
  const uint16_t* qtype1_matrix_range,
  const uint16_t* qtype1_matrix_quant_range) {

  int     i;
  int32_t level;

  //  int16_t dbzero;
  //
  //          dbzero = dct_block[0];        
  //          quantDequant_xmm(dct_block, qtype1_matrix_range, qtype1_matrix_quant_range);
  //          dct_block[0] = dbzero;
  //          return;


  for (i = 1; i < 64; i++) {
    level = dct_block[i];
    if (level == 0) continue;

    level = (level * qtype1_matrix_range[i] + 8192) >> 14;

    if (level == 0)
      dct_block[i] = 0;
    else {
      dct_block[i] = (int16_t)(level * qtype1_matrix_quant_range[i]) >> 3;
      //      level = (level * qtype1_matrix_quant_range[i]) >> 3;
      //      if (level > 7000) level = 7000;
      //      dct_block[i] = (int16_t)level;
    }
  }
  return;
}


AVS_FORCEINLINE void copy_add_16to16_clpsrc(uint16_t* dst, int16_t* const src, int32_t stride) {
#ifdef INTEL_INTRINSICS
  copy_add_16to16_clpsrc_sse2(dst, src, stride);
#else
  copy_add_16to16_clpsrc_c(dst, src, stride);
#endif
}

#ifdef INTEL_INTRINSICS
AVS_FORCEINLINE void copy_add_16to16_clpsrc_sse2(uint16_t* dst, int16_t* const src, int32_t stride) {

  __m128i zero = _mm_setzero_si128();
  __m128i max_val = _mm_set1_epi16(255);

  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x += 8) {
      int pos = y * stride + x;
      __m128i src_vals = _mm_loadu_si128((__m128i*)(src + y * 8 + x));
      __m128i clamped_vals = _mm_max_epi16(zero, _mm_min_epi16(src_vals, max_val));
      __m128i dst_vals = _mm_loadu_si128((__m128i*)(dst + pos));
      dst_vals = _mm_add_epi16(dst_vals, clamped_vals);
      _mm_storeu_si128((__m128i*)(dst + pos), dst_vals);
    }
  }
}
#endif

void quantDequant_c(int16_t* dct_block, const uint16_t* qtype1_matrix, const uint16_t* qtype1_matrix_quant) {

  for (int i = 0; i < 64; ++i) {
    int16_t value = dct_block[i];
    int16_t sign = value & 0x8000; // negBit
    int16_t abs_value = value < 0 ? -value : value;

    int16_t level = (abs_value * qtype1_matrix[i] + 32768) >> 16;
    level = (level * qtype1_matrix_quant[i]) >> 3;

    if (sign) {
      level = -level;
    }

    dct_block[i] = level;
  }
}

// passes qtype11_matrix_quant instead of qtype1_matrix_quant
// and qtype11_matrix instead of qtype1_matrix
void quantDequant_shift14_c(int16_t* dct_block, const uint16_t* qtype1_matrix, const uint16_t* qtype1_matrix_quant)
{
  //quantDequant_C:
// The xvid algorithm used 
// SCALEBITS = 17
// dct_block[] values run from -32768 to 32767
// rounding = 1<<(SCALEBITS-1-3)
// level    = (level * qtype1_matrix[i] + rounding)>>(SCALEBITS-3); 
// 8192     = 32768 >> 2

// why not this one called: quantDequant_c(dct_block, qtype11_matrix, qtype11_matrix_quant);
// Because qtype11 matrix is /4 what quantDequant_c would expect
  for (int i = 1; i < 64; i++) {
    int32_t level = dct_block[i];    // dct_block[] values run from -32768 to 32767

    if (level == 0) continue;

    level = (level * qtype1_matrix[i] + 8192) >> 14;

    if (level == 0)
      dct_block[i] = 0;
    else
      dct_block[i] = (int16_t)(level * qtype1_matrix_quant[i]) >> 3;
  }
}

AVS_FORCEINLINE void quantDequant(int16_t* dct_block, const uint16_t* qtype1_matrix, const uint16_t* qtype1_matrix_quant) {
#ifdef INTEL_INTRINSICS
  quantDequant_sse2(dct_block, qtype1_matrix, qtype1_matrix_quant);
#else
  quantDequant_c(dct_block, qtype1_matrix, qtype1_matrix_quant); // for special case, quantDequant_shift14_c exists
#endif
}

#ifdef INTEL_INTRINSICS
AVS_FORCEINLINE void quantDequant_sse2(int16_t* dct_block, const uint16_t* qtype1_matrix, const uint16_t* qtype1_matrix_quant) {
  __m128i low15bits_vec = _mm_set1_epi16(0x7FFF); // Mask for low 15 bits
  __m128i allFF_vec = _mm_set1_epi16(0xFFFF); // Mask for all bits set
  __m128i negBit_vec = _mm_set1_epi16(0x8000); // Mask for the sign bit

  for (int i = 0; i < 8; ++i) {
    __m128i xmm0 = _mm_load_si128((__m128i*) & dct_block[i * 8]); // Load dct_block
    __m128i xmm6 = _mm_and_si128(xmm0, low15bits_vec); // xmm6 = xmm0 & low15bits

    __m128i xmm1 = _mm_load_si128((__m128i*) & qtype1_matrix[i * 8]); // Load qtype1_matrix
    __m128i xmm2 = _mm_load_si128((__m128i*) & qtype1_matrix_quant[i * 8]); // Load qtype1_matrix_quant

    __m128i xmm7 = _mm_cmpeq_epi16(xmm6, xmm0); // xmm7 = (xmm6 == xmm0)
    xmm7 = _mm_xor_si128(xmm7, allFF_vec); // xmm7 = ~xmm7

    __m128i xmm4 = _mm_setzero_si128(); // xmm4 = 0
    xmm4 = _mm_sub_epi16(xmm4, xmm0); // xmm4 = -xmm0

    xmm0 = _mm_and_si128(xmm0, xmm6); // xmm0 = xmm0 & xmm6
    xmm2 = _mm_and_si128(xmm2, xmm7); // xmm2 = xmm2 & xmm7
    xmm2 = _mm_add_epi16(xmm2, xmm0); // xmm2 = xmm2 + xmm0

    __m128i xmm3 = _mm_mullo_epi16(xmm2, xmm1); // xmm3 = xmm2 * xmm1
    xmm2 = _mm_mulhi_epi16(xmm2, xmm1); // xmm2 = (xmm2 * xmm1) >> 16

    xmm3 = _mm_and_si128(xmm3, negBit_vec); // xmm3 = xmm3 & negBit
    xmm3 = _mm_srli_epi16(xmm3, 15); // xmm3 = xmm3 >> 15
    xmm2 = _mm_add_epi16(xmm2, xmm3); // xmm2 = xmm2 + xmm3

    xmm2 = _mm_mullo_epi16(xmm2, xmm1); // xmm2 = xmm2 * qtype1_matrix_quant
    xmm2 = _mm_srai_epi16(xmm2, 3); // xmm2 = xmm2 >> 3

    xmm4 = _mm_setzero_si128(); // xmm4 = 0
    xmm4 = _mm_sub_epi16(xmm4, xmm2); // xmm4 = -xmm2

    xmm7 = _mm_and_si128(xmm7, xmm4); // xmm7 = xmm7 & xmm4
    xmm6 = _mm_and_si128(xmm6, xmm2); // xmm6 = xmm6 & xmm2
    xmm6 = _mm_add_epi16(xmm6, xmm7); // xmm6 = xmm6 + xmm7

    _mm_store_si128((__m128i*) & dct_block[i * 8], xmm6); // Store result back to dct_block
  }
}
#endif

AVS_FORCEINLINE void transfer_8to16copy(int16_t* dst, uint8_t* src, uint32_t stride) {
#ifdef INTEL_INTRINSICS
  transfer_8to16copy_sse2(dst, src, stride);
#else
  transfer_8to16copy_c(dst, src, stride);
#endif
}

#ifdef INTEL_INTRINSICS
AVS_FORCEINLINE void transfer_8to16copy_sse2(int16_t* dst, uint8_t* src, uint32_t stride) {
  __m128i xmm0 = _mm_setzero_si128(); // Set xmm0 to zero

  for (int i = 0; i < 8; ++i) {
    // C++ __m128i xmm1 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(src)); // Load 8 bytes from src
    __m128i xmm1 = _mm_loadl_epi64((__m128i*)(src)); // Load 8 bytes from src
    src += stride; // Move to the next row
    xmm1 = _mm_unpacklo_epi8(xmm1, xmm0); // Unpack bytes to words
    // c++ _mm_store_si128(reinterpret_cast<__m128i*>(dst), xmm1); // Store the result in dst
    _mm_store_si128((__m128i*)(dst), xmm1); // Store the result in dst
    dst += 8; // Move to the next 8 words in dst
  }
}
#endif


void transfer_8to16copy_c(int16_t* dst, uint8_t* src, uint32_t stride) {
  int16_t temp[8];

  for (int i = 0; i < 8; ++i) {
    for (int j = 0; j < 8; ++j) {
      temp[j] = src[j]; // Copy and extend each byte to int16_t
    }
    memcpy(dst, temp, sizeof(temp)); // Copy the extended values to dst
    src += stride; // Move to the next row
    dst += 8; // Move to the next 8 words in dst
  }
}
