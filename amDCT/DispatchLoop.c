
//#include "DispatchLoop.h"
#include "amDCTtypedefs.h"

#include "DispatchLoop.h"
#include "DctLoop.h"
#include "Threading.h"


/*
// THIS IS CODE TO TEST USING MULTIPLE MOTION COMPENSATED FRAMES 
// WE NEED A TOP LEVEL DispatchLoop() which calls the current DispatchLoop()
// It will call it once for each frame clp, pf1 ... changing BF_srcP to point to the correct frame.

int   DispatchLoop1(FrameInfo_args *FrameInfoArgs) {

	dup_cpu_data(FrameInfoArgs);	
	DispatchLoop1(FrameInfoArgs);	


FrameInfoArgs->psrc = FrameInfoArgs->pf1;
	
	if (FrameInfoArgs->pf1 != NULL) {
		MY_MEMCPY(&FrameInfoArgs->MemoryArgs.BF_src[0][0], FrameInfoArgs->pf1,  sizeof(uint8_t) * FrameInfoArgs->sizeY);  
		dup_cpu_data(FrameInfoArgs);	
		DispatchLoop1(FrameInfoArgs);	
	}
	
FrameInfoArgs->psrc = FrameInfoArgs->nf1;
	
	if (FrameInfoArgs->nf1 != NULL) {
		MY_MEMCPY(&FrameInfoArgs->MemoryArgs.BF_src[0][0], FrameInfoArgs->nf1,  sizeof(uint8_t) * FrameInfoArgs->sizeY);  
		dup_cpu_data(FrameInfoArgs);	
		DispatchLoop1(FrameInfoArgs);	
	}
	
	return;
}
*/




