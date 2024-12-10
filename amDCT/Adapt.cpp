#include "amDCTtypedefs.h"

#include "Adapt.h"
#include "blindPPcode.h"   // needed for deblock_vert() and deblock_horiz()
#include "Utilities.h"
#include "LumaBlockInfo.h"

// quant specifies the minimum amount of smoothing that will be done on the frame.
// FrameQuant, computed below, specifies the actual amount of smoothing that will be done on the frame.
//
// If adapt is larger than quant then the minimum amount of smoothing for the frame
// will be increased up to the value of adapt.  The amount of increase is
// determined by the overall blockiness of the frame which is computed by AdaptQuant().
//
// AdaptQuant() returns a value between 0-31
// AdaptQuant() determines an estimate of the overall blocking of the frame by looking     
// at how much change the blindPP code makes to the frame.
//
// If ANY level of adaptation is specified then the image will retain the mild deblocking
// done by the blindPP code.

// adapt specifies the maximum quant value the system will use.
// quant specifies the minimum quant value the system will use.
// setting adapt to any value other than 0 turns on frame adapt.

// If quant==0 and adapt>0 then the blindpp code is run with adapt strength.

// These 4 variables are used during development to find max and min values across many frames
//static int max_debug = 0;
//static int min_debug = 99999;
//
//static int max_debug2 = 0;
//static int min_debug2 = 99999;

