#include <math.h>

#include "LumaBlockInfo.h"

#include "amDCTtypedefs.h"
//#include "RunningStats.h"
#include "Utilities.h"
#include "FitRange.h"
#include "Memory.h"
#include "FrameInfo.h"
#include "Matrix.h"
#include "DispatchLoop.h"
#include "Threading.h"
#include "DctLoop.h"
#include "avgDctLoop.h"
#include "dct\fdct.h"

void     transfer_8to16copy1_c(int16_t * const dst, uint8_t * src, uint32_t stride);
void	 test_transfer_16to8copy_c(uint8_t * dst, int16_t * src, uint32_t stride);
uint32_t get_coeff8_energy_c(FrameInfo_args *FrameInfoArgs, uint8_t * src, int startPix);
uint32_t coeff8_energy_c(const int16_t *dct);

//void  Get_DCTFrame(FrameInfo_args *FrameInfoArgs);



		//
		//  FUTURE  LOCATIONAL SKEWNESS OF EXTREAM VALUES TO REDUCE EXPANSION OF A BLOCK
		//          SEE WHAT INFO CAN BE GOTTEN FROM LOOKING AT THE DCT BLOCK
		//
		//  What is the minimum amt of info about a DCT block needed to determine the patterns
		//  that are useful for adapt strength, qtype, and matrix to use.
		//  DCT block  64x16bit 64x65,000 = 4,000,000 states.  Just a tad to large to code each one by hand.
		//  Trim the high frequency components, diagonal, row 0 col 7 to row 7 col 0, to row 7 col 7.  That gets rid of 1/2 the data.
		//  Convert the int_16t data to unsigned 4 bit data.  abs(val)>>12 This will probably need to be biased based on the diagonal number.
		//  This leaves us with 32x4bit or 512 states. 320,000 states?
		//  More importantly it leaves us with data that can be displayed side by side with the original displayed block so we can look for patterns.
		//
		// ALSO study Recovering Missing Coefficients in DCT-Transformed Images paper

// LumaBlockInfo() computes the strength that should be used for smoothing, range expansion, and for sharpening for each macro-block in the frame.
//                 It also computes the percentBlksUsed which adapt() uses for more accurate determination of the frame blockiness.
//
//
// The per block adaptive amount for smoothing is stored in LumaPerBlockArgs[curBlock].blkAdaptSmooth
//     It is computed using detDifEC + sad3x3avg.
//
// The per block adaptive amount for range expansion is stored in LumaPerBlockArgs[curBlock].blkAdaptExpand
//     It is computed using the average of the increase between the sad of the source frame block and the sharp frame block, + coeff8_energy.
//	   Note sad is really the sum of the absolute differences from the mean of the items.  NOTE TRY USING REAL SAD OF THE COOROSPONDING PIXELS BETWEEN THE BLOCKS!!!
//
// The per block adaptive amount for sharpening is stored in LumaPerBlockArgs[curBlock].blkAdaptSharp
//     It is computed by taking the average of blkAdaptExpand and coeff8_energy

// USED ITEMS
// adapt()
//   usedBlks
//     detDifECF
//       detE/detC
//         sadE/(float)maxMinDifE
//            pvalArrE[k]) - avgLumaE
//   percentBlksUsed
//
// DCTLoop()
//   blkAdaptSmoothAmt
//     detDifEC   see above
//     sad3x3avg
//   blkAdaptExpandAmt
//     avgLumaVal
//   blkAdaptSharpAmt
//     sadIncrease     // only one that is using sharp frame.
//     coeff8_energy

//LumaBlockInfo()
//	read
//		frame_workSource
//		psrc    // FrameInfoArgs->psrc
//		frame_boundaries

//////////////////////
//////////////////////
//
//  WE SHOULD HAVE A VERSION FOR QUALITY==2 OPTIMIZED THAT WILL JUST DETERMINE THE percent used blocks  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//////////////////////
//////////////////////

