// Converted to intel intrinsics from fdct_sse2_skal.asm
// which contained both fdct and idct implementations

#include <stdint.h>
#include <intrin.h>
#include <emmintrin.h>
#include "fdct.h"

/*
;*
; *  XVID MPEG-4 VIDEO CODEC
; *  - SSE2 forward discrete cosine transform -
; *
; *  Copyright(C) 2003 Pascal Massimino <skal@planet-d.net>
; *
; * $Id: fdct_sse2_skal.asm,v 1.15 2009-09-16 17:07:58 Isibaar Exp $
; *
; ***************************************************************************/
/*
;-----------------------------------------------------------------------------
;
;                          -=FDCT=-
;
; Vertical pass is an implementation of the scheme:
;  Loeffler C., Ligtenberg A., and Moschytz C.S.:
;  Practical Fast 1D DCT Algorithm with Eleven Multiplications,
;  Proc. ICASSP 1989, 988-991.
;
; Horizontal pass is a double 4x4 vector/matrix multiplication,
; (see also Intel's Application Note 922:
;  http://developer.intel.com/vtune/cbts/strmsimd/922down.htm
;  Copyright (C) 1999 Intel Corporation)
;  
; Notes:
;  * tan(3pi/16) is greater than 0.5, and would use the
;    sign bit when turned into 16b fixed-point precision. So,
;    we use the trick: x*tan3 = x*(tan3-1)+x
; 
;  * There's only one SSE-specific instruction (pshufw).
;
;  * There's still 1 or 2 ticks to save in fLLM_PASS, but
;    I prefer having a readable code, instead of a tightly
;    scheduled one...
;
;  * Quantization stage (as well as pre-transposition for the
;    idct way back) can be included in the fTab* constants
;    (with induced loss of precision, somehow)
;
;  * Some more details at: http://skal.planet-d.net/coding/dct.html
;
;-----------------------------------------------------------------------------
;
;                          -=IDCT=-
;
; A little slower than fdct, because the final stages (butterflies and
; descaling) require some unpairable shifting and packing, all on
; the same CPU unit.
;
;-----------------------------------------------------------------------------
*/

alignas(16) static const uint16_t tan1[] = { 0x32ec,0x32ec,0x32ec,0x32ec,0x32ec,0x32ec,0x32ec,0x32ec }; // tan(pi / 16)
alignas(16) static const uint16_t tan2[] = { 0x6a0a,0x6a0a,0x6a0a,0x6a0a,0x6a0a,0x6a0a,0x6a0a,0x6a0a }; // tan(2pi / 16)  (= sqrt(2) - 1)
alignas(16) static const uint16_t tan3[] = { 0xab0e,0xab0e,0xab0e,0xab0e,0xab0e,0xab0e,0xab0e,0xab0e }; // tan(3pi / 16) - 1
alignas(16) static const uint16_t sqrt2[] = { 0x5a82,0x5a82,0x5a82,0x5a82,0x5a82,0x5a82,0x5a82,0x5a82 }; // 0.5 / sqrt(2)

//; ---------------------------------------------------------------------------- -
//; Inverse DCT tables
//; ---------------------------------------------------------------------------- -

alignas(16) static const uint16_t iTab1[] = {
    0x4000, 0x539f, 0x4000, 0x22a3, 0x4000, 0xdd5d, 0x4000, 0xac61,
    0x4000, 0x22a3, 0xc000, 0xac61, 0xc000, 0x539f, 0x4000, 0xdd5d,
    0x58c5, 0x4b42, 0x4b42, 0xee58, 0x3249, 0xa73b, 0x11a8, 0xcdb7,
    0x3249, 0x11a8, 0xa73b, 0xcdb7, 0x11a8, 0x4b42, 0x4b42, 0xa73b
};