int   DispatchLoop(FrameInfo_args *FrameInfoArgs) {

	uint8_t      shift  = FrameInfoArgs->shift;
	uint8_t		 ncpu   = FrameInfoArgs->ncpu;	
	uint8_t	     startj = BLKSIZE;	// just used for readability in DctLoop calls
	uint8_t	     starti = BLKSIZE;
	uint8_t      i, j;
	uint16_t     k;    

	for (k = 0; k < ncpu; k++) {
		DctLoop_args *DctLoopArgs = &FrameInfoArgs->MemoryArgs->DctLoopArgs[k];
	
	
		switch(shift) { 
	
		// Special case for debugging.  It bypasses the inner loop and will give a speed test for the rest of the system.
		case 21:  // This could be made 20-2x and move the block by varying amounts but I can't see the point.
		case 20:  // This could be made 20-2x and move the block by varying amounts but I can't see the point.
			shift = 1;
			DctLoop(starti, startj, DctLoopArgs);
			break;
			
		// For debugging compare original For historical comparison
		// Note: fall through until case 11 then break.
		case 19:
			DctLoop(starti-5, startj-4, DctLoopArgs);
		case 18:
			DctLoop(starti-1, startj-0, DctLoopArgs);
		case 17:
			DctLoop(starti-7, startj-8, DctLoopArgs);
		case 16:
			DctLoop(starti-3, startj-2, DctLoopArgs);
		case 15:
			DctLoop(starti-6, startj-6, DctLoopArgs);
		case 14:
			DctLoop(starti-2, startj-3, DctLoopArgs);
		case 13:
			DctLoop(starti-8, startj-7, DctLoopArgs);  // note that starti is out of range in original version
		case 12:
			DctLoop(starti-0, startj-1, DctLoopArgs);
		case 11:
			shift = shift - 10;
			//DctLoop(starti-4, startj-5, DctLoopArgs);
			DctLoop(starti, startj, DctLoopArgs);
			break;
			
		// SPEED TEST	
		case 10: // Debug. used to provide non inner loop speed for performance testing.
			break;
	
	
		//  l=left  u=up 
		case 9:  // Do All 64 Shifts.  Test original movement, from the reference paper in the docs folder, for the 64 shifts. As expected there is no difference.
	
			starti = BLKSIZE + 4;
			startj = BLKSIZE + 4;
			if (ncpu == 1) {
				for(i = 0; i < BLKSIZE; i++) {
					for (j = 0; j < BLKSIZE; j++) {
						DctLoop(starti-i, startj-j, DctLoopArgs);
					}
				}
				break;
			}
	
			
	
		// Casees 6, 7, and 8 were used to test various shifts around a specific point.  At the time didn't find anything useful.
		case 8:  // 8 shifts 	
						
			if (ncpu == 1) { 
				DctLoop(starti-1, startj-1, DctLoopArgs);	// l6 u2
				DctLoop(starti-1, startj+1, DctLoopArgs);	// l5 u6
				DctLoop(starti+1, startj+1, DctLoopArgs);	// l2 u1
				DctLoop(starti+1, startj-1, DctLoopArgs);	// l1 u5
	
				DctLoop(starti+1, startj-0, DctLoopArgs);	// l6 u2
				DctLoop(starti-1, startj-0, DctLoopArgs);	// l5 u6
				DctLoop(starti-0, startj-1, DctLoopArgs);	// l2 u1
				DctLoop(starti-0, startj+1, DctLoopArgs);	// l1 u5
				break;
			}
	
	
		case 7:  // 4 shifts 	
						
			if (ncpu == 1) { 
				DctLoop(starti-4, startj-3, DctLoopArgs);	// l6 u2
				DctLoop(starti-4, startj-4, DctLoopArgs);	// l5 u6
				DctLoop(starti-3, startj-4, DctLoopArgs);	// l2 u1
				DctLoop(starti-3, startj-3, DctLoopArgs);	// l1 u5
				break;
			}
	
		case 6:  // 4 shifts 	
						
			if (ncpu == 1) { 
				DctLoop(starti-1, startj-1, DctLoopArgs);	// l6 u2
				DctLoop(starti-1, startj+1, DctLoopArgs);	// l5 u6
				DctLoop(starti+1, startj+1, DctLoopArgs);	// l2 u1
				DctLoop(starti+1, startj-1, DctLoopArgs);	// l1 u5
				break;
			}

	
	
	
		//  l=left  u=up 
		case 5:  // Do All 64 Shifts.  
	
			if (ncpu == 1) {
				for(i = 0; i < BLKSIZE; i++) {
					for (j = 0; j < BLKSIZE; j++) {
						DctLoop(starti-i, startj-j, DctLoopArgs);
					}
				}
				break;
			}
	
			if (ncpu > 1) {
				for(i = 0; i < BLKSIZE; i++) {
					for (j = 0; j < BLKSIZE; j++) {
						setShift(starti-i, startj-j, FrameInfoArgs);
					}
				}
				startDctLoop(FrameInfoArgs);
				return(0);
			}
			
	
		case 4:  // Do 32 Shifts.  Use quincunx also known as checkerboard pattern.
			
			if (ncpu == 1) { 
				for(i = 0; i < BLKSIZE; i++) {
					if (i%2 == 0) { // even odd test
						for (j = 0; j < BLKSIZE; j += 2) {
							DctLoop(starti-i, startj-j, DctLoopArgs);
						}
					}
					else {	
						for (j = 1; j <= BLKSIZE; j += 2) {
							DctLoop(starti-i, startj-j, DctLoopArgs);
						}
					}
				}
				break;
			}
	
			if (ncpu > 1) {
				for(i = 0; i < BLKSIZE; i++) {
					if (i%2 == 0) { // even odd test
						for (j = 0; j < BLKSIZE; j += 2) {
							setShift(starti-i, startj-j, FrameInfoArgs);
						}
					}
					else {	
						for (j = 1; j <= BLKSIZE; j += 2) {
							setShift(starti-i, startj-j, FrameInfoArgs);
						}
					}
				}
	
				startDctLoop(FrameInfoArgs);
				return(0);
			}
			
	
		case 3: // 16 shifts based on 4 instances of 4 queens 1 instance in each quadrant.
			// Note: l7 u5 means shift frame left 7 pixels and up 5 pixels.
			if (ncpu == 1) {
				DctLoop(starti-7, startj-5, DctLoopArgs);	//  l7  u5
				DctLoop(starti-7, startj-1, DctLoopArgs);	//  l7  u5
				DctLoop(starti-6, startj-7, DctLoopArgs);	//  l6  u7
				DctLoop(starti-6, startj-3, DctLoopArgs);	//  l6  u3
				DctLoop(starti-5, startj-4, DctLoopArgs);	//  l5  u4
				DctLoop(starti-5, startj-0, DctLoopArgs);	//  l5  u0
				DctLoop(starti-4, startj-6, DctLoopArgs);	//  l4  u6
				DctLoop(starti-4, startj-2, DctLoopArgs);	//  l4  u2
				DctLoop(starti-3, startj-5, DctLoopArgs);	//  l3  u6
				DctLoop(starti-3, startj-1, DctLoopArgs);	//  l3  u1
				DctLoop(starti-2, startj-7, DctLoopArgs);	//  l2  u7
				DctLoop(starti-2, startj-3, DctLoopArgs);	//  l2  u3
				DctLoop(starti-1, startj-4, DctLoopArgs);	//  l1  u4
				DctLoop(starti-1, startj-1, DctLoopArgs);	//  l1  u1
				DctLoop(starti-0, startj-6, DctLoopArgs);	//  l0  u6
				DctLoop(starti-0, startj-1, DctLoopArgs);	//  l0  u2
				break;
				}
	
	
			if (ncpu > 1) {		
				setShift(starti-7, startj-5, FrameInfoArgs);	//  l7  u5
				setShift(starti-7, startj-1, FrameInfoArgs);	//  l7  u5
				setShift(starti-6, startj-7, FrameInfoArgs);	//  l6  u7
				setShift(starti-6, startj-3, FrameInfoArgs);	//  l6  u3
				setShift(starti-5, startj-4, FrameInfoArgs);	//  l5  u4
				setShift(starti-5, startj-0, FrameInfoArgs);	//  l5  u0
				setShift(starti-4, startj-6, FrameInfoArgs);	//  l4  u6
				setShift(starti-4, startj-2, FrameInfoArgs);	//  l4  u2
				setShift(starti-3, startj-6, FrameInfoArgs);	//  l3  u6
				setShift(starti-3, startj-1, FrameInfoArgs);	//  l3  u1
				setShift(starti-2, startj-7, FrameInfoArgs);	//  l2  u7
				setShift(starti-2, startj-3, FrameInfoArgs);	//  l2  u3
				setShift(starti-1, startj-4, FrameInfoArgs);	//  l1  u4
				setShift(starti-1, startj-1, FrameInfoArgs);	//  l1  u1
				setShift(starti-0, startj-6, FrameInfoArgs);	//  l0  u6
				setShift(starti-0, startj-2, FrameInfoArgs);	//  l0  u2
		
				startDctLoop(FrameInfoArgs);
				return(0);
			}
	
	
	
	
	  
		case 2: // Use 1 of the 8 queens solutions as shift positions.
	
			if (ncpu == 1) { 
				DctLoop(starti-6, startj-0, DctLoopArgs);  // original 8 queens
				DctLoop(starti-2, startj-1, DctLoopArgs);
				DctLoop(starti-7, startj-2, DctLoopArgs);
				DctLoop(starti-1, startj-3, DctLoopArgs);
				DctLoop(starti-4, startj-4, DctLoopArgs);
				DctLoop(starti-0, startj-5, DctLoopArgs);
				DctLoop(starti-5, startj-6, DctLoopArgs);
				DctLoop(starti-3, startj-7, DctLoopArgs);
		
				return(0);
				break;
			}
			
			if (ncpu > 1) {			
				setShift(starti-6, startj-0, FrameInfoArgs);
				setShift(starti-2, startj-1, FrameInfoArgs);
				setShift(starti-7, startj-2, FrameInfoArgs);
				setShift(starti-1, startj-3, FrameInfoArgs);
				setShift(starti-4, startj-4, FrameInfoArgs);
				setShift(starti-0, startj-5, FrameInfoArgs);
				setShift(starti-5, startj-6, FrameInfoArgs);
				setShift(starti-3, startj-7, FrameInfoArgs);
		
				startDctLoop(FrameInfoArgs);
				return(0);
			}
	
	
	
		case 1:  // 4 shifts 	
						
			if (ncpu == 1) { 
				DctLoop(starti-6, startj-2, DctLoopArgs);	// l6 u2
				DctLoop(starti-5, startj-4, DctLoopArgs);	// l5 u6
				DctLoop(starti-3, startj-3, DctLoopArgs);	// l2 u1
				DctLoop(starti-1, startj-5, DctLoopArgs);	// l1 u5

				return(0);		      	
				break;
			}
			
			if (ncpu > 1) {		
				setShift(starti - 6, startj - 2, FrameInfoArgs);	// l6 u2
				setShift(starti - 5, startj - 4, FrameInfoArgs);	// l5 u6
				setShift(starti - 3, startj - 3, FrameInfoArgs);	// l2 u1
				setShift(starti - 1, startj - 5, FrameInfoArgs);	// l1 u5
				
				//startDctLoop(FrameInfoArgs->MemoryArgs->DctLoopArgs[0]);
				startDctLoop(FrameInfoArgs);
				return(0);
			}
	
	
		
		case 0:  // 0 shifts. It executes the inner loop once without any shifts.
			shift = 1;
			DctLoop(starti, startj, DctLoopArgs);
			break;
			
		default:
			break;
	
		};
	}
	__asm { emms }


	return(0);
}


