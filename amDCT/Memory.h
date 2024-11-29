#ifndef _MEMORY_H_
#define _MEMORY_H_


#include "amDCTtypedefs.h"


// Allign malloced memory at 16 bytes.
#define ALIGN_16 16



int   malloc_data(FrameInfo_args* FrameInfoArgs);
void dup_cpu_data(FrameInfo_args* FrameInfoArgs);
void clear_data_work_buffers(FrameInfo_args* FrameInfoArgs);
//void clear_frame_work_buffers(FrameInfo_args *FrameInfoArgs); //NOT USED
void free_data(FrameInfo_args* FrameInfoArgs);

#endif // _MEMORY_H_
