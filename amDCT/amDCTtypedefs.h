#ifndef _AMDCT_TYPEDEFS_H_
#define _AMDCT_TYPEDEFS_H_



#include <stdint.h>	  // needed for specifying exact sizes of variables such as uint8_t
#include <stdlib.h>
#include <string.h>


#include "amDCT.h"	      // Converts amDCTmain() interface from C++ to C
#include "amDCTPortab.h"  // needed for MAX MIN, TRUE FALSE,  NULL, and  ROUND_TOINT


#pragma warning(suppress: 28251)
void   *memset(void *src, int c, size_t count);
void *A_memset(void *src, int c, int    count);

#pragma warning(suppress: 28251)
void   *memcpy(void *dest, const void *src, size_t count);
void *A_memcpy(void *dest, const void *src, int    count);


#define MY_MEMSET(src,val,len) memset(src,val,len)
#define MY_MEMCPY(dst,src,len) memcpy(dst,src,len)

// These make use of the Agner Fog routines.  See info in the "CodeCopiedFrom_OriginalSources" directory.
//#define MY_MEMSET(src,val,len) A_memset(src,val,len)
//#define MY_MEMCPY(dst,src,len) A_memcpy(dst,src,len)


// A matrix has 64 entries and the xvid routines may need up to 8 copies of it each one modified for xvids use.
#define MATRIX_SIZE sizeof(uint16_t) * 64 * 8
#define MAX_CPU                4
#define MAX_BLOCK_ADAPT       31
#define MAX_LEVEL_IDX         70
#define BLKSIZE                8
#define BLKNUMPIX             64

//   The following define the sizes and indexing of the lookup tables used for repetitive calculations.
//   Conceptually all of the lookup tables are 2 dimensional, indexed by the srcval "the original pixel value",
//       and diff, the difference between srcval and the new partially computed pixel value.
//   srcval is in the range of 0 to 255.   256 total values.
//   The MAX_..._DIFF values below are the measured maximum differences measured rounded up to the nearest power of 2
//   This allows the precomputing for each src value (0-255) the desired diff value by doing a table lookup.
//          Example call:     newDiff = BrightProtectDifArr[(src << BRIGHT_SMOOTH_SHIFT) + Absdiff];
//  !!!!!!!!!!!!!!!!!!!!!!!!!!!    MAX_ACCUM_LIMIT_SMOOTH_BOUND_DIFF     IS NOT USED  !!!!!!!!!!!!!!!!!!!!
#define MAX_ACCUM_LIMIT_DIFF              256
#define MAX_ACCUM_LIMIT_SHARP_DIFF        128
#define MAX_ACCUM_LIMIT_BOUND_DIFF        128
#define MAX_ACCUM_LIMIT_SMOOTH_BOUND_DIFF 128
#define MAX_BRIGHT_SMOOTH_DIFF            128
#define MAX_DARK_PROTECT_DIFF             128
#define MAX_AVGLUMA_BLOCK_PROTECT         128


// THESE 7 DEFINES MUST MATCH THE 7 DEFINES ABOVE !!!!!!!!!!!
#define ACCUM_LIMIT_SHIFT              7
#define ACCUM_LIMIT_SHARP_SHIFT        7
#define ACCUM_LIMIT_BOUND_SHIFT        7
#define ACCUM_LIMIT_SMOOTH_BOUND_SHIFT 7
#define BRIGHT_SMOOTH_SHIFT            7
#define DARK_PROTECT_SHIFT             7
#define AVGLUMA_BLOCK_PROTECT_SHIFT    7

// These are for simple lookup tables and do not need a _SHIFT define.
#define MAX_SMOOTH_HIGHLIGHT         	  256
#define MAX_BOUNDARY_STRENGTH         	  256


// DEFINE SYSTEM WIDE INTERNAL MATRIX NUMBERS
// NOTE currently MATRIX_SHARP and MATRIX_RANGE_EXPAND have the same values.  This makes it is easy to test different matrix values for sharp or expand independently.
#define MATRIX_SHARP         	  255    // matrix_255_Flat_8     The Sharpening matrix.  All values are 8.
#define MATRIX_PRESMOOTH       	  254    // matrix_254_PreSmooth  Used to presmooth the frame prior to sharpening.
#define MATRIX_FLAT3         	  252	 // matrix_252_Flat_3     All values are 3      USED WHEN YOU WANT MINIMAL CHANGE
#define MATRIX_RANGE_EXPAND    	  251    // matrix_251_Flat_8_RE  The Range Expand Matrix.  All values are 8. NOTE currently the same values as MATRIX_SHARP.




