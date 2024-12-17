#include  <math.h>

#include "Sharp.h"

#include "Utilities.h"
#include "amDCTtypedefs.h"  

#include "Matrix.h"

#include "quant\quant_matrix.h"

#include "DispatchLoop.h"
#include "Memory.h"

#ifdef INTEL_INTRINSICS
#include "emmintrin.h"
#endif




// NOTE: Values going left to right  on the top row       control the strength of sharpening on vertical   edges.
//       Values going Top  to bottom on the left hand col control the strength of sharpening on horizontal edges.


static float sharpmatrix[64] = {   // CURRENTLY BEST MATRIX
    1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.6F, 1.6F, 1.6F,

    1.0F, 1.0F, 1.0F, 1.0F, 1.6F, 1.6F, 1.6F, 1.6F,

    1.0F, 1.0F, 1.0F, 1.6F, 1.6F, 1.6F, 1.6F, 1.0F,

    1.0F, 1.0F, 1.6F, 1.6F, 1.6F, 1.6F, 1.0F, 1.0F,

    1.0F, 1.6F, 1.6F, 1.6F, 1.6F, 1.6F, 1.0F, 1.0F,

    1.6F, 1.6F, 1.6F, 1.6F, 1.6F, 1.6F, 1.0F, 1.0F,

    1.6F, 1.6F, 1.6F, 1.0F, 1.6F, 1.0F, 1.0F, 1.0F,

    1.6F, 1.6F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F
};





//static float sharpmatrix_1080P[64] = { 
//    1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.6F, 1.6F, 1.6F,    
//
//    1.0F, 1.0F, 1.0F, 1.0F, 1.5F, 1.6F, 1.6F, 1.6F,       
//
//    1.0F, 1.0F, 1.6F, 1.4F, 1.6F, 1.6F, 1.6F, 1.0F,      
//
//    1.0F, 1.0F, 1.4F, 1.2F, 1.4F, 1.6F, 1.0F, 1.0F,     
//
//    1.0F, 1.5F, 1.6F, 1.6F, 1.6F, 1.6F, 1.0F, 1.0F,    
//
//    1.6F, 1.6F, 1.6F, 1.6F, 1.6F, 1.6F, 1.0F, 1.0F,   
//
//    1.6F, 1.6F, 1.6F, 1.0F, 1.6F, 1.0F, 1.0F, 1.0F,  
//
//    1.6F, 1.6F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F   
//  };  






