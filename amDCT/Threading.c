#include <process.h>

#include "amDCT.h"
#include "Threading.h"
#include "DctLoop.h"
#include "transfer_add.h"
#include <threads.h>
#include <stdint.h>




// This collects all of the shift information.
void setShift(uint8_t        starti,
  uint8_t        startj,
  FrameInfo_args* args) {

  int cntShift = args->MemoryArgs->DctLoopArgs[0].cntShift;

  args->MemoryArgs->DctLoopArgs[0].starti[cntShift] = starti;
  args->MemoryArgs->DctLoopArgs[0].startj[cntShift] = startj;

  args->MemoryArgs->DctLoopArgs[0].cntShift += 1;

  return;
}



// This distributes the shift information into ncpu groups which are assigned and dispatched to ncpu threads.
// It then collects and combines the returned information from each thread.

int dct_loop_wrapper(void* arg) {
  DctLoopThread(arg);
  return 0;
}

void startDctLoop(FrameInfo_args* args) {
  DctLoop_args* DctLoopArgs = &args->MemoryArgs->DctLoopArgs[0];
  uint8_t ncpu = DctLoopArgs->ncpu;
  thrd_t threads[4];
  // This provides the first approximation of the number of shiftsPerThread
  uint8_t shiftsPerThread = DctLoopArgs->cntShift / ncpu;

  // NOTE: This sets the shiftsPerThread so that the last thread
  // will be within + or - 1 of the total threads to do.
  // The last iteration of the for (k = 0; k < ncpu; k++) loop below
  // will handle the + or - 1.
  if (ncpu == 3) {
    switch (DctLoopArgs->cntShift) {
    case 4:  shiftsPerThread = 1; break; // shifts 1, 1, 2
    case 8:  shiftsPerThread = 3; break; // shifts 3, 3, 2
    case 16: shiftsPerThread = 5; break; // shifts 5, 5, 6
    case 32: shiftsPerThread = 11; break; // shifts 11, 11, 10
    case 64: shiftsPerThread = 21; break; // shifts 21, 21, 22
    }
  }

  uint8_t startShift = 0;
  uint8_t endShift = shiftsPerThread - 1;

  // Store all of the thread specific information.
  for (uint8_t k = 0; k < ncpu; k++) {
    DctLoop_args* DctLoopArgs_ncpu = &args->MemoryArgs->DctLoopArgs[k];
    DctLoopArgs_ncpu->threadNum = k;
    DctLoopArgs_ncpu->startShift = startShift;
    DctLoopArgs_ncpu->endShift = endShift;

    // The 2 ifs handle the cases where the number of threads (ncpu) does not divide evenly into the number of shifts.
    // This only happens if ncpu == 3
    if (k == ncpu - 1) {
      DctLoopArgs_ncpu->endShift = DctLoopArgs->cntShift - 1;
    }
    if (startShift > (DctLoopArgs->cntShift - 1)) {
      DctLoopArgs_ncpu->startShift = DctLoopArgs->cntShift - 1;
    }

    startShift += shiftsPerThread;
    endShift += shiftsPerThread;

    for (int i = 0; i < 64; i++) {
      DctLoopArgs_ncpu->starti[i] = DctLoopArgs->starti[i];
      DctLoopArgs_ncpu->startj[i] = DctLoopArgs->startj[i];
    }
  }

  for (uint8_t k = 0; k < ncpu; k++) {
    thrd_create(&threads[k], dct_loop_wrapper, &args->MemoryArgs->DctLoopArgs[k]);
  }

  for (uint8_t k = 0; k < ncpu; k++) {
    thrd_join(threads[k], NULL);
  }

  const uint32_t len = args->sizeBlocksWork;
  uint16_t* BF_accumP = args->MemoryArgs->DctLoopArgs[0].BF_accumP;

  // Combine the thread return info.  We just add together the BF_accum from each thread and return the result in BF_accum[0]
  switch (ncpu) {
  case 1:
    break;
  case 2: {
    uint16_t* BF_accumPk1 = args->MemoryArgs->DctLoopArgs[1].BF_accumP;
    copy_add_16to16(BF_accumP, BF_accumPk1, len);
    break;
  }
  case 3: {
    uint16_t* BF_accumPk1 = args->MemoryArgs->DctLoopArgs[1].BF_accumP;
    uint16_t* BF_accumPk2 = args->MemoryArgs->DctLoopArgs[2].BF_accumP;
    copy_add3_16to16(BF_accumP, BF_accumPk1, BF_accumPk2, len);
    break;
  }
  case 4: {
    uint16_t* BF_accumPk1 = args->MemoryArgs->DctLoopArgs[1].BF_accumP;
    uint16_t* BF_accumPk2 = args->MemoryArgs->DctLoopArgs[2].BF_accumP;
    uint16_t* BF_accumPk3 = args->MemoryArgs->DctLoopArgs[3].BF_accumP;
    copy_add4_16to16(BF_accumP, BF_accumPk1, BF_accumPk2, BF_accumPk3, len);
    break;
  }
  }
}

// This is called once per thread by _beginthreadex() above.
// It unpacks the shift information the thread is supposed to process.
// It then does a regular call to doDctLoop() for each shift the thread is supposed to process.
unsigned int  __stdcall
DctLoopThread(DctLoop_args* args) {

  unsigned int i;

  for (i = args->startShift; i <= args->endShift; i++) {
    int starti = args->starti[i];
    int startj = args->startj[i];

    DctLoop(starti, startj, args);
  }

  return(0);
}


