
#include <math.h>

#include "amDCTtypedefs.h"
#include "Utilities.h"
#include "Memory.h"
#include "FrameInfo.h"

#include "smoothFilters.h"

//#ifndef ROUND_TO_CURVAL
//  #define ROUND_TO_CURVAL(CURV, X)  (int)(((X > CURV)?(X - 0.5):(X + 0.5)))
//#endif


//#ifndef MAX_DIFF
//  #define MAX_DIFF_TO_CURVAL(CURV, X)  (int)(((X > CURV)?(X - 0.5):(X + 0.5)))
//  #define MAX_DIFF_TO_CURVAL(CURV, X)  (int)(((X > CURV)?(X - 0.5):(X + 0.5)))
//#endif
  

// MAX_RADIUS is used to compute the size of 2 lookup tables, allocated on the stack.
// The lookup tables are used to speed up the smoothing computation.
// Any user specified radius larger than MAX_RADIUS will silently be truncated to MAX_RADIUS.
#ifndef MAX_RADIUS
  #define MAX_RADIUS   11
#endif



//
//    Thank you Ivan Kuckir 
//    for the code you have made available as well as 
//    for some great overviews on a number of algorithms.
//
// The following code was found in the blog of Ivan Kuckir.   
// I translated it into C from the javascript code 
// that I found in Ivan Kuckir blog at
//	   http://blog.ivank.net/fastest-gaussian-blur.html
//
//  which had the following on it.
//    "All code that you see at this blog is free to use, under MIT licence."
//

static 	uint16_t MaxI = 0;	

void boxesForGauss(uint8_t *bxs, uint8_t sigma, uint8_t n);

//void gaussBlur (uint8_t *scl, uint8_t *tcl, uint16_t width, uint16_t height, uint8_t radiusX, uint8_t radiusY, uint8_t strength, uint8_t T2) ;
//void boxBlur   (uint8_t *scl, uint8_t *tcl, uint16_t width, uint16_t height, uint8_t radiusX, uint8_t radiusY, uint8_t strength, uint8_t T2);
void gaussBlur (uint8_t *scl, uint8_t *tcl, uint16_t width, uint16_t height, uint8_t radiusX, uint8_t radiusY, uint8_t strength);
void boxBlur   (uint8_t *scl, uint8_t *tcl, uint16_t width, uint16_t height, uint8_t radiusX, uint8_t radiusY, uint8_t strength);
void boxBlurH  (uint8_t *scl, uint8_t *tcl, uint16_t width, uint16_t height, uint8_t radiusY, uint8_t *iarr, uint16_t *radiusTimesFV, int16_t SrcSmoothDif[]);
void boxBlurV  (uint8_t *scl, uint8_t *tcl, uint16_t width, uint16_t height, uint8_t radiusX, uint8_t *iarr, uint16_t *radiusTimesFV, int16_t SrcSmoothDif[]);


// NOTE MAX_RADIUS is defined above 
void gaussianBlur(FrameInfo_args *FrameInfoArgs, uint8_t radiusX, uint8_t radiusY) { //, uint8_t strengthX, uint8_t strengthY) {
	uint16_t  src_height = FrameInfoArgs->src_height;
	uint16_t  src_width	 = FrameInfoArgs->src_width;
//	uint8_t    T2       = FrameInfoArgs->T2;
	uint8_t   *smooth	 = FrameInfoArgs->frame_smoothed;
	uint8_t   *src  	 = FrameInfoArgs->frame_workSource;
	uint8_t   strength   = FrameInfoArgs->darkStart;

	MY_MEMCPY(smooth, src, src_height*src_width);
	
//	gaussBlur(src, smooth, src_width, src_height, radiusX, radiusY, strength, T2); //, strengthX, strengthY); 
	gaussBlur(src, smooth, src_width, src_height, radiusX, radiusY, strength); //, strengthX, strengthY); 

	return;
} 

