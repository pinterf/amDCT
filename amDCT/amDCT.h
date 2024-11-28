#ifndef _AMDCTFILTER_H_
#define _AMDCTFILTER_H_




#ifdef __cplusplus
extern "C" 




int  __cdecl amDCTmain(  // return=0 no error, return=1 out of memory
const    unsigned char *psrc, 
//    unsigned char *pf1,
//    unsigned char *pb1,
	unsigned char *pdst, 
	unsigned int   src_height,  
	unsigned int   src_width, 
	unsigned int   src_pitch,
	unsigned int   dst_height,  
	unsigned int   dst_width,  
	unsigned int   dst_pitch,  
	int   		   quant,  
	int   		   adapt,  
	int   		   shift,  
	int   		   matrix,  
	int  		   qtype,
	int			   expand,		    // expand
	int			   sharpWPos,		// sharpWPos
	int			   sharpWAmt,		// sharpWAmt
	int   		   sharpTPos,		// sharpTPos
	int   		   sharpTAmt,		// sharpTAmt
	int   		   quality,			   
	int			   brightStart,		
	int			   brightAmt,		
	int			   darkStart,		
	int			   darkAmt,		   
	int			   showMask,
	int			   T2,
	int            ncpu);

#else

int __cdecl amDCTmain(  // return=0 no error, return=1 out of memory
const	unsigned char *psrc, 
//    unsigned char *pf1,
//    unsigned char *pb1,
	unsigned char *pdst, 
	unsigned int   src_height,  
	unsigned int   src_width,  
	unsigned int   src_pitch,
	unsigned int   dst_height,  
	unsigned int   dst_width,  
	unsigned int   dst_pitch,  
	int   		   quant,  
	int   		   adapt,  
	int   		   shift,  
	int   		   matrix,  
	int  		   qtype,
	int			   expand,			// expand
	int			   sharpWPos,		// sharpWPos
	int			   sharpWAmt,		// sharpWAmt
	int   		   sharpTPos,		// sharpTPos
	int   		   sharpTAmt,		// sharpTAmt
	int   		   quality,			
	int			   brightStart,
	int			   brightAmt,
	int			   darkStart,		
	int			   darkAmt,		   
	int			   showMask,
	int			   T2,
	int            ncpu);

#endif




#endif 
/*_AMDCTFILTER_H_*/