alignas(16) static const uint16_t iTab2[] = {
    0x58c5, 0x73fc, 0x58c5, 0x300b, 0x58c5, 0xcff5, 0x58c5, 0x8c04,
    0x58c5, 0x300b, 0xa73b, 0x8c04, 0xa73b, 0x73fc, 0x58c5, 0xcff5,
    0x7b21, 0x6862, 0x6862, 0xe782, 0x45bf, 0x84df, 0x187e, 0xba41,
    0x45bf, 0x187e, 0x84df, 0xba41, 0x187e, 0x6862, 0x6862, 0x84df
};

alignas(16) static const uint16_t iTab3[] = {
    0x539f, 0x6d41, 0x539f, 0x2d41, 0x539f, 0xd2bf, 0x539f, 0x92bf,
    0x539f, 0x2d41, 0xac61, 0x92bf, 0xac61, 0x6d41, 0x539f, 0xd2bf,
    0x73fc, 0x6254, 0x6254, 0xe8ee, 0x41b3, 0x8c04, 0x1712, 0xbe4d,
    0x41b3, 0x1712, 0x8c04, 0xbe4d, 0x1712, 0x6254, 0x6254, 0x8c04
};

alignas(16) static const uint16_t iTab4[] = {
    0x4b42, 0x6254, 0x4b42, 0x28ba, 0x4b42, 0xd746, 0x4b42, 0x9dac,
    0x4b42, 0x28ba, 0xb4be, 0x9dac, 0xb4be, 0x6254, 0x4b42, 0xd746,
    0x6862, 0x587e, 0x587e, 0xeb3d, 0x3b21, 0x979e, 0x14c3, 0xc4df,
    0x3b21, 0x14c3, 0x979e, 0xc4df, 0x14c3, 0x587e, 0x587e, 0x979e
};

alignas(16) static const uint32_t Walken_Idct_Rounders[] = {
    65536, 65536, 65536, 65536,
    3597,  3597,  3597,  3597,
    2260,  2260,  2260,  2260,
    1203,  1203,  1203,  1203,
    0,     0,     0,     0,
    120,   120,   120,   120,
    512,   512,   512,   512,
    512,   512,   512,   512
};

alignas(16) const uint16_t Walken_Idct_Rounders_dw[] = {
    65536 >> 11, 65536 >> 11, 65536 >> 11, 65536 >> 11, 65536 >> 11, 65536 >> 11, 65536 >> 11, 65536 >> 11,
    3597 >> 11,  3597 >> 11,  3597 >> 11,  3597 >> 11,  3597 >> 11,  3597 >> 11,  3597 >> 11,  3597 >> 11,
    2260 >> 11,  2260 >> 11,  2260 >> 11,  2260 >> 11,  2260 >> 11,  2260 >> 11,  2260 >> 11,  2260 >> 11,
    // other rounders are zero...
};