//   gaussianBlur2(FrameInfoArgs,                         psrc,       frame_grainMask,      0,               1,         1);  
void gaussianBlur2(FrameInfo_args *FrameInfoArgs, uint8_t *src, uint8_t *dst, uint8_t radiusX, uint8_t radiusY, uint8_t strength) { 
	uint16_t  src_height = FrameInfoArgs->src_height;
	uint16_t  src_width	 = FrameInfoArgs->src_width;
//	uint8_t 	T2       = FrameInfoArgs->T2;


	MY_MEMCPY(dst, src, src_height*src_width);
	
	//gaussBlur(src, dst, src_width, src_height, radiusX, radiusY, strength, T2); //, strengthX, strengthY); 
	gaussBlur(src, dst, src_width, src_height, radiusX, radiusY, strength); //, strengthX, strengthY); 

	return;
} 

//void gaussBlur(uint8_t *scl, uint8_t *tcl, uint16_t width, uint16_t height, uint8_t radiusX, uint8_t radiusY, uint8_t strength, uint8_t T2) {
void gaussBlur(uint8_t *scl, uint8_t *tcl, uint16_t width, uint16_t height, uint8_t radiusX, uint8_t radiusY, uint8_t strength) {

	uint8_t bxsRX[3]; 
	uint8_t bxsRY[3];

	boxesForGauss(bxsRX, radiusX, 3);
	boxesForGauss(bxsRY, radiusY, 3);
	
//    boxBlur(scl, tcl, width, height, (bxsRX[0]-1)/2, (bxsRY[0]-1)/2, strength, T2);
//    boxBlur(tcl, scl, width, height, (bxsRX[1]-1)/2, (bxsRY[1]-1)/2, strength, T2);
//    boxBlur(scl, tcl, width, height, (bxsRX[2]-1)/2, (bxsRY[2]-1)/2, strength, T2);

	boxBlur(scl, tcl, width, height, (bxsRX[0]-1)/2, (bxsRY[0]-1)/2, strength);
	boxBlur(tcl, scl, width, height, (bxsRX[1]-1)/2, (bxsRY[1]-1)/2, strength);
	boxBlur(scl, tcl, width, height, (bxsRX[2]-1)/2, (bxsRY[2]-1)/2, strength);
	
	return;
}




//void boxesForGauss(int *bxs, int sigma, int n)  // standard deviation, number of boxes
//{
//    double wIdeal, mIdeal; 
//    int wl;  
//    int wu;
//    int m;
//    int i;
//	
//    wIdeal = sqrt((12*sigma*sigma/n)+1);  // Ideal averaging filter width 
//	  wl     = floor(wIdeal);  
//    
//    if(wl%2==0) wl--;
//    wu = wl+2;
//				
//    mIdeal = (12*sigma*sigma - n*wl*wl - 4*n*wl - 3*n)/(-4*wl - 4);
//    m = ROUND_TOINT(mIdeal);
//    // var sigmaActual = Math.sqrt( (m*wl*wl + (n-m)*wu*wu - n)/12 );
//				
//    for(i=0; i<n; i++) bxs[i] = (i<m?wl:wu);
//    return;
//}

//void boxesForGauss(uint8_t *bxs, uint8_t sigma, float strength, uint8_t n)  // standard deviation, number of boxes
void boxesForGauss(uint8_t *bxs, uint8_t sigma, uint8_t n)  // standard deviation, number of boxes
{
	double wIdeal, mIdeal; 
	int wl;  
	int wu;
	int m;
	int i;
	
	wIdeal = sqrt((12*sigma*sigma/n)+1);  // Ideal averaging filter width 
	wl     = (int)floor(wIdeal);  
	   
	if(wl%2==0) wl--;
	wu = wl+2;
				
	mIdeal = (12*sigma*sigma - n*wl*wl - 4*n*wl - 3*n)/(-4*wl - 4);
	//m = ROUND_TOINT(mIdeal); // The original provides more smoothing
	m = (int)floor(mIdeal);
	// var sigmaActual = Math.sqrt( (m*wl*wl + (n-m)*wu*wu - n)/12 );
				
	for(i=0; i<n; i++) bxs[i] = (uint8_t)(i<m?wl:wu);
	return;
}