void  LumaBlockInfo(FrameInfo_args *FrameInfoArgs) {

	LumaPerBlock_args  *LumaPerBlockArgs   = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs;
	uint8_t 	   	   *referenceSource    = FrameInfoArgs->frame_workSource;
//	uint8_t 	   	   *psrc               = FrameInfoArgs->psrc;
//	uint8_t  		   *psmoothed          = FrameInfoArgs->frame_smoothed;
//	uint8_t 	   	   *pBnd               = FrameInfoArgs->frame_boundaries;
	uint32_t            numBlocks_wide     = FrameInfoArgs->numBlocks_wide;
	uint32_t            numBlocks_high     = FrameInfoArgs->numBlocks_high;


//if (FrameInfoArgs->T2==8)	psrc = FrameInfoArgs->frame_grainMask;  // output from DCT has been stored in the grain mask.
//if (FrameInfoArgs->T2==8)	psrc = FrameInfoArgs->frame_smoothed;  // output from DCT has been stored in the grain mask.



					// Arrays of precomputed values.
	uint8_t 	   *AvgLumaBlockProtectArr = FrameInfoArgs->MemoryArgs->AvgLumaBlockProtectArr;
	uint8_t			QtypeNormArr[64];

	uint8_t			pvalArr[64];
	uint8_t			pvalArrC4[16];      // 36 = 4x4  size of center of 8x8 block
	uint8_t			pvalArrC[36];      // 36 = 6x6  size of center of 8x8 block
//	uint8_t			pvalArrE1[36];     // 36 = 10x2 + 8x2 Number of pixels in edge of 10x10 block.
	uint8_t			pvalArrE[64];      // 64 = 28 + 36         28 = 8x2 + 6x2 Number of pixels in inside edge of 8x8 block.  36 = 10x2 + 8x2 Number of pixels in ring around 8x8 block.

	uint16_t   		sadZeroEDet = 1;      // 10x10 & 8x8 edge    used for blkAdaptExpand

	uint8_t  		brightMax   = 0;
	uint8_t  		brightMaxC  = 0;
	uint8_t  		brightMaxC4 = 0;
	uint8_t  		brightMaxE  = 0;
	uint8_t  		brightMaxE1 = 0;

	uint8_t  		brightMin   = 255;
	uint8_t  		brightMinC  = 255;
	uint8_t  		brightMinC4 = 255;
	uint8_t  		brightMinE  = 255;
	uint8_t  		brightMinE1 = 255;

	uint16_t  		maxMinDif   = 0;
	uint16_t  		maxMinDifC  = 0;
//	uint16_t  		maxMinDifC4 = 0;
	uint16_t  		maxMinDifE  = 0;
//	uint16_t  		maxMinDifE1 = 0;

//	uint16_t         cntFlat = 0;
	uint16_t         cntDark = 0;

//  uint16_t  		cntFlatC  = 0;
//	uint16_t  		cntFlatC4 = 0;
//	uint16_t  		cntFlatE  = 0;


	float 			fsad1 = 0.0F;

	uint32_t		usedBlks = 0;
	uint32_t  		numBlks  = FrameInfoArgs->numBlocksY;  // numBlocks_wide * numBlocks_high

	int 			stride   = numBlocks_wide<<3;

	uint8_t	blockQuant = 0;
	int temp2; //, temp3; // , temp4;

	uint16_t  k, kLast;
	int16_t   pixRowS, pixColS;
	uint16_t  blkRow,  blkCol;

	int32_t DifEC_EC2 = 0;


	uint8_t quant   = FrameInfoArgs->quant_a;
	uint8_t adapt   = FrameInfoArgs->adapt_a;
	uint8_t expand  = FrameInfoArgs->expand;
	uint8_t quality = FrameInfoArgs->quality;



	// If Adapt <= Quant then adaptivity is turned off.
	// For each block set blkAdaptSmooth to quant
	//     and         blkAdaptExpandAmt to expand.
	if (quality == 3 && adapt <= quant && expand <= 16) {
		stride = numBlocks_wide<<3;
		for (blkRow = 1; blkRow < numBlocks_high - 1; blkRow++) {         // THESE LOOPS WALK THROUGH THE FRAME BLOCK BY BLOCK.
			uint32_t BlockCntStartRow = blkRow * numBlocks_wide;

			for (blkCol = 1; blkCol < numBlocks_wide - 1; blkCol++) {
				uint32_t curBlock = blkCol + BlockCntStartRow;

				LumaPerBlockArgs[curBlock].blkAdaptSmooth = quant;
				FrameInfoArgs->MemoryArgs->DctLoopArgs[0].blkAdaptSmoothAmt[curBlock] = quant;

				LumaPerBlockArgs[curBlock].blkAdaptExpand = expand;
				FrameInfoArgs->MemoryArgs->DctLoopArgs[0].blkAdaptExpandAmt[curBlock] = expand;
			}
		}

		return;
	}


/*
//	// FUTURE ENHANCEMENT!  ANALYSE DCT VALUES FOR EACH BLOCK TO DETERMINE SPECIFIC CHANGES IN THE BLOCK
//	// NOTE START OF CODE IS IN ROUTINE AT BOTTOM OF THE FILE!!!
//	// Get the dct of the block and see if it is useful
//	__declspec(align(16)) int16_t    dct_block[64];
//
//	// use pvalArr
//	transfer_8to16copy_xmm(dct_block, BF_workP, rowStride);
//	fdct_sse2_skal(dct_block);
*/

	FrameInfoArgs->frameAvgDetE                = 0.0F;
	FrameInfoArgs->frameAvgDetC                = 0.0F;
	FrameInfoArgs->MemoryArgs->usedBlks        = 0;
	FrameInfoArgs->MemoryArgs->percentBlksUsed = 0.0F;


	// Increase the strength of block adaptive smoothing for
	// blocks that are part of areas that have had bright highlight smoothing done on them.
	// These are pixels with value > bright start.
	//	buildLumaCNormVals(LumaCNormArr, FrameInfoArgs->quant_a, FrameInfoArgs->adapt_a);  NOT IMPLIMENTED

	// Make the block quant values fall in the range of quant and adapt strengths.
	buildBlkQuantNormVals(QtypeNormArr, FrameInfoArgs->quant_a, FrameInfoArgs->adapt_a);






	stride = numBlocks_wide<<3;
	for (blkRow = 1; blkRow < numBlocks_high - 1; blkRow++) {         // THESE LOOPS WALK THROUGH THE FRAME BLOCK BY BLOCK.
		uint32_t BlockCntStartRow = blkRow * numBlocks_wide;
		uint32_t PixStartCurRow   = (blkRow<<3)*stride;

		for (blkCol = 1; blkCol < numBlocks_wide - 1; blkCol++) {
			uint32_t curBlock = blkCol + BlockCntStartRow;
			uint32_t startPix = PixStartCurRow + (blkCol<<3);       // THE STARTING PIXEL LOCATION OF THE CURRENT BLOCK

			uint8_t pval              = 0;
			uint8_t pvalC             = 0;
			uint8_t pvalC4            = 0;
//			uint8_t pvalE1            = 0;

			int     sum               = 0;
			int     sumC              = 0;
			int     sumE              = 0;
//			int     sumE1             = 0;

			float   avgLuma           = 0.0F;
			float   avgLumaC          = 0.0F;
			float   avgLumaE          = 0.0F;

			float   sad               = 0.0F;  // Sum  of Absolute Differences
			float   sadC              = 0.0F;  // Sum  of Absolute Differences
			float   sadE              = 0.0F;  // Sum  of Absolute Differences
			float   sadZeroE          = 0.0F;  // Sum  of Absolute Differences  From Zero

			float   detC              = 0.0F;  // detail center of block
			float   detC2             = 0.0F;  // detail center of block
			float   detE              = 0.0F;  // detail edge   of block includes 8x8 edge and 10x10 edge

			float   detDifECF         = 0.0F;
			int     detDifEC          = 0;
			int     detDifEC2         = 0;


			uint8_t	ArrC4I = 0;
			uint8_t	ArrCI  = 0;
			uint8_t	ArrEI  = 0;
			uint8_t	ArrE1I = 0;
			uint8_t	ArrI   = 0;

			uint8_t *pref  = referenceSource;
			uint8_t *puse  = pref;     // pref is slightly better then psrc

			if (FrameInfoArgs->doSmoothFlag)	puse = FrameInfoArgs->frame_smoothed;  // Seems to work slightly better than pref.

			brightMax    = 0;
			brightMaxC4  = 0;
			brightMaxC   = 0;
			brightMaxE   = 0;
			brightMaxE1  = 0;

			brightMin    = 255;
			brightMinC4  = 255;
			brightMinC   = 255;
			brightMinE   = 255;
			brightMinE1  = 255;

			maxMinDif    = 0;
			maxMinDifC   = 0;
			maxMinDifE   = 0;

			memset(pvalArr,   0, 64);
			memset(pvalArrC4, 0, 16);
			memset(pvalArrC,  0, 36);
			memset(pvalArrE,  0, 64);
//			memset(pvalArrE1, 0, 36);


			kLast = 0;


			// Get info for the outside side edge pixels of a block.
			// These are the 'I'ndex of the next location for each array.
			ArrC4I = 0;  // 4x4 block
			ArrCI  = 0;  // 6x6 block
			ArrEI  = 0;  // 10x10 edge and 8x8 edge
			ArrE1I = 0;  // 10x10 edge
			ArrI   = 0;  // 8x8 block


			if (blkRow > 0 && blkRow < numBlocks_high - 1 && blkCol > 0 && blkCol < numBlocks_wide - 1) {
				for (pixRowS = 0; pixRowS < 8; pixRowS++) {
					uint32_t strideXpixRowS = stride*pixRowS;
					for (pixColS = 0; pixColS < 8; pixColS++) {
						//  NOTE startPix = ((blkRow<<3)*stride) + (blkCol<<3); // THE STARTING PIXEL LOCATION OF THE CURRENT BLOCK
						uint32_t idxPixRC = startPix + (pixColS + strideXpixRowS);

						// At the start of an 8x8 block

						if (((pixRowS == 0) || (pixRowS == 7)) && (pixColS >= 0) && (pixColS <= 7) ||
							((pixColS == 0) || (pixColS == 7)) && (pixRowS >= 0) && (pixRowS <= 7)) {

							pval = puse[idxPixRC];

							// Do the edge of the 8x8 block
							pvalArr[ArrI++]   = pval;
							sum              += pval;


							// Do the 8x8 boarder of the  10x10 edge.
							pvalArrE[ArrEI++] = pval;
							sumE             += pval;

							if (pval > brightMax)  brightMax  = pval;  // 8x8 edge of the full 8x8 block
							if (pval < brightMin)  brightMin  = pval;

							if (pval > brightMaxE) brightMaxE = pval;  // 8x8 edge of the 8x8 edge and 10x10 edge
							if (pval < brightMinE) brightMinE = pval;
						}

						if (pixRowS >= 1 && pixRowS <= 6 && pixColS >= 1 && pixColS <= 6) {  // We are in 6x6 section.
							pvalC = puse[idxPixRC];

							// Do the 6x6 info.
							pvalArrC[ArrCI++]  = pvalC;
							sumC              += pvalC;

							// Do the center of the 8x8 info.
							pvalArr[ArrI++]    = pvalC;
							sum               += pvalC;

							if (pvalC > brightMaxC) brightMaxC = pvalC;  // 6x6
							if (pvalC < brightMinC) brightMinC = pvalC;

							if (pvalC > brightMax)  brightMax  = pvalC;  // 6x6 center part of the full 8x8 block
							if (pvalC < brightMin)  brightMin  = pvalC;

							if (pixRowS >= 2 && pixRowS <= 4 && pixColS >= 2 && pixColS <= 4) {  // We are in 4x4 section.
								pvalC4 = puse[idxPixRC];
								if (pvalC4 > brightMaxC4)  brightMaxC4  = pvalC4;
								if (pvalC4 < brightMinC4)  brightMinC4  = pvalC4;

								if (pvalC4 > brightMax)  brightMax  = pvalC4;  // 6x6 center part of the full 8x8 block
								if (pvalC4 < brightMin)  brightMin  = pvalC4;
								pvalArrC4[ArrC4I++]  = pvalC4;
							}
						}
					}
				}
			}


			avgLuma     = ((float)sum)/64.0F;
			avgLumaC    = ((float)sumC)/36.0F;
			avgLumaE    = ((float)sumE)/64.0F;   // 8x8 edge and 10x10 edge.  (16 + 12) + (20 + 16)

			LumaPerBlockArgs[curBlock].AvgLuma = (uint8_t)(avgLuma + .5);
			LumaPerBlockArgs[curBlock].AvgC    = (uint8_t)(avgLumaE + .5);

			maxMinDif    = brightMax   - brightMin;    // 8x8 block
			maxMinDifC   = brightMaxC  - brightMinC;   // 6x6 center of block
			maxMinDifE   = brightMaxE  - brightMinE;   // 10x10 edge of block plus 1 pixel surround. captures the block edge to block edge boundary differences.

			LumaPerBlockArgs[curBlock].brightMin = brightMin;
			LumaPerBlockArgs[curBlock].brightMax = brightMax;

			if (maxMinDif   < 1) maxMinDif   = 1;
			if (maxMinDifC  < 1) maxMinDifC  = 1;
			if (maxMinDifE  < 1) maxMinDifE  = 1;

			LumaPerBlockArgs[curBlock].maxMinDif   = maxMinDif; ///64.0F;


			if (avgLumaC < 64.0F) cntDark++;  // Used to compute the "percent dark blocks" which is used by adapt so dark frames are not over smoothed.


			// The Max possible sad is 8192 = 64 * 128   For edge or whole block. This will occur if half of the pixels in block are 0 and the other half are 255
			// The Max possible sad is 4608 = 36 * 128   For pvalArrE1 or pvalArrC. This will occur if half of the pixels in block are 0 and the other half are 255

			// Add the 8x8 values to the RS_ routines.
//			RS_Init(&pvalArr[0]);
//			RS_Clear();
			//for (k = 0; k < 64; k++) 	RS_Push(pvalArr[k]);
//			LumaPerBlockArgs[curBlock].RS_Mean        = RS_Mean();
//			LumaPerBlockArgs[curBlock].RS_Variance    = RS_Variance();
//			//LumaPerBlockArgs[curBlock].RS_StDeviation = RS_StDeviation(pvalArr);
//			LumaPerBlockArgs[curBlock].RS_StDeviation = RS_StDeviation();
//			LumaPerBlockArgs[curBlock].RS_Skewness    = RS_Skewness();
//			LumaPerBlockArgs[curBlock].RS_Kurtosis    = RS_Kurtosis();

			//LumaPerBlockArgs[curBlock].RS_Mean        = avgLuma;



			for (k = 0; k < 64; k++) 	sad   += (float)fabs(((float)pvalArr[k])   - avgLuma);
			for (k = 0; k < 36; k++)	sadC  += (float)fabs(((float)pvalArrC[k])  - avgLumaC);
			for (k = 0; k < 64; k++) 	sadE  += (float)fabs(((float)pvalArrE[k])  - avgLumaE);

			if ((sadE+sadC) == 0) sadE = 1;

			LumaPerBlockArgs[curBlock].sadf       = sad; ///64.0F;
			LumaPerBlockArgs[curBlock].maxMinDifC = maxMinDifC;


			// These sadZeroE1 make for interesting detectors
			for (k = 0; k < 64; k++)  sadZeroE  += (float)abs(128-pvalArrE[k]);
			sadZeroEDet = (uint16_t)ROUND_TOINT((sadZeroE * maxMinDifE)/64);    // 10x10 + 8x8 edge

			fsad1 = sad;

			if      (sadC       <= 0.5F) detC  = 0.0F;
			else if (maxMinDifC == 0)    detC  = sadC;
			else {                       detC  = sadC/(float)maxMinDifC;	    // range is 0-2048
										 detC2 = sadC*(float)(31-maxMinDifC);	// range is 0-2048          // WHY IS THIS THE ONLY ONE THAT DOES MULT!!!!
			}


			if      (sadE      <= 0.5F)  detE = 0.0F;
			else if (maxMinDifE == 0)    detE = sadE;
			else                         detE = sadE/(float)maxMinDifE;	// range is 0-512

			if (detC  < 1.0F)  detC  = 1.0F;
			if (detC2 < 1.0F)  detC2 = 1.0F;
			if (detE  < 1.0F)  detE  = 1.0F;

			detDifECF = fabsf(detE - detC);
			detDifEC  = (int)(((detE*detC)/(detE+detC)) / 1.0F);
			detDifEC2 = (int)(((detE*detC2)/(detE+detC2)) / 1.0F);

			if (detDifEC2 <  1) detDifEC2 = 1;
			if (detDifEC2 > 31) detDifEC2 = 31;

			if (detDifEC < 1)  detDifEC = 1;


			if (maxMinDifC < 4) sadC = 0;

			LumaPerBlockArgs[curBlock].detDifEC  = (uint16_t)detDifECF;
			LumaPerBlockArgs[curBlock].detDifEC2 = (uint16_t)detDifEC2;

			if (sumE == 0 || sumC == 0) detDifEC = 0;  // These could be set much higher like 16 up-to 32

			LumaPerBlockArgs[curBlock].detC  = (uint16_t)(detC + 0.5F);
			LumaPerBlockArgs[curBlock].detC2 = (uint16_t)(detC2 + 0.5F);
			LumaPerBlockArgs[curBlock].detE  = (uint16_t)(detE + 0.5F);

			FrameInfoArgs->frameAvgDetE	+= detE;
			FrameInfoArgs->frameAvgDetC	+= detC;

			// Doing a SAD on detDifEC might give a good indicator for the whole frame blockiness.
			// On a block by block bases You may want to deblock more on both large AND small values.
			// Very low detC and Very low detDifEC of 2 adjacent blocks and a small but not 0 difference
			// in the avgLumas for each block means a higher deblock strength for each of the blocks since we
			// want to smooth out or eliminate the difference.  The more surrounding blocks that meet this
			// criteria the larger the increase in deblock strength should be.




						// TEST NON AVG ADAPTIVE SMOOTH
						//temp = (uint16_t)(detDifEC + (LumaPerBlockArgs[curBlock].sad) + 0.5) >> 1;
						//if (temp >= MAX_BLOCK_ADAPT) temp = MAX_BLOCK_ADAPT - 1;
						//if (temp <= 0) temp = 0;
						//LumaPerBlockArgs[curBlock].blkAdaptSmooth = (uint8_t)temp;


			temp2 = sadZeroEDet>>9;
			if (temp2 > 31) temp2 = 31;
			temp2 = 31-temp2;


			LumaPerBlockArgs[curBlock].blkAdaptExpand = (uint8_t)temp2;
			FrameInfoArgs->MemoryArgs->DctLoopArgs[0].blkAdaptExpandAmt[curBlock] = (uint8_t)temp2;


			temp2 = LumaPerBlockArgs[curBlock].detDifEC2;
			if (temp2 > 31) temp2 = 31;


			// Eight things affect the amount of smoothing done on a block.
			// Quant_a     The user argument.  It sets the minimum amount of smoothing a block can have.
			// Adapt_a     The user argument.  It sets the maximum amount of smoothing a block can have.  If Adapt <= Quant then adaptivity is turned off and all blocks have quant smoothing and we shouldn't have even come in here.
			// frame_quant The computed maximum amount of smoothing a block can have.
			// detDifEC2   which is (detE*detC2)/(detE+detC2) a measure of blockiness.  THIS IS A POOR MEASURE OF WHAT WE WANT.  REAL GAINS IN ADAPTIVITY CAN BE MADE BY IMPROVING ON THIS!!!!!!!!
			// avgLuma     The average luma of the block.
			// darkStart   The brightness where dark protection starts.
			// darkAmt     The amount of dark protection applied.
			// brightStart The brightness that we start to increase smoothing.
			// brightAmt   The amount of extra smoothing applied.
			// brightMax   The brightest pixel in the frame.
			//
			// FUTURE see if sad of surrounding blocks are similar and look like blocking, low detail etc., and HD then increase low frequency smoothing
			// possibly switching to qtype 2 or 4, for this and all of the surrounding blocks.

			// DifEC_EC2 has a range of 0-12
			DifEC_EC2 = ((detDifEC) - (detDifEC2));
			LumaPerBlockArgs[curBlock].detDifEC4 = DifEC_EC2;
			//LumaPerBlockArgs[curBlock].detDifEC2 = DifEC_EC2;
			// LumaPerBlockArgs[curBlock].detDifEC2 has range 0-31

			blockQuant = (uint8_t)LumaPerBlockArgs[curBlock].detDifEC2;

			// Noise is not as visible in dark   areas. Reduce  smoothing.
			// Noise is more   visible in bright areas. Increase smoothing.
			// TODO.  FOR RELEASE 1.0   USE BOTH avgLuma AND RANGE OF VALUES.  WE WANT A MEASURE OF THE SKEWNESS
			temp2 = AvgLumaBlockProtectArr[((int)(avgLuma) << AVGLUMA_BLOCK_PROTECT_SHIFT) + DifEC_EC2]; // Change block smoothing based upon average brightness of the block.

			LumaPerBlockArgs[curBlock].blkAdaptSmooth = (uint8_t)temp2;
			if (FrameInfoArgs->qtype == 1) {
				FrameInfoArgs->MemoryArgs->DctLoopArgs[0].blkAdaptSmoothAmt[curBlock] = (uint8_t)temp2;
			}
			else {
				FrameInfoArgs->MemoryArgs->DctLoopArgs[0].blkAdaptSmoothAmt[curBlock] = QtypeNormArr[temp2];  // precomputed Int_FitRange_Int(temp2, 1, 31, FrameInfoArgs->quant, FrameInfoArgs->adapt)
			}



			// We don't want to count any blocks that are in dark "flat" areas since they will rarely show blocking.
			// Note that detDifECF looks at a 9x9 block of pixels.  This will pick up the cases were a frame is very bit-starved
			// and adjacent blocks have different average values.  This could be used to switch to a different matrix or to
			//  change qtype on a block by block basis.
			if (detDifECF < 3.0F) continue;
			// if (sadC < 10.0F || maxMinDifC <= 4 || maxMinDifE <= 5) continue;
			if (((sadC + (((uint8_t)avgLumaC)>>5)) < 20) || maxMinDifC <= 4 || maxMinDifE <= 5) continue;

			FrameInfoArgs->MemoryArgs->usedBlks++;
			FrameInfoArgs->usedBlks++;
			usedBlks++;
			LumaPerBlockArgs[curBlock].framedetDifECvalsF = detDifECF;  // We use framedetDifECvalsF as a place to store the values we will use to compute the frame as a whole info.

		}
	}

//	FrameInfoArgs->PercentFlat     = ((float)cntFlat  / (float)FrameInfoArgs->numBlocksY) * 100;
//	FrameInfoArgs->percentFlatC    = ((float)cntFlatC / (float)FrameInfoArgs->numBlocksY) * 100;
//	FrameInfoArgs->percentFlatE    = ((float)cntFlatE / (float)FrameInfoArgs->numBlocksY) * 100;
	FrameInfoArgs->percentBlksUsed = ((float)FrameInfoArgs->MemoryArgs->usedBlks / (float)FrameInfoArgs->numBlocksY) * 100;  // USED BY adapt.c
	FrameInfoArgs->MemoryArgs->percentBlksUsed = FrameInfoArgs->percentBlksUsed;



	// We have been storing the sums now turn them into averages.
	FrameInfoArgs->frameAvgDetE	    = (float)(FrameInfoArgs->frameAvgDetE / (float)(numBlks));
	FrameInfoArgs->frameAvgDetC	    = (float)(FrameInfoArgs->frameAvgDetC / (float)(numBlks));

//	FrameInfoArgs->PercentFlat     = ((float)cntFlat / (float)FrameInfoArgs->numBlocksY) * 100.0F;
//	FrameInfoArgs->PercentDark     = ((float)cntDark / (float)numBlks) * 100.0F;



	return;

 }





