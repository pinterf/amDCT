#include <math.h>

#include "amDCTtypedefs.h"

#include "Matrix.h"
#include "quant\quant_matrix.h"
#include "Utilities.h"

#define SCALEBITS 17

//static int max_debug8=0;
//static int min_debug8=9999;

//void init_matrix_Vals(const uint16_t *quant_intra_matrix, const uint16_t *quant_inter_matrix, uint16_t *matrix_Vals, uint8_t qtype);
void init_matrix_Vals(uint16_t* quant_intra_matrix, uint16_t* quant_inter_matrix, uint16_t* matrix_Vals, uint8_t qtype);

// A version of init_intra_matrix that allows quant to be a float.
// set_matrix() needs fractional quants to implement the small changes in
// smoothing and range expansion strengths that can occur from one block
// to the next when block adaptive processing is activated.
//
// This is only used in set_matrix() below.
void init_intra_matrixF(uint16_t* mpeg_quant_matrices, float quant)
{
  int i;
  uint16_t* intra_matrix = mpeg_quant_matrices + 0 * 64;
  uint16_t* intra_matrix_rec = mpeg_quant_matrices + 1 * 64;

  for (i = 0; i < 64; i++) {
    float temp = (float)(1 << SCALEBITS);
    float div = ((float)(intra_matrix[i])) * quant;

    intra_matrix_rec[i] = (uint16_t)((temp / div) + 0.5F);
  }
}


//void
//init_intra_matrix(uint16_t *mpeg_quant_matrices, float quant) {
//
//  set_intra_matrix(quant_intra_matrix, matrix);
//  set_inter_matrix(quant_inter_matrix, matrix);
//
//}

void set_matrix(FrameInfo_args* FrameInfoArgs, uint8_t matrix_Num, uint8_t qtype, uint8_t quant, uint8_t range_expand)
{
  uint8_t   use_quant = quant;
  uint8_t   min_quant = quant;
  uint8_t   max_quant = FrameInfoArgs->max_quant;  // max_quant is the max(quant, adapt)
  //  uint8_t   T2                  = FrameInfoArgs->T2;
  int       matrixHasLT18 = 0;  // The matrix has values that are Less Than 18 in it.
  int       matrixHasLT8 = 0;  // The matrix has values that are Less Than 8  in it.
  int       matrixHasLT3 = 0;  // The matrix has values that are Less Than 3  in it.
  int       matrixHasLT2 = 0;  // The matrix has values that are Less Than 2  in it.
  int       adapt_range = 0;
  float     maxBlkAdaptF = MAX_BLOCK_ADAPT;
  float     adapt_incF = 0.0F;
  float     curValF = 0.0F;
  float     range_up;
  float     range_down;
  int       i, j;


  //  // If the matrix has 1's in it then the C version (qtype 11) is used since the optimized mmx routine will overflow.
  //  // The [i + 64] index is because XVID stores different versions of a matrix one immediately after the other in memory.
  //  if ((qtype == 1 && matrixHasOnes == 1) || qtype == 11) {
  //    qtype = 11;
  //    for (i = 0; i < 64; i++)   MemoryArgs.qtype1_matrix[0][i] = MemoryArgs.quant_intra_matrix[0][i + 64];
  //  }
  //  else {
  //    // The <<2 allows the mmx version to do a multiply and only use the high 16 bits of the 32 bit result.
  //    for (i = 0; i < 64; i++)   MemoryArgs.qtype1_matrix[0][i] = MemoryArgs.quant_intra_matrix[0][i + 64]<<2;
  //  }


    // init_matrix() is in this file.
  init_matrix(FrameInfoArgs->MemoryArgs->DctLoopArgs[0].quant_intra_matrix,
    FrameInfoArgs->MemoryArgs->DctLoopArgs[0].quant_inter_matrix,
    matrix_Num, qtype);


  // Matrices with 1's in them produce bizarre results unless quant > 2
  for (i = 0; i < 64; i++) {
    if (FrameInfoArgs->MemoryArgs->DctLoopArgs[0].quant_intra_matrix[i] < 18) {
      matrixHasLT18 = 1;
    }
    if (FrameInfoArgs->MemoryArgs->DctLoopArgs[0].quant_intra_matrix[i] < 8) {
      matrixHasLT8 = 1;
    }
    if (FrameInfoArgs->MemoryArgs->DctLoopArgs[0].quant_intra_matrix[i] < 3) {
      matrixHasLT3 = 1;
    }
    if (FrameInfoArgs->MemoryArgs->DctLoopArgs[0].quant_intra_matrix[i] < 2) {
      matrixHasLT2 = 1;
    }
  }

  //min_quant=2;
    //if ((qtype == 1 || qtype == 11 || qtype >= 21) && use_quant <= 1 && matrixHasLT3 == 1)
  if ((qtype == 1 || qtype == 11 || qtype >= 21) && matrixHasLT3 == 1)
    min_quant = 2;

  //if ((qtype == 1 || qtype == 11 || qtype >= 21) && use_quant <= 3 && matrixHasLT2 == 1)
  if ((qtype == 1 || qtype == 11 || qtype >= 21) && matrixHasLT2 == 1)
    min_quant = 3;

  //  if (qtype == 1 &&  matrixHasLT8 == 1 || qtype == 1 && use_quant <= 3) {
  if (qtype == 1 && matrixHasLT8 == 1) {
    qtype = 11;
    FrameInfoArgs->qtype = qtype;
  }

  if (use_quant < min_quant) {
    use_quant = min_quant;
    FrameInfoArgs->quant = use_quant;
  }


  FrameInfoArgs->MemoryArgs->DctLoopArgs[0].quant = use_quant;
  FrameInfoArgs->MemoryArgs->DctLoopArgs[0].qtype = qtype;



  // To maximize speed for mpeg intra "qtype 1" we precompute as much of the quant dequant processing as possible.
  // Using the matrix and quant value specified by the user we produce 2 matrices qtype1_matrix_quant and qtype1_matrix.
  // This allows for a very simple C implementation for the quant dequant processing that runs as fast as the 2 mmx
  // routines it replaces.  It also allowed a much simpler new mmx routine that provided for a nice speedup.
  //
  // MAX_BLOCK_ADAPT defined in amDCT.h specifies how many different strengths of smoothing or range expansion are available.
  // The different strengths are computed below and used in DctLoop().  Block adaptive smoothing and range expansion is
  // implemented by having DctLoop() choose the appropriate value based upon the block adaptive strength, computed in doAdaptMask()
  // and LumaBlockInfo(), for each block that it processes.
  // This loop specifies the quant values that will be used for each of those levels.


  if (FrameInfoArgs->adapt_a > FrameInfoArgs->quant) {
    max_quant = MAX(FrameInfoArgs->adapt_a, FrameInfoArgs->quant);
    min_quant = MIN(use_quant, FrameInfoArgs->quant);
  }
  else {
    max_quant = FrameInfoArgs->quant;
    min_quant = FrameInfoArgs->quant;
  }

  //use_quant=4;
  //maxBlkAdaptF = 29.0;
  maxBlkAdaptF = max_quant;

  adapt_incF = 0.0F;
  adapt_range = 0;
  if (max_quant > (use_quant + 3)) {
    adapt_incF = ((float)(max_quant - use_quant)) / maxBlkAdaptF;
    adapt_range = max_quant - use_quant;
  }

  // When smoothing we build for both qtype=1 fastest but only works for Quant values > 4 and qtype=11 which works for all Quant values.
  // We also specify the quant value for each adaptive weight that is used so DctLoop can switch between qtype1 and qtype11 on a per block bases
  // depending on the Quant value for that block.

  curValF = (float)use_quant;
  for (j = 0; j < MAX_BLOCK_ADAPT; j++) {
    init_intra_matrixF(FrameInfoArgs->MemoryArgs->DctLoopArgs[0].quant_intra_matrix, curValF);

    for (i = 0; i < 64; i++) {
      FrameInfoArgs->MemoryArgs->DctLoopArgs[0].qtype1_matrix_quant[j][i] = (uint16_t)(FrameInfoArgs->MemoryArgs->DctLoopArgs[0].quant_intra_matrix[i] * curValF + 0.5F);
      FrameInfoArgs->MemoryArgs->DctLoopArgs[0].qtype11_matrix_quant[j][i] = (uint16_t)(FrameInfoArgs->MemoryArgs->DctLoopArgs[0].quant_intra_matrix[i] * curValF + 0.5F);

      FrameInfoArgs->MemoryArgs->DctLoopArgs[0].qtype1_matrix[j][i] = (uint16_t)((FrameInfoArgs->MemoryArgs->DctLoopArgs[0].quant_intra_matrix[i + 64]) << 2);
      FrameInfoArgs->MemoryArgs->DctLoopArgs[0].qtype11_matrix[j][i] = (uint16_t)(FrameInfoArgs->MemoryArgs->DctLoopArgs[0].quant_intra_matrix[i + 64]);
    }
    curValF = curValF + adapt_incF;
  }



  // Range Expansion is similar except that we decrease the amount of range expansion as the adaptation strength goes up.
  // NOTE: range_expand has the user argument range of 1-31 so we will need to convert it to 0-30
  if (range_expand > 0) {
    float use_expand = (float)range_expand;

    if (range_expand <= 1) use_expand = 1.001F;
    if (range_expand >= 31) use_expand = (float)31;

    matrix_Num = 242; // Works well when doing strong expand and sharpening

    //    if (FrameInfoArgs->T2==40) matrix_Num = 10;
    //    if (FrameInfoArgs->T2==41) matrix_Num = 152;
    //    if (FrameInfoArgs->T2==42) matrix_Num = 248;  // BEST was 163
    //    if (FrameInfoArgs->T2==43) matrix_Num = 162;



    init_matrix(FrameInfoArgs->MemoryArgs->DctLoopArgs[0].quant_intra_matrix,
      FrameInfoArgs->MemoryArgs->DctLoopArgs[0].quant_inter_matrix,
      matrix_Num, 22);  // init_matrix()   11 is the matrix number. matrix 11 is all 8's.  22 is the qtype


    for (j = 0; j < MAX_BLOCK_ADAPT; j++) {

      curValF = Float_FitRange_Float((float)j, 30.0F, 0.0F, use_expand, 1.0F);  // range_expand values are in the range  1 to range_expand.   range_expand <= 31

      range_up = Float_FitRange_Float(curValF, 31.0F, 0.0F, 1.8F, 1.0F);
      range_down = Float_FitRange_Float(curValF, 31.0F, 0.0F, 0.9F, 1.0F);

      init_intra_matrix(FrameInfoArgs->MemoryArgs->DctLoopArgs[0].quant_intra_matrix, 1);

      // The + 0.2F at the end of the next 2 lines is used to adjust the amount of expansion for expand = 1
      for (i = 0; i < 64; i++)   FrameInfoArgs->MemoryArgs->DctLoopArgs[0].qtype1_matrix_quant_range[j][i] = (uint16_t)((FrameInfoArgs->MemoryArgs->DctLoopArgs[0].quant_intra_matrix[i] * range_down) + 0.2F);
      for (i = 0; i < 64; i++)   FrameInfoArgs->MemoryArgs->DctLoopArgs[0].qtype1_matrix_range[j][i] = (uint16_t)((FrameInfoArgs->MemoryArgs->DctLoopArgs[0].quant_intra_matrix[i + 64] * range_up) + 0.2F);
    }
  }
}






void set_sharp_matrix(FrameInfo_args* FrameInfoArgs)
{

  int   i, j;

  init_matrix(FrameInfoArgs->MemoryArgs->quant_intra_matrix_sharp,
    FrameInfoArgs->MemoryArgs->quant_inter_matrix_sharp,
    //        152, 21);  // 11=matrix all 8s   21=sharp
    //        11, 21);  // 11=matrix all 8s   21=sharp
    MATRIX_SHARP, 21);  // matrix all 8s   21=sharp

  init_intra_matrix(FrameInfoArgs->MemoryArgs->quant_intra_matrix_sharp, 1);

  for (j = 0; j < MAX_BLOCK_ADAPT; j++) {
    // The real values for sharpening will be filled in in Sharp()
    for (i = 0; i < 64; i++)   FrameInfoArgs->MemoryArgs->DctLoopArgs[0].qtype1_matrix_quant_sharp[j][i] = (uint16_t)(FrameInfoArgs->MemoryArgs->quant_intra_matrix_sharp[i]);
    for (i = 0; i < 64; i++)   FrameInfoArgs->MemoryArgs->DctLoopArgs[0].qtype1_matrix_sharp[j][i] = (uint16_t)(FrameInfoArgs->MemoryArgs->quant_intra_matrix_sharp[i + 64]);
  }
}








// matrix names
//        N_     "normal"
//        Flat_  "flat"
//        H_     "Horizontal"  thickness of line found
//        V_     "Vertical"
//        HV_    "Horizontal and Vertical"
//        D_     "Diagonal"   anti aliasing, low angle 45 degree, high angle, thickness of line found
// matrix numbers  1-9   are "normal" looking ordered from weakest to strongest.
// matrix numbers 10-29  are "flat" containing values of either 3, 8, 16, 24, 32, 40, 48, 64, 80, 96, 112, 128, 144, 160, 176, 192, 208, 224, 240
// matrix numbers 30-49  are "Horizontal"
// matrix numbers 50-79  are "Vertical"
// matrix numbers 80-89  are "Horizontal and Vertical"
// matrix numbers 90-110 are "Diagonal"

// In the above naming "Horizontal" means that the primary smoothing is in the horizontal direction.
//         Doing a mt_makediff(original, smoothed) will show vertical lines.
//         It softens vertical block edges.
//   H_2_64        Horizontal           column 2 with value 64
//   H_2_64_8_64   Horizontal  starting column 2 with value 64  ending column 8 with value  64
//   H_2_8_8_240   Horizontal  starting column 2 with value  8  ending column 8 with value 240


//  NOTES:
//  3 is minimum value that can appear in a matrix.
//  Using a value above 64 in row 1 col 2 or row 2 col 1 can cause the math to overflow.
//  Using a value above ???? in row 1 col 1 can cause the math to overflow.
//  Larger values only have an effect at small quant values.
//  Top left corner does nothing in mpeg intra. It only has effects in mpeg inter.

//  If you are using Horizontal, Vertical, "Horizontal and Vertical", or Diagonal to find lines in the image then if you diff the result with the image using Flat_3
//  it will provide a cleaner signal. It allows the use of quant 31.
//
//  It appears that values in a row 1 or column 1 position will have the most effect as follows
//  pos    width in number of lines
//  2      5-6
//  3      4-5
//  4      3-4
//  5      2 some 3
//  6      1 some 2
//  7      1
//  8      1

// 3
// 3 almost == 8  3 does slightly better at smoothing gradients.
// 3 does not smooth sharp transients as well as 9 which means less detail is removed
//   but it will leave some fine line block boundaries not smoothed as much.
//   It probably makes more sense to clean up those boundaries with a second
//   filter designed for that purpose.