//  The size of iarrX and iarrY depends on the radius specified for blurring.
//  iarrX and iarrY are lookup tables that contain the average value of the row or column pixels. 
//
//  The number pixels specified by radius is given by, radius + radius + 1.
//  Each pixel can have 256 values.
//  
//  Below are the measured sizes required for iarrX and iarrY.
//  the minimum size in bytes of iarrX and iarrY
// 		 radius = 11  needs 5865  more 510
//       radius = 10  needs 5355  more 510
//       radius =  9  needs 4845  more 510 
//       radius =  8  needs 4335  more 510
//       radius =  7  needs 3825  more 510                             
//       radius =  6  needs 3315  more 510                              
//       radius =  5  needs 2805  more 510                             
//       radius =  4  needs 2295  more 510                              
//       radius =  3  needs 1785  more 510                              
//       radius =  2  needs 1275  more 510                              
//       radius =  1  needs  765            (radius + center + radius)*255

//  The value of MAX_RADIUS	 is specified at the top of this file.
#define IARRSIZE ((MAX_RADIUS*2 + 1) * 256)

//void boxBlur(uint8_t *scl, uint8_t *tcl, uint16_t width, uint16_t height, uint8_t radiusX, uint8_t radiusY, uint8_t strength, uint8_t T2) {
void boxBlur(uint8_t *scl, uint8_t *tcl, uint16_t width, uint16_t height, uint8_t radiusX, uint8_t radiusY, uint8_t strength) {

	uint16_t i;	
	int16_t i2;	
	
	// To maximize the speed of the filter all multiplies and divisions are precomputed 
	// here and the results are stored in lookup tables.
	//
	//		iarr[i] contains the precomputed averages for the sums of any set of pixel values within the area defined by radiusX and radiusY.
	//				The precomputed averages are rounded to the nearest pixel value with a range of 0 to 255.
	//				The number of pixels that are within the area specified by radiusX and radiusY is  (radiusX + radiusX + 1) * (radiusY + radiusY + 1).
	//				The lookup table is indexed by the value of the sum of the pixels
	//				Assuming that radiusX and radiusY are both equal to 15, which provide a lot of smoothing, the size of the lookup table needs to be
	//						(15+15+1)*(15+15+1)  or 961 values.
	//
	// 				This lookup table replaces 6 multiplies, 6 divides, 2 additions, 1 conversion to float, 1 float compare, 1 float add, 1 conversion to int,  per pixel. 
	//				With 1 table lookup per pixel.
	//

	
	uint8_t iarrX[IARRSIZE];	// Size Needed is (MAX_RADIUS*2 + 1) * 256 
	   for(i=0; i < IARRSIZE; i++) iarrX[i] = (uint8_t)ROUND_TO_CURVAL(i, i*(1.0 / (radiusX+radiusX+1))); 
	   //for(i=0; i < IARRSIZE; i++) iarrX[i] = (uint16_t)((i*(1.0 / (radiusX+radiusX+1)))*3); 
 
	   
	uint8_t iarrY[IARRSIZE];	
	   for(i=0; i < IARRSIZE; i++) iarrY[i] = (uint8_t)ROUND_TO_CURVAL(i, i*(1.0 / (radiusY+radiusY+1))); 
	   //for(i=0; i < IARRSIZE; i++) iarrY[i] = (uint16_t)((i*(1.0 / (radiusY+radiusY+1)))*3); 


	   

	int16_t SrcSmoothDif[513];	
	float   strengthUse = 1.0F;
	if (strength > 1) strengthUse = (strength / 3.0F) + 2.0F/3.0F;
	for (i2 = -255; i2 < 256; i2++) {
		int16_t temp = 0;
			// Modify the smoothing function to better suit our needs.
		   //temp = (int16_t)Int_FitRange_IntFloat((float)i2, -255.0F, 255.0F,  (-255.0F / strengthUse), 255.0F / strengthUse);
		   //temp = (int16_t)ROUND_TO_CURVAL(i2, Float_fitRangeGamma_Float((float)i2, -255.0F, 255.0F,  -0.50F, (-255.0F / strengthUse), 255.0F / strengthUse));
		   //temp = (int16_t)Float_fitRangeGamma_Float((float)i2, -255.0F, 255.0F,  -1.750F, (-255.0F / strengthUse), 255.0F / strengthUse);
			 temp = (int16_t)Float_fitRangeGamma_Float((float)i2, -255.0F, 255.0F,  0.9750F, (-255.0F / strengthUse), 255.0F / strengthUse);   // BETTER
		   //temp = (int16_t)Float_fitRangeGamma_Float((float)i2, -255.0F, 255.0F,  0.9850F, (-255.0F / strengthUse), 255.0F / strengthUse);

		if (strength==1) temp = (int16_t)Int_FitRange_Float((float)i2, -255.0F, 255.0F,  (-255.0F / strengthUse), 255.0F / strengthUse);


		if (temp > 255) temp = 255;
		if (temp < 0)   temp = 0;

		SrcSmoothDif[i2 + 255] = temp;
	}

	   
	   

	uint16_t radiusTimesFVX[256];
		for(i = 0; i < 256; i++) radiusTimesFVX[i] = (radiusX+1)*i;
		
	uint16_t radiusTimesFVY[256];
		for(i = 0; i < 256; i++) radiusTimesFVY[i] = (radiusY+1)*i;
			

	boxBlurV(tcl, scl, width, height, radiusY, iarrY, radiusTimesFVY, SrcSmoothDif);  // Blur from top to bottom.  Horizontal lines are blurred more than vertical   lines.
	boxBlurH(scl, tcl, width, height, radiusX, iarrX, radiusTimesFVX, SrcSmoothDif);  // Blur from left to right.  Vertical   lines are blurred more than Horizontal lines.

	return;
}