/*
// In BuildPerBlockMatrix.c
// buildPerBlockMatrix(FrameInfo_args *FrameInfoArgs)

 //      GET THE DCT FRAME INTO THE DERRING FRAME
void  Get_DCTFrame(FrameInfo_args *FrameInfoArgs) {

	LumaPerBlock_args *LumaPerBlockArgs = FrameInfoArgs->MemoryArgs->LumaPerBlockArgs;
	uint8_t 	   	  *psrc             = FrameInfoArgs->psrc;
	uint32_t           numBlocks_wide   = FrameInfoArgs->numBlocks_wide;
//	uint32_t           numBlocks_high   = FrameInfoArgs->numBlocks_high;
	uint8_t 		   T2 				= FrameInfoArgs->T2;

	uint8_t   save_FrameQuant = FrameInfoArgs->FrameQuant;
	uint8_t   save_quant	  = FrameInfoArgs->quant;
	uint8_t   save_qtype	  = FrameInfoArgs->qtype;
	uint8_t   save_adapt	  = FrameInfoArgs->adapt;
	uint8_t   save_shift      = FrameInfoArgs->shift;
	uint8_t   save_matrix     = FrameInfoArgs->matrix;
	uint8_t   save_expand     = FrameInfoArgs->expand;
	uint8_t   save_sharpWPos  = FrameInfoArgs->sharpWPos;
	uint8_t   save_sharpWAmt  = FrameInfoArgs->sharpWAmt;
	uint8_t   save_sharpTPos  = FrameInfoArgs->sharpTPos;
	uint8_t   save_sharpTAmt  = FrameInfoArgs->sharpTAmt;
	uint8_t   save_showMask   = FrameInfoArgs->showMask;

//	uint32_t  sizeY          = FrameInfoArgs->sizeY;

	uint8_t   FrameQuant      = FrameInfoArgs->FrameQuant;
	uint8_t   use_showMask    = 19;

//	 int16_t  pixRowS, pixColS;
//	uint16_t  blkRow,  blkCol;
//	int 	  stride = numBlocks_wide<<3;

	__declspec(align(16)) int16_t   dct_block[MATRIX_SIZE];
//	__declspec(align(16)) uint8_t   newMatrix[MATRIX_SIZE];



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
		uint8_t use_shift     = 0;
		uint8_t use_quant     = 1;
		uint8_t use_adapt     = 0;
		uint8_t use_qtype     = 1;
		uint8_t use_expand    = 0;
		uint8_t use_matrix    = MATRIX_FLAT3;  // matrix_252_Flat_3
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
		FrameInfoArgs->quant	  = save_quant;
		FrameInfoArgs->qtype	  = save_qtype;
		FrameInfoArgs->adapt	  = save_adapt;
		FrameInfoArgs->shift      = save_shift;
		FrameInfoArgs->matrix     = save_matrix;
		FrameInfoArgs->expand     = save_expand;
		FrameInfoArgs->sharpWPos  = save_sharpWPos;
		FrameInfoArgs->sharpWAmt  = save_sharpWAmt;
		FrameInfoArgs->sharpTPos  = save_sharpTPos;
		FrameInfoArgs->sharpTAmt  = save_sharpTAmt;
		FrameInfoArgs->showMask   = save_showMask;
	}

}

*/












