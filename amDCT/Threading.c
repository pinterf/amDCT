

#include <windows.h>  // Required for threading.
#include <process.h>

#include "amDCT.h"
#include "Threading.h"
#include "DctLoop.h"
#include "transfer_add.h"




// This collects all of the shift information.
void
setShift(uint8_t        starti,
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
void
startDctLoop(FrameInfo_args* args) {  //   DctLoop_arr    *DctLoopArr   =  (FrameInfoArgs->DctLoopArr[0]);


  __declspec(align(16))  uint16_t* BF_accumP;
  __declspec(align(16))  uint16_t* BF_accumPk1;
  __declspec(align(16))  uint16_t* BF_accumPk2;
  __declspec(align(16))  uint16_t* BF_accumPk3;

  __declspec(align(16))  DctLoop_args* DctLoopArgs = &args->MemoryArgs->DctLoopArgs[0];
  __declspec(align(16))   DctLoop_args* DctLoopArgs_ncpu = &args->MemoryArgs->DctLoopArgs[0];


  HANDLE        hThread[MAX_CPU];
  unsigned int  dwThreadId[MAX_CPU];

  uint32_t i, len;
  uint8_t  k;
  uint8_t  shiftsPerThread;
  uint8_t  endShift;
  uint8_t  startShift = 0;
  uint8_t  ncpu = DctLoopArgs->ncpu;
  //  uint32_t sizeY          = args->sizeY;          //DctLoopArgs->src_height * DctLoopArgs->src_width;
  //  uint32_t sizeBlocksWork = args->sizeBlocksWork;

  for (k = 0; k < MAX_CPU; k++) {
    hThread[k] = 0;
    dwThreadId[k] = 0;
  }


  // This provides the first approximation of the number of shiftsPerThread
  shiftsPerThread = (uint8_t)(DctLoopArgs->cntShift / ncpu);


  // NOTE: This sets the shiftsPerThread so that the last thread
  // will be within + or - 1 of the total threads to do.
  // The last iteration of the for (k = 0; k < ncpu; k++) loop below
  // will handle the + or - 1.
  if (ncpu == 3) {
    switch (DctLoopArgs->cntShift) {
    case 4:   // 4 shifts
      shiftsPerThread = 1;  // shifts 1, 1, 2
      break;

    case 8:   // 8 shifts
      shiftsPerThread = 3;  // shifts 3, 3, 2
      break;

    case 16:   // 16 shifts
      shiftsPerThread = 5;  // shifts 5, 5, 6
      break;

    case 32:   // 32 shifts
      shiftsPerThread = 11;  // shifts 11, 11, 10
      break;

    case 64:   // 64 shifts
      shiftsPerThread = 21;  // shifts 21, 21, 22
      break;
    }
  }



  startShift = 0;
  endShift = shiftsPerThread - 1;

  // Store all of the thread specific information.
  for (k = 0; k < ncpu; k++) {
    DctLoopArgs_ncpu = &args->MemoryArgs->DctLoopArgs[k];
    DctLoopArgs_ncpu->threadNum = k;
    DctLoopArgs_ncpu->startShift = startShift;
    DctLoopArgs_ncpu->endShift = endShift;

    // The 2 ifs handle the cases where the number of threads (ncpu) does not divide evenly into the number of shifts.
    // This only happens if ncpu == 3
    if (k == ncpu - 1)                    DctLoopArgs_ncpu->endShift = DctLoopArgs->cntShift - 1;
    if (startShift > (DctLoopArgs->cntShift - 1)) DctLoopArgs_ncpu->startShift = DctLoopArgs->cntShift - 1;

    startShift += shiftsPerThread;
    endShift += shiftsPerThread;

    for (i = 0; i < 64; i++) {
      DctLoopArgs_ncpu->starti[i] = DctLoopArgs->starti[i];
      DctLoopArgs_ncpu->startj[i] = DctLoopArgs->startj[i];
    }
  }

  for (k = 0; k < ncpu; k++) {
    hThread[k] = (HANDLE)_beginthreadex(NULL, 2000, DctLoopThread, (void*)&(args->MemoryArgs->DctLoopArgs[k]), 0, &dwThreadId[k]);
  }


  // And wait for them to finish
  WaitForMultipleObjects(ncpu, hThread, TRUE, INFINITE);

  // Close all thread handles upon completion.
  for (k = 0; k < ncpu; k++) {
    if (hThread[k] != 0) CloseHandle(hThread[k]);
  }

  // see if this helps
  for (k = 0; k < MAX_CPU; k++) {
    hThread[k] = 0;
    dwThreadId[k] = 0;
  }


  // Combine the thread return info.  We just add together the BF_accum from each thread and return the result in BF_accum[0]
  switch (ncpu) {
  case 1:
    break;

  case 2:
    len = args->sizeBlocksWork;

    BF_accumP = args->MemoryArgs->DctLoopArgs[0].BF_accumP;
    BF_accumPk1 = args->MemoryArgs->DctLoopArgs[1].BF_accumP;


    copy_add_16to16_xmm(BF_accumP, BF_accumPk1, len);

    //test_copy_add_16to16_c(BF_accumP, BF_accumPk1, len);
//    test2_copy_add_16to16_c(BF_accumP, BF_accumPk1, len);

//void  copy_16to16_clpsrc_c(uint16_t *dst, int16_t *src, uint32_t len) {

//    copy_16to16_clpsrc_c(uint16_t *dst, int16_t *src, uint32_t len)

    //copy_16to16_clpsrc_c(BF_accumP, BF_accumPk1, len);
    break;

  case 3:
    len = args->sizeBlocksWork;

    BF_accumP = args->MemoryArgs->DctLoopArgs[0].BF_accumP;
    BF_accumPk1 = args->MemoryArgs->DctLoopArgs[1].BF_accumP;
    BF_accumPk2 = args->MemoryArgs->DctLoopArgs[2].BF_accumP;
    copy_add3_16to16_xmm(BF_accumP, BF_accumPk1, BF_accumPk2, len);
    break;

  case 4:
    len = args->sizeBlocksWork;

    BF_accumP = args->MemoryArgs->DctLoopArgs[0].BF_accumP;
    BF_accumPk1 = args->MemoryArgs->DctLoopArgs[1].BF_accumP;
    BF_accumPk2 = args->MemoryArgs->DctLoopArgs[2].BF_accumP;
    BF_accumPk3 = args->MemoryArgs->DctLoopArgs[3].BF_accumP;
    //copy_add4_16to16_xmm(BF_accumP, BF_accumPk1, BF_accumPk2, BF_accumPk3, len);
    copy_add4_16to16_c(BF_accumP, BF_accumPk1, BF_accumPk2, BF_accumPk3, len);
    break;
  }

  return;
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


