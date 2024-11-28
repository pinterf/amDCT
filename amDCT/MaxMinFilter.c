
#include <math.h>
#include <stdbool.h>

#include "amDCTtypedefs.h"
#include "Utilities.h"
#include "Memory.h"
#include "FrameInfo.h"
 

#include "smoothFilters.h"
#include "MaxMinFilter.h"


 


void sadWindowH(uint8_t *src, uint8_t *dst, uint8_t *mean, uint16_t width, uint16_t src_height, uint8_t radiusX);
void sadWindowV(uint8_t *src, uint8_t *BF_tmp_8P, uint8_t *dst, uint8_t *mean, uint16_t width, uint16_t src_height, uint8_t radiusY);

void maxWindowH2(FrameInfo_args *FrameInfoArgs, uint8_t *src, uint8_t *dst, uint16_t width, uint16_t src_height, uint8_t radiusX);
void maxWindowV2(							    uint8_t *src, uint8_t *dst, uint16_t width, uint16_t src_height, uint8_t radiusY);

void minWindowH2(FrameInfo_args *FrameInfoArgs, uint8_t *src, uint8_t *dst, uint16_t width, uint16_t src_height, uint8_t radiusX);
void minWindowV2(							    uint8_t *src, uint8_t *dst, uint16_t width, uint16_t src_height, uint8_t radiusY);

 


 



//#define IARRSIZE ((MAX_RADIUS*2 + 1) * 256)

void sadWindow(FrameInfo_args *FrameInfoArgs, uint8_t *src, uint8_t *dst, uint8_t *mean, uint8_t radiusX, uint8_t radiusY) { 

	uint16_t  src_height = FrameInfoArgs->src_height;
	uint16_t  src_width	 = FrameInfoArgs->src_width;
	uint8_t   *BF_tmp_8P = FrameInfoArgs->MemoryArgs->DctLoopArgs[0].BF_tmp_8P;


	sadWindowH(src, BF_tmp_8P,      mean, src_width, src_height, radiusX);
	sadWindowV(src, BF_tmp_8P, dst, mean, src_width, src_height, radiusY);

	return;
} 




// Do Horizontal pass on the source frame src.  Output to destination frame dst.
void sadWindowH(uint8_t *src, 
				uint8_t *dst, 
				uint8_t *mean, 
				uint16_t width, 
				uint16_t height, 
				uint8_t  radiusX) {

	uint8_t  windowSize = radiusX*2 + 1;	
	uint16_t end_of_Row = width;
	uint16_t temp;
	uint32_t dstIdx, srcIdx, testIdx;

	for (uint16_t h = 0; h < height; h++) { 
		uint32_t startRow = h*width;
		
		srcIdx = startRow;		
		dstIdx = startRow +	(radiusX - 1);
		testIdx = srcIdx;
			
		for (uint16_t w = windowSize; w < end_of_Row; w++) {  // Main loop. Walks across 1 row of pixels in the source frame.
			uint8_t  curSrc = 0;
			uint16_t sad    = 0;
			uint8_t  curMean = mean[testIdx];

			srcIdx++;
			dstIdx++;
			testIdx = srcIdx;
			
			curSrc = src[testIdx];
			for (uint8_t j = 0; j < windowSize; j++) {						
					sad += (uint16_t)abs(src[testIdx++] - curMean);				
			}
			
			temp = (uint16_t)ROUND_TOINT(sad/windowSize);
			if (temp > 255) temp = 255;
			if (temp < 0)   temp = 0;			
			dst[dstIdx] = (uint8_t)temp;
		}
	}
	
	return;
}




void sadWindowV(uint8_t *src, 
				uint8_t *BF_tmp_8P, 
				uint8_t *dst,
				uint8_t *mean, 
				uint16_t width, 
				uint16_t height, 
				uint8_t  radiusY) {

	uint8_t  windowSize = radiusY*2 + 1;	
	uint32_t dstIdx, srcIdx, testIdx;
	uint16_t end_of_Row = width - radiusY;

	int16_t   i, j, temp;

	for(i=0; i<end_of_Row; i++) {
		uint32_t startCol = i;
		
		srcIdx = startCol;		
		dstIdx = startCol + ((radiusY - 1) * width);
			
		for(j=0; j<(height-radiusY); j++) { 
			uint8_t  curSrc = 0;
			uint16_t sad    = 0;
			uint8_t  curMean; 

			srcIdx += width;
			dstIdx += width;
			testIdx = srcIdx;
			
			curSrc  = src[testIdx];
			curMean = mean[testIdx];			
			
			for (uint8_t k = 0; k < windowSize; k++) {
				sad += (uint16_t)abs(src[testIdx++] - curMean);				
			}

			
			temp = (uint16_t)ROUND_TOINT(((sad / windowSize) + BF_tmp_8P[srcIdx])/2.0F);

			if (temp > 255) temp = 255;
			if (temp < 0)   temp = 0;			
			dst[dstIdx] = (uint8_t)temp;			
		}
	}
	
	return;
}






void maxFrame(FrameInfo_args *FrameInfoArgs, uint8_t *src, uint8_t *dst, uint8_t radiusX, uint8_t radiusY) { //, uint8_t strengthX, uint8_t strengthY) {
	uint16_t  src_height 	     = FrameInfoArgs->src_height;
	uint16_t  src_width	 	     = FrameInfoArgs->src_width;

	uint8_t   *BF_tmp_8P         = FrameInfoArgs->MemoryArgs->DctLoopArgs[0].BF_tmp_8P;

	maxWindowH2(FrameInfoArgs, src,       BF_tmp_8P, src_width, src_height, radiusX);
	maxWindowV2(			   BF_tmp_8P,       dst, src_width, src_height, radiusY);

	return;
} 

