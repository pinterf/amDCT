
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

#include "matrix.h"

#include "quant\quant.h"
#include "dct\idct.h"
#include "dct\fdct.h"
#include "quant\quant_matrix.h"

#define USE_NEW_INTRINSICS_DCTLOOP

void init_intra_matrixF(uint16_t *mpeg_quant_matrices, float quant);


/*  // These prototypes are only for testing.
void transfer_8to16copy_mmx(int16_t * const dst, const uint8_t * const src, uint32_t stride);
void transfer8x8_copy_mmx(uint8_t * const dst, const uint8_t * const src, const uint32_t stride);
*/
void copy_add_16to16_clpsrc_c(uint16_t* dst, int16_t* src, int32_t stride);

void transfer_8to16copy_xmm(int16_t *dst, uint8_t *src, uint32_t stride);
void transfer_8to16copy_c(int16_t* dst, uint8_t* src, uint32_t stride);
void copy_add_16to16_clpsrc_xmm(uint16_t *dst, int16_t * const src, int32_t len);
void quantDequant_xmm(int16_t *dct_block, const uint16_t *qtype1_matrix, const uint16_t *qtype1_matrix_quant);

void quantDequant_sharp(int16_t *dct_block, const uint8_t  *quant_sharp_preComp);

void quantDequant_expandRange(int16_t        *dct_block, 
							  const uint16_t *qtype1_matrix_range,
							  const uint16_t *qtype1_matrix_quant_range);



#define SCALEBITS 17

#ifndef USE_NEW_INTRINSICS
__declspec(align(16)) uint16_t  allFF[8]     = {0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff};
__declspec(align(16)) uint16_t  negBit[8]    = {0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000};
__declspec(align(16)) uint16_t  low15bits[8] = {0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff};
__declspec(align(16)) uint16_t  lowB[8]      = {0x00ff, 0x00ff, 0x00ff, 0x00ff, 0x00ff, 0x00ff, 0x00ff, 0x00ff};
#endif



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