//	if (FrameInfoArgs->showMask == 19) {
//		uint8_t use_shift     = FrameInfoArgs->shift;  // WILL BECOME 0  !!!!!!!!!!!!!!!!!!!!!!!!!
//		uint8_t use_quant     = 1;
//		uint8_t use_adapt     = 0;
//		uint8_t use_qtype     = 1;
//		uint8_t use_expand    = 0;
//		uint8_t use_matrix    = MATRIX_FLAT3;
//		uint8_t use_sharpWAmt = 0;
//		uint8_t use_sharpTAmt = 0;
//		uint8_t use_sharpWPos = 7;
//		uint8_t use_sharpTPos = 7;
//
//		doFrameInfo(FrameQuant,
//					use_quant,
//					use_qtype,
//					use_adapt,
//					use_shift,
//					use_matrix,
//
//					use_expand,
//					use_sharpWPos,
//					use_sharpWAmt,
//					use_sharpTPos,
//					use_sharpTAmt,
//
//					FrameInfoArgs,
//
//					use_showMask,
//					T2);
//
//		set_matrix(FrameInfoArgs, use_matrix, use_qtype, use_quant, use_expand);
//		dup_cpu_data(FrameInfoArgs);
//		DispatchLoop(FrameInfoArgs);
//		avgDctLoopAccumDCT(FrameInfoArgs);