/*
; ---------------------------------------------------------------------------- -
; Helper macro iMTX_MULT
; ---------------------------------------------------------------------------- -
*/
static inline void iMTX_MULT(int row_idx, int16_t* src_dst, const uint16_t* table, const uint32_t* rounder, int shift) {
  // Load source data and perform initial shuffling
  __m128i xmm0 = _mm_load_si128((__m128i*)(src_dst + row_idx * 8));

  // Original binary pattern: 11011000 (0xD8)
  // For low/high words: [3 1 2 0] in destination positions
  xmm0 = _mm_shufflelo_epi16(xmm0, 0xD8); // [02134567]
  xmm0 = _mm_shufflehi_epi16(xmm0, 0xD8); // [02134657]

  // _MM_SHUFFLE is correct here since pshufd works differently
  __m128i xmm4 = _mm_shuffle_epi32(xmm0, _MM_SHUFFLE(0, 0, 0, 0)); // [02020202]
  __m128i xmm5 = _mm_shuffle_epi32(xmm0, _MM_SHUFFLE(2, 2, 2, 2)); // [46464646]
  __m128i xmm6 = _mm_shuffle_epi32(xmm0, _MM_SHUFFLE(1, 1, 1, 1)); // [13131313]
  __m128i xmm7 = _mm_shuffle_epi32(xmm0, _MM_SHUFFLE(3, 3, 3, 3)); // [57575757]

  // Perform dot products with matrix coefficients
  xmm4 = _mm_madd_epi16(xmm4, _mm_load_si128((__m128i*)(table + 0*8))); // dot [M00,M01][M04,M05][M08,M09][M12,M13]
  xmm5 = _mm_madd_epi16(xmm5, _mm_load_si128((__m128i*)(table + 1*8))); // dot [M02,M03][M06,M07][M10,M11][M14,M15]
  xmm6 = _mm_madd_epi16(xmm6, _mm_load_si128((__m128i*)(table + 2*8))); // dot [M16,M17][M20,M21][M24,M25][M28,M29]
  xmm7 = _mm_madd_epi16(xmm7, _mm_load_si128((__m128i*)(table + 3*8))); // dot [M18,M19][M22,M23][M26,M27][M30,M31]

  xmm4 = _mm_add_epi32(xmm4, _mm_load_si128((__m128i*)rounder));

  __m128i b = _mm_add_epi32(xmm6, xmm7);     // [b0|b1|b2|b3]
  __m128i a = _mm_add_epi32(xmm4, xmm5);     // [a0|a1|a2|a3]

  __m128i sum = _mm_add_epi32(a, b);         // a+b
  __m128i diff = _mm_sub_epi32(a, b);        // a-b

  sum = _mm_srai_epi32(sum, shift);
  diff = _mm_srai_epi32(diff, shift);

  __m128i result = _mm_packs_epi32(sum, diff); // [01237654]

  // Binary 00011011 = 0x1B
  result = _mm_shufflehi_epi16(result, 0x1B); // [01234567]

  _mm_store_si128((__m128i*)(src_dst + row_idx * 8), result);
}

