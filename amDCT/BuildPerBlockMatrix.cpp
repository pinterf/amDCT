#include "amDCTtypedefs.h"
#include "Memory.h"

#include "Matrix.h"
#include "quant\quant_matrix.h"

#include "DispatchLoop.h"
#include "LumaBlockInfo.h"
#include "FrameInfo.h"
#include "avgDctLoop.h"
#include "Matrix.h"
#include "Utilities.h"

#include "BuildPerBlockMatrix.h"

void init_intra_matrixF(uint16_t* mpeg_quant_matrices, float quant);

/////////////////////////////////////////////
//
//       NOT CURRENTLY BEING USED !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//       LOOKING AT DCT VALUES IN A BLOCK AS A WAY TO DETERMINE
//              STRENGTH  AND OR  MATRIX TO USE OR BUILD
//              FOR EACH BLOCK
//
////////////////////////////////////////////
void buildPerBlockMatrix(FrameInfo_args* FrameInfoArgs) {
  //  LumaPerBlock_args *LumaPerBlockArgs = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs;
  //  uint8_t         *PworkSource     = FrameInfoArgs->frame_workSource;
  //  uint8_t         *Pgrain         = FrameInfoArgs->frame_grainMask;
  //  uint8_t        *PsrcMem        = FrameInfoArgs->MemoryArgs->DctLoopArgs[0].BF_srcP;
  //  uint16_t          *BF_accumP        = FrameInfoArgs->MemoryArgs->DctLoopArgs[0].BF_accumP;
  //  uint8_t           *referenceSource    = FrameInfoArgs->frame_workSource;
  uint8_t* psrc = FrameInfoArgs->psrc;
  uint32_t           numBlocks_wide = FrameInfoArgs->numBlocks_wide;
  uint32_t           numBlocks_high = FrameInfoArgs->numBlocks_high;
  uint8_t        T2 = FrameInfoArgs->T2;

  uint8_t   save_FrameQuant = FrameInfoArgs->FrameQuant;
  uint8_t   save_quant = FrameInfoArgs->quant;
  uint8_t   save_qtype = FrameInfoArgs->qtype;
  uint8_t   save_adapt = FrameInfoArgs->adapt;
  uint8_t   save_shift = FrameInfoArgs->shift;
  uint8_t   save_matrix = FrameInfoArgs->matrix;
  uint8_t   save_expand = FrameInfoArgs->expand;
  uint8_t   save_sharpWPos = FrameInfoArgs->sharpWPos;
  uint8_t   save_sharpWAmt = FrameInfoArgs->sharpWAmt;
  uint8_t   save_sharpTPos = FrameInfoArgs->sharpTPos;
  uint8_t   save_sharpTAmt = FrameInfoArgs->sharpTAmt;
  uint8_t   save_showMask = FrameInfoArgs->showMask;

  //  uint32_t  sizeY          = FrameInfoArgs->sizeY;

  uint8_t   FrameQuant = FrameInfoArgs->FrameQuant;
  uint8_t   use_showMask = 19;

  int16_t  pixRowS, pixColS;
  uint16_t  blkRow, blkCol;
  int     stride = numBlocks_wide << 3;

  alignas(16) int16_t   dct_block[MATRIX_SIZE];
  alignas(16) uint8_t   newMatrix[MATRIX_SIZE];

  // NOTE: Currently this just does the DCT and skips the quant-dequant as well as the IDCT
  //       A second mask could allow the quant-dequant processing, skip the IDCT processing and then return.
  //
  // Show what the DCT transformed image looks like.
  // The only user arguments that has an effect are
  //   adapt which will have done it's work above.
  //   and shift which is used here.
  //   Normally shift will be 0.
  FrameInfoArgs->showMask = use_showMask;
  if (FrameInfoArgs->showMask == 19) {
    uint8_t use_shift = FrameInfoArgs->shift;  // WILL BECOME 0  !!!!!!!!!!!!!!!!!!!!!!!!!
    uint8_t use_quant = 1;
    uint8_t use_adapt = 0;
    uint8_t use_qtype = 1;
    uint8_t use_expand = 0;
    uint8_t use_matrix = MATRIX_FLAT3; // matrix_252_Flat_3;
    uint8_t use_sharpWAmt = 0;
    uint8_t use_sharpTAmt = 0;
    uint8_t use_sharpWPos = 7;
    uint8_t use_sharpTPos = 7;

    doFrameInfo(FrameQuant,
      use_quant,
      use_qtype,
      use_adapt,
      use_shift,
      use_matrix,

      use_expand,
      use_sharpWPos,
      use_sharpWAmt,
      use_sharpTPos,
      use_sharpTAmt,

      FrameInfoArgs,

      use_showMask,
      T2);

    set_matrix(FrameInfoArgs, use_matrix, use_qtype, use_quant, use_expand);
    dup_cpu_data(FrameInfoArgs);
    DispatchLoop(FrameInfoArgs);
    avgDctLoopAccumDCT(FrameInfoArgs);

    FrameInfoArgs->FrameQuant = save_FrameQuant;
    FrameInfoArgs->quant = save_quant;
    FrameInfoArgs->qtype = save_qtype;
    FrameInfoArgs->adapt = save_adapt;
    FrameInfoArgs->shift = save_shift;
    FrameInfoArgs->matrix = save_matrix;
    FrameInfoArgs->expand = save_expand;
    FrameInfoArgs->sharpWPos = save_sharpWPos;
    FrameInfoArgs->sharpWAmt = save_sharpWAmt;
    FrameInfoArgs->sharpTPos = save_sharpTPos;
    FrameInfoArgs->sharpTAmt = save_sharpTAmt;
    FrameInfoArgs->showMask = save_showMask;
  }

  // Now we do the per block arguments
//  psrc = FrameInfoArgs->psrc;
  psrc = FrameInfoArgs->frame_grainMask;

  stride = numBlocks_wide << 3;
  for (blkRow = 1; blkRow < numBlocks_high - 1; blkRow++) {         // THESE LOOPS WALK THROUGH THE FRAME BLOCK BY BLOCK.
    uint32_t BlockCntStartRow = blkRow * numBlocks_wide;
    uint32_t PixStartCurRow = (blkRow << 3) * stride;

    for (blkCol = 1; blkCol < numBlocks_wide - 1; blkCol++) {
      uint32_t   startPix = PixStartCurRow + (blkCol << 3);       // THE STARTING PIXEL LOCATION OF THE CURRENT BLOCK
      uint32_t   curBlock = blkCol + BlockCntStartRow;

      //      uint16_t    *qtype1_matrix        = LumaPerBlockArgs[curBlock].qtype1_matrix;//[MAX_BLOCK_ADAPT];
      //      uint16_t    *qtype1_matrix_quant  = LumaPerBlockArgs[curBlock].qtype1_matrix_quant; //[MAX_BLOCK_ADAPT];
      //      uint16_t    *qtype11_matrix       = LumaPerBlockArgs[curBlock].qtype11_matrix; //[MAX_BLOCK_ADAPT];
      //      uint16_t    *qtype11_matrix_quant = LumaPerBlockArgs[curBlock].qtype11_matrix_quant; //[MAX_BLOCK_ADAPT];

      //      uint8_t    AvgLuma               = LumaPerBlockArgs[curBlock].AvgLuma;
      //      uint8_t    AvgC                  = LumaPerBlockArgs[curBlock].AvgC;
      //      uint16_t  maxMinDif             = LumaPerBlockArgs[curBlock].maxMinDif;  // Range of values in block
            //uint8_t    MinLuma;              = LumaPerBlockArgs[curBlock].
            //uint8_t    brightMin;            = LumaPerBlockArgs[curBlock].
      float curValF;
      //int i;

      for (pixRowS = 0; pixRowS < 8; pixRowS++) {
        uint32_t strideXpixRowS = stride * pixRowS;
        for (pixColS = 0; pixColS < 8; pixColS++) {
          //  NOTE startPix = ((blkRow<<3)*stride) + (blkCol<<3); // THE STARTING PIXEL LOCATION OF THE CURRENT BLOCK
          uint32_t idxPixRC = startPix + (pixColS + strideXpixRowS);
          uint32_t idxNewMatrix = (pixRowS << 3) + pixColS;

          //if (T2==30) psrc[idxPixRC] = (uint8_t)Int_fitRangeGamma_Int(abs(psrc[idxPixRC]), 0, 255, 1.8F, 64, 255);  // Only do this to show the DCT info.

          //psrc[idxPixRC] = (uint8_t)Int_FitRange_Int(abs(psrc[idxPixRC]), 0, 64, 8, 255);

          //psrc[idxPixRC] = (uint8_t)Int_fitRangeGamma_Int(abs(psrc[idxPixRC]), 0, 64, 2.8F, 8, 255);

          dct_block[idxNewMatrix] = psrc[idxPixRC];

          // We want to fill in an 8x8 smoothing matrix.
          if (dct_block[idxNewMatrix] > 255) dct_block[idxNewMatrix] = 255;  // max matrix value;
          //if (dct_block[idxNewMatrix] < 3)   dct_block[idxNewMatrix] = 3;    // min matrix value;
          if (dct_block[idxNewMatrix] < 8)   dct_block[idxNewMatrix] = 8;    // min matrix value;

          newMatrix[idxNewMatrix] = (uint8_t)dct_block[idxNewMatrix];
          //newMatrix[idxNewMatrix] = (uint8_t)32;

//          if (pixRowS == 0 || pixColS == 0) {
//            newMatrix[idxNewMatrix] = (uint8_t)dct_block[idxNewMatrix];
//          }
//          else {
//            newMatrix[idxNewMatrix] = 3;
//          }
        }
      }
      newMatrix[0] = 8;
      // We now have a raw matrix.  Precompute what we can to save work for DctLoop.
      set_intra_matrix(FrameInfoArgs->MemoryArgs->DctLoopArgs[0].quant_intra_matrix, newMatrix);
      set_inter_matrix(FrameInfoArgs->MemoryArgs->DctLoopArgs[0].quant_inter_matrix, newMatrix);

      curValF = (float)FrameInfoArgs->MemoryArgs->DctLoopArgs[0].blkAdaptSmoothAmt[curBlock];
      if (curValF < 3.0F) curValF = 13.0F;
      curValF = 23.0F;
      init_intra_matrixF(FrameInfoArgs->MemoryArgs->DctLoopArgs[0].quant_intra_matrix, curValF);

      //      for (i = 0; i < 64; i++) {
      //        FrameInfoArgs->MemoryArgs->DctLoopArgs[0].qtype11_matrixPS[curBlock].qtype11_matrixPS[i]             = (uint16_t)(FrameInfoArgs->MemoryArgs->DctLoopArgs[0].quant_intra_matrix[i]    * curValF + 0.5F);
      //        FrameInfoArgs->MemoryArgs->DctLoopArgs[0].qtype11_matrix_quantPS[curBlock].qtype11_matrix_quantPS[i] = (uint16_t)(FrameInfoArgs->MemoryArgs->DctLoopArgs[0].quant_intra_matrix[i+64] * curValF + 0.5F);
      //      }
    }
  }



  FrameInfoArgs->showMask = 0; //save_showMask;
  return;
}