//	uint8_t use_shift_arg     = shift;
//	uint8_t use_qtype_arg     = 1;
//	uint8_t use_expand_arg    = 0;
//	uint8_t use_matrix_arg    = 10;  // All matrix 10 values are 3.  Could be user arg for showmask = 18
//	uint8_t use_sharpWAmt_arg = 0;
//	uint8_t use_sharpTAmt_arg = 0;
//	uint8_t use_quant_arg     = 1;
//
//	doFrameInfo(FrameQuant,
//				use_quant_arg,
//				use_qtype_arg,
//				use_adapt,
//				use_shift_arg,
//				use_matrix_arg,
//
//				use_expand_arg,
//				FrameInfoArgs->sharpWPos,
//				use_sharpWAmt_arg,
//				FrameInfoArgs->sharpTPos,
//				use_sharpTAmt_arg,
//
//				&FrameInfoArgs,
//
//				FrameInfoArgs->showMask,
//				FrameInfoArgs->T2);
//
//
//	set_matrix(&FrameInfoArgs, use_matrix_arg, use_qtype_arg, use_quant_arg, use_expand_arg);
//
//	dup_cpu_data(&FrameInfoArgs);
//	DispatchLoop(&FrameInfoArgs);
//
//	// We don't want the DCT transformed image to have any bright or dark post processing done on it.
//	FrameInfoArgs.doDarkFlag   = 0;
//	FrameInfoArgs.darkStart    = 0;
//	FrameInfoArgs.darkAmt      = 0;
//
//	FrameInfoArgs.doBrightFlag = 0;
//	FrameInfoArgs.brightStart  = 255;
//	FrameInfoArgs.brightAmt    = 0;
//
//	avgDctLoopAccum(&FrameInfoArgs);