static inline void illm_pass(short* src_dst) {
  __m128i xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7;

  // Load constant tables (assumed to be already defined)
  const __m128i* tan1_v = (const __m128i*)tan1;
  const __m128i* tan2_v = (const __m128i*)tan2;
  const __m128i* tan3_v = (const __m128i*)tan3;
  const __m128i* sqrt2_v = (const __m128i*)sqrt2;

  // First stage: handle x1, x3, x5, x7
  xmm0 = _mm_load_si128(tan3_v);           // t3-1
  xmm3 = _mm_load_si128((__m128i*)(src_dst + 24)); // x3
  xmm1 = xmm0;                             // t3-1
  xmm5 = _mm_load_si128((__m128i*)(src_dst + 40)); // x5
  xmm4 = _mm_load_si128(tan1_v);          // t1
  xmm6 = _mm_load_si128((__m128i*)(src_dst + 8));  // x1
  xmm7 = _mm_load_si128((__m128i*)(src_dst + 56)); // x7
  xmm2 = xmm4;                             // t1

  // Calculate tm35 and tp35
  xmm0 = _mm_mulhi_epi16(xmm0, xmm3);     // x3*(t3-1)
  xmm1 = _mm_mulhi_epi16(xmm1, xmm5);     // x5*(t3-1)
  xmm0 = _mm_adds_epi16(xmm0, xmm3);      // x3*t3
  xmm1 = _mm_adds_epi16(xmm1, xmm5);      // x5*t3
  xmm0 = _mm_subs_epi16(xmm0, xmm5);      // x3*t3-x5 = tm35
  xmm1 = _mm_adds_epi16(xmm1, xmm3);      // x3+x5*t3 = tp35

  // Calculate tp17 and tm17
  xmm4 = _mm_mulhi_epi16(xmm4, xmm7);     // x7*t1
  xmm2 = _mm_mulhi_epi16(xmm2, xmm6);     // x1*t1
  xmm4 = _mm_adds_epi16(xmm4, xmm6);      // x1+t1*x7 = tp17
  xmm2 = _mm_subs_epi16(xmm2, xmm7);      // x1*t1-x7 = tm17

  // Butterfly operations
  xmm3 = _mm_load_si128(sqrt2_v);
  xmm7 = xmm4;
  xmm6 = xmm2;
  xmm4 = _mm_subs_epi16(xmm4, xmm1);      // tp17-tp35 = t1
  xmm2 = _mm_subs_epi16(xmm2, xmm0);      // tm17-tm35 = b3
  xmm1 = _mm_adds_epi16(xmm1, xmm7);      // tp17+tp35 = b0
  xmm0 = _mm_adds_epi16(xmm0, xmm6);      // tm17+tm35 = t2

  // More butterfly operations
  xmm6 = xmm4;
  xmm4 = _mm_subs_epi16(xmm4, xmm0);      // t1-t2
  xmm0 = _mm_adds_epi16(xmm0, xmm6);      // t1+t2
  xmm4 = _mm_mulhi_epi16(xmm4, xmm3);     // (t1-t2)/(2.sqrt2)
  xmm0 = _mm_mulhi_epi16(xmm0, xmm3);     // (t1+t2)/(2.sqrt2)
  xmm0 = _mm_adds_epi16(xmm0, xmm0);      // 2.(t1+t2) = b1
  xmm4 = _mm_adds_epi16(xmm4, xmm4);      // 2.(t1-t2) = b2

  // Second stage: handle x0, x2, x4, x6
  xmm7 = _mm_load_si128(tan2_v);          // t2
  xmm3 = _mm_load_si128((__m128i*)(src_dst + 16)); // x2
  xmm6 = _mm_load_si128((__m128i*)(src_dst + 48)); // x6
  xmm5 = xmm7;                            // t2

  // Calculate tp26 and tm26
  xmm7 = _mm_mulhi_epi16(xmm7, xmm6);     // x6*t2
  xmm5 = _mm_mulhi_epi16(xmm5, xmm3);     // x2*t2
  xmm7 = _mm_adds_epi16(xmm7, xmm3);      // x2+x6*t2 = tp26
  xmm5 = _mm_subs_epi16(xmm5, xmm6);      // x2*t2-x6 = tm26

  // Handle x0 and x4
  xmm3 = _mm_load_si128((__m128i*)(src_dst + 0));  // x0
  xmm6 = _mm_load_si128((__m128i*)(src_dst + 32)); // x4

  // Store b3 temporarily
  _mm_store_si128((__m128i*)src_dst, xmm2);

  xmm2 = xmm3;
  xmm3 = _mm_subs_epi16(xmm3, xmm6);      // x0-x4 = tm04
  xmm6 = _mm_adds_epi16(xmm6, xmm2);      // x0+x4 = tp04

  // Final butterfly operations
  xmm2 = xmm6;
  xmm6 = _mm_subs_epi16(xmm6, xmm7);
  xmm7 = _mm_adds_epi16(xmm7, xmm2);
  xmm2 = xmm3;
  xmm3 = _mm_subs_epi16(xmm3, xmm5);
  xmm5 = _mm_adds_epi16(xmm5, xmm2);
  xmm2 = xmm5;
  xmm5 = _mm_subs_epi16(xmm5, xmm0);
  xmm0 = _mm_adds_epi16(xmm0, xmm2);
  xmm2 = xmm3;
  xmm3 = _mm_subs_epi16(xmm3, xmm4);
  xmm4 = _mm_adds_epi16(xmm4, xmm2);

  // Reload b3
  xmm2 = _mm_load_si128((__m128i*)src_dst);

  // Shift and store outputs 6,5,1,2
  xmm5 = _mm_srai_epi16(xmm5, 6);         // out6
  xmm3 = _mm_srai_epi16(xmm3, 6);         // out5
  xmm0 = _mm_srai_epi16(xmm0, 6);         // out1
  xmm4 = _mm_srai_epi16(xmm4, 6);         // out2

  _mm_store_si128((__m128i*)(src_dst + 48), xmm5); // out6
  _mm_store_si128((__m128i*)(src_dst + 40), xmm3); // out5
  _mm_store_si128((__m128i*)(src_dst + 8), xmm0);  // out1
  _mm_store_si128((__m128i*)(src_dst + 16), xmm4); // out2

  // Final calculations and stores for outputs 0,7,3,4
  xmm0 = xmm7;
  xmm4 = xmm6;
  xmm7 = _mm_subs_epi16(xmm7, xmm1);      // a0-b0
  xmm6 = _mm_subs_epi16(xmm6, xmm2);      // a3-b3
  xmm1 = _mm_adds_epi16(xmm1, xmm0);      // a0+b0
  xmm2 = _mm_adds_epi16(xmm2, xmm4);      // a3+b3

  xmm1 = _mm_srai_epi16(xmm1, 6);         // out0
  xmm7 = _mm_srai_epi16(xmm7, 6);         // out7
  xmm2 = _mm_srai_epi16(xmm2, 6);         // out3
  xmm6 = _mm_srai_epi16(xmm6, 6);         // out4

  _mm_store_si128((__m128i*)(src_dst + 0), xmm1);  // out0
  _mm_store_si128((__m128i*)(src_dst + 24), xmm2); // out3
  _mm_store_si128((__m128i*)(src_dst + 32), xmm6); // out4
  _mm_store_si128((__m128i*)(src_dst + 56), xmm7); // out7
}