void minFrame(FrameInfo_args *FrameInfoArgs, uint8_t *src, uint8_t *dst, uint8_t radiusX, uint8_t radiusY) { //, uint8_t strengthX, uint8_t strengthY) {
	uint16_t  src_height 	     = FrameInfoArgs->src_height;
	uint16_t  src_width	 	     = FrameInfoArgs->src_width;
	uint8_t   *BF_tmp_8P         = FrameInfoArgs->MemoryArgs->DctLoopArgs[0].BF_tmp_8P;

	minWindowH2(FrameInfoArgs, src,       BF_tmp_8P, src_width, src_height, radiusX);
	minWindowV2( 			   BF_tmp_8P,       dst, src_width, src_height, radiusY);

	return;
} 






// Do Horizontal pass on the source frame src.  Output to destination frame dst.
void maxWindowH2(FrameInfo_args *FrameInfoArgs, 
				 uint8_t 		*src, 
				 uint8_t 		*dst, 
				 uint16_t		 width, 
				 uint16_t		 height, 
				 uint8_t		 radiusX) {

	uint8_t  windowSize = radiusX*2 + 1;	
	uint16_t end_of_Row = width - radiusX;
	uint8_t  maxCurMax  = 0;
	uint32_t dstIdx, srcIdx, testIdx;

	for (uint16_t h = 0; h < height; h++) { 
		uint32_t startRow = h*width;
		
		srcIdx = startRow;		
		dstIdx = startRow +	(radiusX - 1);
			
		for (uint16_t w = 0; w < end_of_Row; w++) {  // Main loop. Walks across 1 row of pixels in the source frame.
			uint8_t curMax = 0;
			uint8_t curSrc = 0;

			srcIdx++;
			dstIdx++;
			testIdx = srcIdx;
			
			curSrc = src[testIdx];
			for (uint8_t j = 0; j < windowSize; j++) {			
				if (curMax < curSrc)  {
					curMax = curSrc;
					if (maxCurMax < curSrc) maxCurMax = curSrc;
				}
				curSrc = src[++testIdx];				
			}
			
			dst[dstIdx] = curMax;
		}
	}
	
	FrameInfoArgs->brightMax = maxCurMax;
	return;
}



// Do Vertical pass on the source frame src.  Output to destination frame dst.
void maxWindowV2(uint8_t *src, uint8_t *dst, uint16_t width, uint16_t height, uint8_t radiusY) {


	uint8_t  windowSize = radiusY*2 + 1;	
	uint32_t dstIdx, srcIdx, testIdx;

	int16_t   i, j;

	for(i=0; i<width; i++) {
		uint32_t startCol = i;
		
		srcIdx = startCol;		
		dstIdx = startCol + ((radiusY - 1) * width);
			
		for(j=0; j<(height-radiusY); j++) { 
			uint8_t curMax = 0;
			uint8_t curSrc = 0;

			srcIdx += width;
			dstIdx += width;
			testIdx = srcIdx;
			
			curSrc = src[testIdx];
			for (uint8_t k = 0; k < windowSize; k++) {
				if (curMax < curSrc)  curMax = curSrc;	
				testIdx += width;
				curSrc = src[testIdx];
			}
			
			dst[dstIdx] = curMax;
		}
	}
	
	return;
}



// Do Horizontal pass on the source frame src.  Output to destination frame dst.
void minWindowH2(FrameInfo_args *FrameInfoArgs, uint8_t *src, uint8_t *dst, uint16_t width, uint16_t height, uint8_t radiusX) {


	uint8_t  windowSize = radiusX*2 + 1;	
	uint32_t dstIdx, srcIdx, testIdx;
	uint16_t end_of_Row = width - radiusX;
	uint8_t  minCurMin=255;

	for (uint16_t h = 0; h < height; h++) { 
		uint32_t startRow = h*width;
		
		srcIdx = startRow;		
		dstIdx = startRow +	(radiusX - 1);
			
		for (uint16_t w = 0; w < end_of_Row; w++) {  // Main loop. Walks across 1 row of pixels in the source frame.
			uint8_t curMin = 255;
			uint8_t curSrc = 255;

			srcIdx++;
			dstIdx++;
			testIdx = srcIdx;
			
			curSrc = src[testIdx];
			for (uint8_t j = 0; j < windowSize; j++) {  // This loop should be replaced by a deQueue but I could not get it to work.
				if (curMin > curSrc)  {
					curMin = curSrc;
					if (minCurMin > curSrc) minCurMin = curSrc;
				}
				curSrc = src[++testIdx];
			}
			
			dst[dstIdx] = curMin;
		}
	}
	
	FrameInfoArgs->brightMin = minCurMin;
	
	return;
}





// Do Vertical pass on the source frame src.  Output to destination frame dst.
void minWindowV2(uint8_t *src, uint8_t *dst, uint16_t width, uint16_t height, uint8_t radiusY) {


	uint8_t  windowSize = radiusY*2 + 1;	
	uint32_t dstIdx, srcIdx, testIdx;
	
	int16_t   i, j;


	for(i=0; i<width; i++) {
		uint32_t startCol = i;
		
		srcIdx = startCol;		
		dstIdx = startCol + ((radiusY - 1) * width);
			
		for(j=0; j<(height-radiusY); j++) { 
			uint8_t curMin = 255;
			uint8_t curSrc = 255;

			srcIdx += width;
			dstIdx += width;
			testIdx = srcIdx;
			
			curSrc = src[testIdx];
			for (uint8_t k = 0; k < windowSize; k++) {   // This loop should be replaced by a deQueue but I could not get it to work.
				if (curSrc < curMin)  curMin = curSrc;	
				testIdx += width;
				curSrc = src[testIdx];
			}
			
			dst[dstIdx] = curMin;
		}
	}
	
	return;
}









