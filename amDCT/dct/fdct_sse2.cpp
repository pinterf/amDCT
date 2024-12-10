// Converted to intel intrinsics from fdct_sse2_skal.asm
// which contained both fdct and idct implementations

#include <stdint.h>
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

//; ---------------------------------------------------------------------------- -
//; Forward DCT tables
//; ---------------------------------------------------------------------------- -

// Coefficient tables aligned on 16-byte boundary
alignas(16) static const uint16_t fTab1[] = {
    0x4000, 0x4000, 0x58c5, 0x4b42, 0xdd5d, 0xac61, 0xa73b, 0xcdb7,
    0x4000, 0x4000, 0x3249, 0x11a8, 0x539f, 0x22a3, 0x4b42, 0xee58,
    0x4000, 0xc000, 0x3249, 0xa73b, 0x539f, 0xdd5d, 0x4b42, 0xa73b,
    0xc000, 0x4000, 0x11a8, 0x4b42, 0x22a3, 0xac61, 0x11a8, 0xcdb7
};

alignas(16) static const uint16_t fTab2[] = {
    0x58c5, 0x58c5, 0x7b21, 0x6862, 0xcff5, 0x8c04, 0x84df, 0xba41,
    0x58c5, 0x58c5, 0x45bf, 0x187e, 0x73fc, 0x300b, 0x6862, 0xe782,
    0x58c5, 0xa73b, 0x45bf, 0x84df, 0x73fc, 0xcff5, 0x6862, 0x84df,
    0xa73b, 0x58c5, 0x187e, 0x6862, 0x300b, 0x8c04, 0x187e, 0xba41
};

alignas(16) static const uint16_t fTab3[] = {
    0x539f, 0x539f, 0x73fc, 0x6254, 0xd2bf, 0x92bf, 0x8c04, 0xbe4d,
    0x539f, 0x539f, 0x41b3, 0x1712, 0x6d41, 0x2d41, 0x6254, 0xe8ee,
    0x539f, 0xac61, 0x41b3, 0x8c04, 0x6d41, 0xd2bf, 0x6254, 0x8c04,
    0xac61, 0x539f, 0x1712, 0x6254, 0x2d41, 0x92bf, 0x1712, 0xbe4d
};

alignas(16) static const uint16_t fTab4[] = {
    0x4b42, 0x4b42, 0x6862, 0x587e, 0xd746, 0x9dac, 0x979e, 0xc4df,
    0x4b42, 0x4b42, 0x3b21, 0x14c3, 0x6254, 0x28ba, 0x587e, 0xeb3d,
    0x4b42, 0xb4be, 0x3b21, 0x979e, 0x6254, 0xd746, 0x587e, 0x979e,
    0xb4be, 0x4b42, 0x14c3, 0x587e, 0x28ba, 0x9dac, 0x14c3, 0xc4df
};

alignas(16) static const uint16_t Fdct_Rnd0[] = { 6,8,8,8, 6,8,8,8 };
alignas(16) static const uint16_t Fdct_Rnd1[] = { 8,8,8,8, 8,8,8,8 };
alignas(16) static const uint16_t Fdct_Rnd2[] = { 10,8,8,8, 8,8,8,8 };
alignas(16) static const uint16_t Rounder1[] = { 1,1,1,1, 1,1,1,1 };

alignas(16) static const uint16_t tan1[] = { 0x32ec,0x32ec,0x32ec,0x32ec,0x32ec,0x32ec,0x32ec,0x32ec }; // tan(pi / 16)
alignas(16) static const uint16_t tan2[] = { 0x6a0a,0x6a0a,0x6a0a,0x6a0a,0x6a0a,0x6a0a,0x6a0a,0x6a0a }; // tan(2pi / 16)  (= sqrt(2) - 1)
alignas(16) static const uint16_t tan3[] = { 0xab0e,0xab0e,0xab0e,0xab0e,0xab0e,0xab0e,0xab0e,0xab0e }; // tan(3pi / 16) - 1
alignas(16) static const uint16_t sqrt2[] = { 0x5a82,0x5a82,0x5a82,0x5a82,0x5a82,0x5a82,0x5a82,0x5a82 }; // 0.5 / sqrt(2)