// DEFINE SYSTEM WIDE INTERNAL SHOWMASK NUMBERS
#define SHOW_REF_SRC	   1			//  if (showMask==1) 	psrcP = FrameInfoArgs->frame_refSource  	+ width*8 + 8;     // The frame within the boarder starts boarder rows down and boarder pixels in
#define SHOW_ADAPT_SRC     2			//  if (showMask==2) 	psrcP = FrameInfoArgs->frame_adaptSource   	+ src_pitch*8 + 8; // The frame after adapt()
#define SHOW_WORK_SRC      3			//	if (showMask==3) 	psrcP = FrameInfoArgs->frame_workSource   	+ width*8 + 8;     // The frame after adapt() and PreSmooth() and brightSmooth() have been run.
#define SHOW_PRESMOOTH_1   3			//	if (showMask==3) 	psrcP = FrameInfoArgs->frame_workSource   	+ width*8 + 8;     // The frame after adapt() and PreSmooth().
#define SHOW_PRESMOOTH_2   4			//	if (showMask==4) 	psrcP = psrc_mem                            + width*8 + 8;     // Output the frame after pre deblocking step.
#define SHOW_DEBLOCK	   5			//	if (showMask==5) 	psrcP = FrameInfoArgs->psrc                 + width*8 + 8;     // Output the frame after pre deblocking step.
#define SHOW_PRESMOOTHED   6			//	if (showMask==6) 	psrcP = psrc_mem                            + width*8 + 8;     //src_width*0 + 0; // Output the frame after pre smoothing. Same frame, different number so can debug the presmoothing step.
#define SHOW_SMOOTHED	   7			//  if (showMask==7) 	psrcP = FrameInfoArgs->frame_smoothed       + src_width*8 + 8;
#define SHOW_SHARP		   8			//  if (showMask==8) 	psrcP = FrameInfoArgs->frame_sharp          + src_width*8 + 8;
#define SHOW_BOUNDARIES	   9			//  if (showMask==9) 	psrcP = FrameInfoArgs->frame_boundaries     + src_width*8 + 8;
#define SHOW_IMPULSENOISE 10			//  if (showMask==10) 	psrcP = FrameInfoArgs->frame_boundaries     + src_width*8 + 8;
#define SHOW_SADM	 	  11			//  if (showMask==11) 	psrcP = FrameInfoArgs->frame_boundaries     + src_width*8 + 8;
#define SHOW_3X3MEAN 	  12			//  if (showMask==12) 	psrcP = FrameInfoArgs->frame_boundaries     + src_width*8 + 8;
#define SHOW_DERING	 	  13			//  if (showMask==13) 	psrcP = FrameInfoArgs->frame_boundaries     + src_width*8 + 8;
#define SHOW_LOCALMAX9X9  15			//  if (showMask==15) 	psrcP = FrameInfoArgs->frame_boundaries     + src_width*8 + 8;
#define SHOW_LOCALMIN9X9  16			//  if (showMask==16) 	psrcP = FrameInfoArgs->frame_boundaries     + src_width*8 + 8;

#define SHOW_DCT1	      18			//  Show the dct values for each block
#define SHOW_DCT2	      18			//  Show the dct values for each block


//*/

/*
#ifdef _DEBUG
#define MY_MEMSETx(src,val,len) memset(src,val,len)
#else
#define MY_MEMSETx(src,val,len) A_memset(src,val,len)
#endif


#ifdef _DEBUG
#define MY_MEMCPYx(dst,src,len) memcpy(dst,src,len)
#else
#define MY_MEMCPYx(dst,src,len) A_memcpy(dst,src,len)
#endif
*/

//
// THESE ARE GLOBAL DEBUGGING VARIABLES
//    THIS TELLS THE COMPILER TO USE THE VARIABLES WITH THESE NAMES THAT WERE CREATED IN amDCTmain.c
extern int T2Global;

extern int MaxDCT0;
extern int MinDCT0;

extern int max_debug4;
extern int min_debug4;

extern int max_debug5;
extern int min_debug5;