int   Sharp(FrameInfo_args* FrameInfoArgs, int noRun) {

  uint8_t  sharpWPos = FrameInfoArgs->sharpWPos;
  float  sharpWAmtF = (float)FrameInfoArgs->sharpWAmt;
  uint8_t  sharpTPos = FrameInfoArgs->sharpTPos;
  float  sharpTAmtF = (float)FrameInfoArgs->sharpTAmt;
  //  int      T2              =        FrameInfoArgs->T2;

  float   adapt_inc = 0.0F;
  float   adapt_rangeF = 0.0F;
  float   maxBlkAdaptF = (float)MAX_BLOCK_ADAPT;
  float   curVal = 0.0F;

  uint8_t use_qtype = 21;
  uint8_t save_qtype = FrameInfoArgs->qtype;

  float   range_up = 1.0F;
  float   range_down = 1.0F;

  float   col_strength_val[8];
  float   sharpmatrix_Use[64];

  int     sharp_width;
  uint8_t blkCol;
  int     col;
  int     start_col;
  int     st_col;
  int     end_col;
  int     row;
  int     end_row;
  int     use_width;
  int     use_col;

  uint16_t  quant_sharp;
  uint16_t  matrix_sharp;
  uint16_t  absLevelOrig;

  int i, j;

  sharpWAmtF = sharpWAmtF / 2.0F;
  sharpTAmtF = sharpTAmtF / 2.0F;

  FrameInfoArgs->qtype = use_qtype;  // Sharpening uses special qtype

  set_sharp_matrix(FrameInfoArgs);


  // This allows the user to only specify 1 sharp?_NumPix argument if both arguments are the same. 
  if (sharpWPos > 0 && sharpTPos == 0) sharpTPos = sharpWPos;
  if (sharpWPos == 0 && sharpTPos > 0) sharpWPos = sharpTPos;


  // This allows the user to only specify 1 strength argument if both arguments are the same. 
  if (sharpWAmtF > 0 && sharpTAmtF == 0) sharpTAmtF = sharpWAmtF;
  if (sharpWAmtF == 0 && sharpTAmtF > 0) sharpWAmtF = sharpTAmtF;



  // The sharpmatrix_Use[] provides a multiplier to the final matrix values set to the sharpening code in DctLoop()
  for (i = 0; i < 64; i++) sharpmatrix_Use[i] = sharpmatrix[i];
  //  if (T2==16)  for (i = 0; i < 64; i++) sharpmatrix_Use[i] = sharpmatrix_1080P[i];  // CODE to test different sharpmatrix weights.

    // Compute the strength values for each column.
  for (blkCol = 0; blkCol < 8; blkCol++) col_strength_val[blkCol] = 1.0F;

  sharp_width = abs(sharpTPos - sharpWPos);
  if (sharp_width == 0) {
    if (sharpWPos < 0 || sharpWPos >= 8) {
      sharpWPos = 7;
      sharpTPos = 7;
    }
    col_strength_val[sharpWPos] = sharpWAmtF;
  }
  else {
    for (blkCol = sharpWPos; blkCol <= sharpTPos; blkCol++) {
      float use_Strength = sharpWAmtF;
      float use_Strength_dif;
      float use_Strength_inc;

      if (blkCol < 0 || blkCol >= 8) continue; // Safety check to keep lint happy.
      if (blkCol == sharpWPos) { // Start col
        col_strength_val[blkCol] = sharpWAmtF;
        continue;
      }
      if (blkCol == sharpTPos) { // End col
        col_strength_val[blkCol] = sharpTAmtF;
        continue;
      }
      if (fabs(sharpWAmtF - sharpTAmtF) < 0.2F) { // All cols the same value.
        col_strength_val[blkCol] = sharpWAmtF;
        continue;
      }

      // This code fills in the values between the endpoints if the endpoint values are different.      
      use_Strength_dif = sharpTAmtF - sharpWAmtF;
      use_Strength_inc = use_Strength_dif / (float)(sharp_width - 1);
      use_Strength += use_Strength_inc;
      col_strength_val[blkCol] = use_Strength;
    }
  }



  // range_up and range_down are used to control the amount of range expansion that is done.
  // They are used to modify the values in qtype1_matrix_quant and qtype1_matrix. 
  // Making either range_up or range_down larger increases the range
  range_up = 1.0F;
  range_down = 1.0F;


  // This code fills in the diagonal arguments. 
  // NOTES: 
  //   1: If sharpWPos == sharpTPos then
  //      A: st_col == end_col
  //      B: sharp_width  == 0
  //      C: use_width    == 0
  //      D: Strength_inc == 0
  start_col = sharpWPos;
  st_col = sharpWPos;
  sharp_width = abs(sharpTPos - sharpWPos);
  use_width = sharp_width;
  end_row = sharpTPos;


  for (row = 0; row <= end_row; row++) {

    st_col = start_col - row;
    end_col = st_col + sharp_width;
    if (st_col < 0) {
      st_col = 0;
      sharpWPos++;
      use_width = abs(sharpTPos - sharpWPos);
    }

    if (end_col > 7)
      end_col = 7;


    for (col = st_col; col <= end_col; col++) {
      float colStrVal;

      use_col = col + row;
      if (use_col < 0)
        use_col = 0;

      if (use_col > 7)
        use_col = 7;

      i = row * 8 + col;

      // Reduce colStrVal for noisier blocks
      // col_strength_val[use_col] contains the user specified sharpening strength (sharpWAmtF, sharpTAmtF) for each column.
      // sharpmatrix_Use[i] contains a multiplier weight for each column.  This is hard coded in the sharpmatrix above.
      adapt_inc = 0.0F;
      adapt_rangeF = col_strength_val[use_col] * sharpmatrix_Use[i];
      if (adapt_rangeF > 1.0F) {
        //        adapt_inc = adapt_rangeF / (maxBlkAdaptF * 2.4F);       
        adapt_inc = adapt_rangeF / (maxBlkAdaptF * 2.8F); //   This works best
      }

      curVal = adapt_rangeF;

      for (j = 0; j < MAX_BLOCK_ADAPT; j++) {
        adapt_rangeF = curVal;

        if (adapt_rangeF <= 1.0F) {
          range_down = 1.0F;
          range_up = 1.0F;
        }
        else {
          colStrVal = adapt_rangeF;
          if (colStrVal <= 1.01F) colStrVal = 1.01F;
          range_down = Float_FitRange_Float(colStrVal, 52.0F, 1.0F, 9.0F, 1.0F);  // Stronger set of values  
          range_up = Float_FitRange_Float(colStrVal, 52.0F, 1.0F, 3.98F, 1.0F);
          //range_down = Float_FitRange_Float(colStrVal, 52.0F, 1.0F,  6.0F,  1.0F);    
          //range_up   = Float_FitRange_Float(colStrVal, 52.0F, 1.0F,  2.6F,  1.0F);
        }

        quant_sharp = (uint16_t)((float)(FrameInfoArgs->MemoryArgs->quant_intra_matrix_sharp[i]) * range_down + 0.5F);
        matrix_sharp = (uint16_t)((float)(FrameInfoArgs->MemoryArgs->quant_intra_matrix_sharp[i + 64]) * range_up + 0.5F);
        FrameInfoArgs->MemoryArgs->DctLoopArgs[0].qtype1_matrix_quant_sharp[j][i] = quant_sharp;
        FrameInfoArgs->MemoryArgs->DctLoopArgs[0].qtype1_matrix_sharp[j][i] = matrix_sharp;

        // To greatly speed up the sharpening routine we precompute all of the values that can be sharpened and 
        //                          store the results in a table which will be passed to the sharpening routine.
        // PreCompute for each position in a macro-block, the sharpened values for the values 0...64 that can be sharpened.
        // 64 is the 64 pixels in an 8x8 block.  MAX_LEVEL_IDX are the 70 resulting 8 bit values for each position in the 8x8 block, which are obtained from precomputing the new DCT values. 
        // They are accessed as a 64x64 array with the first index being the pixel location and the second being the resulting values.        
        // quant_sharp_preComp[0][j][i*64 + absLevelOrig]   
        //                    [0]=NCPU  
        //                    [j]=BLOCK_ADAPT   
        //                    [i*64 + absLevelOrig]=  i= row*8 + col the DCT position     
        //                                            absLevelOrig=0...63 is the input DCT value, the cell contains the returned DCT value
        for (absLevelOrig = 0; absLevelOrig < 64; absLevelOrig++) {
          int32_t   level;
          uint32_t  absLevel = 0;
          uint32_t  difLevel = 0;
          uint32_t  levelCutoff = 64;
          uint32_t  sharpenedCutoff = levelCutoff + 4;

          level = (absLevelOrig * matrix_sharp + 8192) >> 14;
          if (level == 0) {
            FrameInfoArgs->MemoryArgs->DctLoopArgs[0].quant_sharp_preComp[j][i * 64 + absLevelOrig] = 0;  // 0 informs the sharpening routine to not change the existing block value.
            continue;
          }

          level = (int16_t)(level * quant_sharp) >> 3;
          if (level == 0) {
            FrameInfoArgs->MemoryArgs->DctLoopArgs[0].quant_sharp_preComp[j][i * 64 + absLevelOrig] = 0;
            continue;
          }

          absLevel = abs(level);
          if (absLevel > sharpenedCutoff) {
            FrameInfoArgs->MemoryArgs->DctLoopArgs[0].quant_sharp_preComp[j][i * 64 + absLevelOrig] = 0;
            continue;   // Since we are sharpening we know that the level will get larger therefor level needs to be < levelCutoff to have headroom for sharpening.
          }

          if (absLevelOrig <= levelCutoff >> 2) {    // We slightly increase the amount of sharpening done for the smallest 1/4 of the values.
            difLevel = absLevel - absLevelOrig;  // absLevel is >= absLevelOrig since absLevel has been sharpened and hence the values got larger.
            absLevel = Int_FitRange_Int(difLevel, levelCutoff - 0, 1, absLevelOrig + 2, absLevel);
          }
          else {
            levelCutoff = levelCutoff + (levelCutoff >> 2);
            absLevel = Int_FitRange_Int(absLevel, levelCutoff, 1, absLevelOrig + 1, absLevel + 0);
          }

          FrameInfoArgs->MemoryArgs->DctLoopArgs[0].quant_sharp_preComp[j][i * 64 + absLevelOrig] = (uint8_t)absLevel;
        }

        curVal = curVal - adapt_inc;
      }
    }
  }


  FrameInfoArgs->qtype = use_qtype;

  // When the user specifies sharpening, smoothing, and optionally range expansion with quality == 1, then
  // noRun == 1 allows amDCT() to call Sharp() and have it set up everything necessary 
  // so that sharpening, smoothing, and optionally range expansion can be done with one call to dctLoop().
  // If quality > 1 then sharpening will be done in its own call to dctloop().
  if (noRun != 1) {
    dup_cpu_data(FrameInfoArgs);
    DispatchLoop(FrameInfoArgs);
  }

  FrameInfoArgs->qtype = save_qtype;

  return(0);
}