static void mtx_mult(int16_t* input, int offset, const uint16_t* coeffs, const uint16_t* rounder) {
  __m128i xmm0 = _mm_load_si128((__m128i*)(input + offset * 8));
  __m128i xmm1 = _mm_shufflehi_epi16(xmm0, 0b00011011 /* _MM_SHUFFLE(3, 2, 0, 1)*/);
  xmm0 = _mm_shuffle_epi32(xmm0, 0b01000100 /* _MM_SHUFFLE(1, 0, 1, 0)*/);
  xmm1 = _mm_shuffle_epi32(xmm1, 0b11101110 /* _MM_SHUFFLE(3, 3, 2, 2)*/);

  __m128i xmm2 = xmm0;
  xmm0 = _mm_adds_epi16(xmm0, xmm1);
  xmm2 = _mm_subs_epi16(xmm2, xmm1);

  xmm0 = _mm_unpacklo_epi32(xmm0, xmm2);
  xmm2 = _mm_shuffle_epi32(xmm0, 0b01001110 /* _MM_SHUFFLE(2, 3, 0, 1)*/);

  __m128i* coeff_ptr = (__m128i*)coeffs;
  __m128i temp1 = _mm_madd_epi16(xmm2, _mm_load_si128(coeff_ptr + 1));
  __m128i temp2 = _mm_madd_epi16(xmm0, _mm_load_si128(coeff_ptr + 2));
  __m128i temp3 = _mm_madd_epi16(xmm2, _mm_load_si128(coeff_ptr + 3));
  __m128i temp4 = _mm_madd_epi16(xmm0, _mm_load_si128(coeff_ptr));

  xmm0 = _mm_add_epi32(temp4, temp1);
  xmm2 = _mm_add_epi32(temp3, temp2);

  xmm0 = _mm_srai_epi32(xmm0, 16);
  xmm2 = _mm_srai_epi32(xmm2, 16);

  xmm0 = _mm_packs_epi32(xmm0, xmm2);
  xmm0 = _mm_adds_epi16(xmm0, *(__m128i*)rounder);
  xmm0 = _mm_srai_epi16(xmm0, 4);

  _mm_store_si128((__m128i*)(input + offset * 8), xmm0);
}