// This blurs by going Horizontally from Left to Right.  It smooths vertical edges.
// For each horizontal line, this routine blurs by going Horizontally from left to right.  It smooths the edges of vertical lines.
void boxBlurH(uint8_t *scl, uint8_t *tcl, uint16_t width, uint16_t height, uint8_t radiusY, uint8_t iarr[], uint16_t radiusTimesFV[], int16_t SrcSmoothDif[]) {
	int16_t i, j;
	uint32_t  ti  = 0;
	uint32_t  ti2 = 0;
	
	for(i=0; i<height; i++) {               // 27 clocks for routine setup + for loop
		uint32_t li = ti;
		uint32_t ri = ti+radiusY;
		
		uint16_t fv  = scl[ti]; 
		uint16_t lv  = scl[ti+width-1]; 
		uint16_t val = radiusTimesFV[fv];	// (radius+1)*fv;     3 more clocks to here
	   
		for(j=0; j<radiusY; j++) {          // 12 clocks
			val += scl[ti+j];               //  8 clocks
		}

		ti2    = ti;         // 1 clock
		ti    += width;      // 2 clocks

		for(j=0; j<=radiusY; j++) {                                     // 3 clocks
			val        += scl[ri++] - fv;                               // 6 clocks
			tcl[ti2++] -= (uint8_t)SrcSmoothDif[(tcl[ti2] - iarr[val]) + 255];   // 15 clocks
		}

		for(j=radiusY+1; j<width-radiusY; j++) {                        // 6 clocks
			val        += scl[ri++] - scl[li++];						// 9 clocks
			tcl[ti2++] -= (uint8_t)SrcSmoothDif[(tcl[ti2] - iarr[val]) + 255];   // 16 clocks
		}  
  
		for(j=width-radiusY; j<width; j++) {                            // 16 clocks
			val        += lv - scl[li++];                               //  1 clock
			tcl[ti2++] -= (uint8_t)SrcSmoothDif[(tcl[ti2] - iarr[val]) + 255];   // 19 clocks
		}
	 }
	   
	return;
}


// This blurs by going Vertically from Top to Bottom.  It smooths horizontal edges.
// For each Vertical line, this routine blurs by going Vertically from top to bottom.  It smooths the edges of horizontal lines.
void boxBlurV(uint8_t *scl, uint8_t *tcl, uint16_t width, uint16_t height, uint8_t radiusX, uint8_t iarr[], uint16_t radiusTimesFV[], int16_t SrcSmoothDif[]) {
	int16_t   i, j;
	uint32_t  WidthTimesHeightM1 = width*(height-1);
	uint32_t  radiusTimesWidth   = radiusX*width;

	for(i=0; i<width; i++) {
		uint32_t ti = i;
		uint32_t li = ti; 
		uint32_t ri = ti+radiusTimesWidth;
		
		uint16_t fv     = scl[ti];
		uint16_t lv     = scl[ti + WidthTimesHeightM1];    // width*(height-1)
		int16_t  val    = radiusTimesFV[fv];               // (radius+1)*fv;
		uint16_t jWidth = 0;               
		
		for(j=0; j<radiusX; j++) {
			val    += scl[ti + jWidth];    
			jWidth += width;
		}
		
		for(j=0; j<=radiusX; j++) { 
			val    += scl[ri] - fv;  
			tcl[ti] -= (uint8_t)SrcSmoothDif[(tcl[ti] - iarr[val]) + 255];
			ri += width; 
			ti += width; 
		}
		 
		for(j=radiusX+1; j<height-radiusX; j++) { 
			val    += scl[ri] - scl[li];  
			tcl[ti] -= (uint8_t)SrcSmoothDif[(tcl[ti] - iarr[val]) + 255]; 
			li +=width; 
			ri +=width; 
			ti +=width; 
		}

		for(j=height-radiusX; j<height; j++) { 
			val     += lv - scl[li];  
			tcl[ti] -= (uint8_t)SrcSmoothDif[(tcl[ti] - iarr[val]) + 255]; 
			li += width; 
			ti += width; 
		}
	}  
	
	return;
}