void DctLoop(int starti, int startj, DctLoop_args *args) {			  

	unsigned int h;
	int32_t offset;

	int x,y;               
	int32_t  blkStride, rowStride;                                  
	uint32_t blkRowSt, blkColSt;
	int16_t  dbzero;

//int MaxDCT0 = 0;
//int MinDCT0 = 0;

//int16_t c1, c2, c3, c4; //corner vals of block
//int16_t cn1, cn2, cn3, cn4, cn11, cn21, cn31, cn41; 
//int16_t cnn1, cnn2, cnn3, cnn4, cnn11, cnn21, cnn31, cnn41; 
//int16_t cd1, cd2, cd3, cd4; // 1 in toward center from corner vals of block

	__declspec(align(16)) int16_t    dct_block[64];
	__declspec(align(16)) int16_t    coeff_block[64];
	__declspec(align(16)) int16_t    dct_blockTemp[64];  // Used for displaying the DCT values.

	__declspec(align(16)) int16_t    dct_blockOrig[MATRIX_SIZE];


	
	uint32_t  numBlocks_wide  = args->numBlocks_wide;
	uint32_t  numBlocks_high  = args->numBlocks_high;
	uint8_t   qtype           = args->qtype;
	uint8_t   quant           = args->quant;	
	uint32_t  src_width       = args->src_width;	
	uint32_t  src_height      = args->src_height;	

	uint8_t	  doDCTadapt      = args->doDCTadapt; 	 // Flag  if 1 then do range, expand, and sharp strength block adapt in the DCT loop, otherwise just do full strength.. 

	uint8_t	  expand     = args->expand;
	uint8_t	  sharpWAmt  = args->sharpWAmt;
	uint8_t	  sharpTAmt  = args->sharpTAmt;
	uint8_t   sharpMaxWT = args->sharpMaxWT;     // = Max(sharpWAmt, sharpTAmt);
	int	 	  T2         = args->T2;
	int	 	  showMask   = args->showMask;


	uint16_t *quant_intra_matrix  		= args->quant_intra_matrix;
	uint16_t *quant_inter_matrix  		= args->quant_inter_matrix;	
	uint16_t *qtype1_matrix       		= args->qtype1_matrix[0];	
	uint16_t *qtype1_matrix_quant 		= args->qtype1_matrix_quant[0];	
	uint16_t *qtype11_matrix       		= args->qtype11_matrix[0];	
	uint16_t *qtype11_matrix_quant 		= args->qtype11_matrix_quant[0];	
	uint16_t *qtype1_matrix_range      	= args->qtype1_matrix_range[0];	
	uint16_t *qtype1_matrix_quant_range = args->qtype1_matrix_quant_range[0];	
	uint8_t  *quant_sharp_preComp       = args->quant_sharp_preComp[0];	


	uint8_t  *BF_srcP   			 	= args->BF_srcP;
	uint8_t  *BF_workP            		= args->BF_workP;  
	int16_t  *BF_tmp_16P            	= args->BF_tmp_16P;  
	uint16_t *BF_accumP           		= args->BF_accumP;  
	uint16_t *BF_accumStart       		= args->BF_accumP;


// 	uint32_t   currentBlockNum   = 0;
	uint8_t    blkQuantVal       = args->quant;
	uint8_t    blkExpandVal      = args->expand;
	int	blkDetailECMod;

	int blockSize = (sizeof(int16_t))<<6;

	offset    = startj * BLKSIZE * numBlocks_wide + starti;                              
	BF_workP += offset;
	

	
	// Using The Agner Fog library code (look in the CodeCopiedFrom_OriginalSources directory)
	// Gives a 2 percent speed increase when using shift=4 
	// 30.1 to 30.7 fps 720x480p. Just enough to be worth it.
	// Both BF_src and BF_work are aligned data.
	// We are copying from an aligned position in BF_src
	// We are copying to a nonaligned position in BF_workP so that the shifted blocks start at an aligned position. 
	for (h=0; h < src_height; h++) {
		MY_MEMCPY(BF_workP, BF_srcP, src_width);
		BF_srcP  += src_width;
		BF_workP += numBlocks_wide<<3; // * BLKSIZE
	} 
				
	BF_workP   = args->BF_workP;                             
	BF_accumP  = BF_accumP  - offset;
	BF_tmp_16P = BF_tmp_16P - offset;


	// blkRowSt and blkColSt give the start location of the macro-block being processed.
	rowStride = numBlocks_wide<<3;  // numBlocks_wide*8  or numBlocks_wide*BLKSIZE
	blkStride = numBlocks_wide<<6;  // numBlocks_wide*64 or numBlocks_wide*BLKSIZE*BLKSIZE


	// This is the core of the amDCT algorithm 
	// which is based directly on the SmoothD2 algorithm 
	// which is based directly on the SmoothD algorithm
	// which was based on the original "JPEG Post-Processing through Re-application of JPEG" 
	// by Aria Nosratinia.  For more info see "JPEG re-sampling paper.pdf" in the documentation folder. 
	for (blkRowSt=0; blkRowSt < numBlocks_high; blkRowSt++) {	
		for (blkColSt=0; blkColSt < numBlocks_wide; blkColSt++) {
			int blkNum = (blkRowSt * numBlocks_wide) + blkColSt;
			if (BF_accumP < BF_accumStart) {
				BF_accumP  += BLKSIZE;
				BF_workP   += BLKSIZE;
				BF_tmp_16P += BLKSIZE;

				continue;
			}

			// SPEEED TEST 
			 /*
				transfer8x8_copy_mmx(BF_accumP, BF_workP, rowStride);		
				BF_accumP  += BLKSIZE;
				BF_workP   += BLKSIZE;
				continue;
				*/
			// END SPEED TEST
						
			// NOTE: Most of the mmx/xmm/sse2 routines and C equivalents are from XVID.  Thank You XVID.
			//       I tested the available XVID routines for each section of the algorithm.
			//		 The ones that worked I ranked by speed and commented out all but the fastest one.
			

				/*
				 *  1: COPY SHIFTED FRAME TO DCT_BLOCK
				 */
			//transfer_8to16copy_mmx(dct_block, BF_workP, rowStride);    // NOT CURRENTLY COMPILED IN
			//transfer_8to16copy_3dne(dct_block, BF_workP, rowStride);   // This is the same speed as the mmx version  NOT CURRENTLY COMPILED IN
			transfer_8to16copy_xmm(dct_block, BF_workP, rowStride);  // fastest
			// transfer_8to16copy_c(dct_block, BF_workP, rowStride); // test

		   qtype = args->qtype;


		
//goto speedTest;  // Uncomment this AND THE SPEEDTEST label  to test speed without doing the fdct, quant, dequant, idct calls.  label

				/*
				 *  2: DO DCT ON THE BLOCK.
				 */
// FIXME: until external asm source is re-inserted in project.
// Originally called fdct_sse2_skal asm, now fdct_int32 instead.
			fdct_int32(dct_block);       // C version for testing.
			//fdct_mmx_ffmpeg(dct_block);  // This mmx version works.
			//fdct_mmx_skal(dct_block);    // This mmx version is faster.
			//fdct_xmm_ffmpeg(dct_block);  // This xmm version also works even faster.                        
			//fdct_sse2_skal(dct_block);     // This sse2 is the fastest.  
			
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
			
			
			qtype1_matrix_quant	 = args->qtype1_matrix_quant[blkQuantVal];
			qtype1_matrix      	 = args->qtype1_matrix[blkQuantVal];
			
			qtype11_matrix_quant = args->qtype11_matrix_quant[blkQuantVal];
			qtype11_matrix       = args->qtype11_matrix[blkQuantVal];

			if (blkQuantVal <= 5 && qtype == 1) qtype = 11; // qtype == 1 needs quant > 4 to work.

			//if (T2==88) {
			//			qtype = 11;   // WHEN I GET AROUND TO IMPLIMENTING PER BLOCK MATRICIES USING PATTERNS IN DCT VALUES DETERMINE THE SMOOTHING MATRIX TO USE. 
			////			qtype11_matrix_quant = args->qtype11_matrix_quantPS[blkNum].qtype11_matrix_quantPS;
			////			qtype11_matrix       = args->qtype11_matrixPS[blkNum].qtype11_matrixPS;
			//}




			// Get the matrices that specify the range expansion for this block.	
			// The range expansion matrices, created in matrix.c, where built with 31 entries with strengths running from 0 to expand.
			// The values in blkAdaptExpandAmt[blkNum], created in LumaBlockInfo.c, where built with 31 entries with strengths running from 0 to 31
			// and represent the strength of expansion of the current block being processed. 		
			blkExpandVal = args->blkAdaptExpandAmt[blkNum];			
			if (doDCTadapt == 0)	blkExpandVal = expand; // Allows internal routines, in FrameInfo.c, to turn off block adapt.


			// if (T2==83 || T2==85 || T2==86)	blkExpandVal = expand; // FOR TESTING TURN OFF EXPAND BLOCK ADAPT
			if (blkExpandVal <= 0) 	blkExpandVal = 0;		
			if (blkExpandVal >= 30) blkExpandVal = 30;		

			qtype1_matrix_quant_range = args->qtype1_matrix_quant_range[blkExpandVal];
			qtype1_matrix_range       = args->qtype1_matrix_range[blkExpandVal];
	





			// Get the matrices that specifies the sharpening for this block.	
			// BLOCK ADAPT FOR SHARP DOESN"T WORK WELL!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!		
			blkDetailECMod = args->blkAdaptSharpAmt[blkNum];    // This returns the adapt amount of sharpening that should be done on the block.                          
			blkDetailECMod = 31 - blkDetailECMod;
			if (blkDetailECMod < 0) 	blkDetailECMod = 0;		
			if (blkDetailECMod > 30) 	blkDetailECMod = 30;	

			if (doDCTadapt == 0)	blkDetailECMod = sharpMaxWT; // Allows internal routines, in FrameInfo.c, to turn off block adapt.

			// BLOCK ADAPT FOR SHARP DOESN"T WORK WELL  SO IT IS CURRENTLY TURNED OFF!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!		
			// if (T2==84 || T2==85 || T2==86)	blkDetailECMod = sharpWAmt; // FOR TESTING TURN OFF SHARP BLOCK ADAPT             
			blkDetailECMod = sharpMaxWT; 

			quant_sharp_preComp = args->quant_sharp_preComp[blkDetailECMod];


			// doing sharp before range expand gives slightly better results than doing range expand first.
			if (qtype < 20 && (sharpWAmt > 0 || sharpTAmt > 0))  quantDequant_sharp(dct_block, quant_sharp_preComp);							
			if (qtype < 20 &&              expand > 0         )  quantDequant_expandRange(dct_block, qtype1_matrix_range, qtype1_matrix_quant_range);				

			if (quant > 0 || qtype >= 20) {
				/* 
				 * 		3: DO QUANT DEQUANT PROCESSING ON THE BLOCK	
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
					// I couldn't get quant_mpeg_intra_mmx() to work so I wrote a merged quant-dequant in xmm
					// An optimized c version qtype=11 is automatically used if the xmm version would overflow.
					dbzero = dct_block[0];				
					quantDequant_xmm(dct_block, qtype1_matrix, qtype1_matrix_quant);
					dct_block[0] = dbzero;
					break;
							
				case 2:  
#ifdef USE_NEW_INTRINSICS_DCTLOOP
          // _c instead of asm FIXME: implement sse2 x64 friendly
          quant_h263_intra_c(coeff_block, dct_block, quant, 1, quant_intra_matrix);
          dequant_h263_intra_c(dct_block, coeff_block, quant, 1, quant_intra_matrix);
#else
					  quant_h263_intra_sse2(coeff_block, dct_block, quant, 1, quant_intra_matrix);
					dequant_h263_intra_sse2(dct_block, coeff_block, quant, 1, quant_intra_matrix);
#endif
					break;
								
				case 3:   
#ifdef USE_NEW_INTRINSICS_DCTLOOP
          // _c instead of asm FIXME: implement sse2 x64 friendly
          quant_mpeg_inter_c(coeff_block, dct_block, quant, quant_inter_matrix);
          dequant_mpeg_inter_c(dct_block, coeff_block, quant, quant_inter_matrix);
#else
					  quant_mpeg_inter_mmx(coeff_block, dct_block, quant, quant_inter_matrix); 
					dequant_mpeg_inter_mmx(dct_block, coeff_block, quant, quant_inter_matrix);
#endif
					break;
				
				case 4:  
#ifdef USE_NEW_INTRINSICS_DCTLOOP
          // _c instead of asm FIXME: implement sse2 x64 friendly
          quant_h263_inter_c(coeff_block, dct_block, quant, quant_intra_matrix);
          dequant_h263_inter_c(dct_block, coeff_block, quant, quant_intra_matrix);
#else
					  quant_h263_inter_mmx(coeff_block, dct_block, quant, quant_intra_matrix);
					dequant_h263_inter_mmx(dct_block, coeff_block, quant, quant_intra_matrix);
#endif
					break;
          


		
				// These are the C versions which are included for testing.
				// NOTE case 11 is required to process qtype=1 for Matrices with 1's in them.			
				case 11:
							//quantDequant_C:
							// The xvid algorithm used 
							// SCALEBITS = 17
							// dct_block[] values run from -32768 to 32767
							// rounding = 1<<(SCALEBITS-1-3)
							// level    = (level * qtype1_matrix[i] + rounding)>>(SCALEBITS-3); 
							// 8192     = 32768 >> 2
					
					for (int i = 1; i < 64; i++) {
						int32_t level = dct_block[i];  	// dct_block[] values run from -32768 to 32767
	
						if (level == 0) continue;      
	
						level = (level * qtype11_matrix[i] + 8192 )>>14;  
	
						if (level == 0) 				
							dct_block[i] = 0;
						else
							dct_block[i] = (int16_t)(level * qtype11_matrix_quant[i]) >> 3;
					}					
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
#ifdef USE_NEW_INTRINSICS_DCTLOOP
          quant_h263_intra_c(coeff_block, dct_block, quant, 1, quant_intra_matrix);
          dequant_h263_intra_c(dct_block, coeff_block, quant, 1, quant_intra_matrix);
#else
					  quant_h263_intra_mmx(coeff_block, dct_block, quant, 1, quant_intra_matrix);
					dequant_h263_intra_mmx(dct_block, coeff_block, quant, 1, quant_intra_matrix);
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
// temporarily use C (simple_idct_c) instead of external asm idct_sse2_skal
			//idct_int32(dct_block);      // C version for testing.
			simple_idct_c(dct_block);   // C version for testing. (more precise)
			//idct_mmx(dct_block);        // This works.
			//idct_xmm(dct_block);        // This works faster.
			//idct_sse2_skal(dct_block);    // This works fastest.




// showMask == 18 jumps here from near the top of the main loop.
fdct_only:
			//  NOTE IF WE USE THE DCT VALUES DIRECTLY IN LUMABLOCKINFO.C THEN THE FITRANGES NEED TO BE TURNED INTO TABLES!!!
			if (showMask == 18 || showMask == 19) {
				int16_t zval = 0;
				
				// This outputs a frame that shows the DCT values within each block.
				// idx2 is used to flip the output values for each 8x8 block around the diagonal running from top left to bottom right.
				// This allows the dct values representing horizontal and vertical lines to display as horizontal and vertical lines.
				for (x=0; x<64; x++) dct_blockTemp[x] = dct_block[x];
				
				for (x=0; x<8; x++) {
					for (y=0; y<8; y++) {
						uint8_t idx1 = (uint8_t)(x*8 + y);
						uint8_t idx2 = (uint8_t)(y*8 + x);
						int16_t temp = dct_blockTemp[idx1];
						float   gamma = 2.0;
							
						if (T2==25) gamma = 1.0;
						if (T2==26) gamma = 4.0;
						
						if (x==0 && y==0) {
							if (dct_block[0] > MaxDCT0) MaxDCT0 = dct_block[0];
							if (dct_block[0] < MinDCT0) MinDCT0 = dct_block[0];
							
							// THE FOLOWING IS USED TO CALIBRATE dct_block[0] WHICH CONTAINS THE AVERAGE LUMA OF THE BLOCK. 							
							zval = (int16_t)Int_FitRange_Int(dct_block[0], 0, 2044, 0, 255);	// 11 -> 1     251 -> 251
							continue;
						}

						if (temp < 0)     dct_block[idx2] = (int16_t)(255 - Int_fitRangeGamma_Int(abs(temp),   0, 640, gamma, 128, 255));
						else              dct_block[idx2] =        (int16_t)Int_fitRangeGamma_Int(    temp,    0, 640, gamma, 128, 255);
					}	
				}
				dct_block[0] = zval;
			}
											

#ifdef ARCH_IS_IA32
			_mm_empty();
#endif

// speedTest:    //  NOTE UNCOMMENT IF DOING INNER LOOP  BYPASS SPEED TEST
				/*
				 * 		5: ADD BLOCK VALUES TO THOSE IN BF_workP  				
				 * Note that the values in dct_block are not in 0-255 range.	
				 * They are clipped to 0-255 in copy_add_16to16_clpsrc_xmm()	
				 */

			//copy_add_16to16_clpsrc_c(BF_accumP, dct_block, rowStride); // For testing.
			copy_add_16to16_clpsrc_xmm(BF_accumP, dct_block, rowStride); 

			BF_accumP  += BLKSIZE;
			BF_workP   += BLKSIZE;
			BF_tmp_16P += BLKSIZE;
		} // end for blkColSt

		BF_accumP  += blkStride - rowStride; // - rowStride since we already did a rowStride in the blkColSt loop.
		BF_workP   += blkStride - rowStride; // - rowStride since we already did a rowStride in the blkColSt loop.		
		BF_tmp_16P += blkStride - rowStride; // - rowStride since we already did a rowStride in the blkColSt loop.		
	} // end for blkRowSt


#ifdef ARCH_IS_IA32
	_mm_empty();
#endif
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
			int16_t *dct_block, 
	  const uint8_t *quant_sharp_preComp) {

	int     i;
	int32_t level, absLevel, absLevelOrig; 
	uint8_t row, r;
	uint8_t col;
	int 	levelCutoff = MAX_LEVEL_IDX; 
	
	for (row=0; row < 7; row++) {          
		r = row<<3;
		for (col=0; col < 7; col++) {		// if row==0 col goes from 5-7 (5 to 5+2)   row==1 col goes from 4-6 (4 to 4+2)
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
		   absLevel = quant_sharp_preComp[(i<<6) + absLevel]; 

		   if (absLevel == 0) continue;  // No change in value.

			if (row == 0 || col == 0) {  // This keeps us from over sharpening horizontal or vertical lines.
				//absLevel = (absLevel + absLevelOrig + 1)>>1;
				absLevel = (absLevel + absLevelOrig)>>1;
			}

			if (level > 0)	level =  absLevel;
			else            level = -absLevel;
			
			dct_block[i] = (int16_t)level;
		}	
	}	

	return;
}







void quantDequant_expandRange(
			int16_t *dct_block, 
			const uint16_t *qtype1_matrix_range,
			const uint16_t *qtype1_matrix_quant_range) {

	int     i;
	int32_t level;

//	int16_t dbzero;
//
//					dbzero = dct_block[0];				
//					quantDequant_xmm(dct_block, qtype1_matrix_range, qtype1_matrix_quant_range);
//					dct_block[0] = dbzero;
//					return;


	for (i=1; i < 64; i++) { 		
		level = dct_block[i];			
		if (level == 0) continue;      

		level = (level * qtype1_matrix_range[i] + 8192 )>>14;  

		if (level == 0) 				
			dct_block[i] = 0;
		else {			
			dct_block[i] = (int16_t)(level * qtype1_matrix_quant_range[i]) >> 3;
//			level = (level * qtype1_matrix_quant_range[i]) >> 3;
//			if (level > 7000) level = 7000;
//			dct_block[i] = (int16_t)level;
		}
	}	
	return;
}



#ifdef USE_NEW_INTRINSICS
__forceinline void copy_add_16to16_clpsrc_xmm(uint16_t* dst, int16_t* const src, int32_t stride) {
  __m128i lowB = _mm_set1_epi16(0x00ff); // Assuming lowB is a constant value
  __m128i zero = _mm_setzero_si128();

  for (int i = 0; i < 8; ++i) {
    __m128i src0 = _mm_load_si128((__m128i*)(src + i * stride));
    __m128i dst0 = _mm_loadu_si128((__m128i*)(dst + i * stride));

    __m128i src1 = _mm_load_si128((__m128i*)(src + i * stride + 8));
    __m128i dst1 = _mm_loadu_si128((__m128i*)(dst + i * stride + 8));

    __m128i src2 = _mm_load_si128((__m128i*)(src + i * stride + 16));
    __m128i dst2 = _mm_loadu_si128((__m128i*)(dst + i * stride + 16));

    src0 = _mm_max_epi16(src0, zero);
    src1 = _mm_max_epi16(src1, zero);
    src2 = _mm_max_epi16(src2, zero);

    src0 = _mm_min_epi16(src0, lowB);
    src1 = _mm_min_epi16(src1, lowB);
    src2 = _mm_min_epi16(src2, lowB);

    __m128i result0 = _mm_adds_epi16(src0, dst0);
    __m128i result1 = _mm_adds_epi16(src1, dst1);
    __m128i result2 = _mm_adds_epi16(src2, dst2);

    _mm_storeu_si128((__m128i*)(dst + i * stride), result0);
    _mm_storeu_si128((__m128i*)(dst + i * stride + 8), result1);
    _mm_storeu_si128((__m128i*)(dst + i * stride + 16), result2);
  }
}
#else
__forceinline void copy_add_16to16_clpsrc_xmm(uint16_t *dst,  int16_t * const src, int32_t stride) {

	stride=stride<<1;

	__asm {
	
	   ALIGN 16

	   movdqa  xmm6, [lowB]  // 0x00ff   //xmmword ptr [ebx]                                
	   pxor    xmm7,  xmm7   // zero value
	   mov     edx,   dst    // read for add
	   mov     eax,   dst    // write back
	   mov     esi,   src
	   mov     ecx,   stride     


		// Do first 3 rows (of 8x8 block)
		movdqa  xmm0, [ESI]  // The source is aligned.
		movupd  xmm1, [EDX]  // The destination is unaligned.

		add     EDX,  ecx  
		movdqa  xmm2, [ESI + 16]  // The source is aligned.
		movupd  xmm3, [EDX]  // The destination is unaligned.

		add     EDX,  ecx  
		movdqa  xmm4, [ESI + 32]  // The source is aligned.
		movupd  xmm5, [EDX]  // The destination is unaligned.

		
		pmaxsw  xmm0, xmm7
		pmaxsw  xmm2, xmm7

		pminsw  xmm0, xmm6
		pminsw  xmm2, xmm6

		paddsw  xmm0, xmm1  // add 16bits with saturation.  
		paddsw  xmm2, xmm3  // add 16bits with saturation.  

		pmaxsw  xmm4, xmm7

		movupd [EAX], xmm0  // The destination is unaligned.	MOVNTPS 	
		pminsw  xmm4, xmm6
		add     EAX,  ecx  
		paddsw  xmm4, xmm5  // add 16bits with saturation.  
		movupd [EAX], xmm2  // The destination is unaligned.		
		add     EAX,  ecx  
		add     EDX,  ecx  
		movupd [EAX], xmm4  // The destination is unaligned.		

		  
		// Do second 3 rows (of 8x8 block)
		// The add EDX, ecx was done several lines previously.
		movdqa  xmm0, [ESI + 48]  // The source is aligned.
		movupd  xmm1, [EDX]       // The destination is unaligned.

		add     EDX,  ecx  
		movdqa  xmm2, [ESI + 64]  // The source is aligned.
		movupd  xmm3, [EDX]       // The destination is unaligned.

		add     EDX,  ecx  
		movdqa  xmm4, [ESI + 80]  // The source is aligned.
		movupd  xmm5, [EDX]       // The destination is unaligned.

		
		pmaxsw  xmm0, xmm7
		pmaxsw  xmm2, xmm7

		pminsw  xmm0, xmm6
		pminsw  xmm2, xmm6

		paddsw  xmm0, xmm1  // add 16bits with saturation.  
		paddsw  xmm2, xmm3  // add 16bits with saturation.  

		add     EAX,  ecx  
		pmaxsw  xmm4, xmm7

		movupd [EAX], xmm0  // The destination is unaligned.		
		pminsw  xmm4, xmm6
		add     EAX,  ecx  
		paddsw  xmm4, xmm5  // add 16bits with saturation.  
		movupd [EAX], xmm2  // The destination is unaligned.		
		add     EAX,  ecx  
		add     EDX,  ecx  
		movupd [EAX], xmm4  // The destination is unaligned.		


		// Do the last 2 rows (of 8x8 block)
		// The add EDX, ecx was done several lines previously.
		movdqa  xmm0, [ESI + 96]  // The source is aligned.
		movupd  xmm1, [EDX]  // The destination is unaligned.

		add     EDX,  ecx  
		movdqa  xmm2, [ESI + 112]  // The source is aligned.
		movupd  xmm3, [EDX]  // The destination is unaligned.

		pmaxsw  xmm0, xmm7
		pmaxsw  xmm2, xmm7

		pminsw  xmm0, xmm6
		pminsw  xmm2, xmm6

		add     EAX,  ecx  

		paddsw  xmm0, xmm1  // add 16bits with saturation.  
		paddsw  xmm2, xmm3  // add 16bits with saturation.  

		movupd [EAX], xmm0  // The destination is unaligned.		
		add     EAX,  ecx  
		movupd [EAX], xmm2  // The destination is unaligned.		             		

		emms 
	}
	
	return;
}
#endif

void quantDequant_c(int16_t* dct_block, const uint16_t* qtype1_matrix, const uint16_t* qtype1_matrix_quant) {

  int16_t negBit = 0x8000;
 
  for (int i = 0; i < 64; ++i) {
    int16_t value = dct_block[i];
    int16_t sign = value & negBit;
    int16_t abs_value = value < 0 ? -value : value;

    int16_t level = (abs_value * qtype1_matrix[i] + 32768) >> 16;
    level = (level * qtype1_matrix_quant[i]) >> 3;

    if (sign) {
      level = -level;
    }

    dct_block[i] = level;
  }
}


 
#ifdef USE_NEW_INTRINSICS 
__forceinline void quantDequant_xmm(int16_t* dct_block, const uint16_t* qtype1_matrix, const uint16_t* qtype1_matrix_quant) {
  __m128i low15bits = _mm_set1_epi16(0x7FFF); // Mask for low 15 bits
  __m128i allFF = _mm_set1_epi16(0xFFFF); // Mask for all bits set
  __m128i negBit = _mm_set1_epi16(0x8000); // Mask for the sign bit

  for (int i = 0; i < 8; ++i) {
    __m128i xmm0 = _mm_load_si128((__m128i*) & dct_block[i * 8]); // Load dct_block
    __m128i xmm6 = _mm_and_si128(xmm0, low15bits); // xmm6 = xmm0 & low15bits

    __m128i xmm1 = _mm_load_si128((__m128i*) & qtype1_matrix[i * 8]); // Load qtype1_matrix
    __m128i xmm2 = _mm_load_si128((__m128i*) & qtype1_matrix_quant[i * 8]); // Load qtype1_matrix_quant

    __m128i xmm7 = _mm_cmpeq_epi16(xmm6, xmm0); // xmm7 = (xmm6 == xmm0)
    xmm7 = _mm_xor_si128(xmm7, allFF); // xmm7 = ~xmm7

    __m128i xmm4 = _mm_setzero_si128(); // xmm4 = 0
    xmm4 = _mm_sub_epi16(xmm4, xmm0); // xmm4 = -xmm0

    xmm0 = _mm_and_si128(xmm0, xmm6); // xmm0 = xmm0 & xmm6
    xmm2 = _mm_and_si128(xmm2, xmm7); // xmm2 = xmm2 & xmm7
    xmm2 = _mm_add_epi16(xmm2, xmm0); // xmm2 = xmm2 + xmm0

    __m128i xmm3 = _mm_mullo_epi16(xmm2, xmm1); // xmm3 = xmm2 * xmm1
    xmm2 = _mm_mulhi_epi16(xmm2, xmm1); // xmm2 = (xmm2 * xmm1) >> 16

    xmm3 = _mm_and_si128(xmm3, negBit); // xmm3 = xmm3 & negBit
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
#else
__forceinline void quantDequant_xmm( int16_t *dct_block, 
				   const uint16_t *qtype1_matrix, 
				   const uint16_t *qtype1_matrix_quant) {


// xmm0 is level dct_block made positive
// xmm1 is qtype1_matrix
// xmm2 is qtype1_matrix_quant
// xmm3 is 
// xmm4 is temp register
// xmm5 is temp register
// xmm6 contains a mask of 1's if number was negative 
// xmm7 contains a mask of 1's if number was positive 
	__asm {
	   ALIGN 16
	   mov     edx,   dct_block
	   mov     esi,   qtype1_matrix
	   mov     eax,   qtype1_matrix_quant
	   mov     ecx,   128       // The loop counter. Length of dct_block (64 entries x 2 bytes per entry)


//   This is the first thing we want to compute 
//   level[i] = (dct_block[i] * qtype1_matrix[i] + 32768 )>>16;
	movdqa 	xmm0, [EDX]       // dct_block
	movdqa 	xmm6, xmm0         // temp dct_block
	pand    xmm6, [low15bits]  // xmm6 == xmm0 then value was negative
	MainLoop: 
		PCMPEQW xmm6, xmm0    // xmm6 positive value mask contains 1's if value is positive
		movdqa 	xmm7, xmm6        
		pxor	xmm7, [allFF] // xmm7 negative value mask now contains 1's if value is negative. 
		pxor 	xmm4, xmm4
		movdqa 	xmm1, [ESI]   // qtype1_matrix  
		psubw   xmm4, xmm0    // xmm4 = 0 - xmm5
		movdqa 	xmm2, xmm7        
		pand    xmm0, xmm6    // xmm5 contains positive values that had been positive and 0 elsewhere
		pand    xmm2, xmm4    // xmm2 contains positive values that had been negative and 0 elsewhere
		PADDW 	xmm2, xmm0    // xmm2 now only contains positive values

		movdqa 	xmm3, xmm2        
		pmullw 	xmm3, xmm1
		pmulhw 	xmm2, xmm1

		movdqa 	xmm4, [negBit]
		movdqa  xmm5, [EAX]     
		add     EAX,  16  	    		

		pand    xmm3, xmm4
		PSRLW   xmm3, 15           // If there was a bit leave it in low bit
		PADDW   xmm2, xmm3

		PMULLW  xmm2, xmm5         // level * qtype1_matrix_quant[i]                   
		movdqa 	xmm0, [EDX + 16]   // dct_block
		movdqa 	xmm5, xmm0         // temp dct_block
		pand    xmm5, [low15bits]  // xmm6 == xmm0 then value was negative

		add      ESI, 16 
		pxor 	xmm4, xmm4
		PSRAW   xmm2, 3            //  >> 3

		psubw   xmm4, xmm2         // xmm4 = 0 - xmm2
		pand    xmm7, xmm4
		pand    xmm6, xmm2
		PADDW   xmm6, xmm7
		movdqa [EDX], xmm6 
		movdqa 	xmm6, xmm5         // temp dct_block

		add       EDX, 16                         
		sub       ECX, 16                         
		jnz       MainLoop   

		emms   
	}		

return;
}
#endif


#ifdef USE_NEW_INTRINSICS 
__forceinline void transfer_8to16copy_xmm(int16_t* dst, uint8_t* src, uint32_t stride) {
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
#else
__forceinline void transfer_8to16copy_xmm(int16_t *dst, uint8_t *src, uint32_t stride) {

	__asm {
	
	   ALIGN 16
	   
	   mov     edx, dst
	   mov     esi, src
	   mov     ecx, stride


		pxor		xmm0, xmm0
		
		movq        mm1, [ESI]                     
		add    		ESI,  ECX  
		  
		movq        mm2, [ESI]                     
		movq2dq		xmm1, mm1
		add    		ESI,  ECX    
		movq2dq		xmm2, mm2
		punpcklbw	xmm1, xmm0
		
		movq        mm3, [ESI]                     
		add    		ESI,  ECX    
		movdqa      [EDX],      xmm1 
		movq2dq		xmm3, mm3
		punpcklbw	xmm2, xmm0
		
		movq        mm4, [ESI]                     
		add    		ESI,  ECX    
		movdqa      [EDX + 16], xmm2 
		movq2dq		xmm4, mm4
		punpcklbw	xmm3, xmm0
		
		movq        mm5, [ESI]                     
		add    		ESI,  ECX    
		movdqa      [EDX + 32], xmm3 
		movq2dq		xmm5, mm5
		punpcklbw	xmm4, xmm0
		
		movq        mm6, [ESI]                     
		add    		ESI,  ECX    
		movdqa      [EDX + 48], xmm4 
		movq2dq		xmm6, mm6
		punpcklbw	xmm5, xmm0
		
		movq        mm7, [ESI]                     
		add    		ESI,  ECX    
		movdqa      [EDX + 64], xmm5
		movq2dq		xmm7, mm7
		punpcklbw	xmm6, xmm0
		
		movq        mm1, [ESI]                     
		movdqa      [EDX + 80], xmm6
		movq2dq		xmm1, mm1
		punpcklbw	xmm7, xmm0
		punpcklbw	xmm1, xmm0
		
		movdqa      [EDX + 96],  xmm7
		movdqa      [EDX + 112], xmm1 

		emms   
	}
	
	return;
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


//
//
//// src = uint16_t *dct_block[64]
//// dst = uint16_t *acumulator[frameheight * framewidth]
//// stride = frame_width
//__forceinline void copy_add_16to16_xmm_test(int16_t *dst, int16_t * const src, int32_t stride) {
//
//	stride=stride<<1;
//	
//	__asm {
//	   ALIGN 16
//
//	   mov     edx,   dst    // read for add
//	   mov     eax,   dst    // write back
//	   mov     esi,   src
//	   mov     ecx,   stride     
//
//	    movupd    xmm0, [EDX]                     
//	    movdqa    xmm1, [ESI]                     
//
//	    add       EDX,   ECX                                   
//	    movupd    xmm2, [EDX]  
//	    movdqa    xmm3, [ESI + 16]  
//
//	    paddw     xmm0, xmm1                        
//	    movupd   [EAX], xmm0                      
//
//	    add       EDX,   ECX                                   
//	    movupd    xmm4, [EDX]                     
//	    movdqa    xmm5, [ESI + 32]  
//
//	    paddw     xmm2, xmm3                        
//	    add       EAX,  ECX                                   
//	    movupd   [EAX], xmm2                      
//
//	    add       EDX,   ECX                                   
//	    movupd    xmm6, [EDX]                     
//	    movdqa    xmm7, [ESI + 48]  
//
//	    paddw     xmm4, xmm5                        
//	    add       EAX,  ECX                                   
//	    movupd   [EAX], xmm4                      
//                       
//	    add       EDX,   ECX                                   
//	    movupd    xmm0, [EDX]  
//	    movdqa    xmm1, [ESI + 64]  
//
//	    paddw     xmm6, xmm7                        
//	    add       EAX,  ECX                                   
//	    movupd   [EAX], xmm6                      
//                       
//	    add       EDX,   ECX                                   
//	    movupd    xmm2, [EDX]  
//	    movdqa    xmm3, [ESI + 80]  
//
//	    paddw     xmm0, xmm1                        
//	    add       EAX,  ECX                                   
//	    movupd   [EAX], xmm0                      
//
//	    add       EDX,   ECX                                   
//	    movupd    xmm4, [EDX]                     
//	    movdqa    xmm5, [ESI + 96]  
//
//	    paddw     xmm2, xmm3                        
//	    add       EAX,  ECX                                   
//	    movupd   [EAX], xmm2                      
//
//	    add       EDX,   ECX                                   
//	    movupd    xmm6, [EDX]                                                      
//	    movdqa    xmm7, [ESI + 112]  
//
//	    paddw     xmm4, xmm5                        
//	    add       EAX,  ECX                                   
//	    movupd   [EAX], xmm4                      
//
//	    paddw     xmm6, xmm7                        
//	    add       EAX,  ECX                                   
//	    movupd   [EAX], xmm6                      
// 	                                                   
//	}
//	
//	return;
//}
//
//