//	 ncpu;               // The NUMBER of CPUs that can be used specified in the call to amDCT()
//	 sizeY;  	         // The size of the src frame, including the 8 bit boarder on each side, the luma buffer and all internally derived frames, in bytes.  It is equal to src_height*src_width
//
//	 numBlocks_wide;     // The number of columns, in blocks, in the source frame, rounded up + 1.     It is equal to 2*((src_width +15)/16)
//	 numBlocks_high;     // The number of rows,    in blocks, in the source frame, rounded up + 1.     It is equal to 2*((src_height+15)/16)
//	 numBlocksY;         // The total number of blocks in the source frame.                            It is equal to numBlocks_high*numBlocks_wide
//	 sizeBlocksY;        // Size of the buffer that the processing will be done on in pixels.          It is equal to numBlocks_high*numBlocks_wide*64
//	 numBlocksWork;	     // The total number of blocks in the work buffer.  Size of the buffer that the processing will be done on in pixels.  It should be 16 pixels wider and taller then the luma buffer.
//	 sizeBlocksWork;	 // Size of the buffer that the processing will be done on in pixels.  It should be 16 pixels wider and taller then the luma buffer.


// LumaPerBlock_args
typedef struct {

	float       blkSadCent;
	float       blkSumCent;
	float       blkSumEdge1;
	float       blkMeanEdge1;
	float       blkSadEdge1;

//	uint16_t 	curblkPixCount;      // This group is info from adapt()
//	uint16_t 	curdifBinCountUsed;
//	uint16_t 	curblkPixCountUsed;
//	uint16_t 	curblkPixPercent;
	uint16_t 	difBlockTotalUsed;

	// This group,  adap_ is info from adapt()
	uint16_t 	adpt_PixCount1;           // Top row
	uint16_t 	adpt_PixCount2;           // Bottom row
	uint16_t 	adpt_difBinCountUsed1;    // Top row
	uint16_t 	adpt_difBinCountUsed2;    // Bottom row
	uint16_t 	adpt_difPixCountUsed1;    // Top row
	uint16_t 	adpt_difPixCountUsed2;    // Bottom row
	uint16_t 	adpt_difBlockTotalUsed1;  // Top row
	uint16_t 	adpt_difBlockTotalUsed2;  // Bottom row
	uint16_t 	adpt_PixPercent;
	uint16_t 	adpt_PixPercent2;




	uint8_t  	brightMax;	// Block Information for input frame.
	uint8_t  	MaxLuma;
/*
	uint8_t  	MedianLuma;
	uint8_t  	MedianLumaB;
	uint8_t  	MedianLumaC;
	uint8_t  	MedianLumaC1;
	uint8_t  	MedianLumaE;
	uint8_t  	MedianLumaE1;
	uint8_t  	MedianLuma2;
*/
	uint16_t	sumBright;
	uint8_t  	AvgLuma;
	uint8_t  	AvgBound;
	uint8_t  	AvgC;
	uint8_t  	MinLuma;
	uint8_t  	brightMin;
	uint16_t	maxMinDif;  	// Range of values in block
	uint16_t	maxMinDifC;  	// Range of values in block center
	uint16_t	sad;
	float   	sadf;           // floating point sad whole block
	float   	sadCf;          // floating point sadC  center
	float   	sadEf;          // floating point sadE edge
	float   	sadE1f;         // floating point sadE1 edge

	uint16_t   	sadNormEC;      // floating point sadE1 edge

// /*							// sad from median
	float		avgBnd;
	float		sadBMedf;       // 8x8 block
	float		sadCMedf;       // center 6x6
	float		sadC1Medf;      // 8x8 edge. Don't include 6x6 center.
	float   	sadEMedf;       // 9x9 edge and 8x8 edge
	float   	sadZeroE1f;     // 9x9 edge
	float		sadCH2Medf;     // sad of 6x6 minus 2 high and 2 low values.
	float		sadBMedDetf;    // 8x8 block
	float		sadCMedDetf;    // center 6x6
	float		sadC1MedDetf;   // 8x8 edge. Don't include 6x6 center.
	float   	sadEMedDetf;    // 9x9 edge and 8x8 edge
	float   	sadZeroE1Detf;	// 9x9 edge
// */

	float   	std_Dev;
	float   	std_DevC;
	float   	std_DevE;
//	float   	variance;
//	float   	varianceC;
//	float   	varianceE;
//	uint16_t	detH2;
	uint16_t	det;		// detail.  sad divided by range
	uint16_t	detC;
	uint16_t	detC2;
	uint16_t	detC4;
	uint16_t	sadC4;
	uint16_t	detE;
	uint16_t  	detDifEC;
	uint16_t  	detDifEC0;
	uint16_t  	detDifEC1;
	uint16_t  	detDifEC2;
	uint16_t  	detDifEC3;
	int32_t  	detDifEC4;
//	uint32_t    coeff8_energy;


//	uint8_t 	sharp_brightMax;  	// Block Information for sharpened frame.
//	uint8_t 	sharp_MaxLuma;
//	uint8_t 	sharp_MedianLuma;
//	uint8_t 	sharp_MedianLuma2;
//	uint8_t 	sharp_AvgLuma;
//	uint8_t 	sharp_MinLuma;
//	uint8_t 	sharp_brightMin;
//	uint16_t    sharp_maxMinDif;  // Range of values in block
//	uint16_t	sharp_sad;
//	uint16_t    sharp_det;		  // sad divided by range
//	uint16_t	sharp_detC;
//	uint16_t	sharp_detE;
//	uint16_t	sharp_detDifEC;
//	uint32_t	sharp_coeff8_energy;
//
//	uint16_t	coeff8_energyIncrease; 	// Block difference Information input vs. sharpened
	uint16_t	sadIncrease;
	uint16_t	rangeIncrease;
//	uint8_t		blkAdaptRange;   // DONT NEED THIS
	uint16_t	lumModRangeIncrease;
	uint8_t		blockLumaCorrection;
	uint8_t		sad3x3avg;
	uint8_t		coeff8_3x3avg;
	uint8_t		absAvgSharpSmooth;
	uint8_t		blkDetailECMod1;
	//uint8_t		blkAdaptSmoothMatrix[64];
	uint16_t    qtype1_matrix[64];
	uint16_t    qtype1_matrix_quant[64];
	uint16_t    qtype11_matrix[64];
	uint16_t    qtype11_matrix_quant[64];
	uint8_t		blkAdaptSmooth;
	uint8_t		blkAdaptExpand;
	uint8_t		blkAdaptSharp;

//	float  		detDifECvalsSadF;   // Temp for computing frame sad.
	float  		framedetDifECvalsF;
//	int16_t		DCTArr[64];
	uint8_t		adaptDebAmt;
	uint8_t		adaptDebAmt2;

	double  		RS_Mean;
	double  		RS_Variance;
	double  		RS_StDeviation;
	double  		RS_Skewness;
	double  		RS_Kurtosis;
	} LumaPerBlock_args;