// Helper macro/inline function for row testing
static inline int test_row(const short* row) {
  __m128i v = _mm_loadu_si128((__m128i*)row);
  return _mm_movemask_epi8(_mm_cmpeq_epi16(v, _mm_setzero_si128())) != 0xFFFF;
}

/*
; ---------------------------------------------------------------------------- -
; Function idct(this one skips null rows)
; ---------------------------------------------------------------------------- -
; IEEE1180 and Walken compatible version
*/
// Main IDCT function using SSE2

void idct_sse2(short* const src) {
  // Row 0
  if (test_row(src)) {
    iMTX_MULT(0, src, iTab1, &Walken_Idct_Rounders[0 * 4], 11);
  }
  else {
    __m128i rounder = _mm_load_si128((__m128i*) & Walken_Idct_Rounders_dw[0 * 8]);
    _mm_store_si128((__m128i*)src, rounder);
  }

  // Row 1
  if (test_row(src + 8)) {
    iMTX_MULT(1, src, iTab2, &Walken_Idct_Rounders[1 * 4], 11);
  }
  else {
    __m128i rounder = _mm_load_si128((__m128i*) & Walken_Idct_Rounders_dw[1 * 8]);
    _mm_store_si128((__m128i*)(src + 8), rounder);
  }

  // Row 2
  if (test_row(src + 16)) {
    iMTX_MULT(2, src, iTab3, &Walken_Idct_Rounders[2 * 4], 11);
  }
  else {
    __m128i rounder = _mm_load_si128((__m128i*) & Walken_Idct_Rounders_dw[2 * 8]);
    _mm_store_si128((__m128i*)(src + 16), rounder);
  }

  // Row 3
  if (test_row(src + 24)) {
    iMTX_MULT(3, src, iTab4, &Walken_Idct_Rounders[3 * 4], 11);
  }

  // Row 4
  if (test_row(src + 32)) {
    iMTX_MULT(4, src, iTab1, &Walken_Idct_Rounders[4 * 4], 11);
  }

  // Row 5
  if (test_row(src + 40)) {
    iMTX_MULT(5, src, iTab4, &Walken_Idct_Rounders[5 * 4], 11);
  }

  // Row 6
  if (test_row(src + 48)) {
    iMTX_MULT(6, src, iTab3, &Walken_Idct_Rounders[6 * 4], 11);
  }

  // Row 7
  if (test_row(src + 56)) {
    iMTX_MULT(7, src, iTab2, &Walken_Idct_Rounders[7 * 4], 11);
  }

  // Final LLM pass
  illm_pass(src);
}