/*
      //if (T2==31) {
      if (T2==30) {
        init_matrix(&dct_blockOrigMatrix[0],
              &dct_blockOrigMatrix[64],
              12, 1);

        //memcpy(dct_blockOrig, dct_block, blockSize);
        float quantF = 17.0F;
        //uint16_t quantVal[2];

        //uint8_t s1 = (uint8_t)dct_block[0];
        //s1 = dct_block[0];

        int xcnt = 0;
        int ycnt = 0;
        int maxXY = 7;
        int minXY = 1;
        int avgLuma = dct_block[0];
        for (x=minXY; x<maxXY; x++) {
          for (y=minXY; y<=maxXY; y++) {
            int idx1 = x*8 + y;
            int idx2 = y*8 + x;
            int colWeight;
            int dctWeight;
            if (x == 0 || y == 0) {
            if (x == 0 && y >= minXY && abs(dct_block[idx1]) > 80)  ycnt++;
            if (y == 0 && x >= minXY && abs(dct_block[idx1]) > 80)  xcnt++;

            //int temp = Int_fitRangeGamma_Int((float)abs(dct_blockOrig[l]), 0.0, 192.0, 1.0, 48.0, 255.0);
            // Setting gamma to 1.8 makes it easier to see the small value differences.
            //int16_t temp = (int16_t)Int_fitRangeGamma_Int(abs(dct_blockOrig[l]), 0, 255, 1.8, 42, 255);

            //uint8_t temp = (uint8_t)Int_fitRangeGamma_Int(abs(dct_block[idx1]), 0, 255, 1.8, 0, 255);
            //uint8_t temp = (uint8_t)Int_fitRangeGamma_Int(abs(dct_block[idx1]), 0, 255, 2.0, 8, 255);
            dctWeight = abs(dct_block[idx1]);
            if (dctWeight > 255) dctWeight = 255;
            //colWeight = (uint8_t)Int_FitRange_Int(max(x,y), 0, 7, 8, 128);
            //colWeight = (uint8_t)Int_FitRange_Int(max(x,y), 2, maxXY - 1, 64, 128);
            //colWeight = (uint8_t)Int_FitRange_Int(max(x,y), minXY, maxXY, 24, 96);
            colWeight = (uint8_t)Int_FitRange_Int(max(x,y), minXY, maxXY, 3, 128);
//if (T2==30)  colWeight = (uint8_t)Int_FitRange_Int(abs(dct_block[0]), 0, 255, colWeight>>2, colWeight);
            uint8_t temp = (uint8_t)Int_FitRange_Int(dctWeight, 0, 255, 3, colWeight);
            //uint8_t temp = (uint8_t)Int_fitRangeGamma_Int(dctWeight, 0, 255, 1.8, 8, colWeight);
            //if (x==1 || y==1)   temp = temp>>1;
            //if (temp < 8) temp = 8;
            if (temp < 3) temp = 3;
            //dct_blockOrig[idx2] = temp;
            //dct_blockOrig[idx1] = temp;

            dct_blockOrigMatrix[idx1] = temp;
            //dct_blockOrigMatrix[idx2] = temp;
            //dct_block[idx2] = dct_blockOrig[idx1];
            }
          }
        }

//        if (xcnt > 4)   { for (xcnt= 0; xcnt < 7; xcnt++) dct_blockOrigMatrix[xcnt]   = 8; }
//        if (ycnt > 4)   { for (ycnt= 0; ycnt < 7; ycnt++) dct_blockOrigMatrix[ycnt*8] = 8; }

        //dct_blockOrig[0] = 8;

//dct_blockOrigMatrix
//init_intra_matrixF(uint16_t *mpeg_quant_matrices, float quant)

//  init_matrix(dct_blockOrigMatrix[0],
//            dct_blockOrigMatrix[64],
//        11, 1);

    init_intra_matrixF(dct_blockOrigMatrix, quantF);
    for (i = 0; i < 64; i++) {
      //matrix_quant[i]  = (uint16_t)(FrameInfoArgs->MemoryArgs->DctLoopArgs[0].quant_intra_matrix[i] * quantF + 0.5F);
      //matrix[i]        = (uint16_t)((FrameInfoArgs->MemoryArgs->DctLoopArgs[0].quant_intra_matrix[i + 64]) << 2);
      matrix_quant[i]  = (uint16_t)(dct_blockOrigMatrix[i] * quantF + 0.5F);
      matrix[i]        = (uint16_t)((dct_blockOrigMatrix[i + 64]) << 2);
    }
      //qtype1_matrix_quant   = args->qtype1_matrix_quant[blkQuantVal];
      //qtype1_matrix         = args->qtype1_matrix[blkQuantVal];

//        for (x=0; x<8; x++) {
//          for (y=0; y<8; y++) {
//            int idx1 = x*8 + y;
//            //int idx2 = y*8 + x;
//            if (x == 0 || y == 0) continue;
//            dct_blockOrig[idx1] = dct_block[idx1];
//          }
//        }
//        for (x=0; x<8; x++) {
//          for (y=0; y<8; y++) {
//            int idx1 = x*8 + y + 64;
//            // idx2 = y*8 + x;
//            if (x == 0 || y == 0) continue;
//            dct_blockOrig[idx1] = dct_block[idx1];
//          }
//        }

//    for (i = 0; i < 64; i++) {
//      dct_blockOrig[i]  = (uint16_t)(FrameInfoArgs->MemoryArgs->DctLoopArgs[0].quant_intra_matrix[i] * curValF + 0.5F);

        //quantVal[0] = 3;
        //quantVal[1] = 0;
        //dct_blockOrig[0] = 8;

        dbzero = dct_block[0];
        //quantDequant(dct_block, qtype1_matrix, qtype1_matrix_quant);
        //quantDequant(dct_block, &dct_blockOrig[0], &dct_blockOrig[64]);
        //quantDequant(dct_block, &dct_blockOrigMatrix[0], &dct_blockOrigMatrix[64]);
        //quantDequant(dct_blockOrig, &dct_blockOrig[0], &dct_blockOrig[64]);
        quantDequant(dct_block, matrix, matrix_quant);
        dct_block[0] = dbzero;
}

*/

////////////////////////////
