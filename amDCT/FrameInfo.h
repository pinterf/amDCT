#ifndef _FRAMEINFO_H_
#define _FRAMEINFO_H_

#include "amDCTtypedefs.h"




int  create_frame_info(FrameInfo_args  *FrameInfoArgs, 
					   uint8_t         *psrc, 
	//				   uint8_t         *pf1, 
	//				   uint8_t         *nf1, 
					   uint8_t 	       *pdst,
					   uint8_t          quant_a, 
					   uint8_t          adapt_a, 
					   
					   uint16_t         src_width, 
					   uint16_t         src_pitch, 
					   uint16_t         src_height, 
					   uint16_t         dst_width, 
					   uint16_t         dst_pitch, 
					   uint16_t         dst_height, 
	//				   uint8_t	  	    sharpWPos,
	//				   uint8_t	  	    sharpWAmt,
	//				   uint8_t	  	    sharpTPos,
	//				   uint8_t	  	    sharpTAmt,
					   uint8_t          brightStart, 
					   uint8_t          brightAmt, 
					   uint8_t          darkStart, 
					   uint8_t          darkAmt, 
						   
					   uint8_t          doSmoothFlag, 
					   uint8_t          doSharpFlag, 
					   uint8_t          doExpandFlag,
					   uint8_t          doBrightFlag,
					   uint8_t          doDarkFlag,
				   
					   uint8_t          ncpu,
					   uint8_t		    quality,
	 				   uint8_t		    T2);								  
	
	
	void  doFrameInfo(uint8_t			FrameQuant,
					  uint8_t			quant,
					  uint8_t			qtype, 
					  uint8_t			adapt,
					  uint8_t			shift, 
					  uint8_t			matrix, 
	
					  uint8_t			expand,
					  uint8_t			sharpWPos,
					  uint8_t			sharpWAmt,
					  uint8_t			sarpTPos,
					  uint8_t			sharpTAmt,
	
					  FrameInfo_args   *FrameInfoArgs,
					
					  uint8_t			showMask,  
					  uint8_t			T2);



void buildMasks(FrameInfo_args        *FrameInfoArgs);
//void preProcessEdges(FrameInfo_args *FrameInfoArgs);
void smoothedSource(FrameInfo_args    *FrameInfoArgs, uint8_t type);
void sharpSource(FrameInfo_args       *FrameInfoArgs);


#endif // _FRAMEINFO_H_
