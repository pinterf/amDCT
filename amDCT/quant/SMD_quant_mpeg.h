

#ifndef _SMD_QUANT_MPEG_H_
#define _SMD_QUANT_MPEG_H_

#include "../amDCTPortab.h"  // needed for uint32_t etc.  




static __inline void quant_mpeg_intra_SMD_c(int16_t* coeff,
  const int16_t* data,
  const uint32_t  quantd,
  const uint32_t  mult,
  const uint16_t* imdim);


uint32_t quant_mpeg_inter_SMD_c(int16_t* coeff,
  const int16_t* data,
  const uint32_t  quantd,
  const uint32_t  mult,
  const uint16_t* imdim);


#endif /* _SMD_QUANT_MPEG_H_ */