void init_matrix(uint16_t* quant_intra_matrix, uint16_t* quant_inter_matrix, uint8_t matrix_Num, uint8_t qtype) {

  uint8_t const* use_matrix;
  uint8_t matrix[64];  // This matrix gets filled in with a copy of one of the following matrices.
  int     i;


  //    NOTE
  //  Zeros are not allowed in a matrix.
  //  Any matrix that has a 1 in it will not work for quant values < 3.
  //      The system will increase the quant value to 3 so it doesn't produce horrible results but quant values 1, 2, and 3 end up being the same.
  //  Any matrix that has a 2 in it will not work for quant values < 2.
  //      The system will increase the quant value to 2 so it doesn't produce horrible results but quant values 1 and 2 end up being the same.


  /*
  //
  //// matrix_1. It is good for removing noise before sharpening.
  //static const uint8_t matrix_1[64] = {
  //      4,  4,  5,  6,  7,  8,  9, 10,
  //      4,  6,  6,  7,  8,  9, 10, 10,
  //      5,  6,  7,  8,  9, 10, 10, 10,
  //      6,  7,  8,  9, 10, 10, 10, 10,
  //      7,  8,  9, 10, 10, 10, 10, 11,
  //      8,  9, 10, 10, 10, 10, 11, 11,
  //      9, 10, 10, 10, 10, 11, 11, 11,
  //     10, 10, 10, 10, 11, 11, 11, 11
  //  };
  //




  //// matrix_2. Used in the presmoother section of amDCT.c
  //// Smooths areas that are being sharpened more than other areas
  //static const uint8_t matrix_OLD2[64] = {
  //      8,  8,  8,  8,  8, 32, 32, 38,
  //      8,  8,  8,  8, 32, 32, 32,  8,
  //      8,  8,  8, 32, 32, 32,  8,  8,
  //      8,  8, 32, 32, 32,  8,  8,  8,
  //      8, 32, 32, 32,  8,  8,  8,  8,
  //     32, 32, 32,  8,  8,  8,  8,  8,
  //     32, 32,  8,  8,  8,  8,  8,  8,
  //     38,  8,  8,  8,  8,  8,  8,  8
  //  };

  // matrix_2. Used in the presmoother section of amDCT.c
  // Smooths areas that are being sharpened more than other areas        MOVE 2 & 3 to 128 ...  128 to 255 are internal system matrices   Each matrix set consists of normal, thick stronger, thin stronger
  // REMOVES FINE LINES
  //static const uint8_t matrix_2[64] = {      // LAST VERSION !!!!!!!!!!
  //      3,  3,  3,  5,  3,  8, 11, 12,
  //      3,  3,  4,  4,  9, 32, 32,  3,
  //      3,  4,  4,  9, 32, 32,  3,  3,
  //      5,  4,  9, 32, 32,  3,  3,  3,
  //      3,  9, 32, 32,  3,  3,  3,  3,
  //        8, 32, 32,  3,  3,  3,  3,  3,
  //     11, 32,  3,  3,  3,  3,  3,  3,
  //     12,  3,  3,  3,  3,  3,  3,  3
  //  };
  //
  ////static const uint8_t matrix_2[64] = {
  ////      2,  2,  2,  2,  3,  4,  6,  7,
  ////      2,  2,  2,  3,  2,  3,  2,  2,
  ////      2,  2,  4,  2,  3,  2,  2,  2,
  ////      2,  3,  2,  3,  2,  2,  2,  2,
  ////      3,  2,  3,  2,  2,  2,  2,  2,
  ////        4,  3,  2,  2,  2,  2,  2,  2,
  ////      6,  2,  2,  2,  2,  2,  2,  2,
  ////      7,  2,  2,  2,  2,  2,  2,  2
  ////  };
  //
  //
  //
  //
  //
  // matrix_3. This is an inverse of marix_2
  // It deblocks edges and corners.
  // REMOVES THICK LINES
  //static const uint8_t matrix_3[64] = {
  //      8,  8,  8,  8,  8,  3,  3,  3,
  //      8,  8,  8,  8,  3,  3,  3,  8,
  //      8,  8,  8,  3,  3,  3,  8,  8,
  //      8,  8,  3,  3,  3,  8,  8,  8,
  //      8,  3,  3,  3,  8,  8,  8,  8,
  //      3,  3,  3,  8,  8,  8,  8,  8,
  //      3,  3,  8,  8,  8,  8,  8,  8,
  //      3,  8,  8,  8,  8,  8,  8,  8
  //  };





  //  matrix 1
  static const uint8_t matrix_1[64] = {
        6,  7,  8, 10, 13, 13, 14, 14,
        7,  9, 11, 13, 14, 16, 14, 14,
        8, 11, 13, 16, 16, 14, 14, 14,
       10, 13, 16, 17, 14, 14, 14, 14,
       13, 14, 16, 14, 14, 14, 14, 14,
       13, 15, 14, 14, 14, 14, 14, 14,
       14, 14, 14, 14, 14, 14, 14, 14,
       14, 14, 14, 14, 14, 14, 14, 14
    };

  // matrix_num 2
  static const uint8_t matrix_2[64] = {
        6,  7,  8, 10, 10, 26, 30, 39,
        7,  9, 11, 13, 15, 16, 18, 20,
        8, 11, 13, 16, 17, 19, 21, 22,
       10, 13, 16, 18, 20, 22, 24, 26,
       10, 15, 17, 20, 23, 25, 28, 30,
       26, 16, 19, 22, 25, 29, 34, 38,
       30, 18, 21, 24, 28, 34, 46, 52,
       39, 20, 22, 26, 30, 38, 52, 72
    };

  //// matrix_num 2
  //static const uint8_t matrix_8[64] = {
  //      8,  8,  8, 10, 10, 26, 30, 39,
  //      8,  9, 11, 13, 15, 16, 18, 20,
  //      8, 11, 13, 16, 17, 19, 21, 22,
  //     10, 13, 16, 18, 20, 22, 24, 26,
  //     10, 15, 17, 20, 23, 25, 28, 30,
  //     26, 16, 19, 22, 25, 29, 34, 38,
  //     30, 18, 21, 24, 28, 34, 46, 52,
  //     39, 20, 22, 26, 30, 38, 52, 72
  //  };


    // matrix 3
  static const uint8_t matrix_3[64] = {
         8,  8,  8, 12,  8, 18, 19, 12,
         8,  8,  9, 14, 28, 48, 78, 96,
         8,  9, 24, 25, 40, 64, 80, 96,
        12, 14, 25, 40, 58, 76, 96, 96,
         8, 28, 40, 58, 80, 98, 96, 96,
        18, 48, 64, 76, 98, 96, 96, 96,
        19, 78, 80, 96, 96, 96, 96, 96,
        12, 96, 96, 96, 96, 96, 96, 96
    };




  //  matrix 4
  static const uint8_t matrix_4[64] = {
         8,  8,  8, 12,  9, 22, 30, 30,
         8,  8,  9, 14, 28, 48, 78, 96,
         8,  9, 24, 25, 40, 64, 80, 96,
        12, 14, 25, 40, 58, 76, 96, 96,
         9, 28, 40, 58, 80, 98, 96, 96,
        22, 48, 64, 76, 98, 96, 96, 96,
        30, 78, 80, 96, 96, 96, 96, 96,
        30, 96, 96, 96, 96, 96, 96, 96
    };

  ////  matrix 4
  //static const uint8_t matrix_4[64] = {
  //       8,  8,  8, 12,  9, 22, 30, 38,
  //       8,  8,  9, 14, 28, 48, 78, 96,
  //       8,  9, 24, 25, 40, 64, 80, 96,
  //      12, 14, 25, 40, 58, 76, 96, 96,
  //       9, 28, 40, 58, 80, 98, 96, 96,
  //      22, 48, 64, 76, 98, 96, 96, 96,
  //      30, 78, 80, 96, 96, 96, 96, 96,
  //      38, 96, 96, 96, 96, 96, 96, 96
  //  };


  ////  NEW 9 matrix_num 7 Strong horizontal and Vertical line smoothing.
  //// Stronger horizontal single line smoothing then 9
  //static const uint8_t matrix_4[64] = {
  //       8,   8,   8,  12,   9,  24,  64, 44,
  //       8,   8,   9,  14,  28,  48,  78, 96,
  //       8,   9,  24,  25,  40,  64,  80, 96,
  //      12,  14,  25,  40,  58,  76,  96, 96,
  //       9,  28,  40,  58,  80,  98,  96, 96,
  //      24,  48,  64,  76,  98,  96,  96, 96,
  //      64,  78,  80,  96,  96,  96,  96, 96,
  //      44,  96,  96,  96,  96,  96,  96, 96
  //  };

  */


  static const uint8_t matrix_1[64] = {
        5,  5,  5,  6,  7,  8,  9, 10,  // even
        5,  6,  6,  7,  8,  9, 10, 10,
        5,  6,  7,  8,  9, 10, 10, 10,
        6,  7,  8,  9, 10, 10, 10, 10,
        7,  8,  9, 10, 10, 10, 10, 11,
        8,  9, 10, 10, 10, 10, 11, 11,
        9, 10, 10, 10, 10, 11, 11, 11,
       10, 10, 10, 10, 11, 11, 11, 11
  };




  static const uint8_t matrix_2[64] = {
        5,  5,  5,  8,  9, 12, 14, 16,  // vert
        5,  6,  6,  7,  8,  9, 10, 10,
        5,  6,  7,  8,  9, 10, 10, 10,
        6,  7,  8,  9, 10, 10, 10, 10,
        7,  8,  9, 10, 10, 10, 10, 11,
        8,  9, 10, 10, 10, 10, 11, 11,
        9, 10, 10, 10, 10, 11, 11, 11,
       10, 10, 10, 10, 11, 11, 11, 11
  };



  static const uint8_t matrix_3[64] = {
        5,  5,  5,  6,  7,  8,  9, 10,  // horiz
        5,  6,  6,  7,  8,  9, 10, 10,
        5,  6,  7,  8,  9, 10, 10, 10,
        8,  7,  8,  9, 10, 10, 10, 10,
        9,  8,  9, 10, 10, 10, 10, 11,
       12,  9, 10, 10, 10, 10, 11, 11,
       14, 10, 10, 10, 10, 11, 11, 11,
       16, 10, 10, 10, 11, 11, 11, 11
  };




  static const uint8_t matrix_4[64] = {
        5,  5,  5,  8,  9, 12, 14, 16,  // vert & horiz
        5,  6,  6,  7,  8,  9, 10, 10,
        5,  6,  7,  8,  9, 10, 10, 10,
        8,  7,  8,  9, 10, 10, 10, 10,
        9,  8,  9, 10, 10, 10, 10, 11,
       12,  9, 10, 10, 10, 10, 11, 11,
       14, 10, 10, 10, 10, 11, 11, 11,
       16, 10, 10, 10, 11, 11, 11, 11
  };





  static const uint8_t matrix_5[64] = {
        6,  7,  8,  9, 10, 11, 12, 13,  // even add 3 to each value of case 20
        7,  9,  9, 10, 11, 12, 13, 13,
        8,  9, 10, 11, 12, 13, 13, 13,
        9, 10, 11, 12, 13, 13, 13, 13,
       10, 11, 12, 13, 13, 13, 13, 14,
       11, 12, 13, 13, 13, 13, 14, 14,
       12, 13, 13, 13, 13, 14, 14, 14,
       13, 13, 13, 13, 14, 14, 14, 14
  };


  static const uint8_t matrix_6[64] = {
        6,  7,  8, 12, 12, 15, 15, 16,  // vert
        7,  9,  9, 10, 11, 12, 13, 13,
        8,  9, 10, 11, 12, 13, 13, 13,
        9, 10, 11, 12, 13, 13, 13, 13,
       10, 11, 12, 13, 13, 13, 13, 14,
       11, 12, 13, 13, 13, 13, 14, 14,
       12, 13, 13, 13, 13, 14, 14, 14,
       13, 13, 13, 13, 14, 14, 14, 14
  };



  static const uint8_t matrix_7[64] = {
        6,  7,  8,  9, 10, 11, 12, 13,  // horiz
        7,  9,  9, 10, 11, 12, 13, 13,
        8,  9, 10, 11, 12, 13, 13, 13,
       12, 10, 11, 12, 13, 13, 13, 13,
       12, 11, 12, 13, 13, 13, 13, 14,
       15, 12, 13, 13, 13, 13, 14, 14,
       15, 13, 13, 13, 13, 14, 14, 14,
       16, 13, 13, 13, 14, 14, 14, 14
  };

  static const uint8_t matrix_8[64] = {
        7,  7,  8, 12, 12, 15, 15, 16,  // vert & horiz
        7,  9,  9, 10, 11, 12, 13, 13,
        8,  9, 10, 11, 12, 13, 13, 13,
       12, 10, 11, 12, 13, 13, 13, 13,
       12, 11, 12, 13, 13, 13, 13, 14,
       15, 12, 13, 13, 13, 13, 14, 14,
       15, 13, 13, 13, 13, 14, 14, 14,
       16, 13, 13, 13, 14, 14, 14, 14
  };




  //static const uint8_t matrix_9[64] = {
  //      7,  7,  8, 12, 12, 15, 15, 16,  // vert & horiz
  //      7,  9,  9, 10, 11, 12, 13, 16,
  //      8,  9, 10, 11, 12, 13, 13, 16,
  //     12, 10, 11, 12, 13, 13, 16, 16,
  //     12, 11, 12, 13, 13, 16, 16, 16,
  //     15, 12, 13, 13, 16, 16, 16, 16,
  //     15, 13, 13, 16, 16, 16, 16, 16,
  //     16, 16, 16, 16, 16, 16, 16, 16
  //  };

  // smoothD2  matrix_num 9
  //static const uint8_t my_intra_only_1_by_255[64] = {
  //       1,   1,   2,   8,  16,  32,  64, 128,
  //       1,   3,   9,  14,  24,  48,  96, 128,
  //       2,   9,  12,  16,  32,  64,  80, 128,
  //       8,  14,  16,  18,  48,  72,  92, 128,
  //      16,  24,  32,  48,  80,  98, 100, 128,
  //      32,  48,  64,  72,  98, 100, 140, 255,
  //      64,  96,  80,  92, 100, 140, 255, 255,
  //     128, 128, 128, 128, 128, 255, 255, 255
  //  };


  static const uint8_t matrix_9[64] = {
        6,  7,  8, 10, 13, 14, 16, 18,
        7,  9, 11, 13, 15, 16, 18, 20,
        8, 11, 13, 16, 17, 19, 21, 22,
       10, 13, 16, 18, 20, 22, 24, 26,
       13, 15, 17, 20, 23, 25, 28, 30,
       14, 16, 19, 22, 25, 29, 34, 38,
       16, 18, 21, 24, 28, 34, 46, 52,
       18, 20, 22, 26, 30, 38, 52, 72
  };






  //// matrix_243. It is good for removing noise before sharpening.
  static const uint8_t matrix_243[64] = {
        4,  4,  5,  6,  7,  8,  9, 10,
        4,  6,  6,  7,  8,  9, 10, 10,
        5,  6,  7,  8,  9, 10, 10, 10,
        6,  7,  8,  9, 10, 10, 10, 10,
        7,  8,  9, 10, 10, 10, 10, 11,
        8,  9, 10, 10, 10, 10, 11, 11,
        9, 10, 10, 10, 10, 11, 11, 11,
       10, 10, 10, 10, 11, 11, 11, 11
  };


  //// matrix_244.
  static const uint8_t matrix_244[64] = {
        4,  4,  5,  7,  8, 10, 11, 12,
        4,  6,  6,  7,  8,  9, 10, 10,
        5,  6,  7,  8,  9, 10, 10, 10,
        7,  7,  8, 10, 10, 10, 10, 10,
        8,  8,  9, 10, 10, 10, 10, 11,
       10,  9, 10, 10, 10, 10, 11, 11,
       11, 10, 10, 10, 10, 11, 11, 11,
       12, 10, 10, 10, 11, 11, 11, 11
  };

  ///// matrix_245.
  static const uint8_t matrix_245[64] = {
        4,  4,  5,  8, 10, 10, 11, 12,
        4,  7,  6,  7,  8,  9, 10, 10,
        5,  6,  9,  8,  9, 10, 10, 10,
        8,  7,  8, 10, 10, 10, 10, 10,
       10,  8,  9, 10, 10, 10, 10, 11,
       10,  9, 10, 10, 10, 10, 11, 11,
       11, 10, 10, 10, 10, 11, 11, 11,
       12, 10, 10, 10, 11, 11, 11, 11
  };








  //// matrix_num 5   medium smoothing.  More smoothing on large details "low frequencies"
  //static const uint8_t matrix_2[64] = {
  //      5,  6,  7,  9, 13, 20, 33, 49,
  //      6,  6,  8, 13, 15, 16, 18, 20,
  //      7,  8, 13, 16, 17, 19, 21, 22,
  //      9, 13, 16, 18, 20, 22, 24, 26,
  //     13, 15, 17, 20, 23, 25, 28, 30,
  //     20, 16, 19, 22, 25, 29, 34, 38,
  //     33, 18, 21, 24, 28, 34, 46, 52,
  //     49, 20, 22, 26, 30, 38, 52, 72
  //  };

  //  matrix 4  does less smoothing of fine lines that matrix 5.
  //static const uint8_t matrix_4[64] = {
  //      6,  7,  8, 10,  7, 14, 15, 13,
  //      7,  9, 11, 13, 14, 16, 14, 14,
  //      8, 11, 13, 16, 16, 14, 14, 14,
  //     10, 13, 16, 17, 14, 14, 14, 14,
  //      7, 14, 16, 14, 14, 14, 14, 14,
  //     14, 15, 14, 14, 14, 14, 14, 14,
  //     15, 14, 14, 14, 14, 14, 14, 14,
  //     13, 14, 14, 14, 14, 14, 14, 14
  //  };


  ////  matrix 4  does less smoothing of fine lines that matrix 5.
  //static const uint8_t matrix_4[64] = {
  //      6,  7,  8, 10,  7, 14, 15, 13,
  //      7,  9, 11, 13, 14, 16, 14,  6,
  //      8, 11, 13, 16, 16, 14,  6,  6,
  //     10, 13, 16, 17, 14,  6,  6,  6,
  //      7, 14, 16, 14,  6,  6,  6,  6,
  //     14, 15, 14,  6,  6,  6,  6,  6,
  //     15, 14,  6,  6,  6,  6,  6,  6,
  //     13,  6,  6,  6,  6,  6,  6,  6
  //  };



  /*
  //// matrix_num 5   medium smoothing.  More smoothing on large details "low frequencies"
  static const uint8_t matrix_5[64] = {
        6,  7,  8, 10, 13, 14, 16, 18,
        7,  9, 11, 13, 15, 16, 18, 20,
        8, 11, 13, 16, 17, 19, 21, 22,
       10, 13, 16, 18, 20, 22, 24, 26,
       13, 15, 17, 20, 23, 25, 28, 30,
       14, 16, 19, 22, 25, 29, 34, 38,
       16, 18, 21, 24, 28, 34, 46, 52,
       18, 20, 22, 26, 30, 38, 52, 72
    };

  //       8,   8,   8,   8,  16,  32,  64, 90,

  //// matrix_num 5   medium smoothing.  More smoothing on large details "low frequencies"
  //static const uint8_t matrix_2[64] = {
  //      5,  6,  7,  9, 13, 20, 33, 49,
  //      6,  6,  8, 13, 15, 16, 18, 20,
  //      7,  8, 13, 16, 17, 19, 21, 22,
  //      9, 13, 16, 18, 20, 22, 24, 26,
  //     13, 15, 17, 20, 23, 25, 28, 30,
  //     20, 16, 19, 22, 25, 29, 34, 38,
  //     33, 18, 21, 24, 28, 34, 46, 52,
  //     49, 20, 22, 26, 30, 38, 52, 72
  //  };



  //  matrix_num 6 strong smoothing while having less impact on horizontal and vertical lines.
  static const uint8_t matrix_6[64] = {
         8,  8,  8,  8, 11, 22, 30, 40,
         8,  8,  9, 14, 27, 46, 78, 96,
         8,  9, 24, 25, 40, 62, 80, 96,
         8, 14, 25, 40, 58, 76, 96, 96,
        11, 27, 40, 58, 80, 98, 96, 96,
        22, 46, 62, 76, 98, 96, 96, 96,
        30, 78, 80, 96, 96, 96, 96, 96,
        40, 96, 96, 96, 96, 96, 96, 96
    };


  ////  matrix_num 7  BECAME 9 Strong horizontal and Vertical line smoothing.
  //// Stronger horizontal single line smoothing then 9
  static const uint8_t matrix_8[64] = {
         8,   8,   8,   8,  16,  32,  64, 90,
         8,   8,   9,  14,  28,  48,  78, 96,
         8,   9,  24,  25,  40,  64,  80, 96,
         8,  14,  25,  40,  58,  76,  96, 96,
        16,  27,  40,  58,  80,  98,  96, 96,
        32,  46,  62,  76,  98,  96,  96, 96,
        64,  78,  80,  96,  96,  96,  96, 96,
        90,  96,  96,  96,  96,  96,  96, 96
    };


  // matrix_num WAS 9 Same as 8 but slightly stronger horizontal and vertical line smoothing     NEW 7
  static const uint8_t matrix_7[64] = {
         8,  14,  15,  16,  17,  18,  24, 32,
        11,   8,   9,  14,  28,  48,  78, 96,
        12,   9,  24,  25,  40,  64,  80, 96,
        12,  14,  25,  40,  58,  76,  96, 96,
        12,  28,  40,  58,  80,  98,  96, 96,
        18,  48,  64,  76,  98,  96,  96, 96,
        24,  78,  80,  96,  96,  96,  96, 96,
        32,  96,  96,  96,  96,  96,  96, 96
    };
  */




  // matrix_num 8    Inverted matrix.  WORKS REALLY WELL IN REDUCING 1080P LOW BITRATE PROBLEMS.
  //static const uint8_t matrix_8[64] = {
  //       8,   1, 64, 64, 64, 64, 64, 64,
  //     1,  64,  1,  1,  1,  1,  1,  1,
  //      64,   1,  1,  1,  1,  1,  1,  1,
  //    64,   1,  1,  1,  1,  1,  1,  1,
  //    64,   1,  1,  1,  1,  1,  1,  1,
  //    64,   1,  1,  1,  1,  1,  1,  1,
  //    64,   1,  1,  1,  1,  1,  1,  1,
  //    64,   1,  1,  1,  1,  1,  1,  1
  //  };




  //// matrix_num 8    Inverted matrix.  WORKS REALLY WELL IN REDUCING 1080P LOW BITRATE PROBLEMS.
  //static const uint8_t matrix_8[64] = {
  //       8,   8, 99, 99, 99, 99, 99, 99,
  //     8,  99,  8,  8,  8,  8,  8,  8,
  //      99,   8,  8,  8,  8,  8,  8,  8,
  //    99,   8,  8,  8,  8,  8,  8,  8,
  //    99,   8,  8,  8,  8,  8,  8,  8,
  //    99,   8,  8,  8,  8,  8,  8,  8,
  //    99,   8,  8,  8,  8,  8,  8,  8,
  //    99,   8,  8,  8,  8,  8,  8,  8
  //  };

  // matrix_num 9     WORKS REALLY WELL IN REDUCING 1080P LOW BITRATE PROBLEMS.
  static const uint8_t matrix_19[64] = {
       8,   8, 24, 24, 24, 24, 24, 24,
       8,  24,  8,  8,  8,  8,  8,  8,
      24,   8,  8,  8,  8,  8,  8,  8,
      24,   8,  8,  8,  8,  8,  8,  8,
      24,   8,  8,  8,  8,  8,  8,  8,
      24,   8,  8,  8,  8,  8,  8,  8,
      24,   8,  8,  8,  8,  8,  8,  8,
      24,   8,  8,  8,  8,  8,  8,  8
  };




  // matrix_num 8 Strong horizontal and vertical line smoothing   // BEST 8 SO FAR Also better for vertical lines
  //static const uint8_t matrix_3[64] = {
  //       8,   8,   8,  12,   8,  18,  19, 12,
  //       8,   8,   9,  14,  28,  48,  78, 96,
  //       8,   9,  24,  25,  40,  64,  80, 96,
  //      12,  14,  25,  40,  58,  76,  96, 96,
  //       8,  28,  40,  58,  80,  98,  96, 96,
  //      18,  48,  64,  76,  98,  96,  96, 96,
  //      19,  78,  80,  96,  96,  96,  96, 96,
  //      12,  96,  96,  96,  96,  96,  96, 96
  //  };




  ////  NEW 9 matrix_num 7 Strong horizontal and Vertical line smoothing.
  //// Stronger horizontal single line smoothing then 9
  //static const uint8_t matrix_4[64] = {
  //       8,   8,   8,  12,   9,  24,  64, 44,
  //       8,   8,   9,  14,  28,  48,  78, 96,
  //       8,   9,  24,  25,  40,  64,  80, 96,
  //      12,  14,  25,  40,  58,  76,  96, 96,
  //       9,  28,  40,  58,  80,  98,  96, 96,
  //      24,  48,  64,  76,  98,  96,  96, 96,
  //      64,  78,  80,  96,  96,  96,  96, 96,
  //      44,  96,  96,  96,  96,  96,  96, 96
  //  };

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // NOTE!!!   matrix_166  WORKS REALLY WELL IN REDUCING 1080P LOW BITRATE PROBLEMS!!!!!!!!!!!!!!!!
  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  //// matrix_num 9 Same as 8 but slightly stronger horizontal and vertical line smoothing
  //static const uint8_t matrix_7[64] = {
  //       8,  14,  15,  16,  17,  18,  24, 32,
  //      11,   8,   9,  14,  28,  48,  78, 96,
  //      12,   9,  24,  25,  40,  64,  80, 96,
  //      12,  14,  25,  40,  58,  76,  96, 96,
  //      12,  28,  40,  58,  80,  98,  96, 96,
  //      18,  48,  64,  76,  98,  96,  96, 96,
  //      24,  78,  80,  96,  96,  96,  96, 96,
  //      32,  96,  96,  96,  96,  96,  96, 96
  //  };
  //
























  /*
  //152  call as #12
  static const uint8_t Flat_16[64] = {
       8,  128, 64, 32, 16,  8,  4,  2,
       128, 64, 32, 16,  8,  4,  2,  8,
       64,  32, 16,  8,  4,  2,  8,  8,
       32,  16,  8,  4,  2,  8,  8,  8,
       16,   8,  4,  2,  8,  8,  8,  8,
       8,    4,  2,  8,  8,  8,  8,  8,
       4,    2,  8,  8,  8,  8,  8,  8,
       2,    8,  8,  8,  8,  8,  8,  8
       };
  */

  //152  call as #13
  static const uint8_t Flat_24[64] = {
       2,  2,  6,  6,  2,  2,  2,  2,
       2,  2,  2,  2,  2,  2,  2,  2,
       6,  2,  92,  2,  2,  2,  2,  2,
       6,  2,  2,255,  2,  2,  2,  2,
       2,  2,  2,  2,  2,  2,  2,  2,
       2,  2,  2,  2,  2,  2,  2,  2,
       2,  2,  2,  2,  2,  2,  2,  2,
       2,  2,  2,  2,  2,  2,  2,  2
  };

  ////152  call as #13
  //static const uint8_t Flat_24[64] = {
  //     8,    8,  8,  8,  8,  8,  4,  2,
  //     8,    8,  8,  8,  8,  4,  2,  2,
  //     8,    8,  8,  8,  2,  2,  2,  2,
  //     8,    8,  8,  2,  2,  2,  2,  8,
  //     8,    8,  2,  2,  2,  2,  8,  8,
  //     8,    4,  2,  2,  2,  8,  8,  8,
  //     4,    2,  2,  2,  8,  8,  8,  8,
  //     2,    2,  2,  8,  8,  8,  8,  8
  //     };


  // last best
  //// matrix_num 9 Strong horizontal and vertical line smoothing   // BEST 8 SO FAR Also better for vertical lines
  //static const uint8_t my_intra_only_1_by_255[64] = {
  //       8,   9,  11,  12,  12,  18,  24, 32,
  //       9,   8,   9,  14,  28,  48,  78, 96,
  //      11,   9,  24,  25,  40,  64,  80, 96,
  //      12,  14,  25,  40,  58,  76,  96, 96,
  //      12,  28,  40,  58,  80,  98,  96, 96,
  //      18,  48,  64,  76,  98,  96,  96, 96,
  //      24,  78,  80,  96,  96,  96,  96, 96,
  //      32,  96,  96,  96,  96,  96,  96, 96
  //  };




























  // matrix_num 9 Very strong edge smoothing OLD
  static const uint8_t my_intra_only_1_by_255_OLD[64] = {
         2,   2,   2,   8,  16,  32,  64, 128,
         2,   2,   9,  14,  28,  48,  96, 128,
         2,   9,  14,  25,  40,  64,  80, 128,
         8,  14,  25,  40,  58,  76,  92, 128,
        16,  28,  40,  58,  80,  98, 100, 128,
        32,  48,  64,  76,  98, 100, 140, 255,
        64,  96,  80,  92, 100, 140, 255, 255,
       128, 128, 128, 128, 128, 255, 255, 255
  };


  /*
  // matrix_num 9 Very strong edge smoothing
  static const uint8_t my_intra_only_1_by_255[64] = {
         2,   2,   2,  11,   4,  12,  11,  12,
         2,   2,   9,  14,  28,  48,  96, 128,
         2,   9,  14,  25,  40,  64,  80, 128,
        11,  14,  25,  40,  58,  76,  92, 128,
         4,  28,  40,  58,  80,  98, 100, 128,
        12,  48,  64,  76,  98, 100, 140, 255,
        11,  96,  80,  92, 100, 140, 255, 255,
        12, 128, 128, 128, 128, 255, 255, 255
    };
  */








  // matrix numbers 10-29 are "Flat_" containing values of either 3, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 208, 224, 240
  // 10
  static const uint8_t Flat_2[64] = {
       8,  2,  2,  2,  2,  2,  2,  3,
       2,  2,  2,  2,  3,  3,  3,  2,
       2,  2,  2,  2,  3,  3,  2,  2,
       2,  2,  2,  2,  3,  2,  2,  2,
       2,  3,  3,  3,  2,  2,  2,  2,
       2,  3,  3,  2,  2,  2,  2,  2,
       2,  3,  2,  2,  2,  2,  2,  2,
       3,  2,  2,  2,  2,  2,  2,  2
  };

  static const uint8_t Flat_3[64] = {
       3,  3,  3,  3,  3,  3,  3,  3,
       3,  3,  3,  3,  3,  3,  3,  3,
       3,  3,  3,  3,  3,  3,  3,  3,
       3,  3,  3,  3,  3,  3,  3,  3,
       3,  3,  3,  3,  3,  3,  3,  3,
       3,  3,  3,  3,  3,  3,  3,  3,
       3,  3,  3,  3,  3,  3,  3,  3,
       3,  3,  3,  3,  3,  3,  3,  3
  };


  ////11
  //static const uint8_t Flat_16[64] = {
  //     8,  8,  8,  8,  8,  8,  8,  8,
  //     8,  8,  8,  8,  8,  8,  8,  8,
  //     8,  8,  8,  8,  8,  8,  8,  8,
  //     8,  8,  8,  8,  8,  8,  8,  8,
  //     8,  8,  8,  8,  8,  8,  8,  8,
  //     8,  8,  8,  8,  8,  8,  8,  8,
  //     8,  8,  8,  8,  8,  8,  8,  8,
  //     8,  8,  8,  8,  8,  8,  8,  8
  //  };

  //12
  //static const uint8_t Flat_16[64] = {
  //     5,  5,  5,  5,  5,  5,  5,  5,
  //     5,  5,  5,  5,  5,  5,  5,  5,
  //     5,  5,  5,  5,  5,  5,  5,  5,
  //     5,  5,  5,  5,  5,  5,  5,  5,
  //     5,  5,  5,  5,  5,  5,  5,  5,
  //     5,  5,  5,  5,  5,  5,  5,  5,
  //     5,  5,  5,  5,  5,  5,  5,  5,
  //     5,  5,  5,  5,  5,  5,  5,  5
  //  };

  //12
  //static const uint8_t Flat_16[64] = {
  //     16,  16,  16,  16,  16,  16,  16,  16,
  //     16,  16,  16,  16,  16,  16,  16,  16,
  //     16,  16,  16,  16,  16,  16,  16,  16,
  //     16,  16,  16,  16,  16,  16,  16,  16,
  //     16,  16,  16,  16,  16,  16,  16,  16,
  //     16,  16,  16,  16,  16,  16,  16,  16,
  //     16,  16,  16,  16,  16,  16,  16,  16,
  //     16,  16,  16,  16,  16,  16,  16,  16
  //  };


  ////13
  static const uint8_t Flat_16[64] = {
       24,  24,  24,  24,  24,  24,  24,  24,
       24,  24,  24,  24,  24,  24,  24,  24,
       24,  24,  24,  24,  24,  24,  24,  24,
       24,  24,  24,  24,  24,  24,  24,  24,
       24,  24,  24,  24,  24,  24,  24,  24,
       24,  24,  24,  24,  24,  24,  24,  24,
       24,  24,  24,  24,  24,  24,  24,  24,
       24,  24,  24,  24,  24,  24,  24,  24
  };
  //

  //14
  static const uint8_t Flat_32[64] = {
       32,  32,  32,  32,  32,  32,  32,  32,
       32,  32,  32,  32,  32,  32,  32,  32,
       32,  32,  32,  32,  32,  32,  32,  32,
       32,  32,  32,  32,  32,  32,  32,  32,
       32,  32,  32,  32,  32,  32,  32,  32,
       32,  32,  32,  32,  32,  32,  32,  32,
       32,  32,  32,  32,  32,  32,  32,  32,
       32,  32,  32,  32,  32,  32,  32,  32
  };



  //15
  static const uint8_t Flat_40[64] = {
       40,  40,  40,  40,  40,  40,  40,  40,
       40,  40,  40,  40,  40,  40,  40,  40,
       40,  40,  40,  40,  40,  40,  40,  40,
       40,  40,  40,  40,  40,  40,  40,  40,
       40,  40,  40,  40,  40,  40,  40,  40,
       40,  40,  40,  40,  40,  40,  40,  40,
       40,  40,  40,  40,  40,  40,  40,  40,
       40,  40,  40,  40,  40,  40,  40,  40
  };



  //16
  static const uint8_t Flat_48[64] = {
       48,  48,  48,  48,  48,  48,  48,  48,
       48,  48,  48,  48,  48,  48,  48,  48,
       48,  48,  48,  48,  48,  48,  48,  48,
       48,  48,  48,  48,  48,  48,  48,  48,
       48,  48,  48,  48,  48,  48,  48,  48,
       48,  48,  48,  48,  48,  48,  48,  48,
       48,  48,  48,  48,  48,  48,  48,  48,
       48,  48,  48,  48,  48,  48,  48,  48
  };

  //17
  static const uint8_t Flat_56[64] = {
       56,  56,  56,  56,  56,  56,  56,  56,
       56,  56,  56,  56,  56,  56,  56,  56,
       56,  56,  56,  56,  56,  56,  56,  56,
       56,  56,  56,  56,  56,  56,  56,  56,
       56,  56,  56,  56,  56,  56,  56,  56,
       56,  56,  56,  56,  56,  56,  56,  56,
       56,  56,  56,  56,  56,  56,  56,  56,
       56,  56,  56,  56,  56,  56,  56,  56
  };


  //18
  //static const uint8_t Flat_64[64] = {
  //static const uint8_t Flat_80[64] = {
  //     64,  64,  64,  64,  64,  64,  64,  64,
  //     64,  64,  64,  64,  64,  64,  64,  64,
  //     64,  64,  64,  64,  64,  64,  64,  64,
  //     64,  64,  64,  64,  64,  64,  64,  64,
  //     64,  64,  64,  64,  64,  64,  64,  64,
  //     64,  64,  64,  64,  64,  64,  64,  64,
  //     64,  64,  64,  64,  64,  64,  64,  64,
  //     64,  64,  64,  64,  64,  64,  64,  64
  //  };

  //18
  static const uint8_t Flat_80[64] = {
       80,  80,  80,  80,  80,  80,  80,  80,
       80,  80,  80,  80,  80,  80,  80,  80,
       80,  80,  80,  80,  80,  80,  80,  80,
       80,  80,  80,  80,  80,  80,  80,  80,
       80,  80,  80,  80,  80,  80,  80,  80,
       80,  80,  80,  80,  80,  80,  80,  80,
       80,  80,  80,  80,  80,  80,  80,  80,
       80,  80,  80,  80,  80,  80,  80,  80
  };



  //  case 20:  use_matrix = Flat_96;
  //  case 21:  use_matrix = Flat_112;
  //  case 22:  use_matrix = Flat_128;
  //  case 23:  use_matrix = Flat_144;
  //  case 24:  use_matrix = Flat_160;
  //  case 25:  use_matrix = Flat_176;
  //  case 26:  use_matrix = Flat_192;
  //  case 27:  use_matrix = Flat_208;
  //  case 28:  use_matrix = Flat_224;
  //  case 29:  use_matrix = Flat_240;

  //20  TEST NEW MATRICIES
  //static const uint8_t Flat_96[64] = {
  static const uint8_t Flat_96[64] = {      // even
        4,  4,  5,  6,  7,  8,  9, 10,
        4,  6,  6,  7,  8,  9, 10, 10,
        5,  6,  7,  8,  9, 10, 10, 10,
        6,  7,  8,  9, 10, 10, 10, 10,
        7,  8,  9, 10, 10, 10, 10, 11,
        8,  9, 10, 10, 10, 10, 11, 11,
        9, 10, 10, 10, 10, 11, 11, 11,
       10, 10, 10, 10, 11, 11, 11, 11
  };



  //21
  //static const uint8_t Flat_112[64] = {
  static const uint8_t Flat_112[64] = {
        4,  4,  5,  8,  9, 12, 14, 16,  // vert
        4,  6,  6,  7,  8,  9, 10, 10,
        5,  6,  7,  8,  9, 10, 10, 10,
        6,  7,  8,  9, 10, 10, 10, 10,
        7,  8,  9, 10, 10, 10, 10, 11,
        8,  9, 10, 10, 10, 10, 11, 11,
        9, 10, 10, 10, 10, 11, 11, 11,
       10, 10, 10, 10, 11, 11, 11, 11
  };


  //22
  //static const uint8_t Flat_128[64] = {
  static const uint8_t Flat_128[64] = {
        4,  4,  5,  6,  7,  8,  9, 10,  // horiz
        4,  6,  6,  7,  8,  9, 10, 10,
        5,  6,  7,  8,  9, 10, 10, 10,
        8,  7,  8,  9, 10, 10, 10, 10,
        9,  8,  9, 10, 10, 10, 10, 11,
       12,  9, 10, 10, 10, 10, 11, 11,
       14, 10, 10, 10, 10, 11, 11, 11,
       16, 10, 10, 10, 11, 11, 11, 11
  };




  //23
  //static const uint8_t Flat_144[64] = {
  static const uint8_t Flat_144[64] = {
        4,  4,  5,  8,  9, 12, 14, 16,  // vert & horiz
        4,  6,  6,  7,  8,  9, 10, 10,
        5,  6,  7,  8,  9, 10, 10, 10,
        8,  7,  8,  9, 10, 10, 10, 10,
        9,  8,  9, 10, 10, 10, 10, 11,
       12,  9, 10, 10, 10, 10, 11, 11,
       14, 10, 10, 10, 10, 11, 11, 11,
       16, 10, 10, 10, 11, 11, 11, 11
  };


  ////24
  ////static const uint8_t Flat_160[64] = {
  //static const uint8_t Flat_160[64] = {     // even
  //      5,  6,  7,  9, 13, 20, 33, 49,
  //      6,  6,  8, 13, 15, 16, 18, 20,
  //      7,  8, 13, 16, 17, 19, 21, 22,
  //      9, 13, 16, 18, 20, 22, 24, 26,
  //     13, 15, 17, 20, 23, 25, 28, 30,
  //     20, 16, 19, 22, 25, 29, 34, 38,
  //     33, 18, 21, 24, 28, 34, 46, 52,
  //     49, 20, 22, 26, 30, 38, 52, 72
  //  };

  //24
  //static const uint8_t matrix_3[64] = {
  //static const uint8_t Flat_160[64] = {     // even
  //       8,  8,  8, 12,  8, 18, 19, 12,
  //       8,  8,  9, 14, 28, 48, 78, 96,
  //       8,  9, 24, 25, 40, 64, 80, 96,
  //      12, 14, 25, 40, 58, 76, 96, 96,
  //       8, 28, 40, 58, 80, 98, 96, 96,
  //      18, 48, 64, 76, 98, 96, 96, 96,
  //      19, 78, 80, 96, 96, 96, 96, 96,
  //      12, 96, 96, 96, 96, 96, 96, 96
  //  };

  //static const uint8_t Flat_96[64] = {      // even
  //      4,  4,  5,  6,  7,  8,  9, 10,
  //      4,  6,  6,  7,  8,  9, 10, 10,
  //      5,  6,  7,  8,  9, 10, 10, 10,
  //      6,  7,  8,  9, 10, 10, 10, 10,
  //      7,  8,  9, 10, 10, 10, 10, 11,
  //      8,  9, 10, 10, 10, 10, 11, 11,
  //      9, 10, 10, 10, 10, 11, 11, 11,
  //     10, 10, 10, 10, 11, 11, 11, 11
  //  };
  //static const uint8_t Flat_160[64] = {      // even  add 3 to each value TEST
  //      6,  7,  8,  9, 10, 11, 12, 13,
  //      7,  9,  9, 10, 11, 12, 13, 13,
  //      8,  9, 10, 11, 12, 13, 13, 13,
  //      9, 10, 11, 12, 13, 13, 13, 13,
  //     10, 11, 12, 13, 13, 13, 13, 14,
  //     11, 12, 13, 13, 13, 13, 14, 14,
  //     12, 13, 13, 13, 13, 14, 14, 14,
  //     13, 13, 13, 13, 14, 14, 14, 14
  //  };

  ////  matrix 4 5
  //static const uint8_t matrix_4[64] = {
  //       8,  8,  8, 12,  9, 22, 30, 30,
  //       8,  8,  9, 14, 28, 48, 78, 96,
  //       8,  9, 24, 25, 40, 64, 80, 96,
  //      12, 14, 25, 40, 58, 76, 96, 96,
  //       9, 28, 40, 58, 80, 98, 96, 96,
  //      22, 48, 64, 76, 98, 96, 96, 96,
  //      30, 78, 80, 96, 96, 96, 96, 96,
  //      30, 96, 96, 96, 96, 96, 96, 96
  //  };

  //25
  //static const uint8_t Flat_176[64] = {
  //static const uint8_t Flat_176[64] = {     // vert
  //       8,  9, 10, 12,  8, 18, 19, 12,
  //       9, 14,  9, 14, 28, 48, 78, 96,
  //      10,  9, 24, 25, 40, 64, 80, 96,
  //      12, 14, 25, 40, 58, 76, 96, 96,
  //       8, 28, 40, 58, 80, 98, 96, 96,
  //      18, 48, 64, 76, 98, 96, 96, 96,
  //      19, 78, 80, 96, 96, 96, 96, 96,
  //      12, 96, 96, 96, 96, 96, 96, 96
  //  };

  //static const uint8_t Flat_112[64] = {
  //      4,  4,  5,  8,  9, 12, 14, 16,  // vert
  //      4,  6,  6,  7,  8,  9, 10, 10,
  //      5,  6,  7,  8,  9, 10, 10, 10,
  //      6,  7,  8,  9, 10, 10, 10, 10,
  //      7,  8,  9, 10, 10, 10, 10, 11,
  //      8,  9, 10, 10, 10, 10, 11, 11,
  //      9, 10, 10, 10, 10, 11, 11, 11,
  //     10, 10, 10, 10, 11, 11, 11, 11
  //  };

  //  case 20:  use_matrix = Flat_96;   // even
  //  case 21:  use_matrix = Flat_112;  // vert
  //  case 22:  use_matrix = Flat_128;  // horiz
  //  case 23:  use_matrix = Flat_144;  // vert & horiz

  //  case 24:  use_matrix = Flat_160;  // even
  //  case 25:  use_matrix = Flat_176;  // vert
  //  case 26:  use_matrix = Flat_192;  // horiz
  //  case 27:  use_matrix = Flat_208;  // vert & horiz
  //  case 28:  use_matrix = Flat_224;
  //  case 29:  use_matrix = Flat_240;

  //static const uint8_t Flat_96[64] = {      // even
  //      4,  4,  5,  6,  7,  8,  9, 10,
  //      4,  6,  6,  7,  8,  9, 10, 10,
  //      5,  6,  7,  8,  9, 10, 10, 10,
  //      6,  7,  8,  9, 10, 10, 10, 10,
  //      7,  8,  9, 10, 10, 10, 10, 11,
  //      8,  9, 10, 10, 10, 10, 11, 11,
  //      9, 10, 10, 10, 10, 11, 11, 11,
  //     10, 10, 10, 10, 11, 11, 11, 11
  //  };


  // 24
  static const uint8_t Flat_160[64] = {      // even add 3 to each value of case 20
        6,  7,  8,  9, 10, 11, 12, 13,
        7,  9,  9, 10, 11, 12, 13, 13,
        8,  9, 10, 11, 12, 13, 13, 13,
        9, 10, 11, 12, 13, 13, 13, 13,
       10, 11, 12, 13, 13, 13, 13, 14,
       11, 12, 13, 13, 13, 13, 14, 14,
       12, 13, 13, 13, 13, 14, 14, 14,
       13, 13, 13, 13, 14, 14, 14, 14
  };

  // 25
  static const uint8_t Flat_176[64] = {     // vert
        6,  7,  8, 12, 12, 15, 15, 16,
        7,  9,  9, 10, 11, 12, 13, 13,
        8,  9, 10, 11, 12, 13, 13, 13,
        9, 10, 11, 12, 13, 13, 13, 13,
       10, 11, 12, 13, 13, 13, 13, 14,
       11, 12, 13, 13, 13, 13, 14, 14,
       12, 13, 13, 13, 13, 14, 14, 14,
       13, 13, 13, 13, 14, 14, 14, 14
  };


  // 26
  static const uint8_t Flat_192[64] = {    // horiz
        6,  7,  8,  9, 10, 11, 12, 13,
        7,  9,  9, 10, 11, 12, 13, 13,
        8,  9, 10, 11, 12, 13, 13, 13,
       12, 10, 11, 12, 13, 13, 13, 13,
       12, 11, 12, 13, 13, 13, 13, 14,
       15, 12, 13, 13, 13, 13, 14, 14,
       15, 13, 13, 13, 13, 14, 14, 14,
       16, 13, 13, 13, 14, 14, 14, 14
  };
  // 27
  static const uint8_t Flat_208[64] = {
        6,  7,  8, 12, 12, 15, 15, 16,  // vert & horiz
        7,  9,  9, 10, 11, 12, 13, 13,
        8,  9, 10, 11, 12, 13, 13, 13,
       12, 10, 11, 12, 13, 13, 13, 13,
       12, 11, 12, 13, 13, 13, 13, 14,
       15, 12, 13, 13, 13, 13, 14, 14,
       15, 13, 13, 13, 13, 14, 14, 14,
       16, 13, 13, 13, 14, 14, 14, 14
  };




  //static const uint8_t Flat_208[64] = {
  //     208,  208,  208,  208,  208,  208,  208,  208,
  //     208,  208,  208,  208,  208,  208,  208,  208,
  //     208,  208,  208,  208,  208,  208,  208,  208,
  //     208,  208,  208,  208,  208,  208,  208,  208,
  //     208,  208,  208,  208,  208,  208,  208,  208,
  //     208,  208,  208,  208,  208,  208,  208,  208,
  //     208,  208,  208,  208,  208,  208,  208,  208,
  //     208,  208,  208,  208,  208,  208,  208,  208
  //  };
  //

  //28
  static const uint8_t Flat_224[64] = {
       224,  224,  224,  224,  224,  224,  224,  224,
       224,  224,  224,  224,  224,  224,  224,  224,
       224,  224,  224,  224,  224,  224,  224,  224,
       224,  224,  224,  224,  224,  224,  224,  224,
       224,  224,  224,  224,  224,  224,  224,  224,
       224,  224,  224,  224,  224,  224,  224,  224,
       224,  224,  224,  224,  224,  224,  224,  224,
       224,  224,  224,  224,  224,  224,  224,  224
  };



  //29
  static const uint8_t Flat_240[64] = {
       8,  9,  9,  9,  8,  6,  6,  6,
       9,  9,  9,  8,  6,  6,  6,  6,
       9,  9,  8,  6,  6,  6,  6,  6,
       9,  8,  6,  6,  6,  6,  6,  6,
       8,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6
  };



  //   H_2_64_8_64   Horizontal  2 starting column value 64 to ending column 8 value 64 numbers to end
  //   H_2_64_8_240  Horizontal  2 starting column 64-64 numbers to end
  //   H_2_64        Horizontal  2 column value 64


  // Start Column 2


  //30
  static const uint8_t H_2_64[64] = { // This smooths horizontally so vertical block edges are reduced
  1, 64,  1,  1,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  3
  };















  //31  STRONG SMOOTH WIDE VERTICAL LINES
  static const uint8_t matrix_31[64] = { // This smooths horizontally so vertical block edges are reduced
  1, 128, 128, 1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1
  };



  //32  STRONG SMOOTH MEDIUM VERTICAL LINES
  static const uint8_t matrix_32[64] = { // This smooths horizontally so vertical block edges are reduced
  1,  4, 128, 255, 128, 1, 1,  1,
  1,  1,   1,  1,  1,  1,  1,  1,
  1,  1,   1,  1,  1,  1,  1,  1,
  1,  1,   1,  1,  1,  1,  1,  1,
  1,  1,   1,  1,  1,  1,  1,  1,
  1,  1,   1,  1,  1,  1,  1,  1,
  1,  1,   1,  1,  1,  1,  1,  1,
  1,  1,   1,  1,  1,  1,  1,  1
  };



  // };


//33  STRONG SMOOTH THIN VERTICAL LINES
  static const uint8_t matrix_33[64] = { // This smooths horizontally so vertical block edges are reduced
  1,  4, 16, 125, 255, 255, 255, 255,
  1,  1,  1,   1,  1,    1,   1,   1,
  1,  1,  1,   1,  1,    1,   1,   1,
  1,  1,  1,   1,  1,    1,   1,   1,
  1,  1,  1,   1,  1,    1,   1,   1,
  1,  1,  1,   1,  1,    1,   1,   1,
  1,  1,  1,   1,  1,    1,   1,   1,
  1,  1,  1,   1,  1,    1,   1,   1
  };






  //34  STRONG SMOOTH WIDE HORIZONTAL LINES
  static const uint8_t matrix_34[64] = { // This smooths horizontally so vertical block edges are reduced
    1, 1,  1,  1,  1,  1,  1,  1,
  128, 1,  1,  1,  1,  1,  1,  1,
  128, 1,  1,  1,  1,  1,  1,  1,
    1, 1,  1,  1,  1,  1,  1,  1,
    1, 1,  1,  1,  1,  1,  1,  1,
    1, 1,  1,  1,  1,  1,  1,  1,
    1, 1,  1,  1,  1,  1,  1,  1,
    1, 1,  1,  1,  1,  1,  1,  1
  };



  //35  STRONG SMOOTH MEDIUM HORIZONTAL LINES
  static const uint8_t matrix_35[64] = { // This smooths horizontally so vertical block edges are reduced
    1,  1,  1,  1,  1,  1,  1,  1,
    4,  1,  1,  1,  1,  1,  1,  1,
      128,  1,  1,  1,  1,  1,  1,  1,
  255,  1,  1,  1,  1,  1,  1,  1,
  128,  1,  1,  1,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1
  };





  //36  STRONG SMOOTH THIN HORIZONTAL LINES
  static const uint8_t matrix_36[64] = { // This smooths horizontally so vertical block edges are reduced
    1,  1,  1,  1,  1,  1,  1,  1,
    4,  1,  1,  1,  1,  1,  1,  1,
   16,  1,  1,  1,  1,  1,  1,  1,
  125,  1,  1,  1,  1,  1,  1,  1,
  255,  1,  1,  1,  1,  1,  1,  1,
  255,  1,  1,  1,  1,  1,  1,  1,
  255,  1,  1,  1,  1,  1,  1,  1,
  255,  1,  1,  1,  1,  1,  1,  1
  };






  //37  STRONG SMOOTH WIDE DIAGONAL LINES
  static const uint8_t matrix_37[64] = { // This smooths horizontally so vertical block edges are reduced
    1,   1,   1,   1,   1,  1,  1,  1,
    1, 255,  64,   1,   1,  1,  1,  1,
    1,  64, 255,   1,   1,  1,  1,  1,
    1,   1,   1, 255,   1,  1,  1,  1,
    1,   1,   1,   1,   1,  1,  1,  1,
    1,   1,   1,   1,   1,  1,  1,  1,
    1,   1,   1,   1,   1,  1,  1,  1,
    1,   1,   1,   1,   1,  1,  1,  1
  };



  //38  STRONG SMOOTH MEDIUM DIAGONAL LINES
  static const uint8_t matrix_38[64] = { // This smooths horizontally so vertical block edges are reduced
    1,   1,   1,   1,   1,   1,   1,   1,
    1,  32,  32,   1,   1,   1,   1,   1,
    1,  32, 160,  64,   1,   1,   1,   1,
    1,   1,  64, 255,  32,   1,   1,   1,
    1,   1,   1,  32, 255,   1,   1,   1,
    1,   1,   1,   1,   1,   1,   1,   1,
    1,   1,   1,   1,   1,   1,   1,   1,
    1,   1,   1,   1,   1,   1,   1,   1
  };





  //39  STRONG SMOOTH THIN DIAGONAL LINES
  static const uint8_t matrix_39[64] = { // This smooths horizontally so vertical block edges are reduced
    1,   1,   1,   1,   1,   1,   1,   1,
    1,  32,  16,   1,   1,   1,   1,   1,
      1,  16, 128, 128,   1,   1,   1,   1,
    1,  16, 128, 255, 128, 255,   1,   1,
    1,   1,   1, 128, 255, 255, 255, 255,
    1,   1,   1,   1, 255, 255, 255, 255,
    1,   1,   1,   1, 255, 255, 255, 255,
    1,   1,   1,   1, 255, 255, 255, 255
  };


  // TESTING  ###############################################################
  //40
  static const uint8_t H_5_240[64] = { // This smooths horizontally so vertical block edges are reduced
    1,   1,   1,   1,   1,   1,   1,   1,
    1, 255,  48,  24,   1,   1,   1,   1,
      1,  48, 255, 128,   1,   1,   1,   1,
    1,  24, 128, 255, 128, 255,   1,   1,
    1,   1,   1, 128, 255, 255, 255, 255,
    1,   1,   1, 255, 255, 255, 255, 255,
    1,   1,   1,   1, 255, 255, 255, 255,
    1,   1,   1,   1, 255, 255, 255, 255
  };


  //41
  static const uint8_t H_5_240_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
    1,   1,   1,   1,   1,   1,   1,   1,
    1,  32,  32,  16,   1,   1,   1,   1,
      1,  32, 128, 128,   1,   1, 255,   1,
    1,  16, 128, 192, 128, 255, 255, 255,
    1,   1,   1, 128, 255, 255, 255, 255,
    1,   1,   1, 255, 255, 255, 255, 255,
    1,   1, 255, 255, 255, 255, 255, 255,
    1,   1,   1, 255, 255, 255, 255, 255
  };


  //42
  static const uint8_t H_5_64_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
    1,   1,   1,   1,   1,   1,   1,   1,
    1,  32,  16,   1,   1,   1, 255, 255,
      1,  16,  96, 128,   1, 128, 255, 255,
    1,  16, 128, 255, 128, 255, 255, 255,
    1,   1,   1, 128, 255, 255, 255, 255,
    1,   1, 128, 255, 255, 255, 255, 255,
    1, 255, 255, 255, 255, 255, 255, 255,    // The two 1's at the left provide very fine line smothing
    1, 255, 255, 255, 255, 255, 255, 255
  };




  // Start Column 6
  //43
  static const uint8_t H_6_240[64] = { // This smooths horizontally so vertical block edges are reduced
    1,   1,   1,   1,   1,   1,   1,   1,
    1,  40,  16,   1,   1,   1,   1,   1,
      1,  16, 160, 128,   1,   1, 255, 255,
    1,  16, 128, 255, 128, 255, 255, 255,
    1,   1,   1, 128, 255, 255, 255, 255,
    1,   1,   1, 255, 255, 255, 255, 255,
    1,   1, 255, 255, 255, 255, 255, 255,
    1,   1, 255, 255, 255, 255, 255, 255
  };





  //44
  static const uint8_t H_6_240_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
    1,   1,   1,   1,   1,   1,   1,   1,
    1,  32,  16,   1,   1,   1, 255, 255,
      1,   1,  96, 128,   1, 128, 255, 255,
    1,   1,   1, 255, 128, 255, 255, 255,
    1,   1,   1,   1, 255, 255, 255, 255,
    1,   1,   1,   1,   1, 255, 255, 255,
    1,   1,   1,   1,   1,   1, 255, 255,    // The two 1's at the left provide very fine line smothing
    1,   1,   1,   1,   1,   1,   1, 255
  };




  //45
  static const uint8_t H_6_64_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
    1,   1,   1,   1,   1,   1,   1,   1,
    1, 255,  48,  24,   1,   1,   1,   1,
      1,   1, 255, 128,   1,   1,   1,   1,
    1,   1,   1, 255, 128, 255,   1,   1,
    1,   1,   1,   1, 255, 255, 255, 255,
    1,   1,   1,   1,   1, 255, 255, 255,
    1,   1,   1,   1,   1,   1, 255, 255,
    1,   1,   1,   1,   1,   1,   1, 255

  };




  //46
  static const uint8_t H_7_240[64] = { // This smooths horizontally so vertical block edges are reduced
    1,   1,   1,   1,   1,   1,   1,   1,
    1, 255, 255, 255, 255, 255, 255, 255,
      1,   1, 255, 255, 255, 255, 255, 255,
    1,   1,   1, 255, 255, 255, 255, 255,
    1,   1,   1,   1, 255, 255, 255, 255,
    1,   1,   1,   1,   1, 255, 255, 255,
    1,   1,   1,   1,   1,   1, 255, 255,
    1,   1,   1,   1,   1,   1,   1, 255
  };


  //47
  static const uint8_t H_7_240_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
    1,   1,   1,   1,   1,   1,   1,   1,
    1, 255, 255, 255, 255, 255, 255, 255,
      1, 255, 255, 255, 255, 255, 255, 255,
    1, 255, 255, 255, 255, 255, 255, 255,
    1, 255, 255, 255, 255, 255, 255, 255,
    1, 255, 255, 255, 255, 255, 255, 255,
    1, 255, 255, 255, 255, 255, 255, 255,
    1, 255, 255, 255, 255, 255, 255, 255
  };





  //48
  static const uint8_t H_7_128_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
    1,   1,   1,   1,   1,   1,   1,   1,
    1, 255,   1,   1,   1,   1,   1,   1,
      1, 255, 255,   1,   1,   1,   1,   1,
    1, 255, 255, 255,   1,   1,   1,   1,
    1, 255, 255, 255, 255,   1,   1,   1,
    1, 255, 255, 255, 255, 255,   1,   1,
    1, 255, 255, 255, 255, 255, 255,   1,
    1, 255, 255, 255, 255, 255, 255, 255
  };



  // Start Column 8
  //49
  static const uint8_t H_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
    1,   1,   1,   1,   1,   1,   1,   1,
    1,  32,  16,   1,   1,   1,   1,   1,
      1,  16, 128, 128,   1,   1,   1,   1,
    1,  16, 128, 255, 128, 255, 255, 255,
    1,   1,   1, 128, 255, 255, 255, 255,
    1,   1,   1, 255, 255, 255, 255, 255,
    1,   1, 255, 255, 255, 255, 255, 255,
    1,   1, 255, 255, 255, 255, 255, 255
  };



  //     static const uint8_t matrix_39[64] = { // This smooths horizontally so vertical block edges are reduced
  //       1,    4,  16,  125,  255,  255,  255,  255,
  //       4,   16,  125, 255,  255,  255,  255,  255,
  //      16,  125,  255, 255,  255,  255,  255,  255,
  //     125,  255,  255, 255,  255,  255,  255,  255,
  //     255,  255,  255, 255,  255,  255,  255,  255,
  //     255,  255,  255, 255,  255,  255,  255,  255,
  //     255,  255,  255, 255,  255,  255,  255,  255,
  //     255,  255,  255, 255,  255,  255,  255,  255
  //     };



  /*
  //34  smooths horizontal ??????
       static const uint8_t H_3_240[64] = { // This smooths horizontally so vertical block edges are reduced
       1,  64, 48, 32, 16, 16, 16, 16,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1
       };

  //35  good very strong thick lines
       static const uint8_t H_3_240_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
       1,  1, 128, 240, 240, 240, 240, 240,
       1,  64,   1,   1,   1,   1,   1,   1,
       1,  1,   1,   1,   1,   1,   1,   1,
       1,  1,   1,   1,   1,   1,   1,   1,
       1,  1,   1,   1,   1,   1,   1,   1,
       1,  1,   1,   1,   1,   1,   1,   1,
       1,  1,   1,   1,   1,   1,   1,   1,
       1,  1,   1,   1,   1,   1,   1,   1
       };


  //36
       static const uint8_t H_3_16_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
       1,  1, 128, 240, 240, 240, 240, 240,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1
       };


  //37
       static const uint8_t H_4_240[64] = { // This smooths horizontally so vertical block edges are reduced
       1,  1,  1, 240,  240,  1,  1,  1,
       1,  1,  240,   240,  1,  1,  1,  1,
       1,  1,  240,   1,  1,  1,  1,  1,
       1,  1,  1,   1,  1,  1,  1,  1,
       1,  1,  1,   1,  1,  1,  1,  1,
       1,  1,  1,   1,  1,  1,  1,  1,
       1,  1,  1,   1,  1,  1,  1,  1,
       1,  1,  1,   1,  1,  1,  1,  1
       };

  //38
       static const uint8_t H_4_240_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
       1,  1,  1, 240, 240, 240, 240, 240,
       1,  1,  1,   1,   1,   1,   1,   1,
       1,  1,  1,   1,   1,   1,   1,   1,
       1,  1,  1,   1,   1,   1,   1,   1,
       1,  1,  1,   1,   1,   1,   1,   1,
       1,  1,  1,   1,   1,   1,   1,   1,
       1,  1,  1,   1,   1,   1,   1,   1,
       1,  1,  1,   1,   1,   1,   1,   1
       };


  //39
       static const uint8_t H_4_32_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
       1,  1,  1, 32, 64, 128, 240, 240,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1
       };
  */









  //33
  //     static const uint8_t H_2_8_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
  //     1,  8, 16, 32, 64, 128, 240, 240,
  //     1,  1,  1,  1,  1,   1,   1,   1,
  //     1,  1,  1,  1,  1,   1,   1,   1,
  //     1,  1,  1,  1,  1,   1,   1,   1,
  //     1,  1,  1,  1,  1,   1,   1,   1,
  //     1,  1,  1,  1,  1,   1,   1,   1,
  //     1,  1,  1,  1,  1,   1,   1,   1,
  //     1,  1,  1,  1,  1,   1,   1,   1
  //     };













  //
  //// Start Column 3
  ////34
  //     static const uint8_t H_3_240[64] = { // This smooths horizontally so vertical block edges are reduced
  //     1,  64, 48, 32, 16, 16, 16, 16,
  //     1,  1,  1,  1,  1,   1,   1,   1,
  //     1,  1,  1,  1,  1,   1,   1,   1,
  //     1,  1,  1,  1,  1,   1,   1,   1,
  //     1,  1,  1,  1,  1,   1,   1,   1,
  //     1,  1,  1,  1,  1,   1,   1,   1,
  //     1,  1,  1,  1,  1,   1,   1,   1,
  //     1,  1,  1,  1,  1,   1,   1,   1
  //     };
  //
  ////35
  //     static const uint8_t H_3_240_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
  //     1,  1, 240, 240, 240, 240, 240, 240,
  //     1,  1,   1,   1,   1,   1,   1,   1,
  //     1,  1,   1,   1,   1,   1,   1,   1,
  //     1,  1,   1,   1,   1,   1,   1,   1,
  //     1,  1,   1,   1,   1,   1,   1,   1,
  //     1,  1,   1,   1,   1,   1,   1,   1,
  //     1,  1,   1,   1,   1,   1,   1,   1,
  //     1,  1,   1,   1,   1,   1,   1,   1
  //     };
  //
  //
  ////36
  //     static const uint8_t H_3_16_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
  //       1,  1, 1, 1, 1, 1, 1, 1,
  //       1,  1,  1,  1,  1,   1,   1,   1,
  //     240,  1,  1,  1,  1,   1,   1,   1,
  //     240,  1,  1,  1,  1,   1,   1,   1,
  //     240,  1,  1,  1,  1,   1,   1,   1,
  //     240,  1,  1,  1,  1,   1,   1,   1,
  //     240,  1,  1,  1,  1,   1,   1,   1,
  //     240,  1,  1,  1,  1,   1,   1,   1
  //     };


  ////36
  //     static const uint8_t H_3_16_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
  //     1,  1, 16, 32, 64, 128, 240, 240,
  //     1,  1,  1,  1,  1,   1,   1,   1,
  //     1,  1,  1,  1,  1,   1,   1,   1,
  //     1,  1,  1,  1,  1,   1,   1,   1,
  //     1,  1,  1,  1,  1,   1,   1,   1,
  //     1,  1,  1,  1,  1,   1,   1,   1,
  //     1,  1,  1,  1,  1,   1,   1,   1,
  //     1,  1,  1,  1,  1,   1,   1,   1
  //     };


  // Start Column 4
  ////37
  //     static const uint8_t H_4_240[64] = { // This smooths horizontally so vertical block edges are reduced
  //     1,  1,  1, 240,  240,  1,  1,  1,
  //     1,  1,  240,   240,  1,  1,  1,  1,
  //     1,  1,  240,   1,  1,  1,  1,  1,
  //     1,  1,  1,   1,  1,  1,  1,  1,
  //     1,  1,  1,   1,  1,  1,  1,  1,
  //     1,  1,  1,   1,  1,  1,  1,  1,
  //     1,  1,  1,   1,  1,  1,  1,  1,
  //     1,  1,  1,   1,  1,  1,  1,  1
  //     };
  //
  ////38
  //     static const uint8_t H_4_240_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
  //     1,  1,  1, 240, 240, 240, 240, 240,
  //     1,  1,  1,   1,   1,   1,   1,   1,
  //     1,  1,  1,   1,   1,   1,   1,   1,
  //     1,  1,  1,   1,   1,   1,   1,   1,
  //     1,  1,  1,   1,   1,   1,   1,   1,
  //     1,  1,  1,   1,   1,   1,   1,   1,
  //     1,  1,  1,   1,   1,   1,   1,   1,
  //     1,  1,  1,   1,   1,   1,   1,   1
  //     };
  //
  //
  ////39
  //     static const uint8_t H_4_32_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
  //     1,  1,  1, 32, 64, 128, 240, 240,
  //     1,  1,  1,  1,  1,   1,   1,   1,
  //     1,  1,  1,  1,  1,   1,   1,   1,
  //     1,  1,  1,  1,  1,   1,   1,   1,
  //     1,  1,  1,  1,  1,   1,   1,   1,
  //     1,  1,  1,  1,  1,   1,   1,   1,
  //     1,  1,  1,  1,  1,   1,   1,   1,
  //     1,  1,  1,  1,  1,   1,   1,   1
  //     };
  //

  /*
  // Start Column 5
  //40
       static const uint8_t H_5_240[64] = { // This smooths horizontally so vertical block edges are reduced
       1,  1,  1,  1, 240,  1,  1,  1,
       1,  1,  1,  1,   1,  1,  1,  1,
       1,  1,  1,  1,   1,  1,  1,  1,
       1,  1,  1,  1,   1,  1,  1,  1,
       1,  1,  1,  1,   1,  1,  1,  1,
       1,  1,  1,  1,   1,  1,  1,  1,
       1,  1,  1,  1,   1,  1,  1,  1,
       1,  1,  1,  1,   1,  1,  1,  1
       };

  //41
       static const uint8_t H_5_240_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
       1,  1,  1,  1, 240, 240, 240, 240,
       1,  1,  1,  1,   1,   1,   1,   1,
       1,  1,  1,  1,   1,   1,   1,   1,
       1,  1,  1,  1,   1,   1,   1,   1,
       1,  1,  1,  1,   1,   1,   1,   1,
       1,  1,  1,  1,   1,   1,   1,   1,
       1,  1,  1,  1,   1,   1,   1,   1,
       1,  1,  1,  1,   1,   1,   1,   1
       };

  //42
       static const uint8_t H_5_64_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
       1,  1,  1,  1, 64, 128, 240, 240,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1
       };



  // Start Column 6
  //43
       static const uint8_t H_6_240[64] = { // This smooths horizontally so vertical block edges are reduced
       1,  1,  1,  1,  1, 240,  1,  1,
       1,  1,  1,  1,  1,   1,  1,  1,
       1,  1,  1,  1,  1,   1,  1,  1,
       1,  1,  1,  1,  1,   1,  1,  1,
       1,  1,  1,  1,  1,   1,  1,  1,
       1,  1,  1,  1,  1,   1,  1,  1,
       1,  1,  1,  1,  1,   1,  1,  1,
       1,  1,  1,  1,  1,   1,  1,  1
       };




  //44
       static const uint8_t H_6_240_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
       1,  1,  1,  1,  1, 240, 240, 240,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1
       };


  //45
       static const uint8_t H_6_64_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
       1,  1,  1,  1,  1,  64, 128, 240,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1
       };



  // Start Column 7
  //46
       static const uint8_t H_7_240[64] = { // This smooths horizontally so vertical block edges are reduced
       1,  1,  1,  1,  1,  1, 240,  1,
       1,  1,  1,  1,  1,  1,   1,  1,
       1,  1,  1,  1,  1,  1,   1,  1,
       1,  1,  1,  1,  1,  1,   1,  1,
       1,  1,  1,  1,  1,  1,   1,  1,
       1,  1,  1,  1,  1,  1,   1,  1,
       1,  1,  1,  1,  1,  1,   1,  1,
       1,  1,  1,  1,  1,  1,   1,  1
       };

  //47
       static const uint8_t H_7_240_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
       1,  1,  1,  1,  1,   1, 240, 240,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1
       };


  //48
       static const uint8_t H_7_128_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
       1,  1,  1,  1,  1,   1, 128, 240,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1,
       1,  1,  1,  1,  1,   1,   1,   1
       };


  // Start Column 8
  //49
       static const uint8_t H_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
       1,  1,  1,  1,  1,   1,  1, 240,
       1,  1,  1,  1,  1,   1,  1,   1,
       1,  1,  1,  1,  1,   1,  1,   1,
       1,  1,  1,  1,  1,   1,  1,   1,
       1,  1,  1,  1,  1,   1,  1,   1,
       1,  1,  1,  1,  1,   1,  1,   1,
       1,  1,  1,  1,  1,   1,  1,   1,
       1,  1,  1,  1,  1,   1,  1,   1
       };

  // END OF H_ ##############################################################################

  */

  //   V_2_64_8_64   Vertical  2 starting row value 64 to ending row 8 value 64 numbers to end
  //   V_2_64_8_240  Vertical  2 starting row 64-64 numbers to end
  //   V_2_64        Vertical  2 row value 64


  // Start Row 2
  //50
  static const uint8_t V_2_64[64] = { // This smooths horizontally so vertical block edges are reduced
  1,  1,  1,  1,  1,  1,  1,  1,
  64, 1,  1,  1,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1
  };

  //51
  static const uint8_t V_2_64_8_64[64] = { // This smooths horizontally so vertical block edges are reduced
  1,   1,  1,  1,  1,  1,  1,  1,
  64,  1,  1,  1,  1,  1,  1,  1,
  64,  1,  1,  1,  1,  1,  1,  1,
  64,  1,  1,  1,  1,  1,  1,  1,
  64,  1,  1,  1,  1,  1,  1,  1,
  64,  1,  1,  1,  1,  1,  1,  1,
  64,  1,  1,  1,  1,  1,  1,  1,
  64,  1,  1,  1,  1,  1,  1,  1
  };

  //52
  static const uint8_t V_2_64_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
  1,   1,  1,  1,  1,  1,  1,  1,
  64,  1,  1,  1,  1,  1,  1,  1,
  240, 1,  1,  1,  1,  1,  1,  1,
  240, 1,  1,  1,  1,  1,  1,  1,
  240, 1,  1,  1,  1,  1,  1,  1,
  240, 1,  1,  1,  1,  1,  1,  1,
  240, 1,  1,  1,  1,  1,  1,  1,
  240, 1,  1,  1,  1,  1,  1,  1
  };

  //53
  static const uint8_t V_2_8_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
  1,   1,  1,  1,  1,  1,  1,  1,
  8,   1,  1,  1,  1,  1,  1,  1,
  16,  1,  1,  1,  1,  1,  1,  1,
  32,  1,  1,  1,  1,  1,  1,  1,
  64,  1,  1,  1,  1,  1,  1,  1,
  128, 1,  1,  1,  1,  1,  1,  1,
  240, 1,  1,  1,  1,  1,  1,  1,
  240, 1,  1,  1,  1,  1,  1,  1
  };


  // Start Row 3
  //54
  static const uint8_t V_3_240[64] = { // This smooths horizontally so vertical block edges are reduced
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  240, 1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1
  };

  //55
  static const uint8_t V_3_240_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  240, 1,  1,  1,  1,  1,  1,  1,
  240, 1,  1,  1,  1,  1,  1,  1,
  240, 1,  1,  1,  1,  1,  1,  1,
  240, 1,  1,  1,  1,  1,  1,  1,
  240, 1,  1,  1,  1,  1,  1,  1,
  240, 1,  1,  1,  1,  1,  1,  1
  };


  //56
  static const uint8_t V_3_16_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  16,  1,  1,  1,  1,  1,  1,  1,
  32,  1,  1,  1,  1,  1,  1,  1,
  64,  1,  1,  1,  1,  1,  1,  1,
  128, 1,  1,  1,  1,  1,  1,  1,
  240, 1,  1,  1,  1,  1,  1,  1,
  240, 1,  1,  1,  1,  1,  1,  1
  };


  // Start Row 4
  //57
  static const uint8_t V_4_240[64] = { // This smooths horizontally so vertical block edges are reduced
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  240, 1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1
  };

  //58
  static const uint8_t V_4_240_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  240, 1,  1,  1,  1,  1,  1,  1,
  240, 1,  1,  1,  1,  1,  1,  1,
  240, 1,  1,  1,  1,  1,  1,  1,
  240, 1,  1,  1,  1,  1,  1,  1,
  240, 1,  1,  1,  1,  1,  1,  1
  };


  //59
  static const uint8_t V_4_32_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  32,  1,  1,  1,  1,  1,  1,  1,
  64,  1,  1,  1,  1,  1,  1,  1,
  128, 1,  1,  1,  1,  1,  1,  1,
  240, 1,  1,  1,  1,  1,  1,  1,
  240, 1,  1,  1,  1,  1,  1,  1
  };



  // Start Row 5
  //60
  static const uint8_t V_5_240[64] = { // This smooths horizontally so vertical block edges are reduced
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  240, 1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1
  };

  //61
  static const uint8_t V_5_240_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  240, 1,  1,  1,  1,  1,  1,  1,
  240, 1,  1,  1,  1,  1,  1,  1,
  240, 1,  1,  1,  1,  1,  1,  1,
  240, 1,  1,  1,  1,  1,  1,  1
  };

  //62
  static const uint8_t V_5_64_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  64,  1,  1,  1,  1,  1,  1,  1,
  128, 1,  1,  1,  1,  1,  1,  1,
  240, 1,  1,  1,  1,  1,  1,  1,
  240, 1,  1,  1,  1,  1,  1,  1
  };


  // Start Row 6
  //63
  static const uint8_t V_6_240[64] = { // This smooths horizontally so vertical block edges are reduced
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  240, 1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1
  };




  //64
  static const uint8_t V_6_240_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  240, 1,  1,  1,  1,  1,  1,  1,
  240, 1,  1,  1,  1,  1,  1,  1,
  240, 1,  1,  1,  1,  1,  1,  1
  };


  //65
  static const uint8_t V_6_64_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  64,  1,  1,  1,  1,  1,  1,  1,
  128, 1,  1,  1,  1,  1,  1,  1,
  240, 1,  1,  1,  1,  1,  1,  1
  };



  // Start Row 7
  //66
  static const uint8_t V_7_240[64] = { // This smooths horizontally so vertical block edges are reduced
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  240, 1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1
  };

  //67
  static const uint8_t V_7_240_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  240, 1,  1,  1,  1,  1,  1,  1,
  240, 1,  1,  1,  1,  1,  1,  1
  };


  //68
  static const uint8_t V_7_128_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  128, 1,  1,  1,  1,  1,  1,  1,
  240, 1,  1,  1,  1,  1,  1,  1
  };


  // Start Row 8
  //69
  static const uint8_t V_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  240, 1,  1,  1,  1,  1,  1,  1
  };


  // END OF V_ ##############################################################################




  //   HV_2_64_8_64   Horizontal and Vertical  2 starting row and col value 64 to ending row 8 value 64 numbers to end
  //   HV_2_64_8_240  Horizontal and Vertical  2 starting row and col 64-64 numbers to end
  //   HV_2_64        Horizontal and Vertical  2 row and col value 64


  // Start Row 2
  //70
  static const uint8_t HV_2_64[64] = { // This smooths horizontally so vertical block edges are reduced
  1, 64,  1,  1,  1,  1,  1,  1,
  64, 1,  1,  1,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1
  };

  //71
  //     static const uint8_t HV_2_64_8_64[64] = { // This smooths horizontally so vertical block edges are reduced
  //     1,  64, 64, 64, 64, 64, 64, 64,
  //     64,  1,  1,  1,  1,  1,  1,  1,
  //     64,  1,  1,  1,  1,  1,  1,  1,
  //     64,  1,  1,  1,  1,  1,  1,  1,
  //     64,  1,  1,  1,  1,  1,  1,  1,
  //     64,  1,  1,  1,  1,  1,  1,  1,
  //     64,  1,  1,  1,  1,  1,  1,  1,
  //     64,  1,  1,  1,  1,  1,  1,  1
  //     };

  static const uint8_t HV_2_64_8_64[64] = { // This smooths horizontally so vertical block edges are reduced
  1,  240, 240, 240, 240, 240, 240, 240,
  240, 1,   1,   1,   1,   1,   1,   1,
  240, 1,   1,   1,   1,   1,   1,   1,
  240, 1,   1,   1,   1,   1,   1,   1,
  240, 1,   1,   1,   1,   1,   1,   1,
  240, 1,   1,   1,   1,   1,   1,   1,
  240, 1,   1,   1,   1,   1,   1,   1,
  240, 1,   1,   1,   1,   1,   1,   1
  };

  //72
  static const uint8_t HV_2_64_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
  1,  64, 240, 240, 240, 240, 240, 240,
  64,  1,   1,   1,   1,   1,   1,   1,
  240, 1,   1,   1,   1,   1,   1,   1,
  240, 1,   1,   1,   1,   1,   1,   1,
  240, 1,   1,   1,   1,   1,   1,   1,
  240, 1,   1,   1,   1,   1,   1,   1,
  240, 1,   1,   1,   1,   1,   1,   1,
  240, 1,   1,   1,   1,   1,   1,   1
  };

  //73
  static const uint8_t HV_2_8_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
  1,   8, 16, 32, 64, 128, 240, 240,
  8,   1,  1,  1,  1,   1,   1,   1,
  16,  1,  1,  1,  1,   1,   1,   1,
  32,  1,  1,  1,  1,   1,   1,   1,
  64,  1,  1,  1,  1,   1,   1,   1,
  128, 1,  1,  1,  1,   1,   1,   1,
  240, 1,  1,  1,  1,   1,   1,   1,
  240, 1,  1,  1,  1,   1,   1,   1
  };


  // Start Row 3
  //74
  static const uint8_t HV_3_240[64] = { // This smooths horizontally so vertical block edges are reduced
  1,   1, 240,  1,  1,  1,  1,  1,
  1,   1,   1,  1,  1,  1,  1,  1,
  240, 1,   1,  1,  1,  1,  1,  1,
  1,   1,   1,  1,  1,  1,  1,  1,
  1,   1,   1,  1,  1,  1,  1,  1,
  1,   1,   1,  1,  1,  1,  1,  1,
  1,   1,   1,  1,  1,  1,  1,  1,
  1,   1,   1,  1,  1,  1,  1,  1
  };

  //75
  static const uint8_t HV_3_240_240[64] = { // This smooths horizontally so vertical block edges are reduced
  1,   1, 240, 240, 240, 240, 240, 240,
  1,   1,   1,   1,   1,   1,   1,   1,
  240, 1,   1,   1,   1,   1,   1,   1,
  240, 1,   1,   1,   1,   1,   1,   1,
  240, 1,   1,   1,   1,   1,   1,   1,
  240, 1,   1,   1,   1,   1,   1,   1,
  240, 1,   1,   1,   1,   1,   1,   1,
  240, 1,   1,   1,   1,   1,   1,   1
  };


  //76
  static const uint8_t HV_3_16_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
  1,   1, 16, 32, 64, 128, 240, 240,
  1,   1,  1,  1,  1,   1,   1,   1,
  16,  1,  1,  1,  1,   1,   1,   1,
  32,  1,  1,  1,  1,   1,   1,   1,
  64,  1,  1,  1,  1,   1,   1,   1,
  128, 1,  1,  1,  1,   1,   1,   1,
  240, 1,  1,  1,  1,   1,   1,   1,
  240, 1,  1,  1,  1,   1,   1,   1
  };


  // Start Row 4
  //77
  static const uint8_t HV_4_240[64] = { // This smooths horizontally so vertical block edges are reduced
  1,   1,  1, 240,  1,  1,  1,  1,
  1,   1,  1,   1,  1,  1,  1,  1,
  1,   1,  1,   1,  1,  1,  1,  1,
  240, 1,  1,   1,  1,  1,  1,  1,
  1,   1,  1,   1,  1,  1,  1,  1,
  1,   1,  1,   1,  1,  1,  1,  1,
  1,   1,  1,   1,  1,  1,  1,  1,
  1,   1,  1,   1,  1,  1,  1,  1
  };

  //78
  static const uint8_t HV_4_240_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
  1,   1,  1, 240, 240, 240, 240, 240,
  1,   1,  1,   1,   1,   1,   1,   1,
  1,   1,  1,   1,   1,   1,   1,   1,
  240, 1,  1,   1,   1,   1,   1,   1,
  240, 1,  1,   1,   1,   1,   1,   1,
  240, 1,  1,   1,   1,   1,   1,   1,
  240, 1,  1,   1,   1,   1,   1,   1,
  240, 1,  1,   1,   1,   1,   1,   1
  };


  //79
  static const uint8_t HV_4_32_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
  1,   1,  1, 32, 64, 128, 240, 240,
  1,   1,  1,  1,  1,   1,   1,   1,
  1,   1,  1,  1,  1,   1,   1,   1,
  32,  1,  1,  1,  1,   1,   1,   1,
  64,  1,  1,  1,  1,   1,   1,   1,
  128, 1,  1,  1,  1,   1,   1,   1,
  240, 1,  1,  1,  1,   1,   1,   1,
  240, 1,  1,  1,  1,   1,   1,   1
  };



  // Start Row 5
  //80
  static const uint8_t HV_5_240[64] = { // This smooths horizontally so vertical block edges are reduced
  1,   1,  1,  1, 240,  1,  1,  1,
  1,   1,  1,  1,   1,  1,  1,  1,
  1,   1,  1,  1,   1,  1,  1,  1,
  1,   1,  1,  1,   1,  1,  1,  1,
  240, 1,  1,  1,   1,  1,  1,  1,
  1,   1,  1,  1,   1,  1,  1,  1,
  1,   1,  1,  1,   1,  1,  1,  1,
  1,   1,  1,  1,   1,  1,  1,  1
  };

  //81
  static const uint8_t HV_5_240_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
  1,   1,  1,  1, 240, 240, 240, 240,
  1,   1,  1,  1,   1,   1,   1,   1,
  1,   1,  1,  1,   1,   1,   1,   1,
  1,   1,  1,  1,   1,   1,   1,   1,
  240, 1,  1,  1,   1,   1,   1,   1,
  240, 1,  1,  1,   1,   1,   1,   1,
  240, 1,  1,  1,   1,   1,   1,   1,
  240, 1,  1,  1,   1,   1,   1,   1
  };

  //82
  static const uint8_t HV_5_64_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
  1,   1,  1,  1, 64, 128, 240, 240,
  1,   1,  1,  1,  1,   1,   1,   1,
  1,   1,  1,  1,  1,   1,   1,   1,
  1,   1,  1,  1,  1,   1,   1,   1,
  64,  1,  1,  1,  1,   1,   1,   1,
  128, 1,  1,  1,  1,   1,   1,   1,
  240, 1,  1,  1,  1,   1,   1,   1,
  240, 1,  1,  1,  1,   1,   1,   1
  };


  // Start Row 6
  //83
  static const uint8_t HV_6_240[64] = { // This smooths horizontally so vertical block edges are reduced
  1,   1,  1,  1,  1, 240,  1,  1,
  1,   1,  1,  1,  1,   1,  1,  1,
  1,   1,  1,  1,  1,   1,  1,  1,
  1,   1,  1,  1,  1,   1,  1,  1,
  1,   1,  1,  1,  1,   1,  1,  1,
  240, 1,  1,  1,  1,   1,  1,  1,
  1,   1,  1,  1,  1,   1,  1,  1,
  1,   1,  1,  1,  1,   1,  1,  1
  };




  //84
  static const uint8_t HV_6_240_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
  1,   1,  1,  1,  1, 240, 240, 240,
  1,   1,  1,  1,  1,   1,   1,   1,
  1,   1,  1,  1,  1,   1,   1,   1,
  1,   1,  1,  1,  1,   1,   1,   1,
  1,   1,  1,  1,  1,   1,   1,   1,
  240, 1,  1,  1,  1,   1,   1,   1,
  240, 1,  1,  1,  1,   1,   1,   1,
  240, 1,  1,  1,  1,   1,   1,   1
  };


  //85
  static const uint8_t HV_6_64_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
  1,   1,  1,  1,  1,  64, 128, 240,
  1,   1,  1,  1,  1,   1,   1,   1,
  1,   1,  1,  1,  1,   1,   1,   1,
  1,   1,  1,  1,  1,   1,   1,   1,
  1,   1,  1,  1,  1,   1,   1,   1,
  64,  1,  1,  1,  1,   1,   1,   1,
  128, 1,  1,  1,  1,   1,   1,   1,
  240, 1,  1,  1,  1,   1,   1,   1
  };



  // Start Row 7
  //86
  static const uint8_t HV_7_240[64] = { // This smooths horizontally so vertical block edges are reduced
  1,   1,  1,  1,  1,  1, 240,  1,
  1,   1,  1,  1,  1,  1,   1,  1,
  1,   1,  1,  1,  1,  1,   1,  1,
  1,   1,  1,  1,  1,  1,   1,  1,
  1,   1,  1,  1,  1,  1,   1,  1,
  1,   1,  1,  1,  1,  1,   1,  1,
  240, 1,  1,  1,  1,  1,   1,  1,
  1,   1,  1,  1,  1,  1,   1,  1
  };

  //87
  static const uint8_t HV_7_240_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
  1,   1,  1,  1,  1,   1, 240, 240,
  1,   1,  1,  1,  1,   1,   1,   1,
  1,   1,  1,  1,  1,   1,   1,   1,
  1,   1,  1,  1,  1,   1,   1,   1,
  1,   1,  1,  1,  1,   1,   1,   1,
  1,   1,  1,  1,  1,   1,   1,   1,
  240, 1,  1,  1,  1,   1,   1,   1,
  240, 1,  1,  1,  1,   1,   1,   1
  };


  //88
  static const uint8_t HV_7_128_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
  1,   1,  1,  1,  1,   1, 128, 240,
  1,   1,  1,  1,  1,   1,   1,   1,
  1,   1,  1,  1,  1,   1,   1,   1,
  1,   1,  1,  1,  1,   1,   1,   1,
  1,   1,  1,  1,  1,   1,   1,   1,
  1,   1,  1,  1,  1,   1,   1,   1,
  128, 1,  1,  1,  1,   1,   1,   1,
  240, 1,  1,  1,  1,   1,   1,   1
  };


  // Start Row 8
  //89
  static const uint8_t HV_8_240[64] = { // This smooths horizontally so vertical block edges are reduced
  1,   1,  1,  1,  1,   1,  1, 240,
  1,   1,  1,  1,  1,   1,  1,   1,
  1,   1,  1,  1,  1,   1,  1,   1,
  1,   1,  1,  1,  1,   1,  1,   1,
  1,   1,  1,  1,  1,   1,  1,   1,
  1,   1,  1,  1,  1,   1,  1,   1,
  1,   1,  1,  1,  1,   1,  1,   1,
  240, 1,  1,  1,  1,   1,  1,   1
  };


  // END OF HV ##############################################################################





  //   D_2_64_8_64   Diagonal  2 starting row and col value 64 to ending row 8 value 64 numbers to end
  //   D_2_64_8_240  Diagonal  2 starting row and col 64-64 numbers to end
  //   D_2_64        Diagonal  2 row and col value 64


  // Start RC 2
  //90     // max smoothing at 45 degrees for center diagonal
  static const uint8_t D_2_240[64] = { // This smooths diagonally
  1,   1,  1,  1,  1,  1,  1,  1,
  1, 240,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1,
  1,   1,  1,  1,  1,  1,  1,  1
  };


  //91
  static const uint8_t D_2_240_8_240[64] = { // This smooths diagonally
  1,   1,   1,   1,   1,   1,   1,   1,
  1, 240,   1,   1,   1,   1,   1,   1,
  1,   1, 240,   1,   1,   1,   1,   1,
  1,   1,   1, 240,   1,   1,   1,   1,
  1,   1,   1,   1, 240,   1,   1,   1,
  1,   1,   1,   1,   1, 240,   1,   1,
  1,   1,   1,   1,   1,   1, 240,   1,
  1,   1,   1,   1,   1,   1,   1, 240
  };



  //92
  static const uint8_t D_3_240_8_240[64] = { // This smooths diagonally
  1,  1,   1,   1,   1,   1,   1,   1,
  1,  1,   1,   1,   1,   1,   1,   1,
  1,  1, 240,   1,   1,   1,   1,   1,
  1,  1,   1, 240,   1,   1,   1,   1,
  1,  1,   1,   1, 240,   1,   1,   1,
  1,  1,   1,   1,   1, 240,   1,   1,
  1,  1,   1,   1,   1,   1, 240,   1,
  1,  1,   1,   1,   1,   1,   1, 240
  };




  //93
  static const uint8_t D_4_240_8_240[64] = { // This smooths diagonally
  1,  1,  1,   1,   1,   1,   1,   1,
  1,  1,  1,   1,   1,   1,   1,   1,
  1,  1,  1,   1,   1,   1,   1,   1,
  1,  1,  1, 240,   1,   1,   1,   1,
  1,  1,  1,   1, 240,   1,   1,   1,
  1,  1,  1,   1,   1, 240,   1,   1,
  1,  1,  1,   1,   1,   1, 240,   1,
  1,  1,  1,   1,   1,   1,   1, 240
  };



  // Diagonal shift Up. It smooths edges of lines with angles closer to vertical.
  // The larger U is the higher the angle smoothed.