//void  LumaFrameMedian(FrameInfo_args *FrameInfoArgs, uint8_t *src, uint8_t *dst, uint8_t radius) {
//
//	uint16_t  src_height           = FrameInfoArgs->src_height;
//	uint16_t  src_width	           = FrameInfoArgs->src_width;
//
//
//	ctmf(src, dst, src_width, src_height, src_width, src_width, radius, 1, 256*1024);
//		
//	return;
//}       














//  NOTE THE FOLOWING OPTIMALLY FAST MEDIAN ROUTINES ARE NOT CURRENTLY BEING USED !!!!!!

/* The following is from   http://ndevilla.free.fr/median/median/
 * The following routines have been built from knowledge gathered * around the Web. I am not aware of any copyright problem with * them, so use it as you want. * N. Devillard - 1998 */ 
//typedef float pixelvalue ; 
typedef unsigned char pixelvalue ; 
#define PIX_SORT(a,b) { if ((a)>(b)) PIX_SWAP((a),(b)); } 
#define PIX_SWAP(a,b) { pixelvalue temp=(a);(a)=(b);(b)=temp; } 
/*-----Function : opt_med3() In : pointer to array of 3 pixel values Out : a pixelvalue Job 
: optimized search of the median of 3 pixel values Notice : found on sci.image.processing cannot go faster unless assumptions are made on the nature of the input signal. */ 
pixelvalue opt_med3(pixelvalue * p) { 
PIX_SORT(p[0],p[1]) ; 
PIX_SORT(p[1],p[2]) ; 
PIX_SORT(p[0],p[1]) ; 
return(p[1]) ; } 

/* Function : opt_med5() In : pointer to array of 5 pixel values Out : a pixelvalue Job : optimized search of the median of 5 pixel values Notice : f
ound on sci.image.processing cannot go faster unless assumptions are made on the nature of the input signal. ------------------------------------*/ 
pixelvalue opt_med5(pixelvalue * p) { 
PIX_SORT(p[0],p[1]) ; 
PIX_SORT(p[3],p[4]) ; 
PIX_SORT(p[0],p[3]) ; 
PIX_SORT(p[1],p[4]) ; 
PIX_SORT(p[1],p[2]) ; 
PIX_SORT(p[2],p[3]) ; 
PIX_SORT(p[1],p[2]) ; 
return(p[2]) ; } 

/* Function : opt_med6() In : pointer to array of 6 pixel values Out : a pixelvalue Job : optimized search of the median of 6 pixel values Notice 
: from Christoph_John@gmx.de based on a selection network which was proposed in "FAST, EFFICIENT MEDIAN FILTERS WITH EVEN LENGTH WINDOWS" 
J.P. HAVLICEK, K.A. SAKADY, G.R.KATZ If you need larger even length kernels check the paper -------------------------------------*/ 
pixelvalue opt_med6(pixelvalue * p) { 
PIX_SORT(p[1], p[2]); 
PIX_SORT(p[3], p[4]); 
PIX_SORT(p[0], p[1]); 
PIX_SORT(p[2], p[3]); 
PIX_SORT(p[4], p[5]); 
PIX_SORT(p[1], p[2]); 
PIX_SORT(p[3], p[4]); 
PIX_SORT(p[0], p[1]); 
PIX_SORT(p[2], p[3]); 
PIX_SORT(p[4], p[5]); 
PIX_SORT(p[1], p[2]); 
PIX_SORT(p[3], p[4]); 
return ( (p[2] + p[3] + 1) >>1 ); 
//return (p[2] + p[3]) * 0.5;
/* PIX_SORT(p[2], p[3]) results in lower median in p[2] and upper median in p[3] */
}