int doAdapt(FrameInfo_args* FrameInfoArgs) {
  LumaPerBlock_args* LumaPerBlockArgs = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs;

  uint8_t* psrc = FrameInfoArgs->frame_adaptSource;
  uint8_t* BF_tmp_8P = FrameInfoArgs->MemoryArgs->DctLoopArgs[0].BF_tmp_8P;

  uint8_t    quant = FrameInfoArgs->quant_a;
  uint8_t    adapt = FrameInfoArgs->adapt_a;
  uint8_t   quality = FrameInfoArgs->quality;
  uint32_t  numBlks = FrameInfoArgs->numBlocksY;

  uint32_t  numBlocks_wide = FrameInfoArgs->numBlocks_wide;

  uint32_t  sizeY = FrameInfoArgs->sizeY;
  uint16_t  src_height = FrameInfoArgs->src_height;
  uint16_t  src_width = FrameInfoArgs->src_width;
  //  uint8_t   T2             = FrameInfoArgs->T2;

  int       adaptTempMax = 0;
  int       adaptTempMin = 0;
  float     adaptWeightN = 1.0f;
  uint16_t  deblockQuant = 0;             //  The strength passed to the deblock_horiz() and deblock_vert()
  uint32_t  blockPixCount = 0;
  uint32_t  blockPixCountUsed = 0;

  uint32_t  blkPixCount = 0;
  uint32_t  blkPixCount1 = 0;
  uint32_t  blkPixCount2 = 0;
  uint32_t  blkPixCount3 = 0;
  uint32_t  blkPixCount4 = 0;

  uint32_t  difBinCount[255];
  uint32_t  difBinCountZero = 0;

  uint8_t    adaptWeight = 0;
  uint8_t   use_quant = quant;
  uint8_t   FrameQuant = 0;
  uint8_t   adapt2 = adapt;

  int32_t   sumAbDif = 0;
  int16_t   sumAbDif1 = 0;
  int16_t   sumAbDif2 = 0;
  int16_t   sumAbDif3 = 0;
  int16_t   sumAbDif4 = 0;

  uint32_t  curBlock = 0;
  uint16_t  curdifBlkTotalUsed = 0;
  uint16_t  curblkPixCountUsed = 0;
  uint16_t  curBlkTotalPixCount = 0;
  uint16_t  blkPixCountUsed = 0;
  uint16_t  difBlockTotalUsed = 0;

  uint16_t  blkSumCent = 0;
  float     blkMeanCent = 0.0F;
  float     blkSadCent = 0;

  int16_t  blkSumEdge1 = 0;
  float    blkSadEdge1 = 0.0F;

  int16_t  blkSumEdge2 = 0;
  float     blkMeanEdge2 = 0.0F;
  float    blkSadEdge2 = 0.0F;

  float     db1, db2, db3;

  int       adaptTemp;
  float     adaptTempF;
  float     tempF;
  float     PercentPixUsedTotalPix;
  float    percentBlksUsed = 50.0f;

  uint32_t  framepixCnt = 0;
  uint32_t  framepixUsedCnt = 0;
  uint32_t  framedifTotal = 0;

  unsigned int i, h, x;

  if (adapt == 0) return(0);

  psrc = FrameInfoArgs->frame_adaptSource;

  if (quality == 1) {  // quality==1 does no adaptivity.
    deblock_vert(psrc, src_height, src_width, src_width, adapt);  // Note: Doing deblock_vert() first works slightly better then doing it after deblock_horiz()
    deblock_horiz(psrc, src_height, src_width, src_width, adapt);
    FrameInfoArgs->FrameQuant = quant;
    FrameInfoArgs->adaptWeight = adapt;

    MY_MEMCPY(FrameInfoArgs->MemoryArgs->DctLoopArgs[0].BF_srcP, psrc, sizeY);
    MY_MEMCPY(FrameInfoArgs->MemoryArgs->frame_workSource, psrc, sizeY);
    return(0);
  }

  ///*
  if (quality == 2) {
    // Fast exit if adapt not enough larger than quant for adaptation to make a noticeable difference.
    if (adapt <= quant + 2 || (quant == 0 && adapt <= 8)) {
      deblock_vert(psrc, src_height, src_width, src_width, adapt);  // Note: Doing deblock_vert() first works slightly better then doing it after deblock_horiz()
      deblock_horiz(psrc, src_height, src_width, src_width, adapt);
      FrameInfoArgs->FrameQuant = quant;

      MY_MEMCPY(FrameInfoArgs->MemoryArgs->DctLoopArgs[0].BF_srcP, psrc, sizeY);
      MY_MEMCPY(FrameInfoArgs->MemoryArgs->frame_workSource, psrc, sizeY);
      return(0);
    }
  }
  //*/
    // Use the value that was computed in LumaBlockInfo()
  if (quality >= 3)   percentBlksUsed = FrameInfoArgs->percentBlksUsed;

  MY_MEMCPY(BF_tmp_8P, psrc, FrameInfoArgs->sizeY);

  // Using Horizontal Vertical or both makes very little  difference.
  deblock_horiz(BF_tmp_8P, src_height, src_width, src_width, 15);
  //deblock_horiz(BF_tmp_8P, src_height, src_width, src_width, 31);

  //  TRY.  This info is available from LumaBlockInfo()
  //     Count the amount of dif on the edge of the block and use that to influence the block adaptive smoothing amount.
  //     Comparing it to the average block edge diff of the frame might give the best results.
  //     To get the final value factoring in the bright amount and the dark amount might help.
  //     Also factoring the amount of detail in the block, It might be the case that very low detail and very high detail should be smoothed more
  //     or it might be the detail difference between the edge and center that is important.
  //  for (i = 0; i < 4; i++) difBlockTotalUsed[i] = 0;
  //  MY_MEMSET(difBinCount, 255 * sizeof(uint32_t), 0);

  blkPixCount = 0;
  framedifTotal = 0;
  blkPixCount1 = 0;
  blkPixCount2 = 0;
  blkPixCount3 = 0;

  blkSumCent = 0;
  blkMeanCent = 0.0F;
  blkSadCent = 0.0;

  blkSumEdge1 = 0;
  //   blkMeanEdge1  = 0.0F;
  blkSadEdge1 = 0.0F;

  blkSumEdge2 = 0;
  blkMeanEdge2 = 0.0F;
  blkSadEdge2 = 0.0;

  difBinCount[0] = 0;
  difBinCount[1] = 0;
  difBinCount[2] = 0;

  sumAbDif = 0;
  curBlock = 0;  // (blkRow * numBlocks_wide) + blkCol;

  for (h = 8; h < (unsigned int)(src_height - 7); h++) {  // Start at the second row of block.  Start at the top edge of the block.
    if (h % 8 == 0 || h % 8 == 7) {  // Only look at horizontal lines on block boundaries.  Using vertical block boundaries doesn't improve the analysis.
      for (x = 8; x < (unsigned int)src_width - 7; x++) {
        uint32_t idx = h * src_width + x;
        uint8_t  tmp = (uint8_t)abs(BF_tmp_8P[idx] - psrc[idx]);

        // This collects information from the top row of the block which is needed when the bottom row of the block is done.
        // We store the top row per block information into LumaPerBlockArgs[curBlock]... so we can use it when we get to the bottom row of the block.

        // This finishes the processing of the top edge of the block.
        if (h % 8 == 0 && x % 8 == 0) {  // This finishes the processing of the top edge of the block.
          curBlock = (h >> 3) * numBlocks_wide + ((x >> 3));

          LumaPerBlockArgs[curBlock].adpt_PixCount1 = (uint8_t)curBlkTotalPixCount;
          LumaPerBlockArgs[curBlock].adpt_difBlockTotalUsed1 = (uint8_t)curdifBlkTotalUsed;
          LumaPerBlockArgs[curBlock].adpt_difPixCountUsed1 = (uint8_t)curblkPixCountUsed;

          curdifBlkTotalUsed = 0;
          curblkPixCountUsed = 0;
          curBlkTotalPixCount = 0;

          continue;
        }

        // This finishes the processing of the block.
        if (h % 7 == 0 && x % 8 == 0) {
          curBlock = (h >> 3) * numBlocks_wide + ((x >> 3));

          LumaPerBlockArgs[curBlock].adpt_PixCount2 = (uint8_t)curBlkTotalPixCount;
          LumaPerBlockArgs[curBlock].adpt_difBlockTotalUsed2 = (uint8_t)curdifBlkTotalUsed;
          LumaPerBlockArgs[curBlock].adpt_difPixCountUsed2 = (uint8_t)curblkPixCountUsed;

          curdifBlkTotalUsed = 0;
          curblkPixCountUsed = 0;
          curBlkTotalPixCount = 0;

          continue;
        }

        framepixCnt++;             // The total number of pixels looked at for the frame.
        curBlkTotalPixCount++;     // The total number of pixels looked at for the block.

        difBinCount[tmp]++;
        framedifTotal += tmp;
        sumAbDif += tmp;

        if (tmp > 1) {             // Get counts for pixels that have changed after deblock_horiz() has been done.
          framepixUsedCnt++;     // The total number of pixels that have a changed value.
          curblkPixCountUsed++;  // The number of pixels in the current block that have a changed value.
          curdifBlkTotalUsed += tmp;
        }

        if (tmp == 0) {
          difBinCountZero++;
        }

        if (tmp == 1) {
          sumAbDif1 += tmp;
          blkPixCount1++;
        }
        if (tmp == 2) {
          sumAbDif2 += tmp;
          blkPixCount2++;
        }
        if (tmp == 3) {
          sumAbDif3 += tmp;
          blkPixCount3++;
        }
        if (tmp == 4) {          // NOT CURRENTLY USING THIS INFO.
          sumAbDif4 += tmp;
          blkPixCount4++;
        }
      }
    }
  }

  // Now go through the collected block information and compute the frame as a whole information.
  for (i = 0; i < numBlks; i++) {

    curBlock = i;

    blockPixCount = LumaPerBlockArgs[curBlock].adpt_PixCount1 + LumaPerBlockArgs[curBlock].adpt_PixCount2;         // Number of pixels looked at in block.
    blockPixCountUsed = LumaPerBlockArgs[curBlock].adpt_difPixCountUsed1 + LumaPerBlockArgs[curBlock].adpt_difPixCountUsed2;  // Number of pixels used in block.
    difBlockTotalUsed = LumaPerBlockArgs[curBlock].adpt_difBlockTotalUsed1 + LumaPerBlockArgs[curBlock].adpt_difBlockTotalUsed2;

    if (blockPixCountUsed < 1) LumaPerBlockArgs[curBlock].adpt_PixPercent = 0;
    else                       LumaPerBlockArgs[curBlock].adpt_PixPercent = (uint8_t)(((blockPixCountUsed << 6 / blockPixCount)));

    //
    //    // THIS IS A TEST TO SEE IF WE CAN GET A BETTER PERCENT  NOT HELPING MUCH!!!!!!!!
    //    // TRY ALL DIFFS TO DIF 1 INSTEAD.
    //    if (difBlockTotalUsed > blockPixCountUsed) difBlockTotalUsed = difBlockTotalUsed * 22;
    //    if (blockPixCountUsed < 1) LumaPerBlockArgs[curBlock].adpt_PixPercent2 = 0;
    //    else                       LumaPerBlockArgs[curBlock].adpt_PixPercent2 = (uint8_t)(((float)(difBlockTotalUsed / (float)blockPixCount) * 100) + .5);
    //
    //
    ////    if (blockPixCountUsed <= 1) zeroBlks += 1;
    //    if (LumaPerBlockArgs[curBlock].adpt_PixPercent  <= 5) zeroBlks  += 1;
    //    if (LumaPerBlockArgs[curBlock].adpt_PixPercent2 <= 5) zeroBlks2 += 1;
    //}
  }

  //  ALL OF THE PER BLOCK INFO HAS BEEN COMPUTED.  THE REST OF THE CODE IS DONE ONCE PER FRAME SO SPEED OPTIMIZATIONS ARE NOT NEEDED.
  // These have already been computed for the frame.
  //    totalPixCount;           // The total number of pixels looked at.
  //    totalPixCountUsed;       // The total number of pixels that have a changed value.
  //    totalPixDifCount;        // The total of absolute differences of the pixels that have a changed value.

  //sumAbDif = totalPixDifCount;           // The total of the absolute differences.
  sumAbDif = framedifTotal;                // The total of the absolute differences.

  FrameInfoArgs->totalPercentedge = 0;

  if (framepixCnt < 1)   framepixCnt = 1;
  if (framedifTotal < 1) framedifTotal = 1;
  PercentPixUsedTotalPix = ((float)(framedifTotal * 100) / (float)framepixCnt);

  FrameInfoArgs->PercentPixUsedTotalPix = PercentPixUsedTotalPix;     // range VR5           3-30   absolute max is 48
  // range Dilbert       3- 6
  // range TestHdDeBlock 1-22
  // range Smoke         6-11
  // range Blackboard    3-9
  // range Battlestar Galactica test new 1-10
  // range TheFlash2     1-19

  FrameInfoArgs->avgtotalPercentedge = (uint16_t)(framepixCnt / numBlks);

  //  FrameInfoArgs->avgtotalPercentedge = (uint16_t)((FrameInfoArgs->totalPercentedge)/(numBlks*2));
  //  FrameInfoArgs->zeroBlksPercentedge = (uint16_t)(((numBlks-zeroBlks) * 100)/numBlks);
  //  FrameInfoArgs->zeroBlksPercentedge  = (uint16_t)(((numBlks - zeroBlks)  * 200)/numBlks);
  //  FrameInfoArgs->zeroBlksPercentedge2 = (uint16_t)(((numBlks - zeroBlks2) * 200)/numBlks);

  if (framepixCnt < 1) framepixCnt = 1;
  if (blkPixCountUsed < 1) blkPixCountUsed = 1;

  //  FrameInfoArgs->percentBlksUsed2 = Float_FitRange_Float(PercentPixUsedTotalPix, 30, 1, 84, 1);

  FrameInfoArgs->framepixCnt = framepixCnt;
  FrameInfoArgs->sumAbDif = sumAbDif;
  FrameInfoArgs->sumAbDif1 = sumAbDif1;                 // Dif == 1 to count Raw Adaptive Difference from adapt() TESTING
  FrameInfoArgs->sumAbDif2 = sumAbDif2;                 // Dif == 2 to count Raw Adaptive Difference from adapt() TESTING
  FrameInfoArgs->sumAbDif3 = sumAbDif3;                 // Dif == 3 to count Raw Adaptive Difference from adapt() TESTING
  FrameInfoArgs->sumAbDif4 = sumAbDif4;                 // Dif == 4 to count Raw Adaptive Difference from adapt() TESTING
  FrameInfoArgs->difBlockTotalUsed = difBlockTotalUsed;         // Number of pixels counted Raw Adaptive Difference from adapt() TESTING
  FrameInfoArgs->sumAbDifPixCnt = blkPixCount;               // Number of pixels counted Raw Adaptive Difference from adapt() TESTING
  FrameInfoArgs->blkPixCountUsed = blkPixCountUsed;           // Number of pixels counted Raw Adaptive Difference from adapt() TESTING
  FrameInfoArgs->sumAbDifPixCnt1 = framedifTotal;             // Number of pixels counted Raw Adaptive Difference from adapt() TESTING
  FrameInfoArgs->sumAbDifPixCnt2 = blkPixCount2;              // Number of pixels counted Raw Adaptive Difference from adapt() TESTING
  FrameInfoArgs->sumAbDifPixCnt3 = blkPixCount3;              // Number of pixels counted Raw Adaptive Difference from adapt() TESTING
  FrameInfoArgs->sumAbDifPixCnt4 = blkPixCount4;              // Number of pixels counted Raw Adaptive Difference from adapt() TESTING

  db1 = (float)(difBinCount[1]);
  db2 = (float)(difBinCount[2]);
  db3 = (float)(difBinCount[3]);

  if (db1 > 0.1) db1 = 1;
  if (db2 > 0.1) db2 = 1;
  if (db3 > 0.1) db3 = 1;

  adaptTempF = (db1 - (db2 / 2.0F)) + db2 + db3;
  adaptWeightN = (float)numBlks / 2896.0F;   // This provides a correction for the frame size.

  adaptTemp = (int)((adaptTempF / adaptWeightN) + 0.5F);
  adaptTempF = adaptTempF / adaptWeightN;

  //    adaptTemp = Int_FitRange_Float(PercentPixUsedTotalPix, 41.0, 2.0, 31.0, 1.0);  // 41 is max recorded.  2 is min recorded. Hand test VR5.
  FrameInfoArgs->adaptTemp = adaptTemp;

  if (adapt > quant)  adaptTemp = Int_FitRange_Float(adaptTempF, 5000.0, 1200.0, (float)adapt, (float)quant);
  else           adaptTemp = adapt;

  adaptTempMax = 0;
  adaptTempMin = 0;

  //  if (max_debug < adaptTemp) max_debug = adaptTemp;
  //  if (min_debug > adaptTemp) min_debug = adaptTemp;
  FrameInfoArgs->adaptTemp = adaptTemp;

  // The following are for adaptWeightN = (float)numBlks / 3496.0F;
  // actual max and min recorded  5471  151 allTests 1920x1080  2,073,600 pix   32,400 blocks  522,784 blkPixCount       24.7x larger
  // actual max and min recorded  6565  335  flash    704x480  good quality cable               86,132 blkPixCount
  // actual max and min recorded  7346  266   VR5     352x240      84,480 pix    1,320 blocks    21,948 blkPixCount

  // The following are for adaptWeightN = (float)numBlks / 3096.0F;
  // actual max and min recorded  4845  116 allTests 1920x1080  2,073,600 pix   32,400 blocks           24.7x larger
  // actual max and min recorded  5814  198  flash    704x480  good quality cable
  // actual max and min recorded  6505  780   VR5     352x240      84,480 pix    1,320 blocks

  // The following are for adaptWeightN = (float)numBlks / 2896.0F;
  // actual max and min recorded  4532  101 allTests 1920x1080  2,073,600 pix   32,400 blocks           24.7x larger
  // actual max and min recorded  5483  278  flash    704x480  good quality cable
  // actual max and min recorded  6065  287   VR5     352x240      84,480 pix    1,320 blocks

  // The following are for adaptWeightN = (float)numBlks / 2696.0F;
  // actual max and min recorded  3845  101 allTests 1920x1080  2,073,600 pix   32,400 blocks           24.7x larger
  // actual max and min recorded  5062  278  flash    704x480  good quality cable
  // actual max and min recorded  5647  205   VR5     352x240      84,480 pix    1,320 blocks

  // vr5 test clip
  // min black screen is  11 need to trim out all black areas <  32.
  // Max white screen is 205 need to trim out all white areas > 200.

  //  adaptWeight = (uint8_t)Int_FitRange_Int(adaptTemp, 5500, 200, adapt, quant);  // actual max and min recorded 9533 1070
  //  adaptWeight = (uint8_t)Int_FitRange_Int(adaptTemp, 4800, 450, adapt, quant);  // actual max and min recorded 9533 1070
  //  adaptWeight = Int_FitRange_Float(PercentPixUsedTotalPix, 41.0, 2.0, 31.0, 1.0);  // 41 is max recorded.  2 is min recorded. Hand test VR5.

  if (adapt > quant) {
    tempF = Float_FitRange_Float(PercentPixUsedTotalPix, 20.0, 2.0, (float)adapt, (float)quant);  // 41 is max recorded.  2 is min recorded. Hand test VR5.
    adaptWeight = (uint8_t)Int_FitRange_Float(percentBlksUsed, 81.0, 10.0, tempF, (float)quant);  // 81 is max recorded.  1 is min recorded. Hand test VR5.
    if (adaptWeight > adapt) adaptWeight = adapt;

    FrameInfoArgs->adaptWeight = adaptWeight;
    FrameQuant = adaptWeight;
    use_quant = adaptWeight;
  }
  else {
    adaptWeight = adapt;
    FrameInfoArgs->adaptWeight = adaptWeight;
    FrameQuant = quant;
    use_quant = quant;
  }

  if (use_quant < quant) use_quant = quant;

  FrameInfoArgs->FrameQuant = (uint8_t)FrameQuant;
  FrameInfoArgs->quant = (uint8_t)use_quant;

  //  adaptTempF = ((float)sumAbDif1/(float)framedifTotal) / ((float)sumAbDif2/(float)blkPixCount2);
  //  adaptTempF = ((float)sumAbDif1-(float)sumAbDif2); // / ((float)blkPixCount1-(float)blkPixCount2);

  //      TESTING VARIOUS WAYS TO COMPUTE THE ADAPT WEIGHT
  //  FrameInfoArgs->adaptWeight1 = (uint16_t)Int_FitRange_Int(sumAbDif1,    blkPixCount,                        3, FrameInfoArgs->adapt_a, FrameInfoArgs->quant_a);        // NEW METRIC !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  //  FrameInfoArgs->adaptWeight2 = (uint16_t)Int_FitRange_Int(blkPixCount3, (blkPixCount1 + blkPixCount2) >> 1, 3, FrameInfoArgs->adapt_a, FrameInfoArgs->quant_a);        // NEW METRIC !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  //  FrameInfoArgs->adaptWeight3 = (uint16_t)Int_FitRange_Int(blkPixCount3, blkPixCount2,                       3, FrameInfoArgs->adapt_a, FrameInfoArgs->quant_a);        // NEW METRIC !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  //  FrameInfoArgs->adaptWeight4 = (uint16_t)Int_FitRange_Int(blkPixCount4, blkPixCount2,                       3, FrameInfoArgs->adapt_a, 3);                             // NEW METRIC !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

  // percentBlksUsed = FrameInfoArgs->MemoryArgs->percentBlksUsed;  // This is computed in LumaBlockInfo.c
  // For quality == 2  we use a default value of 50% which has been set when percentBlksUsed is created at the top of this routine.

  if (quality >= 3) {   // We need LumaBlockInfo() so we have a decent measure of percentBlksUsed.
    percentBlksUsed = FrameInfoArgs->MemoryArgs->percentBlksUsed;  // This is computed in LumaBlockInfo.c
    adapt2 = (uint8_t)Int_FitRange_Float(percentBlksUsed, 54.0f, 6.0f, (float)adaptWeight, 0.0f);

    if (adapt2 > adaptWeight) adapt2 = (uint8_t)adaptWeight;
    if (adapt2 < 1)           adapt2 = 1;
    if (adapt2 < quant)       adapt2 = quant;

    FrameInfoArgs->adapt = adapt2;
    adaptWeight = adapt2;
    FrameQuant = adapt2;
    use_quant = adapt2;
    deblockQuant = adapt2;
    FrameInfoArgs->quant = adapt2;
    FrameInfoArgs->max_quant = MAX(quant, FrameInfoArgs->adapt);
  }

  deblockQuant = adaptWeight;

  // In the case of a small image growing out of a dark background such as a meteor coming at you from a night sky.
  //   Reduce the max adaptation amount for frames that have a high percentage of blocks
  //   that are smooth in the center of the block with only a few areas of detail in the frame.
  //   If the adaptation amount is not reduced then the frame will tend to become over-smoothed
  //   with a visible loss of detail.
  // We need LumaBlockInfo() to provide a good enough measure of percentBlksUsed for the following to work
  if (quality >= 3) {
    if (percentBlksUsed < 10.0) {
      deblockQuant = (uint8_t)Int_FitRange_Float(percentBlksUsed, 10.0f, 1.0f, (float)adaptWeight, (float)FrameInfoArgs->adapt_a);  // This increases the edge deblocking done below.
    }

    //    if (FrameQuant > 6 && FrameQuant < (int)FrameInfoArgs->frameAvgDetE)  FrameQuant = (uint8_t)((FrameQuant + (int)(FrameInfoArgs->frameAvgDetE)+1) >> 1);   // sadEMedDetFrameAvg IS CURRENTLY ALWYS 0 !!!!!!!!!!!!!!!!!!!!!!!!
    //    if (FrameQuant > 6 && FrameQuant < (int)sadEMedDetFrameAvg)  FrameQuant = (uint8_t)((FrameQuant + (int)(sadEMedDetFrameAvg)+1) >> 1);   // sadEMedDetFrameAvg IS CURRENTLY ALWYS 0 !!!!!!!!!!!!!!!!!!!!!!!!
    FrameInfoArgs->FrameQuant = (uint8_t)FrameQuant;
  }

  // Increase deblocking according to the amount of blocking found in the frame.
  deblockQuant = (deblockQuant + adapt + 1) >> 1;
  if (deblockQuant > 1) {  // deblockQuant==1 produces no visible effect.
    //deblock_horiz(psrc, src_height, src_width, src_width, deblockQuant);
    deblock_vert(psrc, src_height, src_width, src_width, deblockQuant);  // Note: Doing deblock_vert() first works slightly better then doing it after deblock_horiz()
    deblock_horiz(psrc, src_height, src_width, src_width, deblockQuant);
  }

  MY_MEMCPY(FrameInfoArgs->MemoryArgs->DctLoopArgs[0].BF_srcP, psrc, sizeY);
  MY_MEMCPY(FrameInfoArgs->psrc, psrc, sizeY);  // The copy from psrc is the adapt frame.
  MY_MEMCPY(FrameInfoArgs->MemoryArgs->frame_workSource, psrc, sizeY);

  FrameInfoArgs->FrameQuant = FrameQuant;
  FrameInfoArgs->adaptWeight = deblockQuant;

  return(0);
}  //  END OF ADAPT