//	return;
// */















/*
	// THIS BLOCK OF THE CODE IS NOT USED BUT IS LEFT IN SO DEBUG INFORMATION AND MASKS CAN BE OUTPUT
	//if (showMask > 0) {
	if (1==0) {
			// This code computes for the frame as a whole the difference between the detail of the center of the blocks
		// and the detail of the edges of the blocks.
		// This value might be useful as a way to normalize the values of each block.  The difference between the frame value and the block value might be useful.
		//
		// Now we compute the sad of the sads of all of the blocks.
		smoothBlockCor = (float)sadZeroE1 / (float)((numBlocks_wide - 2) * (numBlocks_high - 2));  // (numBlocks_high - 2) minus 2 because we don't use the edge of frame blocks.
		framedetDifECavg = 0.0F;

		for (blkRow = 1; blkRow < numBlocks_high-1; blkRow++) {
			for (blkCol = 1; blkCol < numBlocks_wide-1; blkCol++) {
				uint32_t curBlock = blkCol + (blkRow * numBlocks_wide);

				framedetDifECavg  += LumaPerBlockArgs[curBlock].framedetDifECvalsF;
			}
		}


		// We use percentBlksUsed as a correction for the number of smooth blocks skipped since if frame is 90% flat black background with a person
		// highlighted in the center of it with just a few macro-blocks that we compute are blocky we don't want to over-smooth it.
		framedetDifECavg = framedetDifECavg / sadZeroE1; // * FrameInfoArgs->MemoryArgs->percentBlksUsed;
		framedetDifECsad = 0.0F;

		for (blkRow = 1; blkRow < numBlocks_high-1; blkRow++) {
			for (blkCol = 1; blkCol < numBlocks_wide-1; blkCol++) {
				uint32_t curBlock = blkCol + (blkRow * numBlocks_wide);

				framedetDifECsad += (float)fabs((float)(LumaPerBlockArgs[curBlock].framedetDifECvalsF - framedetDifECavg));
			}
		}

		framedetDifECsad = (framedetDifECsad / blkCnt) * FrameInfoArgs->MemoryArgs->percentBlksUsed;
		//framedetDifECsad = (framedetDifECsad/blkCnt)*100.0F;
		FrameInfoArgs->frameDetDifECsad = (float)(framedetDifECsad * 10.0F);
		FrameInfoArgs->framedetDifECavg = (float)(framedetDifECavg * 10.0F);
	} // END SHOWMASK CODE
*/