/* Function : opt_med7() In : pointer to array of 7 pixel values Out : a pixelvalue Job : optimized search of the median of 7 pixel values Notice : 
found on sci.image.processing cannot go faster unless assumptions are made on the nature of the input signal. */ 
pixelvalue opt_med7(pixelvalue * p) { 
PIX_SORT(p[0], p[5]) ; 
PIX_SORT(p[0], p[3]) ; 
PIX_SORT(p[1], p[6]) ; 
PIX_SORT(p[2], p[4]) ; 
PIX_SORT(p[0], p[1]) ; 
PIX_SORT(p[3], p[5]) ; 
PIX_SORT(p[2], p[6]) ; 
PIX_SORT(p[2], p[3]) ; 
PIX_SORT(p[3], p[6]) ; 
PIX_SORT(p[4], p[5]) ; 
PIX_SORT(p[1], p[4]) ; 
PIX_SORT(p[1], p[3]) ; 
PIX_SORT(p[3], p[4]) ; 
return (p[3]) ; } 

/* Function : opt_med9() In : pointer to an array of 9 pixelvalues Out : a pixelvalue Job : optimized search of the median of 9 pixelvalues Notice : 
in theory, cannot go faster without assumptions on the signal. Formula from: XILINX XCELL magazine, vol. 23 by John L. Smith 
The input array is modified in the process The result array is guaranteed to contain the median value in middle position, but other elements are NOT sorted. */ 
pixelvalue opt_med9(pixelvalue * p) { 
PIX_SORT(p[1], p[2]) ; 
PIX_SORT(p[4], p[5]) ; 
PIX_SORT(p[7], p[8]) ; 
PIX_SORT(p[0], p[1]) ; 
PIX_SORT(p[3], p[4]) ; 
PIX_SORT(p[6], p[7]) ; 
PIX_SORT(p[1], p[2]) ; 
PIX_SORT(p[4], p[5]) ; 
PIX_SORT(p[7], p[8]) ; 
PIX_SORT(p[0], p[3]) ; 
PIX_SORT(p[5], p[8]) ; 
PIX_SORT(p[4], p[7]) ; 
PIX_SORT(p[3], p[6]) ; 
PIX_SORT(p[1], p[4]) ; 
PIX_SORT(p[2], p[5]) ; 
PIX_SORT(p[4], p[7]) ; 
PIX_SORT(p[4], p[2]) ; 
PIX_SORT(p[6], p[4]) ; 
PIX_SORT(p[4], p[2]) ; 
return(p[4]) ; } 

