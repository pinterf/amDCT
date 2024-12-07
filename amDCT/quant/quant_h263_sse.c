/*****************************************************************************
 *
 *  This program is free software ; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation ; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY ; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program ; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 ****************************************************************************/

#include <stdint.h>

#include <emmintrin.h>
#include <immintrin.h>

#include "quant.h"

// intrinsics rewrite by pinterf

// Based on the logic of XVID MPEG - 4 VIDEO CODEC MPEG4 Quantization H263
// mmx and xmm asm implementations

uint32_t quant_h263_intra_sse2(int16_t* coeff, const int16_t* const data,
  const uint32_t quant, const uint32_t dcscalar,
  const uint16_t* mpeg_matrices) {

  // Load and process DC coefficient
  int32_t dc = (int16_t)data[0];
  int32_t tmp0 = dcscalar >> 1;
  int32_t tmp1 = dc;

  dc = dc + tmp0;
  tmp1 = tmp1 - tmp0;
  if (dc < 0) dc = tmp1;

  int32_t quotient = dc / (int32_t)dcscalar;

  __m128i xmm7 = _mm_set1_epi16((short)((1 << 16) / (quant * 2) + 1));

  if (quant == 1) {
    // Low quantization path
    for (int i = 0; i < 8; i++) {
      __m128i src = _mm_load_si128((__m128i*)(data + i * 8));
      __m128i sign = _mm_cmpgt_epi16(_mm_setzero_si128(), src);
      src = _mm_sub_epi16(src, sign);
      src = _mm_srai_epi16(src, 1);
      _mm_store_si128((__m128i*)(coeff + i * 8), src);
    }
  }
  else {
    // Normal quantization path
    for (int i = 0; i < 8; i++) {
      __m128i src = _mm_load_si128((__m128i*)(data + i * 8));
      __m128i sign = _mm_cmpgt_epi16(_mm_setzero_si128(), src);
      __m128i dst = _mm_mulhi_epi16(src, xmm7);
      dst = _mm_sub_epi16(dst, sign);
      _mm_store_si128((__m128i*)(coeff + i * 8), dst);
    }
  }

  // Store DC coefficient
  coeff[0] = (int16_t)quotient;

  return 0;
}

uint32_t quant_h263_inter_sse2(int16_t* coeff, const int16_t* data, uint32_t quant, const uint16_t* mpeg_matrices) {
  __m128i sum = _mm_setzero_si128();
  __m128i sub = _mm_set1_epi16((short)(quant / 2));

  if (quant == 1) {
    for (int i = 0; i < 8; i += 2) {
      __m128i vals1 = _mm_load_si128((__m128i*)(data + i * 8));
      __m128i vals2 = _mm_load_si128((__m128i*)(data + i * 8 + 8));

      __m128i sign1 = _mm_cmpgt_epi16(_mm_setzero_si128(), vals1);
      __m128i sign2 = _mm_cmpgt_epi16(_mm_setzero_si128(), vals2);

      vals1 = _mm_sub_epi16(_mm_xor_si128(vals1, sign1), sign1);
      vals2 = _mm_sub_epi16(_mm_xor_si128(vals2, sign2), sign2);

      vals1 = _mm_subs_epu16(vals1, sub);
      vals2 = _mm_subs_epu16(vals2, sub);

      vals1 = _mm_srli_epi16(vals1, 1);
      vals2 = _mm_srli_epi16(vals2, 1);

      sum = _mm_add_epi16(sum, _mm_add_epi16(vals1, vals2));

      vals1 = _mm_sub_epi16(_mm_xor_si128(vals1, sign1), sign1);
      vals2 = _mm_sub_epi16(_mm_xor_si128(vals2, sign2), sign2);

      _mm_store_si128((__m128i*)(coeff + i * 8), vals1);
      _mm_store_si128((__m128i*)(coeff + i * 8 + 8), vals2);
    }
  }
  else {
    // in the original code this was a long "divide by 2Q" table
    // which use a shift of 16 to take full advantage of _pmulhw_
    // for q = 1, _pmulhw_ will overflow so it is treated seperately, see above
      __m128i divider = _mm_set1_epi16((short)((1 << 16) / (quant * 2) + 1));

    for (int i = 0; i < 8; i += 2) {
      __m128i vals1 = _mm_load_si128((__m128i*)(data + i * 8));
      __m128i vals2 = _mm_load_si128((__m128i*)(data + i * 8 + 8));

      __m128i sign1 = _mm_cmpgt_epi16(_mm_setzero_si128(), vals1);
      __m128i sign2 = _mm_cmpgt_epi16(_mm_setzero_si128(), vals2);

      vals1 = _mm_sub_epi16(_mm_xor_si128(vals1, sign1), sign1);
      vals2 = _mm_sub_epi16(_mm_xor_si128(vals2, sign2), sign2);

      vals1 = _mm_subs_epu16(vals1, sub);
      vals2 = _mm_subs_epu16(vals2, sub);

      vals1 = _mm_mulhi_epi16(vals1, divider);
      vals2 = _mm_mulhi_epi16(vals2, divider);

      sum = _mm_add_epi16(sum, vals1);
      sum = _mm_add_epi16(sum, vals2);

      vals1 = _mm_sub_epi16(_mm_xor_si128(vals1, sign1), sign1);
      vals2 = _mm_sub_epi16(_mm_xor_si128(vals2, sign2), sign2);

      _mm_store_si128((__m128i*)(coeff + i * 8), vals1);
      _mm_store_si128((__m128i*)(coeff + i * 8 + 8), vals2);
    }
  }

  // Sum all elements
  sum = _mm_madd_epi16(sum, _mm_set1_epi16(1));
  sum = _mm_hadd_epi32(sum, sum); // Horizontal add of adjacent pairs
  sum = _mm_hadd_epi32(sum, sum); // Repeat to add the results

  return _mm_cvtsi128_si32(sum); // Return the lower 32 bits
}