//94     // more vertical 65 degrees?
  static const uint8_t D_U1_3_240_8_240[64] = { // This smooths diagonally
  1,  1,   1,   1,   1,   1,   1,   1,
  1,  1, 240,   1,   1,   1,   1,   1,
  1,  1,   1, 240,   1,   1,   1,   1,
  1,  1,   1,   1, 240,   1,   1,   1,
  1,  1,   1,   1,   1, 240,   1,   1,
  1,  1,   1,   1,   1,   1, 240,   1,
  1,  1,   1,   1,   1,   1,   1, 240,
  1,  1,   1,   1,   1,   1,   1,   1
  };


  //95
  static const uint8_t D_U2_4_240_8_240[64] = { // This smooths diagonally
  1,  1,  1,  16,   1,   1,   1,   1,
  1,  1,  1, 240,   1,   1,   1,   1,
  1,  1,  1,   1, 240,   1,   1,   1,
  1,  1,  1,   1,   1, 240,   1,   1,
  1,  1,  1,   1,   1,   1, 240,   1,
  1,  1,  1,   1,   1,   1,   1, 240,
  1,  1,  1,   1,   1,   1,   1,   1,
  1,  1,  1,   1,   1,   1,   1,   1
  };


  //96     // more horizontal 22 degrees?
  static const uint8_t D_D1_3_240_8_240[64] = { // This smooths diagonally
  1,   1,   1,   1,   1,   1,   1,   1,
  1,   1,   1,   1,   1,   1,   1,   1,
  1, 240,   1,   1,   1,   1,   1,   1,
  1,   1, 240,   1,   1,   1,   1,   1,
  1,   1,   1, 240,   1,   1,   1,   1,
  1,   1,   1,   1, 240,   1,   1,   1,
  1,   1,   1,   1,   1, 240,   1,   1,
  1,   1,   1,   1,   1,   1, 240,   1
  };



  //97
  static const uint8_t D_D2_4_240_8_240[64] = { // This smooths diagonally
  1,   1,   1,   1,   1,   1,   1,   1,
  1,   1,   1,   1,   1,   1,   1,   1,
  1,   1,   1,   1,   1,   1,   1,   1,
  16, 240,   1,   1,   1,   1,   1,   1,
  1,   1, 240,   1,   1,   1,   1,   1,
  1,   1,   1, 240,   1,   1,   1,   1,
  1,   1,   1,   1, 240,   1,   1,   1,
  1,   1,   1,   1,   1, 240,   1,   1
  };



  //98
  static const uint8_t D_O1_240_8_240[64] = { // This smooths diagonally
  1,   1,   1,   1,   1,   1,   1,   1,
  1, 240, 240,   1,   1,   1,   1,   1,
  1, 240, 240, 240,   1,   1,   1,   1,
  1,   1, 240, 240, 240,   1,   1,   1,
  1,   1,   1, 240, 240, 240,   1,   1,
  1,   1,   1,   1, 240, 240, 240,   1,
  1,   1,   1,   1,   1, 240, 240, 240,
  1,   1,   1,   1,   1,   1, 240, 240
  };



  //99
  static const uint8_t D_O3_240_8_240[64] = { // This smooths diagonally
  1,   1,   1,   1,   1,   1,   1,   1,
  1, 240, 240, 240, 240,   1,   1,   1,
  1, 240, 240, 240, 240, 240,   1,   1,
  1, 240, 240, 240, 240, 240, 240,   1,
  1, 240, 240, 240, 240, 240, 240, 240,
  1,   1, 240, 240, 240, 240, 240, 240,
  1,   1,   1, 240, 240, 240, 240, 240,
  1,   1,   1,   1, 240, 240, 240, 240
  };
































  //100
       //Block boundaries as good as VeryFineHVSmoothXX
       //Less streak artifacting, may not be any.
       //Not as good anti-aliasing
  static const uint8_t D_UD1_240_8_240[64] = {
  1,   1,   1,   1,   1,   1,   1,    1,
  1,   1, 240,   1,   1,   1,   1,    1,
  1, 240,   1, 240,   1,   1,   1,    1,
  1,   1, 240,   1, 240,   1,   1,    1,
  1,   1,   1, 240,   1, 240,   1,    1,
  1,   1,   1,   1, 240,   1, 240,    1,
  1,   1,   1,   1,   1, 240,   1,  240,
  1,   1,   1,   1,   1,   1, 240,    1
  };


  //101
  static const uint8_t D_UD1_64_8_240[64] = {
  1,   1,   1,   1,   1,   1,   1,    1,
  1,   1,  64,   1,   1,   1,   1,    1,
  1,  64,   1, 240,   1,   1,   1,    1,
  1,   1, 240,   1, 240,   1,   1,    1,
  1,   1,   1, 240,   1, 240,   1,    1,
  1,   1,   1,   1, 240,   1, 240,    1,
  1,   1,   1,   1,   1, 240,   1,  240,
  1,   1,   1,   1,   1,   1, 240,    1
  };


  //102
  static const uint8_t D_UD2_240_8_240[64] = {
  1,   1,   1,   1,   1,   1,   1,    1,
  1,   1,   1, 240,   1,   1,   1,    1,
  1,   1,   1,   1, 240,   1,   1,    1,
  1, 240,   1,   1,   1, 240,   1,    1,
  1,   1, 240,   1,   1,   1, 240,    1,
  1,   1,   1, 240,   1,   1,   1,  240,
  1,   1,   1,   1, 240,   1,   1,    1,
  1,   1,   1,   1,   1, 240,   1,    1
  };


  //103
  static const uint8_t D_UD3_240_8_240[64] = {
  1,   1,   1,   1,   1,   1,   1,    1,
  1,   1,   1,   1, 240,   1,   1,    1,
  1,   1,   1,   1,   1, 240,   1,    1,
  1,   1,   1,   1,   1,   1, 240,    1,
  1, 240,   1,   1,   1,   1,   1,  240,
  1,   1, 240,   1,   1,   1,   1,    1,
  1,   1,   1, 240,   1,   1,   1,    1,
  1,   1,   1,   1, 240,   1,   1,    1
  };


  //104
  static const uint8_t D_UD4_240_8_240[64] = {
  1,   1,   1,   1,   1,   1,   1,    1,
  1,   1,   1,   1,   1, 240,   1,    1,
  1,   1,   1,   1,   1,   1, 240,    1,
  1,   1,   1,   1,   1,   1,   1,  240,
  1,   1,   1,   1,   1,   1,   1,    1,
  1, 240,   1,   1,   1,   1,   1,    1,
  1,   1, 240,   1,   1,   1,   1,    1,
  1,   1,   1, 240,   1,   1,   1,    1
  };


  //105
  static const uint8_t D_UD4F_240_8_240[64] = {
  1,   1,   1,   1,   1,   1,   1,    1,
  1,   1,   1,   1,   1, 240, 240,  240,
  1,   1,   1,   1,   1,   1, 240,  240,
  1,   1,   1,   1,   1,   1,   1,  240,
  1,   1,   1,   1,   1,   1,   1,    1,
  1, 240,   1,   1,   1,   1,   1,    1,
  1, 240, 240,   1,   1,   1,   1,    1,
  1, 240, 240, 240,   1,   1,   1,    1
  };



  //106
  static const uint8_t D_UD3F_240_8_240[64] = {
  1,   1,   1,   1,   1,   1,   1,    1,
  1,   1,   1,   1, 240, 240, 240,  240,
  1,   1,   1,   1,   1, 240, 240,  240,
  1,   1,   1,   1,   1,   1, 240,  240,
  1, 240,   1,   1,   1,   1,   1,  240,
  1, 240, 240,   1,   1,   1,   1,    1,
  1, 240, 240, 240,   1,   1,   1,    1,
  1, 240, 240, 240, 240,   1,   1,    1
  };



  //107
  static const uint8_t D_UD2F_240_8_240[64] = {
  1,   1,   1,   1,   1,   1,   1,    1,
  1,   1,   1, 240, 240, 240, 240,  240,
  1,   1,   1,   1, 240, 240, 240,  240,
  1, 240,   1,   1,   1, 240, 240,  240,
  1, 240, 240,   1,   1,   1, 240,  240,
  1, 240, 240, 240,   1,   1,   1,  240,
  1, 240, 240, 240, 240,   1,   1,    1,
  1, 240, 240, 240, 240, 240,   1,    1
  };


  //108
  static const uint8_t D_UD1F_240_8_240[64] = {
  1,   1,   1,   1,   1,   1,   1,    1,
  1,   1, 240, 240, 240, 240, 240,  240,
  1, 240,   1, 240, 240, 240, 240,  240,
  1, 240, 240,   1, 240, 240, 240,  240,
  1, 240, 240, 240,   1, 240, 240,  240,
  1, 240, 240, 240, 240,   1, 240,  240,
  1, 240, 240, 240, 240, 240,   1,  240,
  1, 240, 240, 240, 240, 240, 240,    1
  };


  ////109
  //     static const uint8_t D_UD1F_32_8_240[64] = {
  //     1,   1,   1,   1,   1,   1,   1,    1,
  //     1,   1,  32, 240, 240, 240, 240,  240,
  //     1,  32,   1, 240, 240, 240, 240,  240,
  //     1, 240, 240,   1, 240, 240, 240,  240,
  //     1, 240, 240, 240,   1, 240, 240,  240,
  //     1, 240, 240, 240, 240,   1, 240,  240,
  //     1, 240, 240, 240, 240, 240,   1,  240,
  //     1, 240, 240, 240, 240, 240, 240,    1
  //     };


  //109  VERY STRONG DIAGONAL SMOOTHING
  static const uint8_t DIAG_240[64] = {
      9,   1,   1,   1,   1,   1,   1,    1,
  1, 240, 240, 240, 240, 240, 240,  240,
  1, 240, 240, 240, 240, 240, 240,  240,
  1, 240, 240, 240, 240, 240, 240,  240,
  1, 240, 240, 240, 240, 240, 240,  240,
  1, 240, 240, 240, 240, 240, 240,  240,
  1, 240, 240, 240, 240, 240, 240,  240,
  1, 240, 240, 240, 240, 240, 240,  240
  };


  //110   does diagonal lines.  does not do vertical or horizontal lines
  static const uint8_t D_UD0F_240_8_240[64] = {
  9,   1,   1,   1,   1,   1,   1,    1,
  1, 240, 240, 240, 240, 240, 240,  240,
  1, 240, 240, 240, 240, 240, 240,  240,
  1, 240, 240, 240, 240, 240, 240,  240,
  1, 240, 240, 240, 240, 240, 240,  240,
  1, 240, 240, 240, 240, 240, 240,  240,
  1, 240, 240, 240, 240, 240, 240,  240,
  1, 240, 240, 240, 240, 240, 240,  240
  };

  // 111
  static const uint8_t Hpos1[64] = { // 64 is the largest value that can be used in top left corner without clipping the signal
       9,   1,   1,   1,   1,   1,   1,    1,
       1, 240, 240, 240, 240, 240, 240,  240,
       1, 240, 240, 240, 240, 240, 240,  240,
       1, 240, 240, 240, 240, 240, 240,  240,
       1, 240, 240, 240, 240, 240, 240,  240,
       1, 240, 240, 240, 240, 240, 240,  240,
       1, 240, 240, 240, 240, 240, 240,  240,
       1, 240, 240, 240, 240, 240, 240,  240
  };


  //static const uint8_t Hpos1[64] = { // 64 is the largest value that can be used in top left corner without clipping the signal
  //    64,  1,  1,  1,  1,  1,  1,  1,
  //     1,  1,  1,  1,  1,  1,  1,  1,
  //     1,  1,  1,  1,  1,  1,  1,  1,
  //     1,  1,  1,  1,  1,  1,  1,  1,
  //     1,  1,  1,  1,  1,  1,  1,  1,
  //     1,  1,  1,  1,  1,  1,  1,  1,
  //     1,  1,  1,  1,  1,  1,  1,  1,
  //     1,  1,  1,  1,  1,  1,  1,  1
  //     };





  // These I did for testing

  // NOTE top left corner does nothing in mpeg intra. It only has effects in mpeg inter
  // 111
  //static const uint8_t Hpos1[64] = { // 64 is the largest value that can be used in top left corner without clipping the signal
  //    64,  1,  1,  1,  1,  1,  1,  1,
  //     1,  1,  1,  1,  1,  1,  1,  1,
  //     1,  1,  1,  1,  1,  1,  1,  1,
  //     1,  1,  1,  1,  1,  1,  1,  1,
  //     1,  1,  1,  1,  1,  1,  1,  1,
  //     1,  1,  1,  1,  1,  1,  1,  1,
  //     1,  1,  1,  1,  1,  1,  1,  1,
  //     1,  1,  1,  1,  1,  1,  1,  1
  //     };


  // 112
  static const uint8_t DAA1[64] = {
       1,  1,   1, 220,   1,   1,   1,   1,
       1,  8,   8,   1,   1,   1,   1,   1,
       1,  8,  90,  16,   1,   1,   1,   1,
       220,  1,  16, 220,  32,   1,   1,   1,
       1,  1,   1,  32, 220,  32,   1,   1,
       1,  1,   1,   1,  32, 220, 220,   1,
       1,  1,   1,   1,   1, 220, 220, 220,
       1,  1,   1,   1,   1,   1, 220, 220
  };


  // matrix for Smoothing after Sharpening                      POSSIBLY WANT THIS AS A STANDARD MATIX
  // Combination of matrix 8 and 112
  // 113
  static const uint8_t smooth_sharp_matrix[64] = {
         2,   2,   3,  40,  16,  32,  64, 128,
         2,   8,   9,  14,  28,  48,  96, 128,
         2,   5,  90,  25,  40,  64,  80, 128,
        30,  12,  24, 120,  58,  76,  92, 128,
        11,  23,  40,  58, 140,  98, 100, 128,
        22,  34,  50,  76,  98, 220, 180, 255,
        30,  48,  80,  92, 100, 140, 255, 255,
        40, 128, 128, 128, 128, 128, 128, 255
  };

  // // matrix 8 name for test
  // // This is working and is good for reducing halos introduced when sharpening
  //static const uint8_t default_intra_matrix[64] = {
  //       2,   2,   3,   8,  16,  32,  64, 128,
  //       2,   2,   9,  14,  28,  48,  96, 128,
  //       2,   5,  14,  25,  40,  64,  80, 128,
  //       4,  12,  24,  40,  58,  76,  92, 128,
  //      11,  23,  40,  58,  80,  98, 100, 128,
  //      22,  34,  50,  76,  98, 100, 140, 255,
  //      30,  48,  80,  92, 100, 134, 255, 255,
  //      40, 128, 128, 128, 128, 128, 128, 255
  //  };





  //148
  static const uint8_t findBlock4x[64] = { // This smooths horizontally so vertical block edges are reduced
  1,   1,  1,  1,  1,  16, 128, 240,
  1,   1,  1,  1, 64,   1,   1,   1,
  1,   1,  1, 64,  1,   1,   1,   1,
  1,   1, 64,  1,  1,   1,   1,   1,
  1,  64,  1,  1,  1,   1,   1,   1,
  16,  1,  1,  1,  1,   1,   1,   1,
  128, 1,  1,  1,  1,   1,   1,   1,
  240, 1,  1,  1,  1,   1,   1,   1
  };


  //149
  static const uint8_t my_intra_only_1_by_255xx[64] = {
         8,   8,   8,  12,  16,  32,  64, 128,
         8,   8,   9,  14,  24,  48,  96, 128,
         8,   9,  12,  16,  32,  64,  80, 128,
        12,  14,  16,  18,  48,  72,  92, 128,
        16,  24,  32,  48,  80,  98, 100, 128,
        32,  48,  64,  72,  98, 100, 140, 255,
        64,  96,  80,  92, 100, 140, 255, 255,
       128, 128, 128, 128, 128, 255, 255, 255
  };



  //150
  static const uint8_t my_sharp[64] = {
       8,  8,  8, 10, 10, 14, 20, 24,
       8,  8,  9, 12, 16, 22, 26, 28,
       8,  9, 15, 16, 24, 26, 28, 30,
      10, 12, 16, 24, 26, 28, 30, 30,
      10, 16, 24, 26, 28, 30, 30, 30,
      14, 22, 26, 28, 30, 30, 30, 30,
      20, 26, 28, 30, 30, 30, 30, 30,
      24, 28, 30, 30, 30, 30, 30, 30
  };


  //  START TEST COPY OF FOLLOWING BLOCKED OUT
  //151
  static const uint8_t my_sharp_2[64] = {
       8,  64,  64,  32,  8,  8,  8,  8,
       64,  64,  32,  8,  8,  8,  8,  8,
       64,  32,  8,  8,  8,  8,  8,  8,
       32,  8,  8,  8,  8,  8,  8,  8,
       8,  8,  8,  8,  8,  8,  8,  8,
       8,  8,  8,  8,  8,  8,  8,  8,
       8,  8,  8,  8,  8,  8,  8,  8,
       8,  8,  8,  8,  8,  8,  8,  8
  };


  //152
  static const uint8_t HV_2_2_8_128[64] = {
       8,  128,  64,  32,  16,  8,  4,  2,
       128,  64,  32,  16,  8,  4,  2,  8,
       64,  32,  16,  8,  4,  2,  8,  8,
       32,  16,  8,  4,  2,  8,  8,  8,
       16,  8,  4,  2,  8,  8,  8,  8,
       8,  4,  2,  8,  8,  8,  8,  8,
       4,  2,  8,  8,  8,  8,  8,  8,
       2,  8,  8,  8,  8,  8,  8,  8
  };

  //matrix 1
  //static const uint8_t my_intra_matrix_3_11[64] = {
  //     3,   4,  5,  6,  7,  8,  9, 10,
  //     4,   5,  6,  7,  8,  9, 10, 10,
  //     5,   6,  7,  8,  9, 10, 10, 10,
  //     6,   7,  8,  9, 10, 10, 10, 10,
  //     7,   8,  9, 10, 10, 10, 10, 11,
  //     8,   9, 10, 10, 10, 10, 11, 11,
  //     9,  10, 10, 10, 10, 11, 11, 11,
  //     10, 10, 10, 10, 11, 11, 11, 11
  //  };

  // matrix_num 161      64
  static const uint8_t matrix_161[64] = {
       2,  3,  5, 10, 10, 14, 20, 24,
       3,  5,  9, 12, 16, 22, 26, 28,
       5,  9, 15, 16, 24, 26, 28, 30,
      10, 12, 16, 24, 26, 28, 30, 30,
      10, 16, 24, 26, 28, 30, 30, 30,
      14, 22, 26, 28, 30, 30, 30, 30,
      20, 26, 28, 30, 30, 30, 30, 30,
      24, 28, 30, 30, 30, 30, 30, 30
  };

  //  END TEST COPY OF FOLLOWING BLOCKED OUT


  /*
  //151
  static const uint8_t my_sharp_2[64] = {
       4,  4,  4,  5,  7,  7,  8,  8,
       4,  5,  6,  7,  8, 12, 15,  4,
       4,  6,  8,  8, 12, 15,  5,  4,
       5,  7,  8, 12, 15, 15,  8,  4,
       7,  8, 12, 15, 15,  8,  4,  4,
       7, 12, 15, 15,  8,  4,  4,  4,
       8, 15,  6,  8,  4,  4,  4,  4,
       8,  5,  4,  4,  4,  4,  4,  4
    };


  //152
  static const uint8_t HV_2_2_8_128[64] = {
       1,   2,  4,  8, 16, 32, 64, 128,
       2,   1,  1,  1,  1,  1,  1,   1,
       4,   1,  1,  1,  1,  1,  1,   1,
       8,   1,  1,  1,  1,  1,  1,   1,
       16,  1,  1,  1,  1,  1,  1,   1,
       32,  1,  1,  1,  1,  1,  1,   1,
       64,  1,  1,  1,  1,  1,  1,   1,
       128, 1,  1,  1,  1,  1,  1,   1
       };

  //matrix 1
  //static const uint8_t my_intra_matrix_3_11[64] = {
  //     3,   4,  5,  6,  7,  8,  9, 10,
  //     4,   5,  6,  7,  8,  9, 10, 10,
  //     5,   6,  7,  8,  9, 10, 10, 10,
  //     6,   7,  8,  9, 10, 10, 10, 10,
  //     7,   8,  9, 10, 10, 10, 10, 11,
  //     8,   9, 10, 10, 10, 10, 11, 11,
  //     9,  10, 10, 10, 10, 11, 11, 11,
  //     10, 10, 10, 10, 11, 11, 11, 11
  //  };

  // matrix_num 161
  static const uint8_t matrix_161[64] = {
       3,   4,  5,  6,  7,  8,  9, 10,
       4,   6,  6,  7,  8,  9, 10, 10,
       5,   6,  7,  8,  9, 10, 10, 10,
       6,   7,  8,  9, 10, 10, 10, 10,
       7,   8,  9, 10, 10, 10, 10, 11,
       8,   9, 10, 10, 10, 10, 11, 11,
       9,  10, 10, 10, 10, 11, 11, 11,
       10, 10, 10, 10, 11, 11, 11, 11
    };

  */




  // matrix_num 2
  //static const uint8_t my_intra_matrix_8_11[64] = {
  //     8,   8,  8,  9,  9,  9,  9, 10,
  //     8,   8,  9,  9,  9,  9, 10, 10,
  //     8,   9,  9,  9,  9, 10, 10, 10,
  //     9,   9,  9,  9, 10, 10, 10, 10,
  //     9,   9,  9, 10, 10, 10, 10, 11,
  //     9,   9, 10, 10, 10, 10, 11, 11,
  //     9,  10, 10, 10, 10, 11, 11, 11,
  //     10, 10, 10, 10, 11, 11, 11, 11
  //  };

  // matrix_num 162
  static const uint8_t matrix_162[64] = {
       8,   8,  8,  9,  9,  9,  9, 10,
       8,   8,  9,  9,  9,  9, 10, 10,
       8,   9,  9,  9,  9, 10, 10, 10,
       9,   9,  9,  9, 10, 10, 10, 10,
       9,   9,  9, 10, 10, 10, 10, 11,
       9,   9, 10, 10, 10, 10, 11, 11,
       9,  10, 10, 10, 10, 11, 11, 11,
       10, 10, 10, 10, 11, 11, 11, 11
  };



  //// matrix_num 163    // MOVED TO SYSTEM NUMBERS
  //static const uint8_t matrix_163[64] = {
  //     8,   8,  8,  8,  8,  8,  8,  8,
  //     8,   8,  8,  8,  8,  8,  8,  8,
  //     8,   8,255,  8,  8,  8,  8,  8,
  //     8,   8,  8,  8,  8,  8,  8,  8,
  //     8,   8,  8,  8,  8,  8,  8,  8,
  //     8,   8,  8,  8,  8,  8,  8,  8,
  //     8,   8,  8,  8,  8,  8,  8,  8,
  //     8,   8,  8,  8,  8,  8,  8,  8
  //  };
  //



  // matrix_num 164    Inverted matrix.  Smooths thick lines much more than thin lines.
  static const uint8_t matrix_164[64] = {
       8,  64,  1,  1,  1,  1,  1,  1,
      64,   1,  1,  1,  1,  1,  1,  1,
       1,   1,  1,  1,  1,  1,  1,  1,
       1,   1,  1,  1,  1,  1,  1,  1,
       1,   1,  1,  1,  1,  1,  1,  1,
       1,   1,  1,  1,  1,  1,  1,  1,
       1,   1,  1,  1,  1,  1,  1,  1,
       1,   1,  1,  1,  1,  1,  1,  1
  };


  // matrix_num 165    Inverted matrix.  Smooths thick lines much more than thin lines.
  static const uint8_t matrix_165[64] = {
       8,   1, 64,  1,  1,  1,  1,  1,
       1,  64,  1,  1,  1,  1,  1,  1,
      64,   1,  1,  1,  1,  1,  1,  1,
       1,   1,  1,  1,  1,  1,  1,  1,
       1,   1,  1,  1,  1,  1,  1,  1,
       1,   1,  1,  1,  1,  1,  1,  1,
       1,   1,  1,  1,  1,  1,  1,  1,
       1,   1,  1,  1,  1,  1,  1,  1
  };

  // matrix_num 166    Inverted matrix.  Smooths thick lines much more than thin lines.
  static const uint8_t matrix_166[64] = {
       8,  64, 64,  1,  1,  1,  1,  1,
      64,  64,  1,  1,  1,  1,  1,  1,
      64,   1,  1,  1,  1,  1,  1,  1,
       1,   1,  1,  1,  1,  1,  1,  1,
       1,   1,  1,  1,  1,  1,  1,  1,
       1,   1,  1,  1,  1,  1,  1,  1,
       1,   1,  1,  1,  1,  1,  1,  1,
       1,   1,  1,  1,  1,  1,  1,  1
  };



  //
  //// matrix_num 166    Inverted matrix.  Smooths thick lines much more than thin lines.
  //static const uint8_t matrix_166[64] = {
  //       8,   1, 64, 64, 64, 64, 64, 64,
  //     1,  64,  1,  1,  1,  1,  1,  1,
  //      64,   1,  1,  1,  1,  1,  1,  1,
  //    64,   1,  1,  1,  1,  1,  1,  1,
  //    64,   1,  1,  1,  1,  1,  1,  1,
  //    64,   1,  1,  1,  1,  1,  1,  1,
  //    64,   1,  1,  1,  1,  1,  1,  1,
  //    64,   1,  1,  1,  1,  1,  1,  1
  //  };

  // matrix_num 166    Inverted matrix.  Smooths thick lines much more than thin lines.
  //static const uint8_t matrix_166[64] = {
  //         8,   1,  1, 64,  1,  1,  1,  1,
  //     1,   1, 64,  1,  1,  1,  1,  1,
  //     1,  64,  1,  1,  1,  1,  1,  1,
  //    64,   1,  1,  1,  1,  1,  1,  1,
  //     1,   1,  1,  1,  1,  1,  1,  1,
  //     1,   1,  1,  1,  1,  1,  1,  1,
  //     1,   1,  1,  1,  1,  1,  1,  1,
  //     1,   1,  1,  1,  1,  1,  1,  1
  //  };





  // matrix_num 167    Inverted matrix.  Smooths thick lines much more than thin lines.
  static const uint8_t matrix_167[64] = {
       8,   1,  1,  1, 64,  1,  1,  1,
       1,   1,  1, 64,  1,  1,  1,  1,
       1,   1, 64,  1,  1,  1,  1,  1,
       1,  64,  1,  1,  1,  1,  1,  1,
      64,   1,  1,  1,  1,  1,  1,  1,
       1,   1,  1,  1,  1,  1,  1,  1,
       1,   1,  1,  1,  1,  1,  1,  1,
       1,   1,  1,  1,  1,  1,  1,  1
  };





  // matrix_num 168    Inverted matrix.  Smooths thick lines much more than thin lines.
  static const uint8_t matrix_168[64] = {
       8,   1,  1,  1,  1, 64,  1,  1,
       1,   1,  1,  1, 64,  1,  1,  1,
       1,   1,  1, 64,  1,  1,  1,  1,
       1,   1, 64,  1,  1,  1,  1,  1,
       1,  64,  1,  1,  1,  1,  1,  1,
      64,   1,  1,  1,  1,  1,  1,  1,
       1,   1,  1,  1,  1,  1,  1,  1,
       1,   1,  1,  1,  1,  1,  1,  1
  };





  // matrix_num 169    Inverted matrix.  Smooths thick lines much more than thin lines.
  static const uint8_t matrix_169[64] = {
       8,   1,  1,  1,  1,  1, 64,  1,
       1,   1,  1,  1,  1, 64,  1,  1,
       1,   1,  1,  1, 64,  1,  1,  1,
       1,   1,  1, 64,  1,  1,  1,  1,
       1,   1, 64,  1,  1,  1,  1,  1,
       1,  64,  1,  1,  1,  1,  1,  1,
      64,   1,  1,  1,  1,  1,  1,  1,
       1,   1,  1,  1,  1,  1,  1,  1
  };




  // matrix_num 170    Inverted matrix.  Smooths thick lines much more than thin lines.
  static const uint8_t matrix_170[64] = {
       8,   1,  1,  1,  1,  1,  1, 64,
       1,   1,  1,  1,  1,  1, 64,  1,
       1,   1,  1,  1,  1, 64,  1,  1,
       1,   1,  1,  1, 64,  1,  1,  1,
       1,   1,  1, 64,  1,  1,  1,  1,
       1,   1, 64,  1,  1,  1,  1,  1,
       1,  64,  1,  1,  1,  1,  1,  1,
      64,   1,  1,  1,  1,  1,  1,  1
  };



  //// matrix_num 171    Inverted matrix.  Smooths thick lines much more than thin lines.
  //static const uint8_t matrix_171[64] = {
  //         8,   1,  1,  1,  1,  1,  1, 64,
  //     1,   1,  1,  1,  1,  1, 64, 64,
  //     1,   1,  1,  1,  1, 64, 64, 64,
  //     1,   1,  1,  1, 64, 64, 64, 64,
  //     1,   1,  1, 64, 64, 64, 64, 64,
  //     1,   1, 64, 64, 64, 64, 64, 64,
  //     1,  64, 64, 64, 64, 64, 64, 64,
  //        64,  64, 64, 64, 64, 64, 64, 64
  //  };


  // matrix_num 171    Inverted matrix.  Smooths thick lines much more than thin lines.
  static const uint8_t matrix_171[64] = {
       8,   1,  1,  1,  1,  1,  1,  1,
       1,   1,  1,  1,  1,  1,  4, 8,
       1,   1,  1,  1,  1, 64, 64, 64,
       1,   1,  1,  1, 64, 64, 64, 64,
       1,   1,  1, 64, 64, 64, 64, 64,
       1,   1, 64, 64, 64, 64, 64, 64,
       1,   4, 64, 64, 64, 64, 64, 64,
       1,  8, 64, 64, 64, 64, 64, 64
  };



  // matrix_num 172    Inverted matrix.  Smooths thick lines much more than thin lines.
  static const uint8_t matrix_172[64] = {
       8,   1,  1,  1,  1,  1,  1, 32,
       1,   1,  1,  1,  1,  1, 32, 48,
       1,   1,  1,  1,  1, 32, 48, 64,
       1,   1,  1,  1, 32, 48, 64, 64,
       1,   1,  1, 32, 48, 64, 64, 64,
       1,   1, 32, 48, 64, 64, 64, 64,
       1,  32, 48, 64, 64, 64, 64, 64,
      32,  48, 64, 64, 64, 64, 64, 64
  };




  // matrix_num 173    Inverted matrix.  Smooths thick lines much more than thin lines.
  static const uint8_t matrix_173[64] = {
       8,   1,  1,  1,  1,  8, 16, 32,
       1,   1,  1,  1,  1,  1, 24, 48,
       1,   1,  1,  1,  1, 24, 48, 64,
       1,   1,  1,  1, 24, 48, 64, 64,
       1,   1,  1, 24, 48, 64, 64, 64,
       8,   1, 24, 48, 64, 64, 64, 64,
      16,  24, 48, 64, 64, 64, 64, 64,
      32,  48, 64, 64, 64, 64, 64, 64
  };



  static const uint8_t matrix_174[64] = {
        8,  1,  1,  1,  1, 32, 32, 31,
        1,  1,  1,  1, 32, 32, 32,  1,
        1,  1,  1, 32, 32, 32,  1,  1,
        1,  1, 32, 32, 32,  1,  1,  1,
        1, 32, 32, 32,  1,  1,  1,  1,
       32, 32, 32,  1,  1,  1,  1,  1,
       32, 32,  1,  1,  1,  1,  1,  1,
       31,  1,  1,  1,  1,  1,  1,  1
  };


  static const uint8_t matrix_175[64] = {
        8,  1,  1,  1,  1,  8, 32, 31,
        1,  1,  1,  1,  8, 32, 32,  1,
        1,  1,  1,  8, 32, 32,  1,  1,
        1,  1,  8, 32, 32,  1,  1,  1,
        1,  8, 32, 32,  1,  1,  1,  1,
        8, 32, 32,  1,  1,  1,  1,  1,
       32, 32,  1,  1,  1,  1,  1,  1,
       31,  1,  1,  1,  1,  1,  1,  1
  };

  //    /////////////////////////////////////////////////////////////////////////////////////////
  static const uint8_t matrix_176[64] = {
        8,  1,  1,  1,  1,  8, 24, 31,
        1,  1,  1,  1,  8, 32, 32,  1,
        1,  1,  1,  8, 32, 32,  1,  1,
        1,  1,  8, 32, 32,  1,  1,  1,
        1,  8, 32, 32,  1,  1,  1,  1,
        8, 32, 32,  1,  1,  1,  1,  1,
       24, 32,  1,  1,  1,  1,  1,  1,
       31,  1,  1,  1,  1,  1,  1,  1
  };


  static const uint8_t matrix_177[64] = {
        8,  1,  1,  4,  1,  8, 24, 31,
        1,  1,  1,  1,  8, 32, 32,  1,
        1,  1,  1,  8, 32, 32,  1,  1,
        4,  1,  8, 32, 32,  1,  1,  1,
        1,  8, 32, 32,  1,  1,  1,  1,
        8, 32, 32,  1,  1,  1,  1,  1,
       24, 32,  1,  1,  1,  1,  1,  1,
       31,  1,  1,  1,  1,  1,  1,  1
  };



  static const uint8_t matrix_178[64] = {
        8,  1,  1,  4,  1,  8, 24, 31,
        1,  1,  4,  1,  8, 32, 32,  1,
        1,  4,  4,  8, 32, 32,  1,  1,
        4,  1,  8, 32, 32,  1,  1,  1,
        1,  8, 32, 32,  1,  1,  1,  1,
        8, 32, 32,  1,  1,  1,  1,  1,
       24, 32,  1,  1,  1,  1,  1,  1,
       31,  1,  1,  1,  1,  1,  1,  1
  };




  static const uint8_t matrix_179[64] = {
        8,  1,  1,  6,  6,  6, 24, 28,
        1,  1,  4,  4,  8, 32, 32,  1,
        1,  4,  4,  8, 32, 32,  1,  1,
        6,  4,  8, 32, 32,  1,  1,  1,
        6,  8, 32, 32,  1,  1,  1,  1,
        6, 32, 32,  1,  1,  1,  1,  1,
       24, 32,  1,  1,  1,  1,  1,  1,
       28,  1,  1,  1,  1,  1,  1,  1
  };


  /*
  static const uint8_t matrix_180[64] = {
        8,  1,  1,  5,  1,  5, 20, 28,
        1,  1,  4,  4,  8, 32, 32,  1,
        1,  4,  4,  8, 32, 32,  1,  1,
        5,  4,  8, 32, 32,  1,  1,  1,
        1,  8, 32, 32,  1,  1,  1,  1,
        5, 32, 32,  1,  1,  1,  1,  1,
       20, 32,  1,  1,  1,  1,  1,  1,
       28,  1,  1,  1,  1,  1,  1,  1
    };



  static const uint8_t matrix_181[64] = {
        8,  1,  1,  5,  1,  5, 18, 27,
        1,  1,  4,  4,  8, 32, 32,  1,
        1,  4,  4,  8, 32, 32,  1,  1,
        5,  4,  8, 32, 32,  1,  1,  1,
        1,  8, 32, 32,  1,  1,  1,  1,
        5, 32, 32,  1,  1,  1,  1,  1,
       18, 32,  1,  1,  1,  1,  1,  1,
       27,  1,  1,  1,  1,  1,  1,  1
    };



  static const uint8_t matrix_182[64] = {
        8,  1,  1,  5,  1,  3, 16, 27,
        1,  1,  4,  4,  8, 32, 32,  1,
        1,  4,  4,  8, 32, 32,  1,  1,
        5,  4,  8, 32, 32,  1,  1,  1,
        1,  8, 32, 32,  1,  1,  1,  1,
        3, 32, 32,  1,  1,  1,  1,  1,
       16, 32,  1,  1,  1,  1,  1,  1,
       27,  1,  1,  1,  1,  1,  1,  1
    };


  static const uint8_t matrix_183[64] = {
        8,  3,  3,  5,  3,  3, 16, 27,
        3,  3,  4,  4,  8, 32, 32,  3,
        3,  4,  4,  8, 32, 32,  3,  3,
        5,  4,  8, 32, 32,  3,  3,  3,
        3,  8, 32, 32,  3,  3,  3,  3,
        3, 32, 32,  3,  3,  3,  3,  3,
       16, 32,  3,  3,  3,  3,  3,  3,
       27,  3,  3,  3,  3,  3,  3,  3
    };


  static const uint8_t matrix_184[64] = {
        8,  3,  3,  5,  3,  3, 14, 24,
        3,  3,  4,  4,  9, 32, 32,  3,
        3,  4,  4,  9, 32, 32,  3,  3,
        5,  4,  9, 32, 32,  3,  3,  3,
        3,  9, 32, 32,  3,  3,  3,  3,
        3, 32, 32,  3,  3,  3,  3,  3,
       14, 32,  3,  3,  3,  3,  3,  3,
       24,  3,  3,  3,  3,  3,  3,  3
    };



  static const uint8_t matrix_185[64] = {
        8,  3,  3,  5,  3,  3, 11, 22,
        3,  3,  4,  4,  9, 32, 32,  3,
        3,  4,  4,  9, 32, 32,  3,  3,
        5,  4,  9, 32, 32,  3,  3,  3,
        3,  9, 32, 32,  3,  3,  3,  3,
        3, 32, 32,  3,  3,  3,  3,  3,
       11, 32,  3,  3,  3,  3,  3,  3,
       22,  3,  3,  3,  3,  3,  3,  3
    };



  static const uint8_t matrix_186[64] = {
        8,  3,  3,  5,  3,  8, 11, 20,
        3,  3,  4,  4,  9, 32, 32,  3,
        3,  4,  4,  9, 32, 32,  3,  3,
        5,  4,  9, 32, 32,  3,  3,  3,
        3,  9, 32, 32,  3,  3,  3,  3,
        8, 32, 32,  3,  3,  3,  3,  3,
       11, 32,  3,  3,  3,  3,  3,  3,
       20,  3,  3,  3,  3,  3,  3,  3
    };



  static const uint8_t matrix_187[64] = {
        8,  3,  3,  5,  3,  8, 11, 16,
        3,  3,  4,  4,  9, 32, 32,  3,
        3,  4,  4,  9, 32, 32,  3,  3,
        5,  4,  9, 32, 32,  3,  3,  3,
        3,  9, 32, 32,  3,  3,  3,  3,
        8, 32, 32,  3,  3,  3,  3,  3,
       11, 32,  3,  3,  3,  3,  3,  3,
       16,  3,  3,  3,  3,  3,  3,  3
    };



  static const uint8_t matrix_188[64] = {
        8,  3,  3,  5,  3,  8, 11, 12,
        3,  3,  4,  4,  9, 32, 32,  3,
        3,  4,  4,  9, 32, 32,  3,  3,
        5,  4,  9, 32, 32,  3,  3,  3,
        3,  9, 32, 32,  3,  3,  3,  3,
        8, 32, 32,  3,  3,  3,  3,  3,
       11, 32,  3,  3,  3,  3,  3,  3,
       12,  3,  3,  3,  3,  3,  3,  3
    };




  static const uint8_t matrix_189[64] = {
        8,  3,  3,  5,  4,  9, 16, 24,
        3,  3,  4,  4,  9, 32, 32,  3,
        3,  4,  4,  9, 32, 32,  3,  3,
        5,  4,  9, 32, 32,  3,  3,  3,
        4,  9, 32, 32,  3,  3,  3,  3,
        9, 32, 32,  3,  3,  3,  3,  3,
       16, 32,  3,  3,  3,  3,  3,  3,
       24,  3,  3,  3,  3,  3,  3,  3
    };
  */



  //#######################################################  START TEST MATIRCIES



  //static const uint8_t matrix_1[64] = {      // even
  //      5,  5,  5,  6,  7,  8,  9, 10,
  //      5,  6,  6,  7,  8,  9, 10, 10,
  //      5,  6,  7,  8,  9, 10, 10, 10,
  //      6,  7,  8,  9, 10, 10, 10, 10,
  //      7,  8,  9, 10, 10, 10, 10, 11,
  //      8,  9, 10, 10, 10, 10, 11, 11,
  //      9, 10, 10, 10, 10, 11, 11, 11,
  //     10, 10, 10, 10, 11, 11, 11, 11
  //  };



  //static const uint8_t matrix_180[64] = {
  //      240,  240,  240, 240,  240,  240, 240, 240,
  //      1,   1,  1,  1,  1,  1,  1,  1,
  //      1,   1,  1,  1,  1,  1,  1,  1,
  //      1,   1,  1,  1,  1,  1,  1,  1,
  //      1,   1,  1,  1,  1,  1,  1,  1,
  //      1,   1,  1,  1,  1,  1,  1,  1,
  //      1,   1,  1,  1,  1,  1,  1,  1,
  //      1,   1,  1,  1,  1,  1,  1,  1
  //  };

  //// matrix_num 2
  //static const uint8_t matrix_8[64] = {
  //      8,  8,  8, 10, 10, 26, 30, 39,
  //      8,  9, 11, 13, 15, 16, 18, 20,
  //      8, 11, 13, 16, 17, 19, 21, 22,
  //     10, 13, 16, 18, 20, 22, 24, 26,
  //     10, 15, 17, 20, 23, 25, 28, 30,
  //     26, 16, 19, 22, 25, 29, 34, 38,
  //     30, 18, 21, 24, 28, 34, 46, 52,
  //     39, 20, 22, 26, 30, 38, 52, 72
  //  };

  //static const uint8_t matrix_181[64] = {      // even
  //     63, 12, 24,  6,  7,  8,  9, 10,
  //     12, 24,  6,  7,  8,  9, 10, 10,
  //     24,  6,  7,  8,  9, 10, 10, 10,
  //      6,  7,  8,  9, 10, 10, 10, 10,
  //      7,  8,  9, 10, 10, 10, 10, 11,
  //      8,  9, 10, 10, 10, 10, 11, 11,
  //      9, 10, 10, 10, 10, 11, 11, 11,
  //     10, 10, 10, 10, 11, 11, 11, 11
  //  };


  static const uint8_t Flat_8[64] = {
       8,  8,  8,  8,  8,  8,  8,  8,
       8,  8,  8,  8,  8,  8,  8,  8,
       8,  8,  8,  8,  8,  8,  8,  8,
       8,  8,  8,  8,  8,  8,  8,  8,
       8,  8,  8,  8,  8,  8,  8,  8,
       8,  8,  8,  8,  8,  8,  8,  8,
       8,  8,  8,  8,  8,  8,  8,  8,
       8,  8,  8,  8,  8,  8,  8,  8
  };




  static const uint8_t matrix_180[64] = {      // even
        245,  4,  5,  6,  7,  8,  9, 10,
        4,  6,  6,  7,  8,  9, 10, 10,
        5,  6,  7,  8,  9, 10, 10, 10,
        6,  7,  8,  9, 10, 10, 10, 10,
        7,  8,  9, 10, 10, 10, 10, 11,
        8,  9, 10, 10, 10, 10, 11, 11,
        9, 10, 10, 10, 10, 11, 11, 11,
       10, 10, 10, 10, 11, 11, 11, 11
  };






  // old matrix_num 9
  static const uint8_t matrix_181[64] = {
         2,   1,   2,   8,  16,  32,  64, 128,
         1,   3,   9,  14,  24,  48,  96, 128,
         2,   9,  12,  16,  32,  64,  80, 128,
         8,  14,  16,  18,  48,  72,  92, 128,
        16,  24,  32,  48,  80,  98, 100, 128,
        32,  48,  64,  72,  98, 100, 140, 225,
        64,  96,  80,  92, 100, 140, 225, 225,
       128, 128, 128, 128, 128, 225, 225, 225
  };


  static const uint8_t matrix_182[64] = {      // even
       1, 1, 25, 35, 80, 98,127,137,
       1, 20, 25, 80, 98,127,137,137,
       25, 25, 80, 98,127,137,137,137,
       35, 80, 98,127,137,137,137,137,
       80, 98,127,137,137,137,137,142,
       98,127,137,137,137,137,142,142,
      127,137,137,137,137,142,142,142,
      137,137,137,137,142,142,142,142
  };

  //static const uint8_t matrix_182[64] = {      // even
  //     16, 25, 35, 45, 80, 98,127,137,
  //     25, 20, 25, 80, 98,127,137,137,
  //     35, 25, 80, 98,127,137,137,137,
  //     45, 80, 98,127,137,137,137,137,
  //     80, 98,127,137,137,137,137,142,
  //     98,127,137,137,137,137,142,142,
  //    127,137,137,137,137,142,142,142,
  //    137,137,137,137,142,142,142,142
  //  };




  /*
  static const uint8_t matrix_181[64] = {      // even
        127,  5,  5,  6,  7,  8,  9, 10,
        5,  6,  6,  7,  8,  9, 10, 10,
        5,  6,  7,  8,  9, 10, 10, 10,
        6,  7,  8,  9, 10, 10, 10, 10,
        7,  8,  9, 10, 10, 10, 10, 11,
        8,  9, 10, 10, 10, 10, 11, 11,
        9, 10, 10, 10, 10, 11, 11, 11,
       10, 10, 10, 10, 11, 11, 11, 11
    };


  static const uint8_t matrix_182[64] = {      // even
        254,  5,  5,  6,  7,  8,  9, 10,
        5,  6,  6,  7,  8,  9, 10, 10,
        5,  6,  7,  8,  9, 10, 10, 10,
        6,  7,  8,  9, 10, 10, 10, 10,
        7,  8,  9, 10, 10, 10, 10, 11,
        8,  9, 10, 10, 10, 10, 11, 11,
        9, 10, 10, 10, 10, 11, 11, 11,
       10, 10, 10, 10, 11, 11, 11, 11
    };

  */


  //  // matrix 3
  //static const uint8_t matrix_181[64] = {
  //       8,  8,  8, 12,  8, 18, 19, 12,
  //       8,  8,  9, 14, 28, 48, 78, 96,
  //       8,  9, 24, 25, 40, 64, 80, 96,
  //      12, 14, 25, 40, 58, 76, 96, 96,
  //       8, 28, 40, 58, 80, 98, 96, 96,
  //      18, 48, 64, 76, 98, 96, 96, 96,
  //      19, 78, 80, 96, 96, 96, 96, 96,
  //      12, 96, 96, 96, 96, 96, 96, 96
  //  };




  //  matrix 4
  //static const uint8_t matrix_182[64] = {
  //       8,  8,  8, 12,  9, 22, 30, 30,
  //       8,  8,  9, 14, 28, 48, 78, 96,
  //       8,  9, 24, 25, 40, 64, 80, 96,
  //      12, 14, 25, 40, 58, 76, 96, 96,
  //       9, 28, 40, 58, 80, 98, 96, 96,
  //      22, 48, 64, 76, 98, 96, 96, 96,
  //      30, 78, 80, 96, 96, 96, 96, 96,
  //      30, 96, 96, 96, 96, 96, 96, 96
  //  };

    // matrix_num 9 Very strong edge smoothing OLD
  static const uint8_t matrix_183[64] = {
         2,   2,   2,   8,  16,  32,  64, 128,
         2,   2,   9,  14,  28,  48,  96, 128,
         2,   9,  14,  25,  40,  64,  80, 128,
         8,  14,  25,  40,  58,  76,  92, 128,
        16,  28,  40,  58,  80,  98, 100, 128,
        32,  48,  64,  76,  98, 100, 140, 255,
        64,  96,  80,  92, 100, 140, 255, 255,
       128, 128, 128, 128, 128, 255, 255, 255
  };


  static const uint8_t matrix_184[64] = {
        8,  3,  3,  5,  3,  3, 14, 24,
        3,  3,  4,  4,  9, 64, 64,  3,
        3,  4,  4,  9, 64, 64,  3,  3,
        5,  4,  9, 64, 64,  3,  3,  3,
        3,  9, 64, 64,  3,  3,  3,  3,
        3, 64, 64,  3,  3,  3,  3,  3,
       14, 64,  3,  3,  3,  3,  3,  3,
       24,  3,  3,  3,  3,  3,  3,  3
  };


  //static const uint8_t matrix_181[64] = {
  //      240,  240,  240, 240,  240,  240, 240, 240,
  //      240,  240,  1,  1,  1,  1,  1,  1,
  //      240,   1,  1,  1,  1,  1,  1,  1,
  //      240,   1,  1,  1,  1,  1,  1,  1,
  //      240,   1,  1,  1,  1,  1,  1,  1,
  //      240,   1,  1,  1,  1,  1,  1,  1,
  //      240,   1,  1,  1,  1,  1,  1,  1,
  //      240,   1,  1,  1,  1,  1,  1,  1
  //  };

  //static const uint8_t matrix_182[64] = {
  //      240,  240,  140, 240,  240,  240, 240, 240,
  //      240,  240, 240,  240,  1,  1,  1,  1,
  //      240,  240, 240,  1,  1,  1,  1,  1,
  //      240,  240,  1,  1,  1,  1,  1,  1,
  //      240,   1,  1,  1,  1,  1,  1,  1,
  //      240,   1,  1,  1,  1,  1,  1,  1,
  //      240,   1,  1,  1,  1,  1,  1,  1,
  //      240,   1,  1,  1,  1,  1,  1,  1
  //  };

  //static const uint8_t matrix_182[64] = {
  //      8,  8,  12, 64,  64,  64, 64, 64,
  //      8,  16, 32, 32,  1,  1,  1,  1,
  //      12, 32, 32,  1,  1,  1,  1,  1,
  //      32,  32,  1,  1,  1,  1,  1,  1,
  //      64,  1,  1,  1,  1,  1,  1,  1,
  //      64,  1,  1,  1,  1,  1,  1,  1,
  //      64,  1,  1,  1,  1,  1,  1,  1,
  //      64,  1,  1,  1,  1,  1,  1,  1
  //  };
  //
  //
  //static const uint8_t matrix_183[64] = {
  //      240,  240,  140, 240,  240,  240, 240, 240,
  //      240,  240,  64,  1,  1,  1,  1,  1,
  //      140,   64,  1,  1,  1,  1,  1,  1,
  //      240,   1,  1,  1,  1,  1,  1,  1,
  //      240,   1,  1,  1,  1,  1,  1,  1,
  //      240,   1,  1,  1,  1,  1,  1,  1,
  //      240,   1,  1,  1,  1,  1,  1,  1,
  //      240,   1,  1,  1,  1,  1,  1,  1
  //  };


  // matrix_num 9 Very strong edge smoothing OLD
  //static const uint8_t matrix_183[64] = {
  //       2,   2,   2,   8,  16,  32,  64, 128,
  //       2,   2,   9,  14,  28,  48,  96, 128,
  //       2,   9,  14,  25,  40,  64,  80, 128,
  //       8,  14,  25,  40,  58,  76,  92, 128,
  //      16,  28,  40,  58,  80,  98, 100, 128,
  //      32,  48,  64,  76,  98, 100, 140, 255,
  //      64,  96,  80,  92, 100, 140, 255, 255,
  //     128, 128, 128, 128, 128, 255, 255, 255
  //  };



  /*
  static const uint8_t matrix_181[64] = {
        4,  1,  1,  5,  1,   5,  18, 27,
        1,  1,  4,  4,  8, 128, 128,  1,
        1,  4,  4,  8, 128, 128,  1,  1,
        5,  4,  8, 128, 128,  1,  1,  1,
        1,  8, 128, 128,  1,  1,  1,  1,
        5, 128, 128,  1,  1,  1,  1,  1,
       18, 128,  1,  1,  1,  1,  1,  1,
       27,  1,  1,  1,  1,  1,  1,  1
    };



  static const uint8_t matrix_182[64] = {
        8,  1,  1,  5,  1,  3, 16, 27,
        1,  1,  4,  4,  8, 128, 128,  1,
        1,  4,  4,  8, 128, 128,  1,  1,
        5,  4,  8, 128, 128,  1,  1,  1,
        1,  8, 128, 128,  1,  1,  1,  1,
        3, 128, 128,  1,  1,  1,  1,  1,
       16, 128,  1,  1,  1,  1,  1,  1,
       27,  1,  1,  1,  1,  1,  1,  1
    };


  static const uint8_t matrix_183[64] = {
       16,  3,  3,  5,  3,  3, 16, 27,
        3,  3,  4,  4,  8, 64, 64,  3,
        3,  4,  4,  8, 64, 64,  3,  3,
        5,  4,  8, 64, 64,  3,  3,  3,
        3,  8, 64, 64,  3,  3,  3,  3,
        3, 64, 64,  3,  3,  3,  3,  3,
       16, 64,  3,  3,  3,  3,  3,  3,
       27,  3,  3,  3,  3,  3,  3,  3
    };


  static const uint8_t matrix_184[64] = {
        8,  3,  3,  5,  3,  3, 14, 24,
        3,  3,  4,  4,  9, 64, 64,  3,
        3,  4,  4,  9, 64, 64,  3,  3,
        5,  4,  9, 64, 64,  3,  3,  3,
        3,  9, 64, 64,  3,  3,  3,  3,
        3, 64, 64,  3,  3,  3,  3,  3,
       14, 64,  3,  3,  3,  3,  3,  3,
       24,  3,  3,  3,  3,  3,  3,  3
    };

  */

  static const uint8_t matrix_185[64] = {
        8,  3,  3,  5,  3,  3, 11, 22,
        3,  3,  4,  4,  9, 64, 64,  3,
        3,  4,  4,  9, 64, 64,  3,  3,
        5,  4,  9, 64, 64,  3,  3,  3,
        3,  9, 64, 64,  3,  3,  3,  3,
        3, 64, 64,  3,  3,  3,  3,  3,
       11, 64,  3,  3,  3,  3,  3,  3,
       22,  3,  3,  3,  3,  3,  3,  3
  };



  static const uint8_t matrix_186[64] = {
        8,  3,  3,  5,  3,  8, 11, 20,
        3,  3,  4,  4,  9, 64, 64,  3,
        3,  4,  4,  9, 64, 64,  3,  3,
        5,  4,  9, 64, 64,  3,  3,  3,
        3,  9, 64, 64,  3,  3,  3,  3,
        8, 64, 64,  3,  3,  3,  3,  3,
       11, 64,  3,  3,  3,  3,  3,  3,
       20,  3,  3,  3,  3,  3,  3,  3
  };



  static const uint8_t matrix_187[64] = {
        8,  3,  3,  5,  3,  8, 11, 16,
        3,  3,  4,  4,  9, 64, 64,  3,
        3,  4,  4,  9, 64, 64,  3,  3,
        5,  4,  9, 64, 64,  3,  3,  3,
        3,  9, 64, 64,  3,  3,  3,  3,
        8, 64, 64,  3,  3,  3,  3,  3,
       11, 64,  3,  3,  3,  3,  3,  3,
       16,  3,  3,  3,  3,  3,  3,  3
  };



  static const uint8_t matrix_188[64] = {
        8,  3,  3,  5,  3,  8, 11, 12,
        3,  3,  4,  4,  9, 64, 64,  3,
        3,  4,  4,  9, 64, 64,  3,  3,
        5,  4,  9, 64, 64,  3,  3,  3,
        3,  9, 64, 64,  3,  3,  3,  3,
        8, 64, 64,  3,  3,  3,  3,  3,
       11, 64,  3,  3,  3,  3,  3,  3,
       12,  3,  3,  3,  3,  3,  3,  3
  };




  static const uint8_t matrix_189[64] = {
        8,  3,  3,  5,  4,  9, 16, 24,
        3,  3,  4,  4,  9, 64, 64,  3,
        3,  4,  4,  9, 64, 64,  3,  3,
        5,  4,  9, 64, 64,  3,  3,  3,
        4,  9, 64, 64,  3,  3,  3,  3,
        9, 64, 64,  3,  3,  3,  3,  3,
       16, 64,  3,  3,  3,  3,  3,  3,
       24,  3,  3,  3,  3,  3,  3,  3
  };


  //#######################################################  END TEST MATIRCIES


  // matrix_2. Used in the presmoother section of amDCT.c
  // Smooths areas that are being sharpened more than other areas
  static const uint8_t matrix_2XXX[64] = {
        8,  8,  8,  8,  8, 32, 32, 38,
        8,  8,  8,  8, 32, 32, 32,  8,
        8,  8,  8, 32, 32, 32,  8,  8,
        8,  8, 32, 32, 32,  8,  8,  8,
        8, 32, 32, 32,  8,  8,  8,  8,
       32, 32, 32,  8,  8,  8,  8,  8,
       32, 32,  8,  8,  8,  8,  8,  8,
       38,  8,  8,  8,  8,  8,  8,  8
  };



  // matrix_num 8 Strong horizontal and vertical line smoothing MATRIX 6
  static const uint8_t matrix_190[64] = {
         8,   8,   8,   8,  16,  32,  64, 90,
         8,   8,   9,  14,  28,  48,  78, 96,
         8,   9,  24,  25,  40,  64,  80, 96,
         8,  14,  25,  40,  58,  76,  96, 96,
        16,  28,  40,  58,  80,  98,  96, 96,
        32,  48,  64,  76,  98,  96,  96, 96,
        64,  78,  80,  96,  96,  96,  96, 96,
        90,  96,  96,  96,  96,  96,  96, 96
  };


  // matrix_num 8 Strong horizontal and vertical line smoothing
  static const uint8_t matrix_191[64] = {
         8,   8,   8,   8,   8,  28,  30, 30,
         8,   8,   9,  14,  28,  48,  78, 96,
         8,   9,  24,  25,  40,  64,  80, 96,
         8,  14,  25,  40,  58,  76,  96, 96,
         8,  28,  40,  58,  80,  98,  96, 96,
        28,  48,  64,  76,  98,  96,  96, 96,
        30,  78,  80,  96,  96,  96,  96, 96,
        30,  96,  96,  96,  96,  96,  96, 96
  };


  // matrix_num 8 Strong horizontal and vertical line smoothing
  static const uint8_t matrix_192[64] = {
         8,   8,   8,   8,   8,  18,  24, 26,
         8,   8,   9,  14,  28,  48,  78, 96,
         8,   9,  24,  25,  40,  64,  80, 96,
         8,  14,  25,  40,  58,  76,  96, 96,
         8,  28,  40,  58,  80,  98,  96, 96,
        18,  48,  64,  76,  98,  96,  96, 96,
        24,  78,  80,  96,  96,  96,  96, 96,
        26,  96,  96,  96,  96,  96,  96, 96
  };


  // matrix_num 8 Strong horizontal and vertical line smoothing
  static const uint8_t matrix_193[64] = {
         8,   8,   8,   8,   8,  18,  22, 16,
         8,   8,   9,  14,  28,  48,  78, 96,
         8,   9,  24,  25,  40,  64,  80, 96,
         8,  14,  25,  40,  58,  76,  96, 96,
         8,  28,  40,  58,  80,  98,  96, 96,
        18,  48,  64,  76,  98,  96,  96, 96,
        22,  78,  80,  96,  96,  96,  96, 96,
        16,  96,  96,  96,  96,  96,  96, 96
  };


  // matrix_num 8 Strong horizontal and vertical line smoothing
  static const uint8_t matrix_194[64] = {
         8,   8,   8,  12,   8,  18,  19, 12,
         8,   8,   9,  14,  28,  48,  78, 96,
         8,   9,  24,  25,  40,  64,  80, 96,
        12,  14,  25,  40,  58,  76,  96, 96,
         8,  28,  40,  58,  80,  98,  96, 96,
        18,  48,  64,  76,  98,  96,  96, 96,
        19,  78,  80,  96,  96,  96,  96, 96,
        12,  96,  96,  96,  96,  96,  96, 96
  };


  // matrix_num 9 Strong horizontal and vertical line smoothing
  static const uint8_t matrix_195[64] = {
         2,   2,   2,   8,  16,  32,  64, 128,
         2,   2,   9,  14,  28,  48,  96, 128,
         2,   9,  14,  25,  40,  64,  80, 128,
         8,  14,  25,  40,  58,  76,  92, 128,
        16,  28,  40,  58,  80,  98, 100, 128,
        32,  48,  64,  76,  98, 100, 140, 255,
        64,  96,  80,  92, 100, 140, 255, 255,
       128, 128, 128, 128, 128, 255, 255, 255
  };


  // matrix_num 9 Strong horizontal and vertical line smoothing
  static const uint8_t matrix_196[64] = {
         32,   2,   2,   8,  16,  32,  64, 128,
         2,   2,   9,  14,  28,  48,  96, 128,
         2,   9,  14,  25,  40,  64,  80, 128,
         8,  14,  25,  40,  58,  76,  92, 128,
        16,  28,  40,  58,  80,  98, 100, 128,
        32,  48,  64,  76,  98, 100, 140, 255,
        64,  96,  80,  92, 100, 140, 255, 255,
       128, 128, 128, 128, 128, 255, 255, 255
  };



  // matrix_num 9 Strong horizontal and vertical line smoothing
  //static const uint8_t matrix_196[64] = {
  //       2,   2,   2,  12,   8,  18,  19,  12,
  //       2,   2,   9,  14,  28,  48,  96, 128,
  //       2,   9,  14,  25,  40,  64,  80, 128,
  //      12,  14,  25,  40,  58,  76,  92, 128,
  //       8,  28,  40,  58,  80,  98, 100, 128,
  //      18,  48,  64,  76,  98, 100, 140, 255,
  //      19,  96,  80,  92, 100, 140, 255, 255,
  //      12, 128, 128, 128, 128, 255, 255, 255
  //  };

  // matrix_num 9 Strong horizontal and vertical line smoothing
  static const uint8_t matrix_197[64] = {
         2,   2,   2,  12,   8,  15,  14,  13,
         2,   2,   9,  14,  28,  48,  96, 128,
         2,   9,  14,  25,  40,  64,  80, 128,
        12,  14,  25,  40,  58,  76,  92, 128,
         8,  28,  40,  58,  80,  98, 100, 128,
        15,  48,  64,  76,  98, 100, 140, 255,
        14,  96,  80,  92, 100, 140, 255, 255,
        13, 128, 128, 128, 128, 255, 255, 255
  };

  // matrix_num 9 Strong horizontal and vertical line smoothing
  static const uint8_t matrix_198[64] = {
         2,   2,   2,  12,   8,  17,  17,  12,
         2,   2,   9,  14,  28,  48,  96, 128,
         2,   9,  14,  25,  40,  64,  80, 128,
        12,  14,  25,  40,  58,  76,  92, 128,
         8,  28,  40,  58,  80,  98, 100, 128,
        17,  48,  64,  76,  98, 100, 140, 255,
        17,  96,  80,  92, 100, 140, 255, 255,
        12, 128, 128, 128, 128, 255, 255, 255
  };

  // matrix_num 9 Strong horizontal and vertical line smoothing
  static const uint8_t matrix_199[64] = {
         2,   2,   2,  10,   8,  12,  11,  10,
         2,   2,   9,  14,  28,  48,  96, 128,
         2,   9,  14,  25,  40,  64,  80, 128,
        10,  14,  25,  40,  58,  76,  92, 128,
         8,  28,  40,  58,  80,  98, 100, 128,
        12,  48,  64,  76,  98, 100, 140, 255,
        11,  96,  80,  92, 100, 140, 255, 255,
        10, 128, 128, 128, 128, 255, 255, 255
  };

  // matrix_num 9 Strong horizontal and vertical line smoothing
  static const uint8_t matrix_200[64] = {
         2,   2,   2,  11,   4,  12,  11,  12,
         2,   2,   9,  14,  28,  48,  96, 128,
         2,   9,  14,  25,  40,  64,  80, 128,
        11,  14,  25,  40,  58,  76,  92, 128,
         4,  28,  40,  58,  80,  98, 100, 128,
        12,  48,  64,  76,  98, 100, 140, 255,
        11,  96,  80,  92, 100, 140, 255, 255,
        12, 128, 128, 128, 128, 255, 255, 255
  };

  // matrix_num 9 Strong horizontal and vertical line smoothing
  static const uint8_t matrix_201[64] = {
         8,   3,   4,   9,   6,  22,  28,  15,
         3,   2,   9,  14,  28,  48,  96, 128,
         4,   9,  14,  25,  40,  64,  80, 128,
         9,  14,  25,  40,  58,  76,  92, 128,
         6,  28,  40,  58,  80,  98, 100, 128,
        22,  48,  64,  76,  98, 100, 140, 255,
        28,  96,  80,  92, 100, 140, 255, 255,
        15, 128, 128, 128, 128, 255, 255, 255
  };

  //  matrix 4  does less smoothing of fine lines that matrix 5.
  //static const uint8_t matrix_229[64] = {
  //      8,   3,  4,  9,  6, 22, 28, 15,
  //      3,   1,  1,  1,  1,  1,  1,  1,
  //      4,   1,  1,  1,  1,  1,  1,  1,
  //      9,   1,  1,  1,  1,  1,  1,  1,
  //      6,   1,  1,  1,  1,  1,  1,  1,
  //     22,   1,  1,  1,  1,  1,  1,  1,
  //     28,   1,  1,  1,  1,  1,  1,  1,
  //     15,   1,  1,  1,  1,  1,  1,  1
  //  };



  // matrix_num 9 Strong horizontal and vertical line smoothing
  static const uint8_t matrix_9XXX[64] = {
         2,   2,   2,  11,   4,  12,  12,  12,
         2,   2,   9,  14,  28,  48,  96, 128,
         2,   9,  14,  25,  40,  64,  80, 128,
        11,  14,  25,  40,  58,  76,  92, 128,
         4,  28,  40,  58,  80,  98, 100, 128,
        12,  48,  64,  76,  98, 100, 140, 255,
        12,  96,  80,  92, 100, 140, 255, 255,
        12, 128, 128, 128, 128, 255, 255, 255
  };



  //  matrix 4  does less smoothing of fine lines that matrix 5. ORIGINAL
  static const uint8_t matrix_220XXX[64] = {
        6,  7,  8, 10, 13, 13, 14, 14,
        7,  9, 11, 13, 14, 16, 14, 14,
        8, 11, 13, 16, 16, 14, 14, 14,
       10, 13, 16, 17, 14, 14, 14, 14,
       13, 14, 16, 14, 14, 14, 14, 14,
       13, 15, 14, 14, 14, 14, 14, 14,
       14, 14, 14, 14, 14, 14, 14, 14,
       14, 14, 14, 14, 14, 14, 14, 14
  };

  //  matrix 4  does less smoothing of fine lines that matrix 5.
  static const uint8_t matrix_220[64] = {
       1,  1,  1,  1,  1,  1,  1,  1,
       1,  1,  1,  1,  1,  1,  1,  1,
       1,  1,  1,  1,  1,  1,  1,  1,
       1,  1,  1,  1,  1,  1,  1,  1,
       1,  1,  1,  1,  1,  1,  1,  1,
       1,  1,  1,  1,  1,  1,  1,  1,
       1,  1,  1,  1,  1,  1,  1,  1,
       1,  1,  1,  1,  1,  1,  1,  1
  };



  //  matrix 4  does less smoothing of fine lines that matrix 5.
  static const uint8_t matrix_221[64] = {
        8,   1,  1,  1,  1,  1,  1,  1,
        3,   1,  1,  1,  1,  1,  1,  1,
        3,   1,  1,  1,  1,  1,  1,  1,
        6,   1,  1,  1,  1,  1,  1,  1,
        6,   1,  1,  1,  1,  1,  1,  1,
       24,   1,  1,  1,  1,  1,  1,  1,
       28,   1,  1,  1,  1,  1,  1,  1,
       26,   1,  1,  1,  1,  1,  1,  1
  };


  //  matrix 4  does less smoothing of fine lines that matrix 5.
  static const uint8_t matrix_222[64] = {
        8,   1,  1,  1,  1,  1,  1,  1,
        3,   1,  1,  1,  1,  1,  1,  1,
        3,   1,  1,  1,  1,  1,  1,  1,
        6,   1,  1,  1,  1,  1,  1,  1,
        6,   1,  1,  1,  1,  1,  1,  1,
       24,   1,  1,  1,  1,  1,  1,  1,
       26,   1,  1,  1,  1,  1,  1,  1,
       32,   1,  1,  1,  1,  1,  1,  1
  };





  //  matrix 4  does less smoothing of fine lines that matrix 5.
  static const uint8_t matrix_223[64] = {
        8,   1,  1,  1,  1,  1,  1,  1,
        3,   1,  1,  1,  1,  1,  1,  1,
        3,   1,  1,  1,  1,  1,  1,  1,
        8,   1,  1,  1,  1,  1,  1,  1,
        6,   1,  1,  1,  1,  1,  1,  1,
       18,   1,  1,  1,  1,  1,  1,  1,
       26,   1,  1,  1,  1,  1,  1,  1,
       38,   1,  1,  1,  1,  1,  1,  1
  };




  //  matrix 4  does less smoothing of fine lines that matrix 5.
  static const uint8_t matrix_224[64] = {
        8,   1,  1,  1,  1,  1,  1,  1,
        3,   1,  1,  1,  1,  1,  1,  1,
        3,   1,  1,  1,  1,  1,  1,  1,
        8,   1,  1,  1,  1,  1,  1,  1,
        6,   1,  1,  1,  1,  1,  1,  1,
       18,   1,  1,  1,  1,  1,  1,  1,
       24,   1,  1,  1,  1,  1,  1,  1,
       12,   1,  1,  1,  1,  1,  1,  1  // 34  making 6 removed large gap thin lines. in frame 1169 // 12 better than 6
  };





  //  matrix 4  does less smoothing of fine lines that matrix 5.
  static const uint8_t matrix_225[64] = {
        8,   1,  1,  1,  1,  1,  1,  1,
        3,   1,  1,  1,  1,  1,  1,  1,
        3,   1,  1,  1,  1,  1,  1,  1,
        8,   1,  1,  1,  1,  1,  1,  1,  // 12 might be a bit better. doesn't seem to make much difference
        6,   1,  1,  1,  1,  1,  1,  1,
       22,   1,  1,  1,  1,  1,  1,  1,
       28,   1,  1,  1,  1,  1,  1,  1,
       12,   1,  1,  1,  1,  1,  1,  1  // 12 better than 6
  };







  //  matrix 4  does less smoothing of fine lines that matrix 5.
  static const uint8_t matrix_226[64] = {
        8,   1,  1,  1,  1,  1,  1,  1,
        3,   1,  1,  1,  1,  1,  1,  1,
        4,   1,  1,  1,  1,  1,  1,  1,
        9,   1,  1,  1,  1,  1,  1,  1,  // 12 might be a bit better. doesn't seem to make much difference
        6,   1,  1,  1,  1,  1,  1,  1,
       22,   1,  1,  1,  1,  1,  1,  1,
       29,   1,  1,  1,  1,  1,  1,  1,
       15,   1,  1,  1,  1,  1,  1,  1  // 14 better than 12
  };





  //  matrix 4  does less smoothing of fine lines that matrix 5.
  static const uint8_t matrix_227[64] = {
        8,   1,  1,  1,  1,  1,  1,  1,
        3,   1,  1,  1,  1,  1,  1,  1,
        4,   1,  1,  1,  1,  1,  1,  1,
        9,   1,  1,  1,  1,  1,  1,  1,
        6,   1,  1,  1,  1,  1,  1,  1,
       22,   1,  1,  1,  1,  1,  1,  1,
       28,   1,  1,  1,  1,  1,  1,  1,
       15,   1,  1,  1,  1,  1,  1,  1
  };







  //  matrix 4  does less smoothing of fine lines that matrix 5.
  static const uint8_t matrix_228[64] = {
        8,   1,  1,  1,  1,  1,  1,  1,
        3,   1,  1,  1,  1,  1,  1,  1,
        6,   1,  1,  1,  1,  1,  1,  1,
        9,   1,  1,  1,  1,  1,  1,  1,
       13,   1,  1,  1,  1,  1,  1,  1,
       17,   1,  1,  1,  1,  1,  1,  1,
       21,   1,  1,  1,  1,  1,  1,  1,
       25,   1,  1,  1,  1,  1,  1,  1
  };




  //  matrix 4  does less smoothing of fine lines that matrix 5.
  static const uint8_t matrix_229[64] = {
        8,   3,  4,  9,  6, 22, 28, 15,
        3,   1,  1,  1,  1,  1,  1,  1,
        4,   1,  1,  1,  1,  1,  1,  1,
        9,   1,  1,  1,  1,  1,  1,  1,
        6,   1,  1,  1,  1,  1,  1,  1,
       22,   1,  1,  1,  1,  1,  1,  1,
       28,   1,  1,  1,  1,  1,  1,  1,
       15,   1,  1,  1,  1,  1,  1,  1
  };







  //// matrix_num 5   medium smoothing.  More smoothing on large details "low frequencies"
  static const uint8_t matrix_230[64] = {
        6,  7,  8, 10, 13, 14, 16, 18,
        7,  9, 11, 13, 15, 16, 18, 20,
        8, 11, 13, 16, 17, 19, 21, 22,
       10, 13, 16, 18, 20, 22, 24, 26,
       13, 15, 17, 20, 23, 25, 28, 30,
       14, 16, 19, 22, 25, 29, 34, 38,
       16, 18, 21, 24, 28, 34, 46, 52,
       18, 20, 22, 26, 30, 38, 52, 72
  };




  // matrix_num 5   medium smoothing.  More smoothing on large details "low frequencies"
  static const uint8_t matrix_231[64] = {
        6,  7,  8, 12,  7, 15, 15, 13,
        7,  9, 11, 13, 15, 16, 18, 20,
        8, 11, 13, 16, 17, 19, 21, 22,
       12, 13, 16, 18, 20, 22, 24, 26,
        7, 15, 17, 20, 23, 25, 28, 30,
       15, 16, 19, 22, 25, 29, 34, 38,
       15, 18, 21, 24, 28, 34, 46, 52,
       13, 20, 22, 26, 30, 38, 52, 72
  };




  // matrix_num 5   medium smoothing.  More smoothing on large details "low frequencies"
  static const uint8_t matrix_232[64] = {
        8,  3,  3, 11,  4, 12, 11, 12,
        3,  9, 11, 13, 15, 16, 18, 20,
        3, 11, 13, 16, 17, 19, 21, 22,
       11, 13, 16, 18, 20, 22, 24, 26,
        4, 15, 17, 20, 23, 25, 28, 30,
       12, 16, 19, 22, 25, 29, 34, 38,
       11, 18, 21, 24, 28, 34, 46, 52,
       12, 20, 22, 26, 30, 38, 52, 72
  };






  // matrix_num 5   medium smoothing.  More smoothing on large details "low frequencies"
  static const uint8_t matrix_233[64] = {
        8,  4,  5, 11,  4, 12, 11, 12,
        4,  9, 11, 13, 15, 16, 18, 20,
        5, 11, 13, 16, 17, 19, 21, 22,
       11, 13, 16, 18, 20, 22, 24, 26,
        4, 15, 17, 20, 23, 25, 28, 30,
       12, 16, 19, 22, 25, 29, 34, 38,
       11, 18, 21, 24, 28, 34, 46, 52,
       12, 20, 22, 26, 30, 38, 52, 72
  };







  // matrix_num 5   medium smoothing.  More smoothing on large details "low frequencies"
  static const uint8_t matrix_234[64] = {
        8,  6,  7, 11,  5, 12, 11, 12,
        6,  9, 11, 13, 15, 16, 18, 20,
        7, 11, 13, 16, 17, 19, 21, 22,
       11, 13, 16, 18, 20, 22, 24, 26,
        5, 15, 17, 20, 23, 25, 28, 30,
       12, 16, 19, 22, 25, 29, 34, 38,
       11, 18, 21, 24, 28, 34, 46, 52,
       12, 20, 22, 26, 30, 38, 52, 72
  };









  // matrix_num 5   medium smoothing.  More smoothing on large details "low frequencies"
  static const uint8_t matrix_235[64] = {
        8,  7,  8, 11,  6, 12, 11, 12,
        7,  9, 11, 13, 15, 16, 18, 20,
        8, 11, 13, 16, 17, 19, 21, 22,
       11, 13, 16, 18, 20, 22, 24, 26,
        6, 15, 17, 20, 23, 25, 28, 30,
       12, 16, 19, 22, 25, 29, 34, 38,
       11, 18, 21, 24, 28, 34, 46, 52,
       12, 20, 22, 26, 30, 38, 52, 72
  };






  // matrix_num 5   medium smoothing.  More smoothing on large details "low frequencies"
  static const uint8_t matrix_236[64] = {
        8,  7,  8, 11,  6, 14, 16, 11,
        7,  9, 11, 13, 15, 16, 18, 20,
        8, 11, 13, 16, 17, 19, 21, 22,
       11, 13, 16, 18, 20, 22, 24, 26,
        6, 15, 17, 20, 23, 25, 28, 30,
       14, 16, 19, 22, 25, 29, 34, 38,
       16, 18, 21, 24, 28, 34, 46, 52,
       11, 20, 22, 26, 30, 38, 52, 72
  };






  // matrix_num 5   medium smoothing.  More smoothing on large details "low frequencies"
  static const uint8_t matrix_237[64] = {
        8,  7,  8, 11,  6, 15, 16, 14,
        7,  9, 11, 13, 15, 16, 18, 20,
        8, 11, 13, 16, 17, 19, 21, 22,
       11, 13, 16, 18, 20, 22, 24, 26,
        6, 15, 17, 20, 23, 25, 28, 30,
       15, 16, 19, 22, 25, 29, 34, 38,
       16, 18, 21, 24, 28, 34, 46, 52,
       14, 20, 22, 26, 30, 38, 52, 72
  };




  // matrix_num 5   medium smoothing.  More smoothing on large details "low frequencies"
  static const uint8_t matrix_238[64] = {
        8,  7,  8, 11,  6, 15, 14, 14,
        7,  9, 11, 13, 15, 16, 18, 20,
        8, 11, 13, 16, 17, 19, 21, 22,
       11, 13, 16, 18, 20, 22, 24, 26,
        6, 15, 17, 20, 23, 25, 28, 30,
       15, 16, 19, 22, 25, 29, 34, 38,
       14, 18, 21, 24, 28, 34, 46, 52,
       14, 20, 22, 26, 30, 38, 52, 72
  };



  // matrix_num 5   medium smoothing.  More smoothing on large details "low frequencies"
  static const uint8_t matrix_239[64] = {
        8,  7,  8, 11,  6, 15, 12, 11,
        7,  9, 11, 13, 15, 16, 18, 20,
        8, 11, 13, 16, 17, 19, 21, 22,
       11, 13, 16, 18, 20, 22, 24, 26,
        6, 15, 17, 20, 23, 25, 28, 30,
       15, 16, 19, 22, 25, 29, 34, 38,
       12, 18, 21, 24, 28, 34, 46, 52,
       11, 20, 22, 26, 30, 38, 52, 72
  };



  /*
  ///////////////////////////////////////////////////////////////////////////////
  //
  //    matrices 240-255   RESERVED FOR INTERNAL amDCT USE
  //
  //////////////////////////////////////////////////////////////////////////////
  */

  // matrix_num 5   medium smoothing.  More smoothing on large details "low frequencies"
  static const uint8_t matrix_240[64] = {
        8,  7,  8, 11, 10, 14, 10, 10,
        7,  9, 11, 13, 15, 16, 18, 20,
        8, 11, 13, 16, 17, 19, 21, 22,
       11, 13, 16, 18, 20, 22, 24, 26,
       10, 15, 17, 20, 23, 25, 28, 30,
       14, 16, 19, 22, 25, 29, 34, 38,
       10, 18, 21, 24, 28, 34, 46, 52,
       10, 20, 22, 26, 30, 38, 52, 72
  };




  // matrix_num 5   medium smoothing.  More smoothing on large details "low frequencies"
  static const uint8_t matrix_241[64] = {
        8,  7,  8, 10, 11, 13, 15, 12,
        7,  9, 11, 13, 15, 16, 18, 20,
        8, 11, 13, 16, 17, 19, 21, 22,
       10, 13, 16, 18, 20, 22, 24, 26,
       11, 15, 17, 20, 23, 25, 28, 30,
       13, 16, 19, 22, 25, 29, 34, 38,
       15, 18, 21, 24, 28, 34, 46, 52,
       12, 20, 22, 26, 30, 38, 52, 72
  };











  // matrix_num 5   medium smoothing.  More smoothing on large details "low frequencies"
  //static const uint8_t matrix_242[64] = {
  //      3,  7,  8, 9,  10, 10,  9,  9,
  //      7,  9, 11, 13, 15, 16, 18, 20,
  //      8, 11, 13, 16, 17, 19, 21, 22,
  //      9, 13, 16, 18, 20, 22, 24, 26,
  //     10, 15, 17, 20, 23, 25, 28, 30,
  //     10, 16, 19, 22, 25, 29, 34, 38,
  //      9, 18, 21, 24, 28, 34, 46, 52,
  //      9, 20, 22, 26, 30, 38, 52, 72
  //  };

  // matrix_num 242
  static const uint8_t matrix_242[64] = {
       8,   8,  8,  9,  9,  9,  9, 10,
       8,   8,  9,  9,  9,  9, 10, 10,
       8,   9,  9,  9,  9, 10, 10, 10,
       9,   9,  9,  9, 10, 10, 10, 10,
       9,   9,  9, 10, 10, 10, 10, 11,
       9,   9, 10, 10, 10, 10, 11, 11,
       9,  10, 10, 10, 10, 11, 11, 11,
       10, 10, 10, 10, 11, 11, 11, 11
  };


  /*
  // matrix_num 5   medium smoothing.  More smoothing on large details "low frequencies"
  static const uint8_t matrix_242[64] = {
        3,  7,  8, 9,  10, 10,  9,  9,
        7,  9, 11, 13, 15, 16, 18, 20,
        8, 11, 13, 16, 17, 19, 21, 22,
        9, 13, 16, 18, 20, 22, 24, 26,
       10, 15, 17, 20, 23, 25, 28, 30,
       10, 16, 19, 22, 25, 29, 34, 38,
        9, 18, 21, 24, 28, 34, 46, 52,
        9, 20, 22, 26, 30, 38, 52, 72
    };
  */


  // TEST
  //static const uint8_t matrix_254[64] = {
  //      3,  1,  1,  1,  1,  1,  1,  1,
  //      1,  8,  1,  1,  1,  1,  1,  1,
  //      1,  1, 16,  1,  1,  1,  1,  1,
  //      1,  1,  1, 32,  1,  1,  1,  1,
  //      1,  1,  1,  1, 64,  1,  1,  1,
  //      1,  1,  1,  1,  1, 64,  1,  1,
  //      1,  1,  1,  1,  1,  1, 64,  1,
  //      1,  1,  1,  1,  1,  1,  1, 64
  //  };


  //// TEST
  //static const uint8_t matrix_254[64] = {
  //      6,  7,  8, 10, 13, 14, 16, 18,
  //      7,  1,  1,  1,  1,  1,  1,  1,
  //      8,  1,  1,  1,  1,  1,  1,  1,
  //     10,  1,  1,  1,  1,  1,  1,  1,
  //     13,  1,  1,  1,  1,  1,  1,  1,
  //     14,  1,  1,  1,  1,  1,  1,  1,
  //     16,  1,  1,  1,  1,  1,  1,  1,
  //     18,  1,  1,  1,  1,  1,  1,  1
  //  };






  // matrix_num 5   medium smoothing.  More smoothing on large details "low frequencies"     GOOD
  //static const uint8_t matrix_242[64] = {
  //      8,  7,  8, 10, 11, 12, 13, 14,
  //      7,  9, 11, 13, 15, 16, 18, 20,
  //      8, 11, 13, 16, 17, 19, 21, 22,
  //     10, 13, 16, 18, 20, 22, 24, 26,
  //     11, 15, 17, 20, 23, 25, 28, 30,
  //     12, 16, 19, 22, 25, 29, 34, 38,
  //     13, 18, 21, 24, 28, 34, 46, 52,
  //     14, 20, 22, 26, 30, 38, 52, 72
  //  };







  //// matrix_num 5   medium smoothing.  More smoothing on large details "low frequencies"
  //static const uint8_t matrix_5[64] = {
  //      6,  7,  8, 10, 13, 14, 16, 18,
  //      7,  9, 11, 13, 15, 16, 18, 20,
  //      8, 11, 13, 16, 17, 19, 21, 22,
  //     10, 13, 16, 18, 20, 22, 24, 26,
  //     13, 15, 17, 20, 23, 25, 28, 30,
  //     14, 16, 19, 22, 25, 29, 34, 38,
  //     16, 18, 21, 24, 28, 34, 46, 52,
  //     18, 20, 22, 26, 30, 38, 52, 72
  //  };
  //

  // matrix_num 5   medium smoothing.  More smoothing on large details "low frequencies"
  static const uint8_t matrix_250[64] = {
        3,  3,  3, 18, 88, 88, 88, 88,
        3,  3,  3,  3,  3, 88,  3, 88,
        3,  3, 88,  3, 88,  3, 88,  3,
       18,  3,  3, 88,  3, 88,  3, 88,
       88,  3, 88,  3, 88,  3, 88,  3,
       88, 88,  3, 88,  3, 88,  3, 88,
       88,  3, 88,  3, 88,  3, 88,  3,
       88, 88,  3, 88,  3, 88,  3, 88
  };

  static const uint8_t Flat_8_expand[64] = {
       8,  8,  8,  8,  8,  8,  8,  8,
       8,  8,  8,  8,  8,  8,  8,  8,
       8,  8,  8,  8,  8,  8,  8,  8,
       8,  8,  8,  8,  8,  8,  8,  8,
       8,  8,  8,  8,  8,  8,  8,  8,
       8,  8,  8,  8,  8,  8,  8,  8,
       8,  8,  8,  8,  8,  8,  8,  8,
       8,  8,  8,  8,  8,  8,  8,  8
  };


  // matrix_num 252      was matrix_163
  static const uint8_t matrix_248[64] = {
       8,   8,  8,  8,  8,  8,  8,  8,
       8,   8,  8,  8,  8,  8,  8,  8,
       8,   8,255,  8,  8,  8,  8,  8,
       8,   8,  8,  8,  8,  8,  8,  8,
       8,   8,  8,  8,  8,  8,  8,  8,
       8,   8,  8,  8,  8,  8,  8,  8,
       8,   8,  8,  8,  8,  8,  8,  8,
       8,   8,  8,  8,  8,  8,  8,  8
  };


  // MATRIX_PRESMOOTH
  // matrix_254. Used in the presmoother section of amDCT.c
  // Smooths areas that are being sharpened more than other areas.
  // It is used in the presmoothing pass when doing strong sharpening and range expansion.
  // It is designed to maximally reduce noise that will create artifacts, while doing the minimum amount of smoothing.
  static const uint8_t matrix_249_PreSmooth_TEST[64] = {
       2,  2, 3, 4, 8,12,13,14,
       2,  2, 3, 4, 8, 8, 8, 3,
       3,  3, 5, 8, 8, 8, 3, 3,
       4,  4, 8, 8, 8, 3, 3, 3,
       8,  8, 8, 8, 6, 3, 3, 3,
       12, 8, 8, 3, 3, 3, 3, 3,
       13, 8, 3, 3, 3, 3, 3, 3,
       14, 3, 3, 3, 3, 3, 3, 3
  };

  // Matrix Number 253  INTERNAL MATRIX
  //  This matrix was designed so doing range expansion would produce the least
  //  artifacts, especially when also doing sharpening.
  static const uint8_t matrix_249[64] = {
         8, 14, 16, 17, 18, 18, 24, 32,
        14,  8,  9, 14, 28, 48, 78, 96,
        16,  9, 24, 25, 40, 64, 80, 96,
        17, 14, 25, 40, 58, 76, 96, 96,
        18, 28, 40, 58, 80, 98, 96, 96,
        18, 48, 64, 76, 98, 96, 96, 96,
        24, 78, 80, 96, 96, 96, 96, 96,
        32, 96, 96, 96, 96, 96, 96, 96
  };


  // Matrix Number 253  INTERNAL MATRIX
  //  This matrix was designed so doing range expansion would produce the least
  //  artifacts, especially when also doing sharpening.
  static const uint8_t Expand_Sharp_Strong_253[64] = {
         8, 18, 18, 18, 18, 18, 24, 32,
        18,  8,  9, 14, 28, 48, 78, 96,
        18,  9, 24, 25, 40, 64, 80, 96,
        18, 14, 25, 40, 58, 76, 96, 96,
        18, 28, 40, 58, 80, 98, 96, 96,
        18, 48, 64, 76, 98, 96, 96, 96,
        24, 78, 80, 96, 96, 96, 96, 96,
        32, 96, 96, 96, 96, 96, 96, 96
  };



  static const uint8_t matrix_251[64] = {
        3, 3, 3, 3, 3, 3, 5, 3,
        3, 3, 3, 3, 3, 4, 3, 3,
        3, 3, 3, 4, 4, 3, 3, 3,
        3, 3, 4, 4, 3, 3, 3, 3,
        3, 3, 4, 3, 5, 3, 3, 3,
        3, 4, 3, 3, 3, 3, 3, 3,
        5, 3, 3, 3, 3, 3, 3, 3,
        3, 3, 3, 3, 3, 3, 3, 3
  };



  //static const uint8_t matrix_252[64] = {
  //      6, 3, 3, 3, 3, 3, 5, 3,
  //      3, 3, 3, 3, 3, 4, 3, 3,
  //      3, 3, 3, 4, 4, 3, 3, 3,
  //      3, 3, 4, 4, 3, 3, 3, 3,
  //      3, 3, 4, 3, 5, 3, 3, 3,
  //        3, 4, 3, 3, 3, 3, 3, 3,
  //      5, 3, 3, 3, 3, 3, 3, 3,
  //      3, 3, 3, 3, 3, 3, 3, 3
  //  };



  //  copy of matrix 1
  //static const uint8_t matrix_253[64] = {
  //      6,  7,  8, 10, 13, 13, 14, 14,
  //      7,  9, 11, 13, 14, 16, 14, 14,
  //      8, 11, 13, 16, 16, 14, 14, 14,
  //     10, 13, 16, 17, 14, 14, 14, 14,
  //     13, 14, 16, 14, 14, 14, 14, 14,
  //     13, 15, 14, 14, 14, 14, 14, 14,
  //     14, 14, 14, 14, 14, 14, 14, 14,
  //     14, 14, 14, 14, 14, 14, 14, 14
  //  };

  //  was matrix 1
  static const uint8_t matrix_252[64] = {
        22,  6, 7,  8, 10, 10, 12, 12,
        6,  6,  9, 10, 12, 15, 12, 12,
        7,  9, 10, 14, 15, 12, 12, 12,
        8, 10, 14, 16, 12, 12, 12, 12,
       10, 12, 15, 12, 12, 12, 12, 12,
       10, 15, 12, 12, 12, 12, 12, 12,
       12, 12, 12, 12, 12, 12, 12, 12,
       12, 12, 12, 12, 12, 12, 12, 12
  };

  //  was matrix 1
  //static const uint8_t matrix_253[64] = {
  //      4,  6,  7,  8, 10, 10, 12, 12,
  //      6,  6,  9, 10, 12, 15, 12, 12,
  //      7,  9, 10, 14, 15, 12, 12, 12,
  //      8, 10, 14, 16, 12, 12, 12, 12,
  //     10, 12, 15, 12, 12, 12, 12, 12,
  //     10, 15, 12, 12, 12, 12, 12, 12,
  //     12, 12, 12, 12, 12, 12, 12, 12,
  //     12, 12, 12, 12, 12, 12, 12, 12
  //  };



  //static const uint8_t matrix_253[64] = {
  //      6, 2, 2, 2, 3, 5, 5, 3,
  //      2, 2, 3, 2, 5, 5, 5, 2,
  //      2, 3, 3, 5, 5, 5, 2, 2,
  //      2, 2, 5, 5, 5, 2, 2, 2,
  //      3, 5, 5, 5, 5, 2, 2, 2,
  //        5, 5, 5, 2, 2, 2, 2, 2,
  //      5, 5, 2, 2, 2, 2, 2, 2,
  //      3, 2, 2, 2, 2, 2, 2, 2
  //  };



  //  MATRIX_RANGE_EXPAND   range expand matrix
  static const uint8_t matrix_251_Flat_8_RE[64] = {
       8,  8,  8,  8,  8,  8,  8,  8,
       8,  8,  8,  8,  8,  8,  8,  8,
       8,  8,  8,  8,  8,  8,  8,  8,
       8,  8,  8,  8,  8,  8,  8,  8,
       8,  8,  8,  8,  8,  8,  8,  8,
       8,  8,  8,  8,  8,  8,  8,  8,
       8,  8,  8,  8,  8,  8,  8,  8,
       8,  8,  8,  8,  8,  8,  8,  8
  };



  static const uint8_t matrix_252_Flat_3[64] = {
       3,  3,  3,  3,  3,  3,  3,  3,
       3,  3,  3,  3,  3,  3,  3,  3,
       3,  3,  3,  3,  3,  3,  3,  3,
       3,  3,  3,  3,  3,  3,  3,  3,
       3,  3,  3,  3,  3,  3,  3,  3,
       3,  3,  3,  3,  3,  3,  3,  3,
       3,  3,  3,  3,  3,  3,  3,  3,
       3,  3,  3,  3,  3,  3,  3,  3
  };


  // This is another possible matrix to use for sharpening.
  static const uint8_t matrix_253[64] = {
       8, 2, 2, 2, 2, 2, 2, 3,
       2, 2, 2, 2, 3, 3, 3, 2,
       2, 2, 2, 2, 3, 3, 2, 2,
       2, 2, 2, 2, 3, 2, 2, 2,
       2, 3, 3, 3, 2, 2, 2, 2,
       2, 3, 3, 2, 2, 2, 2, 2,
       2, 3, 2, 2, 2, 2, 2, 2,
       3, 2, 2, 2, 2, 2, 2, 2
  };

  //static const uint8_t matrix_253[64] = {
  //     8, 4, 4, 4, 4, 4, 4, 4,
  //     4, 4, 4, 4, 4, 4, 4, 4,
  //     4, 4, 4, 4, 4, 4, 4, 4,
  //     4, 4, 4, 4, 4, 4, 4, 4,
  //     4, 4, 4, 4, 4, 4, 4, 4,
  //     4, 4, 4, 4, 4, 4, 4, 4,
  //     4, 4, 4, 4, 4, 4, 4, 4,
  //     4, 4, 4, 4, 4, 4, 4, 4
  //  };


  //// matrix matrix_254 EXPERIMENTAL.  It is used to test other matrix values against matrix_254_PreSmooth.
  //static const uint8_t matrix_254[64] = {
  //      2, 2, 3, 3, 3,12, 8, 3,
  //      2, 3, 3, 4, 8, 8, 8, 3,
  //      3, 3, 4, 7, 7, 8, 3, 3,
  //      3, 4, 7,10, 9, 3, 3, 3,
  //      3, 8, 7, 9, 8, 3, 3, 3,
  //       12, 8, 8, 3, 3, 3, 3, 3,
  //      8, 8, 3, 3, 3, 3, 3, 3,
  //      3, 3, 3, 3, 3, 3, 3, 3
  //  };




  //// MATRIX_PRESMOOTH  OLD
  //// matrix_254. Used in the presmoother section of amDCT.c
  //// Smooths areas that are being sharpened more than other areas.
  //// It is used in the presmoothing pass when doing strong sharpening and range expansion.
  //// It is designed to maximally reduce noise that will create artifacts, while doing the minimum amount of smoothing.
  //static const uint8_t matrix_254_PreSmooth[64] = {
  //     2, 2, 3, 3, 3, 8, 8, 3,
  //     2, 3, 3, 4, 8, 8, 8, 3,
  //     3, 3, 6, 8, 8, 8, 3, 3,
  //     3, 4, 8, 8, 8, 3, 3, 3,
  //     3, 8, 8, 8, 6, 3, 3, 3,
  //     8, 8, 8, 3, 3, 3, 3, 3,
  //     8, 8, 3, 3, 3, 3, 3, 3,
  //     3, 3, 3, 3, 3, 3, 3, 3
  //  };


  // MATRIX_PRESMOOTH
  // matrix_254. Used in the presmoother section of amDCT.c
  // Smooths areas that are being sharpened more than other areas.
  // It is used in the presmoothing pass when doing strong sharpening and range expansion.
  // It is designed to maximally reduce noise that will create artifacts, while doing the minimum amount of smoothing.
  static const uint8_t matrix_254_PreSmooth[64] = {
       2,  2,  3,  4,  8, 12, 13, 14,
       2,  2,  3,  4,  8,  8,  8,  3,
       3,  3,  5,  8,  8,  8,  3,  3,
       4,  4,  8,  8,  8,  3,  3,  3,
       8,  8,  8,  8,  6,  3,  3,  3,
       12, 8,  8,  3,  3,  3,  3,  3,
       13, 8,  3,  3,  3,  3,  3,  3,
       14, 3,  3,  3,  3,  3,  3,  3
  };


  // MATRIX_SHARP   Sharpening matrix    Flat_8 used for the sharpening matrix.
  static const uint8_t matrix_255_Flat_8[64] = {
       8, 8, 8, 8, 8, 8, 8, 8,
       8, 8, 8, 8, 8, 8, 8, 8,
       8, 8, 8, 8, 8, 8, 8, 8,
       8, 8, 8, 8, 8, 8, 8, 8,
       8, 8, 8, 8, 8, 8, 8, 8,
       8, 8, 8, 8, 8, 8, 8, 8,
       8, 8, 8, 8, 8, 8, 8, 8,
       8, 8, 8, 8, 8, 8, 8, 8
  };
  //MOVE 2 & 3 to 128 ...  128 to 255 are internal system matrices   Each matrix set consists of normal, thick stronger, thin stronger






















  use_matrix = matrix_3; // default

  switch (matrix_Num) {
  case 0: // default matrix
    set_intra_matrix(quant_intra_matrix, get_default_intra_matrix());
    set_inter_matrix(quant_inter_matrix, get_default_inter_matrix());
    return;

  case 1:
    use_matrix = matrix_1;
    break;

  case 2:
    use_matrix = matrix_2;
    break;

  case 3:
    use_matrix = matrix_3;
    break;

  case 4:
    use_matrix = matrix_4;
    break;

  case 5:
    use_matrix = matrix_5;
    break;

  case 6:
    use_matrix = matrix_6;
    break;

  case 7:
    use_matrix = matrix_7;
    break;

  case 8:
    use_matrix = matrix_8;
    break;

  case 9:
    use_matrix = matrix_9;
    break;



    // matrix numbers 10-29 are "flat" containing values of either 1, 8, 16, 24, 32, 40, 48, 64, 80, 96, 112, 128, 144, 160, 176, 192, 208, 224, 240
  case 10:
    use_matrix = Flat_8; // was flat_3
    break;

  case 11:
    use_matrix = Flat_8;
    break;

  case 12:
    use_matrix = Flat_16;
    break;

  case 13:
    use_matrix = Flat_24;
    break;

  case 14:
    use_matrix = Flat_32;
    break;

  case 15:
    use_matrix = Flat_40;
    break;

  case 16:
    use_matrix = Flat_48;
    break;

  case 17:
    use_matrix = Flat_56;
    break;

  case 18:
    use_matrix = Flat_80;
    break;

  case 19:
    use_matrix = matrix_19;
    break;

  case 20:
    use_matrix = Flat_96;
    break;

  case 21:
    use_matrix = Flat_112;
    break;

  case 22:
    use_matrix = Flat_128;
    break;

  case 23:
    use_matrix = Flat_144;
    break;

  case 24:
    use_matrix = Flat_160;
    break;

  case 25:
    use_matrix = Flat_176;
    break;

  case 26:
    use_matrix = Flat_192;
    break;

  case 27:
    use_matrix = Flat_208;
    break;

  case 28:
    use_matrix = Flat_224;
    break;

  case 29:
    use_matrix = Flat_240;
    break;



    //  matrix numbers 30-49 are "Horizontal"
    //
    //   H_2_64_8_64   Horizontal  2 starting column value 64 to ending column 8 value 64 numbers to end
    //   H_2_64_8_240  Horizontal  2 starting column 64-64 numbers to end
    //   H_2_64        Horizontal  2 column value 64

  case 30: // Start Column 2
    use_matrix = H_2_64;
    break;

  case 31:
    use_matrix = matrix_31;
    break;

  case 32:
    use_matrix = matrix_32;
    break;

  case 33:
    use_matrix = matrix_33;
    break;

  case 34:
    use_matrix = matrix_34;
    break;

  case 35:
    use_matrix = matrix_35;
    break;

  case 36:
    use_matrix = matrix_36;
    break;

  case 37:
    use_matrix = matrix_37;
    break;

  case 38:
    use_matrix = matrix_38;
    break;

  case 39:
    use_matrix = matrix_39;
    break;

  case 40: // Start Column 5
    use_matrix = H_5_240;
    break;

  case 41:
    use_matrix = H_5_240_8_240;
    break;

  case 42:
    use_matrix = H_5_64_8_240;
    break;

  case 43: // Start Column 6
    use_matrix = H_6_240;
    break;

  case 44:
    use_matrix = H_6_240_8_240;
    break;

  case 45:
    use_matrix = H_6_64_8_240;
    break;

  case 46: // Start Column 7
    use_matrix = H_7_240;
    break;

  case 47:
    use_matrix = H_7_240_8_240;
    break;

  case 48:
    use_matrix = H_7_128_8_240;
    break;

  case 49: // Start Column 8
    use_matrix = H_8_240;
    break;


    //  matrix numbers 50-69 are "Horizontal"
    //
    //   V_2_64_8_64   Vertical  2 starting row value 64 to ending row 8 value 64 numbers to end
    //   V_2_64_8_240  Vertical  2 starting row 64-64 numbers to end
    //   V_2_64        Vertical  2 row value 64


  case 50: // Start Row 2
    use_matrix = V_2_64;
    break;

  case 51:
    use_matrix = V_2_64_8_64;
    break;

  case 52:
    use_matrix = V_2_64_8_240;
    break;

  case 53:
    use_matrix = V_2_8_8_240;
    break;

  case 54: // Start Row 3
    use_matrix = V_3_240;
    break;

  case 55:
    use_matrix = V_3_240_8_240;
    break;

  case 56:
    use_matrix = V_3_16_8_240;
    break;

  case 57: // Start Row 4
    use_matrix = V_4_240;
    break;

  case 58:
    use_matrix = V_4_240_8_240;
    break;

  case 59:
    use_matrix = V_4_32_8_240;
    break;

  case 60: // Start Row 5
    use_matrix = V_5_240;
    break;

  case 61:
    use_matrix = V_5_240_8_240;
    break;

  case 62:
    use_matrix = V_5_64_8_240;
    break;

  case 63: // Start Row 6
    use_matrix = V_6_240;
    break;

  case 64:
    use_matrix = V_6_240_8_240;
    break;

  case 65:
    use_matrix = V_6_64_8_240;
    break;

  case 66: // Start Row 7
    use_matrix = V_7_240;
    break;

  case 67:
    use_matrix = V_7_240_8_240;
    break;

  case 68:
    use_matrix = V_7_128_8_240;
    break;

  case 69: // Start Row 8
    use_matrix = V_8_240;
    break;



    //  matrix numbers 70-89 are "Horizontal and Vertical"
    //
    //   HV_2_64_8_64   Horizontal and Vertical  2 starting row and col value 64 to ending row 8 value 64 numbers to end
    //   HV_2_64_8_240  Horizontal and Vertical  2 starting row and col 64-64 numbers to end
    //   HV_2_64        Horizontal and Vertical  2 row and col value 64

  case 70: // Start Row and Col 2
    use_matrix = HV_2_64;
    break;

  case 71:
    use_matrix = HV_2_64_8_64;
    break;

  case 72:
    use_matrix = HV_2_64_8_240;
    break;

  case 73:
    use_matrix = HV_2_8_8_240;
    break;

  case 74: // Start Row and Col 3
    use_matrix = HV_3_240;
    break;

  case 75:
    use_matrix = HV_3_240_240;
    break;

  case 76:
    use_matrix = HV_3_16_8_240;
    break;

  case 77: // Start Row and Col 4
    use_matrix = HV_4_240;
    break;

  case 78:
    use_matrix = HV_4_240_8_240;
    break;

  case 79:
    use_matrix = HV_4_32_8_240;
    break;

  case 80: // Start Row and Col 5
    use_matrix = HV_5_240;
    break;

  case 81:
    use_matrix = HV_5_240_8_240;
    break;

  case 82:
    use_matrix = HV_5_64_8_240;
    break;

  case 83: // Start Row and Col 6
    use_matrix = HV_6_240;
    break;

  case 84:
    use_matrix = HV_6_240_8_240;
    break;

  case 85:
    use_matrix = HV_6_64_8_240;
    break;

  case 86: // Start Row and Col 7
    use_matrix = HV_7_240;
    break;

  case 87:
    use_matrix = HV_7_240_8_240;
    break;

  case 88:
    use_matrix = HV_7_128_8_240;
    break;

  case 89: // Start Row and Col 8
    use_matrix = HV_8_240;
    break;



    //  matrix numbers 90-99 are "Diagonal"
    //
    //   D_2_64_8_64   Diagonal  2 starting row and col value 64 to ending row 8 value 64 numbers to end
    //   D_2_64_8_240  Diagonal  2 starting row and col 64-64 numbers to end
    //   D_2_64        Diagonal  2 row and col value 64

  case 90: // Start RC 2
    use_matrix = D_2_240;
    break;

  case 91:
    use_matrix = D_2_240_8_240;
    break;

  case 92: // Start RC 3
    use_matrix = D_3_240_8_240;
    break;

  case 93: // Start RC 4
    use_matrix = D_4_240_8_240;
    break;



  case 94: // Diagonal shift Up
    use_matrix = D_U1_3_240_8_240;
    break;

  case 95:
    use_matrix = D_U2_4_240_8_240;
    break;


  case 96: // Diagonal shift Down
    use_matrix = D_D1_3_240_8_240;
    break;

  case 97:
    use_matrix = D_D2_4_240_8_240;
    break;



  case 98: // Diagonal Outward shift
    use_matrix = D_O1_240_8_240;
    break;


  case 99:
    use_matrix = D_O3_240_8_240;
    break;






  case 100:
    use_matrix = D_UD1_240_8_240;
    break;


  case 101:
    use_matrix = D_UD1_64_8_240;
    break;

  case 102:
    use_matrix = D_UD2_240_8_240;
    break;

  case 103:
    use_matrix = D_UD3_240_8_240;
    break;

  case 104:
    use_matrix = D_UD4_240_8_240;
    break;

  case 105:
    use_matrix = D_UD4F_240_8_240;
    break;

  case 106:
    use_matrix = D_UD3F_240_8_240;
    break;

  case 107:
    use_matrix = D_UD2F_240_8_240;
    break;

  case 108:
    use_matrix = D_UD1F_240_8_240;
    break;

  case 109:
    use_matrix = DIAG_240;
    break;

  case 110:
    use_matrix = D_UD0F_240_8_240;
    break;


    // Specials used for testing
  case 111: // Value 64 in the top left corner.  Value 1 everywhere else.
    use_matrix = Hpos1;
    break;

  case 112: // Test for Anti Aliasing.
    use_matrix = DAA1;
    break;

  case 113: // Smooth after sharpening.
    use_matrix = smooth_sharp_matrix;
    break;

  case 149: // Test for Anti Aliasing.
    use_matrix = my_intra_only_1_by_255xx;
    break;

  case 150: // Test for Sharp.
    use_matrix = my_sharp;
    break;

  case 151: // Test for Sharp.
    use_matrix = my_sharp_2;
    break;

  case 152: // Test for Sharp.
    use_matrix = HV_2_2_8_128;
    break;


  case 161: // Test for Sharp.
    use_matrix = matrix_161;
    break;

  case 162: // Test for Sharp.
    use_matrix = matrix_162;
    break;

    //  case 163: // Test for Sharp.
    //    use_matrix = matrix_163;
    //    break;

  case 164: // Test for Sharp.
    use_matrix = matrix_164;
    break;


  case 165: // Test for Sharp.
    use_matrix = matrix_165;
    break;


  case 166: // Test for Sharp.
    use_matrix = matrix_166;
    break;


  case 167: // Test for Sharp.
    use_matrix = matrix_167;
    break;


  case 168: // Test for Sharp.
    use_matrix = matrix_168;
    break;

  case 169: // Test for Sharp.
    use_matrix = matrix_169;
    break;


  case 170: // Test for Sharp.
    use_matrix = matrix_170;
    break;


  case 171: // Test for Sharp.
    use_matrix = matrix_171;
    break;

  case 172: // Test for Sharp.
    use_matrix = matrix_172;
    break;


  case 173: // Test for Sharp.
    use_matrix = matrix_173;
    break;


  case 174: // Test for Sharp.
    use_matrix = matrix_174;
    break;


  case 175: // Test for Sharp.
    use_matrix = matrix_175;
    break;



  case 176: // Test for Sharp.
    use_matrix = matrix_176;
    break;



  case 177: // Test for Sharp.
    use_matrix = matrix_177;
    break;



  case 178: // Test for Sharp.
    use_matrix = matrix_178;
    break;


  case 179: // Test for Sharp.
    use_matrix = matrix_179;
    break;


  case 180: // Test for Sharp.
    use_matrix = matrix_180;
    break;


  case 181: // Test for Sharp.
    use_matrix = matrix_181;
    break;


  case 182: // Test for Sharp.
    use_matrix = matrix_182;
    break;

  case 183: // Test for Sharp.
    use_matrix = matrix_183;
    break;


  case 184: // Test for Sharp.
    use_matrix = matrix_184;
    break;


  case 185: // Test for Sharp.
    use_matrix = matrix_185;
    break;


  case 186: // Test for Sharp.
    use_matrix = matrix_186;
    break;


  case 187: // Test for Sharp.
    use_matrix = matrix_187;
    break;


  case 188: // Test for Sharp.
    use_matrix = matrix_188;
    break;


  case 189: // Test for Sharp.
    use_matrix = matrix_189;
    break;


  case 190: // Test for Sharp.
    use_matrix = matrix_190;
    break;


  case 191: // Test for Sharp.
    use_matrix = matrix_191;
    break;


  case 192: // Test for Sharp.
    use_matrix = matrix_192;
    break;


  case 193: // Test for Sharp.
    use_matrix = matrix_193;
    break;


  case 194: // Test for Sharp.
    use_matrix = matrix_194;
    break;


  case 195: // Test for Sharp.
    use_matrix = matrix_195;
    break;


  case 196: // Test for Sharp.
    use_matrix = matrix_196;
    break;


  case 197: // Test for Sharp.
    use_matrix = matrix_197;
    break;


  case 198: // Test for Sharp.
    use_matrix = matrix_198;
    break;



  case 199: // Test for Sharp.
    use_matrix = matrix_199;
    break;


  case 200: // Test for Sharp.
    use_matrix = matrix_200;
    break;


  case 201: // Test for Sharp.
    use_matrix = matrix_201;
    break;


  case 220: // Test for Sharp.
    use_matrix = matrix_220;
    break;

  case 221: // Test for Sharp.
    use_matrix = matrix_221;
    break;

  case 222: // Test for Sharp.
    use_matrix = matrix_222;
    break;


  case 223: // Test for Sharp.
    use_matrix = matrix_223;
    break;


  case 224: // Test for Sharp.
    use_matrix = matrix_224;
    break;


  case 225: // Test for Sharp.
    use_matrix = matrix_225;
    break;


  case 226: // Test for Sharp.
    use_matrix = matrix_226;
    break;


  case 227: // Test for Sharp.
    use_matrix = matrix_227;
    break;


  case 228: // Test for Sharp.
    use_matrix = matrix_228;
    break;


  case 229: // Test for Sharp.
    use_matrix = matrix_229;
    break;


  case 230: // Test for Sharp.
    use_matrix = matrix_230;
    break;


  case 231: // Test for Sharp.
    use_matrix = matrix_231;
    break;


  case 232: // Test for Sharp.
    use_matrix = matrix_232;
    break;

  case 233: // Test for Sharp.
    use_matrix = matrix_233;
    break;

  case 234: // Test for Sharp.
    use_matrix = matrix_234;
    break;

  case 235: // Test for Sharp.
    use_matrix = matrix_235;
    break;

  case 236: // Test for Sharp.
    use_matrix = matrix_236;
    break;

  case 237: // Test for Sharp.
    use_matrix = matrix_237;
    break;

  case 238: // Test for Sharp.
    use_matrix = matrix_238;
    break;

  case 239: // Test for Sharp.
    use_matrix = matrix_239;
    break;


    /*
    ///////////////////////////////////////////////////////////////////////////////
    //
    //    matrices 240-255   RESERVED FOR INTERNAL amDCT USE
    //
    //////////////////////////////////////////////////////////////////////////////
    */
  case 240: // Test for Sharp.
    use_matrix = matrix_240;
    break;

  case 241: // Test for Sharp.
    use_matrix = matrix_241;
    break;

  case 242: // Range Expand Matrix.
    use_matrix = matrix_242;
    break;



  case 243: // Test for Sharp.
    use_matrix = matrix_243;
    break;


  case 244: // Test for Sharp.
    use_matrix = matrix_244;
    break;


  case 245: // Test for Sharp.
    use_matrix = matrix_245;
    break;





  case 249: // Test for Sharp.
    //use_matrix = matrix_249;
    use_matrix = matrix_249_PreSmooth_TEST;
    break;



  case 250: //
    use_matrix = matrix_250;
    break;


  case 251:    // MATRIX_RANGE_EXPAND
    use_matrix = matrix_251_Flat_8_RE;
    break;


    //#####################################  THE ROUTINE THAT IMPLIMENTS THIS SHOULD BE RUN AT THE BEGINNING OF THE ADAPT CODE
    //################## USE FOR BLOCK ADAPTIVE PER BLOCK MATRIX WEIGHTING
    //################## quant=1 and matrix=252 taske advantage of a bug 
    //################## that overflow the the quant dequant routines so that 
    //################## in a block are magnified.  The amount of magnification
    //################## is used to control the per block adaptivity.
    //########################################
    //shift   = 0
    //adapt   = 5
    //quant1  = 1   # 1  MUST BE 1
    //quality = 1
    //matrix  = 252  # MUST BE 252
  case 252: // MATRIX_FLAT3
    use_matrix = matrix_252_Flat_3;
    break;



    //  case 253: // Test Expand_Sharp_Strong Sharp.
    //    use_matrix = Expand_Sharp_Strong_253;
    //    matrix_253
    //    break;

  case 253:  // This matrix is used for sharpening.
    use_matrix = matrix_253;
    break;


  case 254:   // MATRIX_PRESMOOTH  Pre smoothing matrix.  Used before sharpening.
    use_matrix = matrix_254_PreSmooth;
    break;

  case 255:   // MATRIX_SHARP Sharpening matrix   Flat_8
    use_matrix = matrix_255_Flat_8;
    break;




  default: // default matrix
    set_intra_matrix(quant_intra_matrix, get_default_intra_matrix());
    set_inter_matrix(quant_inter_matrix, get_default_inter_matrix());
    return;

  }



  // quant_mpeg_inter - dequant_mpeg_inter require that matrix values be >= to 3
  // and that matrix[0] be >= 8 and <= 32
  for (i = 0; i < 64; i++) {
    if ((qtype == 3 || qtype == 13) && (use_matrix[i] < 3)) matrix[i] = 3;
    else
      matrix[i] = use_matrix[i];
  }

  if ((qtype == 3 || qtype == 13) && use_matrix[0] < 8)  matrix[0] = 8;
  if ((qtype == 3 || qtype == 13) && use_matrix[0] > 32) matrix[0] = 32;


  set_intra_matrix(quant_intra_matrix, matrix);
  set_inter_matrix(quant_inter_matrix, matrix);

  // set_intra_matrix sets quant_intra_matrix[0] to 8 which we need to undo for sharpening or range expansion.
  if (qtype == 21 || qtype == 22 || qtype == 23) {
    quant_intra_matrix[0] = matrix[0];
  }

  return;

}