//  DctLoop_args
typedef struct {
	uint8_t		 ncpu;
	uint8_t		 numThreads;      // The number of threads to use. The range is 1 to ncpu
	uint32_t 	 numBlocks_wide;
	uint32_t	 numBlocks_high;
	uint32_t     src_width;
	uint32_t     src_height;
	uint8_t      qtype;
	uint8_t      quant;
	uint8_t      max_quant;       // MAX(quant, adapt)
	uint8_t		 sharpWPos;
	uint8_t		 sharpWAmt;
	uint8_t		 expand;
	uint8_t		 sharpTPos;
	uint8_t		 sharpTAmt;
	uint8_t   	 sharpMaxWT;     // = Max(sharpWAmt, sharpTAmt);
	uint8_t   	 quality;
	uint8_t		 showMask;
	uint8_t		 T2;

	uint8_t	  	 doDCTadapt; 	 // Flag  if 1 then do range and sharp strength block adapt in the DCT loop, otherwise just do full strength.  Used internally to reduce halos.
	uint8_t	     cntShift;       // Number of shifts to be done.  Also index of next empty starti, startj slot.
	uint8_t	     numShift;       // Number of shifts to be done.  Also index of next empty starti, startj slot.
	uint8_t	  	 starti[64];	 // 64 is the maximum number of shifts the dct block can make.
	uint8_t	     startj[64];
	uint8_t      threadNum;      // specifies which BF_accum and BF_work to use
	uint8_t      startShift;     // the starting entry of the shifts list the thread should do
	uint8_t      endShift;	     // the ending   entry of the shifts list the thread should do

	uint8_t     *BF_srcP;
	uint8_t     *BF_workP;
	uint8_t     *BF_tmp_8P;      // Local Pointer Name ptmpP_8
	int16_t     *BF_tmp_16P;
//	int32_t     *BF_tmp_32P;
	uint16_t    *BF_accumP;

	uint16_t    *quant_intra_matrix;  // These 2 are used for qtypes  2, 3, 4, and 12, 13, 14.
	uint16_t    *quant_inter_matrix;


	uint16_t    *qtype1_matrix[MAX_BLOCK_ADAPT];
	uint16_t    *qtype1_matrix_quant[MAX_BLOCK_ADAPT];
	uint16_t    *qtype11_matrix[MAX_BLOCK_ADAPT];
	uint16_t    *qtype11_matrix_quant[MAX_BLOCK_ADAPT];
	uint16_t    *qtype1_matrix_range[MAX_BLOCK_ADAPT];
	uint16_t    *qtype1_matrix_quant_range[MAX_BLOCK_ADAPT];
	uint16_t    *qtype1_matrix_sharp[MAX_BLOCK_ADAPT];
	uint16_t    *qtype1_matrix_quant_sharp[MAX_BLOCK_ADAPT];
	uint8_t     *quant_sharp_preComp[MAX_BLOCK_ADAPT];

//	uint8_t		*blkAdaptSmoothMatrix;  // The 64 matrix values for each block of the frame.
	uint8_t		*blkAdaptSmoothAmt;    // The quant vals for each block of the frame.
	uint8_t		*blkAdaptExpandAmt;    // Each of them contain numBlocksWork quant values.
	uint8_t		*blkAdaptSharpAmt;
	} DctLoop_args;