/*  Function : opt_med25() In : pointer to an array of 25 pixelvalues Out : a pixelvalue Job : optimized search of the median of 25 pixelvalues Notice : 
in theory, cannot go faster without assumptions on the signal. Code taken from Graphic Gems. */ 
pixelvalue opt_med25(pixelvalue * p) { 
PIX_SORT(p[0], p[1]) ; 
PIX_SORT(p[3], p[4]) ; 
PIX_SORT(p[2], p[4]) ; 
PIX_SORT(p[2], p[3]) ; 
PIX_SORT(p[6], p[7]) ; 
PIX_SORT(p[5], p[7]) ; 
PIX_SORT(p[5], p[6]) ; 
PIX_SORT(p[9], p[10]) ; 
PIX_SORT(p[8], p[10]) ; 
PIX_SORT(p[8], p[9]) ; 
PIX_SORT(p[12], p[13]) ; 
PIX_SORT(p[11], p[13]) ; 
PIX_SORT(p[11], p[12]) ; 
PIX_SORT(p[15], p[16]) ; 
PIX_SORT(p[14], p[16]) ; 
PIX_SORT(p[14], p[15]) ; 
PIX_SORT(p[18], p[19]) ; 
PIX_SORT(p[17], p[19]) ; 
PIX_SORT(p[17], p[18]) ; 
PIX_SORT(p[21], p[22]) ; 
PIX_SORT(p[20], p[22]) ; 
PIX_SORT(p[20], p[21]) ; 
PIX_SORT(p[23], p[24]) ; 
PIX_SORT(p[2], p[5]) ; 
PIX_SORT(p[3], p[6]) ; 
PIX_SORT(p[0], p[6]) ; 
PIX_SORT(p[0], p[3]) ; 
PIX_SORT(p[4], p[7]) ; 
PIX_SORT(p[1], p[7]) ; 
PIX_SORT(p[1], p[4]) ; 
PIX_SORT(p[11], p[14]) ; 
PIX_SORT(p[8], p[14]) ; 
PIX_SORT(p[8], p[11]) ; 
PIX_SORT(p[12], p[15]) ; 
PIX_SORT(p[9], p[15]) ; 
PIX_SORT(p[9], p[12]) ; 
PIX_SORT(p[13], p[16]) ; 
PIX_SORT(p[10], p[16]) ; 
PIX_SORT(p[10], p[13]) ; 
PIX_SORT(p[20], p[23]) ; 
PIX_SORT(p[17], p[23]) ; 
PIX_SORT(p[17], p[20]) ; 
PIX_SORT(p[21], p[24]) ; 
PIX_SORT(p[18], p[24]) ; 
PIX_SORT(p[18], p[21]) ; 
PIX_SORT(p[19], p[22]) ; 
PIX_SORT(p[8], p[17]) ; 
PIX_SORT(p[9], p[18]) ; 
PIX_SORT(p[0], p[18]) ; 
PIX_SORT(p[0], p[9]) ; 
PIX_SORT(p[10], p[19]) ; 
PIX_SORT(p[1], p[19]) ; 
PIX_SORT(p[1], p[10]) ; 
PIX_SORT(p[11], p[20]) ; 
PIX_SORT(p[2], p[20]) ; 
PIX_SORT(p[2], p[11]) ; 
PIX_SORT(p[12], p[21]) ; 
PIX_SORT(p[3], p[21]) ; 
PIX_SORT(p[3], p[12]) ; 
PIX_SORT(p[13], p[22]) ; 
PIX_SORT(p[4], p[22]) ; 
PIX_SORT(p[4], p[13]) ; 
PIX_SORT(p[14], p[23]) ; 
PIX_SORT(p[5], p[23]) ; 
PIX_SORT(p[5], p[14]) ; 
PIX_SORT(p[15], p[24]) ; 
PIX_SORT(p[6], p[24]) ; 
PIX_SORT(p[6], p[15]) ; 
PIX_SORT(p[7], p[16]) ; 
PIX_SORT(p[7], p[19]) ; 
PIX_SORT(p[13], p[21]) ; 
PIX_SORT(p[15], p[23]) ; 
PIX_SORT(p[7], p[13]) ; 
PIX_SORT(p[7], p[15]) ; 
PIX_SORT(p[1], p[9]) ; 
PIX_SORT(p[3], p[11]) ; 
PIX_SORT(p[5], p[17]) ; 
PIX_SORT(p[11], p[17]) ; 
PIX_SORT(p[9], p[17]) ; 
PIX_SORT(p[4], p[10]) ; 
PIX_SORT(p[6], p[12]) ; 
PIX_SORT(p[7], p[14]) ; 
PIX_SORT(p[4], p[6]) ; 
PIX_SORT(p[4], p[7]) ; 
PIX_SORT(p[12], p[14]) ; 
PIX_SORT(p[10], p[14]) ; 
PIX_SORT(p[6], p[7]) ; 
PIX_SORT(p[10], p[12]) ; 
PIX_SORT(p[6], p[10]) ; 
PIX_SORT(p[6], p[17]) ; 
PIX_SORT(p[12], p[17]) ; 
PIX_SORT(p[7], p[17]) ; 
PIX_SORT(p[7], p[10]) ; 
PIX_SORT(p[12], p[18]) ; 
PIX_SORT(p[7], p[12]) ; 
PIX_SORT(p[10], p[18]) ; 
PIX_SORT(p[12], p[20]) ; 
PIX_SORT(p[10], p[20]) ; 
PIX_SORT(p[10], p[12]) ; 
return (p[12]); } 

#undef PIX_SORT 
#undef PIX_SWAP 






  