uint32_t dequant_h263_inter_sse2(int16_t* data, const int16_t* const coeff, const uint32_t quant, const uint16_t* mpeg_matrices) {
  
  __m128i xmm6, xmm7, xmm4, xmm5;
  __m128i* pcoeff = (__m128i*)coeff;
  __m128i* pdata = (__m128i*)data;

  // Adjust quant as per the original assembly logic
  uint32_t adjusted_quant = (quant + 1) & 1;

  // Load quantization values
  __m128i quant_val = _mm_set1_epi16((short)quant);
  __m128i quant_double = _mm_add_epi16(quant_val, quant_val);
  __m128i max_val = _mm_set1_epi16(2047);
  __m128i min_val = _mm_set1_epi16(-2048);

  __m128i quant_adj = _mm_sub_epi16(quant_val, _mm_set1_epi16((short)adjusted_quant));

  for (int i = 0; i < 8; ++i) {
    __m128i xmm0, xmm2, xmm3;

    // Load coefficients
    __m128i vals = _mm_load_si128(&pcoeff[i]);

    // Process coefficients
    __m128i sign = _mm_cmpgt_epi16(_mm_setzero_si128(), vals);
    __m128i zero_mask = _mm_cmpeq_epi16(_mm_setzero_si128(), vals);
    vals = _mm_mullo_epi16(vals, quant_double);
    __m128i adj = _mm_andnot_si128(zero_mask, quant_adj);
    vals = _mm_sub_epi16(vals, sign);
    sign = _mm_xor_si128(sign, adj);
    vals = _mm_add_epi16(vals, sign);
    vals = _mm_min_epi16(vals, max_val);
    vals = _mm_max_epi16(vals, min_val);

    // Store results
    _mm_store_si128(&pdata[i], vals);
  }

  return 0;
}


uint32_t dequant_h263_intra_sse2(int16_t* data, const int16_t* coeff, uint32_t quant, uint32_t dcscalar, const uint16_t* mpeg_matrices) {
  __m128i quant_val = _mm_set1_epi16((short)quant);
  __m128i quant_double = _mm_add_epi16(quant_val, quant_val);

  // Calculate quant & 1 mask
  __m128i quant_mask = _mm_set1_epi16(quant & 1 ? 0 : -1);
  __m128i quant_adj = _mm_add_epi16(quant_val, quant_mask);

  __m128i max_val = _mm_set1_epi16(2047);
  __m128i min_val = _mm_set1_epi16(-2048);

  // Process AC coefficients
  for (int i = 0; i < 8; i+=2) {
    // unrolled 2
    __m128i vals1 = _mm_load_si128((__m128i*)(coeff + i * 8));
    __m128i vals2 = _mm_load_si128((__m128i*)(coeff + i * 8 + 8));

    __m128i sign1 = _mm_cmpgt_epi16(_mm_setzero_si128(), vals1);
    __m128i zero1 = _mm_cmpeq_epi16(vals1, _mm_setzero_si128());
    __m128i sign2 = _mm_cmpgt_epi16(_mm_setzero_si128(), vals2);
    __m128i zero2 = _mm_cmpeq_epi16(vals2, _mm_setzero_si128());

    vals1 = _mm_mullo_epi16(vals1, quant_double);
    vals2 = _mm_mullo_epi16(vals2, quant_double);


    vals1 = _mm_sub_epi16(vals1, sign1);
    vals2 = _mm_sub_epi16(vals2, sign2);

    sign1 = _mm_xor_si128(sign1, quant_adj);
    sign2 = _mm_xor_si128(sign2, quant_adj);

    __m128i adj1 = _mm_andnot_si128(zero1, sign1);
    __m128i adj2 = _mm_andnot_si128(zero2, sign2);

    vals1 = _mm_add_epi16(vals1, adj1);
    vals2 = _mm_add_epi16(vals2, adj2);

    vals1 = _mm_min_epi16(vals1, max_val);
    vals2 = _mm_min_epi16(vals2, max_val);
    vals1 = _mm_max_epi16(vals1, min_val);
    vals2 = _mm_max_epi16(vals2, min_val);

    _mm_store_si128((__m128i*)(data + i * 8), vals1);
    _mm_store_si128((__m128i*)(data + i * 8 + 8), vals2);
  }

  // Handle DC coefficient
  int32_t dc = (int32_t)coeff[0] * dcscalar;
  __m128i dc_val = _mm_set1_epi32(dc);
  dc_val = _mm_min_epi16(dc_val, max_val);
  dc_val = _mm_max_epi16(dc_val, min_val);
  data[0] = (int16_t)_mm_cvtsi128_si32(dc_val);

  return 0;
}