//  Memory_args
typedef struct  {
	uint8_t  *frame_refSource;        // Stores a copy of the original source frame.                                        It is used as a reference frame and is not changed during processing.
	uint8_t  *frame_adaptSource;      // Stores a copy of the original source frame after adapt has been done.              It is used as a reference frame and is not changed during processing.
	uint8_t  *frame_workSource;       // Stores a copy of the source frame after bright smoothing and adapt has been done.  It is used as a reference frame and is not changed during processing.

	uint8_t  *frame_3x3sadm;
	uint8_t  *frame_3x3mean;          // Mean value of a 3x3 box centered at the pixel.
	uint8_t  *frame_ImpulseNoise;
	uint8_t  *frame_smoothed;
	uint8_t  *frame_sharp;             // Used when sharpening
	uint8_t  *frame_boundaries;
	uint8_t  *frame_deringMask;
	uint8_t  *frame_grainMask;
	uint8_t  *frame_localMax9x9;
	uint8_t  *frame_localMin9x9;
	uint8_t  *frame_sharpSmoothDif;


	uint16_t *quant_intra_matrix_sharp; // Used by sharp.c to precompute the sharpening lookup table
	uint16_t *quant_inter_matrix_sharp; // stored in DctLoopArgs[ncpu].quant_sharp_preComp[i]
	uint8_t	 *pixcntBlkArr;	 //USE TWO OF THESE INSTEAD OF THE STRUCTURE  NOT CURRENTLY USED !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	uint16_t *curblkPixCount;
//	uint16_t *curdifBinCountUsed;
//	uint16_t *curdifBlockTotalUsed;
	uint16_t *curblkPixCountUsed;

	uint8_t	 *BrightProtectDifArr;	     // The lookup table to speed up bright protection processing.
	uint8_t	 *DarkProtectDifArr;	     // The lookup table to speed up dark protection processing.
	uint8_t	 *AccumLimitArr;	         // The lookup table to speed up expand processing.
//	uint8_t	 *AccumLimitArr;			 // The lookup table to speed up sharp processing.
	uint8_t	 *AccumLimitBoundArr;	     // The lookup table to speed up expand and sharp processing at boundaries.
	uint8_t	 *AccumLimitSmoothBoundArr;  // The lookup table to speed up expand and sharp processing at boundaries when the user arg brightStart is specified.         NOT USED!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	uint8_t	 *AvgLumaBlockProtectArr;    // The lookup table to speed up LumaBlockInfo()
	uint8_t	 *SmoothDeHighlightArr;
	uint8_t	 *BoundaryStrengthArr;

	uint32_t  usedBlks;          // These 2 are derived from LumaPerBlock_args.
	float     percentBlksUsed;   // they are used in determining the adaptive weight of the frame.

	DctLoop_args       DctLoopArgs[MAX_CPU];
//	PixCount_args     *PixCountArgs;
	LumaPerBlock_args *LumaPerBlockArgs;  // There is 1 entry per block in the frame. "numBlocksY * sizeof(LumaPerBlock_args)"
} Memory_args;



//typedef struct {
//	uint16_t curblkPixCount;
//	uint16_t curdifBinCountUsed;
//	uint16_t curblkPixCountUsed;
//	uint16_t curblkPixPercent;
//} PixCount_args;