//	return;
//}



/* returns squared deviates (mean(v*v)-mean(v)^2) of a 8x8 block */
double sqr_dev(uint8_t v[64])
{
	double     sum  = 0.0;
	double     sum2 = 0.0;
	uint8_t    n;
	for (n=0; n < 64; n++)
	{
		sum  += v[n];
		sum2 += v[n]*v[n];
	}
	sum2 /= n;
	sum  /= n;
	return(sum2 - sum*sum);
}






static const int16_t iMask_Coeff[64] = {
		0, 29788, 32767, 20479, 13653, 8192, 6425, 5372,
	27306, 27306, 23405, 17246, 12603, 5650, 5461, 5958,
	23405, 25205, 20479, 13653,  8192, 5749, 4749, 5851,
	23405, 19275, 14894, 11299,  6425, 3766, 4096, 5285,
	18204, 14894,  8856,  5851,  4819, 3006, 3181, 4255,
	13653,  9362,  5958,  5120,  4045, 3151, 2900, 3562,
	 6687,  5120,  4201,  3766,  3181, 2708, 2730, 3244,
	 4551,  3562,  3449,  3344,  2926, 3277, 3181, 3310
};


uint32_t get_coeff8_energy_c(FrameInfo_args *FrameInfoArgs, uint8_t * src, int startPix) {
	int     stride = FrameInfoArgs->numBlocks_wide<<3;

	__declspec(align(16)) int16_t dct_block[64];

	src = src + startPix;
	transfer_8to16copy1_c(dct_block, src, stride);

	fdct_sse2_skal(dct_block);

	return(coeff8_energy_c(dct_block));
}