static void llm_pass(int16_t* data, int shift) {
  __m128i* ptr = (__m128i*)data;

  // Load input data
  __m128i xmm0 = _mm_load_si128(ptr + 0);    // In0
  __m128i xmm2 = _mm_load_si128(ptr + 2);    // In2
  __m128i xmm3 = xmm0;
  __m128i xmm4 = xmm2;
  __m128i xmm7 = _mm_load_si128(ptr + 7);    // In7
  __m128i xmm5 = _mm_load_si128(ptr + 5);    // In5

  // First stage
  xmm0 = _mm_subs_epi16(xmm0, xmm7);         // t7 = In0-In7
  xmm7 = _mm_adds_epi16(xmm7, xmm3);         // t0 = In0+In7
  xmm2 = _mm_subs_epi16(xmm2, xmm5);         // t5 = In2-In5
  xmm5 = _mm_adds_epi16(xmm5, xmm4);         // t2 = In2+In5

  xmm3 = _mm_load_si128(ptr + 3);            // In3
  xmm4 = _mm_load_si128(ptr + 4);            // In4
  __m128i xmm1 = xmm3;
  xmm3 = _mm_subs_epi16(xmm3, xmm4);         // t4 = In3-In4
  xmm4 = _mm_adds_epi16(xmm4, xmm1);         // t3 = In3+In4

  __m128i xmm6 = _mm_load_si128(ptr + 6);            // In6
  xmm1 = _mm_load_si128(ptr + 1);            // In1
  __m128i tmp = xmm1;
  xmm1 = _mm_subs_epi16(xmm1, xmm6);         // t6 = In1-In6
  xmm6 = _mm_adds_epi16(xmm6, tmp);          // t1 = In1+In6

  // Second stage
  __m128i tm03 = _mm_subs_epi16(xmm7, xmm4); // tm03 = t0-t3
  __m128i tm12 = _mm_subs_epi16(xmm6, xmm5); // tm12 = t1-t2
  xmm4 = _mm_adds_epi16(xmm4, xmm4);         // 2.t3
  xmm5 = _mm_adds_epi16(xmm5, xmm5);            // 2.t2

  __m128i tp03 = _mm_adds_epi16(xmm4, tm03); // tp03 = t0+t3
  __m128i tp12 = _mm_adds_epi16(xmm5, tm12); // tp12 = t1+t2

  // Shift operations
  xmm2 = _mm_slli_epi16(xmm2, shift + 1);    // shift t5
  xmm1 = _mm_slli_epi16(xmm1, shift + 1);    // shift t6
  tp03 = _mm_slli_epi16(tp03, shift);        // shift t3
  tp12 = _mm_slli_epi16(tp12, shift);        // shift t2
  tm03 = _mm_slli_epi16(tm03, shift);        // shift t0
  tm12 = _mm_slli_epi16(tm12, shift);        // shift t1
  xmm3 = _mm_slli_epi16(xmm3, shift);        // shift t4
  xmm0 = _mm_slli_epi16(xmm0, shift);        // shift t7

  // Output calculations
  __m128i out4 = _mm_subs_epi16(tp03, tp12); // out4 = tp03-tp12
  __m128i diff = _mm_subs_epi16(xmm1, xmm2); // t6-t5
  tp12 = _mm_adds_epi16(tp12, tp12);
  xmm2 = _mm_adds_epi16(xmm2, xmm2);
  __m128i out0 = _mm_adds_epi16(tp12, out4); // out0 = tp03+tp12

  _mm_store_si128(ptr + 4, out4);            // store out4
  __m128i sum = _mm_adds_epi16(xmm2, diff);  // t6+t5
  _mm_store_si128(ptr + 0, out0);            // store out0

  // Tan2 calculations
  __m128i tan2v = _mm_load_si128((__m128i*)tan2);
  __m128i tmp1 = _mm_mulhi_epi16(tan2v, tm03);
  __m128i out6 = _mm_subs_epi16(tmp1, tm12); // out6 = tm03*tan2 - tm12
  __m128i tmp2 = _mm_mulhi_epi16(tan2v, tm12);
  __m128i out2 = _mm_adds_epi16(tmp2, tm03); // out2 = tm12*tan2 + tm03

  // Sqrt2 and rounder corrections
  __m128i sqrt2v = _mm_load_si128((__m128i*)sqrt2);
  __m128i rounder = _mm_load_si128((__m128i*)Rounder1);

  __m128i tp65 = _mm_mulhi_epi16(sum, sqrt2v);
  out2 = _mm_or_si128(out2, rounder);
  out6 = _mm_or_si128(out6, rounder);
  __m128i tm65 = _mm_mulhi_epi16(diff, sqrt2v);
  tp65 = _mm_or_si128(tp65, rounder);

  _mm_store_si128(ptr + 2, out2);            // store out2
  _mm_store_si128(ptr + 6, out6);            // store out6

  // Final calculations
  __m128i tm465 = _mm_subs_epi16(xmm3, tm65);
  __m128i tm765 = _mm_subs_epi16(xmm0, tp65);
  __m128i tp765 = _mm_adds_epi16(tp65, xmm0);
  __m128i tp465 = _mm_adds_epi16(tm65, xmm3);

  __m128i tan3v = _mm_load_si128((__m128i*)tan3);
  __m128i tan1v = _mm_load_si128((__m128i*)tan1);

  __m128i tmp3 = _mm_mulhi_epi16(tm465, tan3v);
  __m128i tmp4 = _mm_mulhi_epi16(tp465, tan1v);
  tmp3 = _mm_adds_epi16(tmp3, tm465);

  __m128i tmp5 = _mm_mulhi_epi16(tm765, tan3v);
  tmp5 = _mm_adds_epi16(tmp5, tm765);
  __m128i tmp6 = _mm_mulhi_epi16(tp765, tan1v);

  __m128i out1 = _mm_adds_epi16(tmp4, tp765);
  __m128i out3 = _mm_subs_epi16(tm765, tmp3);
  __m128i out5 = _mm_adds_epi16(tm465, tmp5);
  __m128i out7 = _mm_subs_epi16(tmp6, tp465);

  _mm_store_si128(ptr + 1, out1);
  _mm_store_si128(ptr + 3, out3);
  _mm_store_si128(ptr + 5, out5);
  _mm_store_si128(ptr + 7, out7);
}

// fdct_sse2 skal version converted to intel intrinsics
void fdct_sse2(short* const In) {
  llm_pass(In, 3);

  mtx_mult(In, 0, fTab1, Fdct_Rnd0);
  mtx_mult(In, 1, fTab2, Fdct_Rnd2);
  mtx_mult(In, 2, fTab3, Fdct_Rnd1);
  mtx_mult(In, 3, fTab4, Fdct_Rnd1);
  mtx_mult(In, 4, fTab1, Fdct_Rnd0);
  mtx_mult(In, 5, fTab4, Fdct_Rnd1);
  mtx_mult(In, 6, fTab3, Fdct_Rnd1);
  mtx_mult(In, 7, fTab2, Fdct_Rnd1);
}