//  FrameInfo_args
// NOTE: FrameInfo_args is allocated on the stack in amDCT.c
// All of the frame level information is stored here.
typedef struct {

	// Input Arguments to amDCT()
	uint8_t  *psrc;
	uint8_t  *pdst;
//	uint8_t  *pf1;
//	uint8_t  *nf1;
	uint16_t  src_width;
	uint16_t  src_pitch;
	uint16_t  src_height;
	uint16_t  dst_width;
	uint16_t  dst_pitch;
	uint16_t  dst_height;
	uint8_t	  quant_a;     // The unmodified quant argument
	uint8_t	  quant;
	uint8_t	  qtype;
	uint8_t	  adapt_a;     // The unmodified adapt argument
	uint8_t	  adapt;
	uint8_t	  adaptWeightRaw;
	uint8_t	  FrameQuant;  // The quant value that is being used for this frame. range between quant and adapt.
	uint8_t	  shift;
	uint8_t   num_frames;  // For future if using multiple motion compensated frames.
	uint8_t	  total_shift;
	uint8_t   matrix;
	uint8_t	  expand;
	uint8_t	  sharpWPos;
	uint8_t	  sharpWAmt;
	uint8_t	  sharpTPos;
	uint8_t	  sharpTAmt;
	uint8_t   doSharpFlag;
	uint8_t   doExpandFlag;
	uint8_t   doBlkAdapExpandFlag;
	uint8_t   doDarkFlag;
	uint8_t   doBrightFlag;
	uint8_t   doBoundFlag;   // Not needed  !!!!!!!!!!!!!!
	uint8_t   doSmoothFlag;
	uint8_t   quality;
	uint8_t	  ncpu;

	uint8_t	  brightStart;
	uint8_t	  brightAmt;
	uint8_t	  brightStartUser;
	uint8_t	  brightAmtUser;

	uint8_t	  darkStart;
	uint8_t	  darkAmt;
	uint8_t	  darkStartUser;
	uint8_t	  darkAmtUser;

	uint8_t	  percentBright;




	uint8_t	  showMask;
	uint8_t   T2;

		//  		NOTE:  BUFFER SIZES.
		//  Avisynth delivers amDCT a source frame with a width and height number of pixels.
		//  The filter operates upon 8x8 blocks of pixels so the src XXXXXXXXXXXXXXXXX
		//  In order to process the edges of the frame without producing visible artifacts it is necessary to mirror the 8 pixels of each edge of the frame.
		//  This gives us a new frame that is 16 pixels wider and higher than the original frame.
		//  This new frame is considered the src frame and is passed into amDCT().  It is the frame that the filter operates on.
		//  The core DCT re-sampling algorithm requires a work buffer that is 1 block higher and wider than the src frame.
		//  size xxx Y  is the number of pixels in a buffer of xxx
		//  num xxx     is the count xxx
		//  The size of all frame_xxx are sizeY bytes.
	uint32_t  sizeY;  	          // The size of the frame src frame, including the 8 bit boarder on each side, the luma buffer and all internally derived frames, in bytes.  It is equal to src_height*src_width
	uint32_t  numBlocks_sizeY;
	uint32_t  numBlocks_wide;     // The number of columns, in blocks, in the source frame, rounded up + 1.     It is equal to 2*((src_width +15)/16)
	uint32_t  numBlocks_high;     // The number of rows,    in blocks, in the source frame, rounded up + 1.     It is equal to 2*((src_height+15)/16)
	uint32_t  numBlocksY;         // The total number of blocks in the source frame.                            It is equal to numBlocks_high*numBlocks_wide
	uint32_t  sizeBlocksY;        // Size of the buffer that the processing will be done on, in pixels.         It is equal to numBlocks_high*numBlocks_wide*64
	uint32_t  numBlocksWork;	  // The total number of blocks in the work buffer.  Size of the buffer that the processing will be done on, in pixels.  It should be 16 pixels wider and taller then the luma buffer.
	uint32_t  sizeBlocksWork;	  // Size of the buffer that the processing will be done on, in pixels.  It should be 16 pixels wider and taller then the luma buffer.


	uint8_t	  max_quant;

	uint8_t	  numShifts[8];       // Actual number of shifts done.
	uint8_t	  numShiftsHiBit[8];  // Number of bits to right shift the accumulator to do the average.
	uint8_t	  round[8];			  // Number to add to the accumulator so the result of the right shift will be rounded appropriately.

	uint8_t	  buildMasksDone;      // Flag  if 1 then buildMasks() has been done at least once.  Set and Tested by doFrameInfo().
	uint8_t   smoothFrameDone;     // Flag  if 1 then the smooth frame has been made.  Set and Tested by doFrameInfo().
	uint8_t   sharpFrameDone;      // Flag  if 1 then the sharp frame has been made.   Set and Tested by doFrameInfo().
	uint8_t	  SharpSmoothDifDone;  // Flag  if 1 then do not do not build sharp and range expansion information.  Set and Tested by doFrameInfo().
	uint8_t	  LumaFrameInfoDone;   // Flag  if 1 then do not do not build LumaFrame Info.  Set and Tested by doFrameInfo().
	uint8_t	  LumaBlockInfoDone;   // Flag  if 1 then do not do not build LumaBlock Info.  Set and Tested by doFrameInfo().
	uint8_t	  buildBrightProtectDifValsDone;   // Flag  if 1 then Bright smooth lookup table built.  Set and Tested by doFrameInfo().
	uint8_t	  buildDarkProtectDifValsDone;     // Flag  if 1 then Dark Protect lookup table built.  Set and Tested by doFrameInfo().
	uint8_t	  quality6Done;
	uint8_t	  doDCTadapt; 	       // Flag  if 1 then do range and sharp strength block adapt in the DCT loop, otherwise just do full strength.  Used internally to reduce halos.

	// The following are the internally derived frames.  They are all sizeY in length.
	uint8_t  *frame_refSource;        // Stores a copy of the original source frame.                                        It is used as a reference frame and is not changed during processing.
	uint8_t  *frame_adaptSource;      // Stores a copy of the source frame after adapt(), doBrightSmoothed() and preProcessEdges() are done.  It is used as a reference frame and is not changed during processing.
	uint8_t  *frame_workSource;       // This is the source frame that gets modified by each processing step.
	uint8_t  *frame_sharp;            // Internal very sharpened frame.
	uint8_t  *frame_smoothed;         // Internal qtype 4 smoothed.
	//Used before sharpening to presmooth pixels that would have become over sharpened.  "precog" "future crime" prevention. http://en.wikipedia.org/wiki/Minority_Report_(film)

	uint8_t  *frame_3x3sadm;          // Value of a 3x3 sad centered at the pixel.        // NOTE NOT USING THIS !!!!!!!!!!!!!!!!!!!!!!!
	uint8_t  *frame_3x3mean;          // Mean value of a 3x3 box centered at the pixel.// NOTE NOT USING THIS !!!!!!!!!!!!!!!!!!!!!!!
	uint8_t  *frame_boundaries;       // (max9x9-min9x9) + sad)/3.0F  Used by preProcessEdges()

	uint8_t  *frame_deringMask;
	uint8_t  *frame_ImpulseNoise;
	uint8_t  *frame_grainMask; 	// NOTE!!!!!!!!!!!!!!! We can use this as grain mask  WAS frame_sharpRangeMask NOW frame_grainMask
	uint8_t  *frame_localMax9x9;
	uint8_t  *frame_localMin9x9;
	uint8_t  *frame_sharpSmoothDif;
	uint8_t  *frame_psrc_mem;


	// These are set in avgDctLoop.c      LumaFrameInfo.c and are only contain valid values if quality > 2.
	uint8_t	  frame_Max;              // The value of the brightest pixel in the src frame.
	uint8_t	  frame_Avg;			  // The Average value of the pixels in the srcframe.
	uint8_t	  frame_Min;			  // The value of the darkest pixel in the srcframe.
	uint8_t	  frame_SmoothMax;        // The value of the brightest pixel in the smoothed frame.
	uint8_t	  frame_SmoothMin;		  // The value of the darkest pixel in the smoothed frame.
	uint8_t	  frame_SmoothAvgLuma;	  // The average brightness of the smoothed frame.
	uint8_t	  frame_5x5_SmoothMax;    // The value of the brightest pixel in the smoothed frame.    NOT CURRENTLY USED
	uint8_t	  frame_5x5_SmoothMin;	  // The value of the darkest pixel in the smoothed frame.      NOT CURRENTLY USED
	uint8_t	  frame_3x3_SmoothMax;    // The value of the brightest pixel in the smoothed frame.    NOT CURRENTLY USED
	uint8_t	  frame_3x3_SmoothMin;	  // The value of the darkest pixel in the smoothed frame.      NOT CURRENTLY USED
	uint8_t	  frame_SmoothUseMax;	  // The Max value that will be used in the smoothed frame.

//	uint8_t	  frame_SmoothAvgLuma;
//	uint8_t	  frame_Avg;
	uint64_t  frame_totalBrightSrc;
	uint64_t  frame_totalBrightSmooth;

	uint64_t  frame_avgBrightSrc;
	uint64_t  frame_avgBrightSmooth;
//Debug variables
	uint8_t	  minSmoothCutoff;
	int       maxNumBrightPix;
	int	 	  minMaxSmooth;
	int	 	  maxSmooth;

	uint64_t  totalBrightSrc;
	uint64_t  totalBrightSmooth;

// end debug variables

	uint8_t	  mean3x3BrightMax;
	uint8_t	  mean3x3BrightMin;
	uint8_t   brightMax;
	uint8_t   brightMin;

//	uint8_t   brightMax2;
	float	  framedetDifECavg;
	float	  frameDetDifECsad;
	float	  frameAvgDetE;
	float	  frameAvgDetC;
	float	  sharp_frameDetDifECsad;
	uint16_t  FrameSadIncrease;
	uint16_t  FrameRangeIncrease;
	uint32_t  totalPercentedge;
	uint16_t  avgtotalPercentedge;
	float	  PercentFlat;
	float	  PercentDark;

	int32_t   sumAbDif;                  // Raw Adaptive Difference from adapt() TESTING
	uint16_t  sumAbDif1;                  // Raw Adaptive Difference from adapt() TESTING
	uint16_t  sumAbDif2;                  // Raw Adaptive Difference from adapt() TESTING
	uint16_t  sumAbDif3;                 // NOTE NOT CURRENTLY BEING USED Dif > 1 to cound Raw Adaptive Difference from adapt() TESTING
	uint16_t  sumAbDif4;                 // NOTE NOT CURRENTLY BEING USED Dif > 1 to cound Raw Adaptive Difference from adapt() TESTING
//	uint32_t  difBinCountUsed;
	uint32_t  difBlockTotalUsed;
//	uint32_t  difBinCountZero;
//	uint32_t  difBinCount[255];
//	uint32_t  *difBinCountArr;
//
//	float		sadBlockMedFrameAvg;
//	float		sadCMedFrameAvg;
//	float		sadC1MedFrameAvg;
//	float		sadEMedFrameAvg;
//	float		sadZeroE1FrameAvg;
//	float		sadEMedDetFrameAvg;

	float		sadBlockFrameAvg;
	float		sadCFrameAvg;
	float		sadC1FrameAvg;
	float		sadEFrameAvg;
	float		sadE1FrameAvg;
	float		sadEDetFrameAvg;

	float		percentFlatC;
	float		percentFlatE;

//	float	    frameAvgDetE;
//	float		detDifECF;
	float		sadCMedf;
//	float   	sadEf;      // floating point sadE edge
	float   	sadEMedf;   // floating point sadE edge

	uint16_t  detDifEC2Max;
	uint16_t  detDifEC2Min;

	float 	  PercentPixUsedTotalPix;
	uint16_t  zeroBlksPercentedge;
	uint16_t  zeroBlksPercentedge2;

	uint32_t  framepixCnt;
	uint32_t  blkPixCountUsed;
	uint32_t  sumAbDifPixCnt;             // Number of pixels counted Raw Adaptive Difference from adapt() TESTING
	uint32_t  sumAbDifPixCnt1;            // Number of pixels counted Raw Adaptive Difference from adapt() TESTING
	uint32_t  sumAbDifPixCnt2;            // Number of pixels counted Raw Adaptive Difference from adapt() TESTING
	uint32_t  sumAbDifPixCnt3;            // Number of pixels counted Raw Adaptive Difference from adapt() TESTING
	uint32_t  sumAbDifPixCnt4;            // Number of pixels counted Raw Adaptive Difference from adapt() TESTING
	float     sumDifDivUsed;
	uint16_t  adaptWeight;                // New adapt() TESTING
	uint16_t  adaptWeight1;               // New adapt() TESTING
	uint16_t  adaptWeight2;               // New adapt() TESTING
	uint16_t  adaptWeight3;               // New adapt() TESTING
	uint16_t  adaptWeight4;               // New adapt() TESTING
	int       adaptTemp;                  // New adapt() TESTING
	uint8_t   AvgLuma;
	uint8_t   sharpMaxWT; // = Max(sharpWAmt, sharpTAmt);

	uint8_t   debugTemp;  //  For debugging.  A temp to keep a value that will be printed on the screen when amDCT exits.

//	uint8_t	  mean3x3BrightMax;
//	uint8_t	  mean3x3BrightMin;

	float  percentBlksUsed;
	float  percentBlksUsed2;

	uint32_t  usedBlks;

	uint8_t	max_debug7;
//	uint8_t	max_debug7;
//	uint8_t	max_debug7;

//	PixCount_args *PixCountArgs;
	Memory_args   *MemoryArgs;
	} FrameInfo_args;

#endif // _AMDCT_TYPEDEFS_H_