/* Calculate CSF weighted energy of DCT coefficients */
uint32_t
coeff8_energy_c(const int16_t * dct)
{
	int x, y;
	uint32_t sum_a = 0;

	for (y = 0; y < 8; y += 2) {
		for (x = 0; x < 8; x += 2) {
			int16_t a0 = ((dct[y*8+x]<<4) * iMask_Coeff[y*8+x]) >> 16;
			int16_t a1 = ((dct[y*8+x+1]<<4) * iMask_Coeff[y*8+x+1]) >> 16;
			int16_t a2 = ((dct[(y+1)*8+x]<<4) * iMask_Coeff[(y+1)*8+x]) >> 16;
			int16_t a3 = ((dct[(y+1)*8+x+1]<<4) * iMask_Coeff[(y+1)*8+x+1]) >> 16;

			sum_a += ((a0*a0 + a1*a1 + a2*a2 + a3*a3) >> 3);
		}
	}

	return sum_a;
}


/*
 * SRC - the source buffer
 * DST - the destination buffer
 *
 * Then the function does the 8->16 bit transfer and this serie of operations :
 *
 *    SRC (8bit)  = SRC
 *    DST (16bit) = SRC
 */
void
transfer_8to16copy1_c(int16_t * const dst,
					 uint8_t * src,
					 uint32_t stride)
{
	int i, j;
	for (j = 0; j < 8; j++) {
		for (i = 0; i < 8; i++) {
			dst[(j<<3) + i] = (int16_t) src[j * stride + i];
		}
	}
}



/*
#ifndef RUNNINGSTATS_H
#define RUNNINGSTATS_H

class RunningStats
{
public:
	RunningStats();
	void Clear();
	void Push(double x);
	long long NumDataValues() const;
	double Mean() const;
	double Variance() const;
	double StandardDeviation() const;
	double Skewness() const;
	double Kurtosis() const;

	friend RunningStats operator+(const RunningStats a, const RunningStats b);
	RunningStats& operator+=(const RunningStats &rhs);

private:
	long long n;
	double M1, M2, M3, M4;
};

#endif

// And here is the implementation file RunningStats.cpp.

#include "RunningStats.h"
#include <cmath>
#include <vector>

RunningStats::RunningStats()
{
	Clear();
}

void RunningStats::Clear()
{
	n = 0;
	M1 = M2 = M3 = M4 = 0.0;
}

void RunningStats::Push(double x)
{
	double delta, delta_n, delta_n2, term1;

	long long n1 = n;
	n++;
	delta = x - M1;
	delta_n = delta / n;
	delta_n2 = delta_n * delta_n;
	term1 = delta * delta_n * n1;
	M1 += delta_n;
	M4 += term1 * delta_n2 * (n*n - 3*n + 3) + 6 * delta_n2 * M2 - 4 * delta_n * M3;
	M3 += term1 * delta_n * (n - 2) - 3 * delta_n * M2;
	M2 += term1;
}

long long RunningStats::NumDataValues() const
{
	return n;
}

double RunningStats::Mean() const
{
	return M1;
}

double RunningStats::Variance() const
{
	return M2/(n-1.0);
}

double RunningStats::StandardDeviation() const
{
	return sqrt( Variance() );
}

double RunningStats::Skewness() const
{
	return sqrt(double(n)) * M3/ pow(M2, 1.5);
}

double RunningStats::Kurtosis() const
{
	return double(n)*M4 / (M2*M2) - 3.0;
}

RunningStats operator+(const RunningStats a, const RunningStats b)
{
	RunningStats combined;

	combined.n = a.n + b.n;

	double delta = b.M1 - a.M1;
	double delta2 = delta*delta;
	double delta3 = delta*delta2;
	double delta4 = delta2*delta2;

	combined.M1 = (a.n*a.M1 + b.n*b.M1) / combined.n;

	combined.M2 = a.M2 + b.M2 +
				  delta2 * a.n * b.n / combined.n;

	combined.M3 = a.M3 + b.M3 +
				  delta3 * a.n * b.n * (a.n - b.n)/(combined.n*combined.n);
	combined.M3 += 3.0*delta * (a.n*b.M2 - b.n*a.M2) / combined.n;

	combined.M4 = a.M4 + b.M4 + delta4*a.n*b.n * (a.n*a.n - a.n*b.n + b.n*b.n) /
				  (combined.n*combined.n*combined.n);
	combined.M4 += 6.0*delta2 * (a.n*a.n*b.M2 + b.n*b.n*a.M2)/(combined.n*combined.n) +
				  4.0*delta*(a.n*b.M3 - b.n*a.M3) / combined.n;

	return combined;
}

RunningStats& RunningStats::operator+=(const RunningStats& rhs)
{
		RunningStats combined = *this + rhs;
		*this = combined;
		return *this;
}

*/