#ifndef _FITRANGE_H_
#define _FITRANGE_H_

// #include "amDCTtypedefs.h" 


 

void testFitrange(char *msgBuf);


 /*
__inline  int fitRange(int curVal, int srcMax, int srcMin, int dstMax, int dstMin) {
	int d = (curVal-srcMin) * (dstMax-dstMin);
	return( (( d + (d>>1)) / (srcMax-srcMin) )  + dstMin );
}
*/

/*
__inline  int fitRange(int curVal, int srcMax, int srcMin, int dstMax, int dstMin) {
//	int d = ((curVal-srcMin) * (dstMax-dstMin));
	return( (( ((curVal-srcMin) * (dstMax-dstMin)) + (((curVal-srcMin) * (dstMax-dstMin))>>1)) / (srcMax-srcMin) )  + dstMin );
}
*/


//#define fitRange(curVal, srcMax, srcMin, dstMax, dstMin) \
// 	( ((curVal) - (srcMin)) * ( ( ((dstMax)-(dstMin))  ) / ((srcMax)-(srcMin)) )  + (dstMin) )

#define fitRange(curVal, srcMax, srcMin, dstMax, dstMin) \
	( ((curVal) - (srcMin)) * ( ( ((dstMax)-(dstMin)) + ( ((srcMax)-(srcMin)) >> 1 ) ) / ((srcMax)-(srcMin)) )  + (dstMin) )



//#define Int_FitRange_Int(curVal, srcMax, srcMin, dstMax, dstMin) \
// 	( ( ((curVal) - (srcMin)) * ( ( ((dstMax)-(dstMin)) + ( ((srcMax)-(srcMin)) >> 1 ) ) / ((srcMax)-(srcMin)) )  + (dstMin) ) > (dstMax)?(dstMax):
// 	
//	 MIN(X, Y) ((X)<(Y)?(X):(Y))






//  NAME SCHEMA FOR THE FITRANGE ROUTINES
//  XXXFitRangeYYYZZZ
//  	XXX either Float or Int. The data type being returned.
//		YYY either Float or Int. The data type of the arguments.
//		NoClip     If specified the return value is NOT clipped to the dest arguments.  The only function that does NOT clip the returned arguments is Float_FitRange_Float_NoClip()
//Int_FitRangeInt
//Int_FitRangeFloat
//
//Int_FitRangeIntClip
//Int_FitRangeFloatClip
//
//Float_FitRangeInt
//Float_FitRangeFloat
//
//Float_FitRangeIntClip
//Float_FitRangeFloatClip



int
Int_FitRange_Int(
int curval, 
int srcMax, 
int srcMin, 
int dstMax, 
int dstMin);



int
Int_FitRange_Float(
float curVal, 
float srcMax, 
float srcMin, 
float dstMax, 
float dstMin);



float
Float_FitRange_Float_NoClip(
float curVal, 
float srcMax, 
float srcMin, 
float dstMax, 
float dstMin);


float
Float_FitRange_Float(
float curVal, 
float srcMax, 
float srcMin, 
float dstMax, 
float dstMin);




int
Int_fitRangeGamma_Int(
int   curVal, 
int   srcMin, 
int   srcMax, 
float gamma,
int   dstMin, 
int   dstMax);


float
Float_fitRangeGamma_Float(
float curVal, 
float srcMin, 
float srcMax, 
float gamma,
float dstMin, 
float dstMax);




#endif // _FITRANGE_H_
