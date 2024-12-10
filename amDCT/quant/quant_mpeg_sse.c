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

#include "../avs/config.h"
#include "quant.h"
#include "quant_matrix.h"

#include <immintrin.h>
#include <emmintrin.h>
#include <smmintrin.h>

// intrinsics rewrite by pinterf

// dequant_mpeg_inter_sse41
// quant_mpeg_inter_ssse3
// not ported: dequant_mpeg_intra_mmx and quant_mpeg_intra_mmx: a directly written quantDequant is used instead

// let's be safe, using 32 bit intermediates, though the original asm is using 16 bit
// based on C version
#if defined(GCC) || defined(CLANG)
__attribute__((__target__("sse4.1")))
#endif
uint32_t dequant_mpeg_inter_sse41(int16_t* data,
  const int16_t* coeff,
  const uint32_t quant,
  const uint16_t* mpeg_quant_matrices)
{
  const uint16_t* inter_matrix = get_inter_matrix(mpeg_quant_matrices);
  __m128i sum_vec = _mm_setzero_si128();
  __m128i quant_vec = _mm_set1_epi32(quant);

  // Process 8 elements (16 bytes) at a time
  for (int i = 0; i < 64; i += 8) {
    // Load 8 coefficients and matrix values (16 bytes each)
    __m128i coeff_vec = _mm_load_si128((__m128i*)(coeff + i));
    __m128i matrix_vec = _mm_load_si128((__m128i*)(inter_matrix + i));

    // Split into low and high parts for 32-bit processing
    __m128i abs_coeff = _mm_abs_epi16(coeff_vec);
    __m128i signs = _mm_srai_epi16(coeff_vec, 15);

    // Multiply by 2 and add 1: (2 * level + 1)
    __m128i level = _mm_add_epi16(_mm_slli_epi16(abs_coeff, 1), _mm_set1_epi16(1));

    // Process low 4 elements
    __m128i level_low = _mm_unpacklo_epi16(level, _mm_setzero_si128());
    __m128i matrix_low = _mm_unpacklo_epi16(matrix_vec, _mm_setzero_si128());
    __m128i mul_low = _mm_mullo_epi32(level_low, matrix_low);
    mul_low = _mm_mullo_epi32(mul_low, quant_vec);
    mul_low = _mm_srai_epi32(mul_low, 4);

    // Process high 4 elements
    __m128i level_high = _mm_unpackhi_epi16(level, _mm_setzero_si128());
    __m128i matrix_high = _mm_unpackhi_epi16(matrix_vec, _mm_setzero_si128());
    __m128i mul_high = _mm_mullo_epi32(level_high, matrix_high);
    mul_high = _mm_mullo_epi32(mul_high, quant_vec);
    mul_high = _mm_srai_epi32(mul_high, 4);

    // Pack back to 16-bit with saturation
    __m128i result = _mm_packs_epi32(mul_low, mul_high);

    // Apply signs
    result = _mm_xor_si128(result, signs);
    result = _mm_sub_epi16(result, signs);

    // Zero out results where coefficients are zero
    __m128i zero_mask = _mm_cmpeq_epi16(coeff_vec, _mm_setzero_si128());
    result = _mm_andnot_si128(zero_mask, result);

    // Store 8 results (16 bytes)
    _mm_store_si128((__m128i*)(data + i), result);

    // Update sum for mismatch control
    sum_vec = _mm_xor_si128(sum_vec, result);
  }

  // Horizontal XOR for mismatch control
  __m128i sum_16 = _mm_xor_si128(sum_vec, _mm_srli_si128(sum_vec, 8));
  sum_16 = _mm_xor_si128(sum_16, _mm_srli_si128(sum_16, 4));
  sum_16 = _mm_xor_si128(sum_16, _mm_srli_si128(sum_16, 2));
  int sum = _mm_extract_epi16(sum_16, 0);

  // Mismatch control
  if ((sum & 1) == 0) {
    data[63] ^= 1;
  }

  return 0;
}

// from the original quantize_mpeg_mmx.asm block
// "Divide by 2Q table" uint16_t mmx_div[31*4] was eliminated
// mmx_one was eliminated

// Logic based on quant_mpeg_inter_mmx version from quantize_mpeg_mmx.asm
// Original quant_mpeg_inter special cases for quant=1 or quant=2 are implemented in distinct functions.

#if defined(GCC) || defined(CLANG)
__attribute__((__target__("ssse3")))
#endif
static uint32_t quant_mpeg_inter_notq1or2_ssse3(int16_t* coeff, const int16_t* data, const uint32_t quant, const uint16_t* mpeg_matrices) {
  __m128i xmm_one = _mm_set1_epi16(1);
  __m128i sum = _mm_setzero_si128();
  //__m128i divider = _mm_loadl_epi64((__m128i*)(mmx_div + quant * 4 - 4)); // load the divider from mmx_div from old 64 bit asm
  __m128i divider = _mm_set1_epi16((short)((1 << 17) / (quant * 2) + 1)); // Calculate divider dynamically

  for (int ecx = 0; ecx < 8; ecx++) {
    __m128i mm0 = _mm_load_si128((__m128i*)(data + 8 * ecx)); // 8 pixels at a time, a whole block
    __m128i mm1 = _mm_setzero_si128();
    __m128i mm2 = _mm_setzero_si128();

    mm1 = _mm_cmpgt_epi16(mm1, mm0);
    mm0 = _mm_xor_si128(mm0, mm1);
    mm0 = _mm_sub_epi16(mm0, mm1);
    mm0 = _mm_slli_epi16(mm0, 4);

    mm2 = _mm_loadu_si128((__m128i*)(mpeg_matrices + 256 + 8 * ecx));
    mm2 = _mm_srli_epi16(mm2, 1);
    mm0 = _mm_add_epi16(mm0, mm2);
    mm2 = _mm_loadu_si128((__m128i*)(mpeg_matrices + 384 + 8 * ecx)); // points to a sub-table in mpeg_matrices
    mm0 = _mm_mulhi_epi16(mm0, mm2);

    mm0 = _mm_mulhi_epi16(mm0, divider);
    mm0 = _mm_srli_epi16(mm0, 1);

    sum = _mm_add_epi16(sum, mm0);
    mm0 = _mm_xor_si128(mm0, mm1);
    mm0 = _mm_sub_epi16(mm0, mm1);

    _mm_store_si128((__m128i*)(coeff + 8 * ecx), mm0);
  }

  sum = _mm_madd_epi16(sum, xmm_one);
  sum = _mm_hadd_epi32(sum, sum); // Horizontal add of adjacent pairs
  sum = _mm_hadd_epi32(sum, sum); // Repeat to add the results

  return _mm_cvtsi128_si32(sum); // Return the lower 32 bits
}


#if defined(GCC) || defined(CLANG)
__attribute__((__target__("ssse3")))
#endif
static uint32_t quant_mpeg_inter_q1_ssse3(int16_t* coeff, const int16_t* data, const uint32_t quant, const uint16_t* mpeg_matrices) {
  // quant is not used intentionally, here fixed 1
  __m128i xmm_one = _mm_set1_epi16(1);
  __m128i sum = _mm_setzero_si128();

  for (int ecx = 0; ecx < 8; ecx++) {
    __m128i mm0 = _mm_load_si128((__m128i*)(data + 8 * ecx)); // Load 128-bit data
    __m128i mm1 = _mm_setzero_si128();
    __m128i mm2 = _mm_setzero_si128();

    mm1 = _mm_cmpgt_epi16(mm1, mm0);
    mm0 = _mm_xor_si128(mm0, mm1);
    mm0 = _mm_sub_epi16(mm0, mm1);
    mm0 = _mm_slli_epi16(mm0, 4);

    mm2 = _mm_loadu_si128((__m128i*)(mpeg_matrices + 256 + 8 * ecx)); // Load 128-bit data
    mm2 = _mm_srli_epi16(mm2, 1);
    mm0 = _mm_add_epi16(mm0, mm2);
    mm2 = _mm_loadu_si128((__m128i*)(mpeg_matrices + 384 + 8 * ecx)); // Load next 128-bit data
    mm0 = _mm_mulhi_epi16(mm0, mm2);

    mm0 = _mm_srli_epi16(mm0, 1); // mm0 >>= 1 (/2)

    sum = _mm_add_epi16(sum, mm0);
    mm0 = _mm_xor_si128(mm0, mm1);
    mm0 = _mm_sub_epi16(mm0, mm1);

    _mm_store_si128((__m128i*)(coeff + 8 * ecx), mm0); // Store 128-bit data
  }

  sum = _mm_madd_epi16(sum, xmm_one);
  sum = _mm_hadd_epi32(sum, sum); // Horizontal add of adjacent pairs
  sum = _mm_hadd_epi32(sum, sum); // Repeat to add the results

  return _mm_cvtsi128_si32(sum); // Return the lower 32 bits
}

#if defined(GCC) || defined(CLANG)
__attribute__((__target__("ssse3")))
#endif
static uint32_t quant_mpeg_inter_q2_ssse3(int16_t* coeff, const int16_t* data, const uint32_t quant, const uint16_t* mpeg_matrices) {
  // quant is not used intentionally, here fixed 2
  __m128i xmm_one = _mm_set1_epi16(1);
  __m128i sum = _mm_setzero_si128();

  for (int ecx = 0; ecx < 8; ecx++) {
    __m128i mm0 = _mm_load_si128((__m128i*)(data + 8 * ecx)); // Load 128-bit data
    __m128i mm1 = _mm_setzero_si128();
    __m128i mm2 = _mm_setzero_si128();

    mm1 = _mm_cmpgt_epi16(mm1, mm0);
    mm0 = _mm_xor_si128(mm0, mm1);
    mm0 = _mm_sub_epi16(mm0, mm1);
    mm0 = _mm_slli_epi16(mm0, 4);

    mm2 = _mm_loadu_si128((__m128i*)(mpeg_matrices + 256 + 8 * ecx)); // Load 128-bit data
    mm2 = _mm_srli_epi16(mm2, 1);
    mm0 = _mm_add_epi16(mm0, mm2);
    mm2 = _mm_loadu_si128((__m128i*)(mpeg_matrices + 384 + 8 * ecx)); // Load next 128-bit data
    mm0 = _mm_mulhi_epi16(mm0, mm2);

    mm0 = _mm_srli_epi16(mm0, 2); // mm0 >>= 2 (/4)

    sum = _mm_add_epi16(sum, mm0);
    mm0 = _mm_xor_si128(mm0, mm1);
    mm0 = _mm_sub_epi16(mm0, mm1);

    _mm_store_si128((__m128i*)(coeff + 8 * ecx), mm0); // Store 128-bit data
  }

  sum = _mm_madd_epi16(sum, xmm_one);
  sum = _mm_hadd_epi32(sum, sum); // Horizontal add of adjacent pairs
  sum = _mm_hadd_epi32(sum, sum); // Repeat to add the results

  return _mm_cvtsi128_si32(sum); // Return the lower 32 bits
}


uint32_t quant_mpeg_inter_ssse3(int16_t* coeff,
  const int16_t* data,
  const uint32_t quant,
  const uint16_t* mpeg_quant_matrices)
{
  if (quant == 1)
    return quant_mpeg_inter_q1_ssse3(coeff, data, quant, mpeg_quant_matrices);
  else if (quant == 2)
    return quant_mpeg_inter_q2_ssse3(coeff, data, quant, mpeg_quant_matrices);
  else
    return quant_mpeg_inter_notq1or2_ssse3(coeff, data, quant, mpeg_quant_matrices);
}

#if 0
// this one utilize 64 bit inside, mimics the original asm code
uint32_t quant_mpeg_inter_notq1or2_sse2(int16_t* coeff, const int16_t* data, const uint32_t quant, const uint16_t* mpeg_matrices) {
  __m128i xmm_one = _mm_set1_epi16(1);
  __m128i sum = _mm_setzero_si128();
  //__m128i divider = _mm_loadl_epi64((__m128i*)(mmx_div + quant * 4 - 4)); // load the divider from mmx_div from old 64 bit asm
  __m128i divider = _mm_set1_epi16((1 << 17) / (quant * 2) + 1); // Calculate divider dynamically

  for (int ecx = 0; ecx < 16; ecx += 2) {
    __m128i mm0 = _mm_loadl_epi64((__m128i*)(data + 4 * ecx)); // Load 64-bit data
    __m128i mm3 = _mm_loadl_epi64((__m128i*)(data + 4 * ecx + 4)); // Load next 64-bit data
    __m128i mm1 = _mm_setzero_si128();
    __m128i mm4 = _mm_setzero_si128();

    mm1 = _mm_cmpgt_epi16(mm1, mm0);
    mm4 = _mm_cmpgt_epi16(mm4, mm3);
    mm0 = _mm_xor_si128(mm0, mm1);
    mm3 = _mm_xor_si128(mm3, mm4);
    mm0 = _mm_sub_epi16(mm0, mm1);
    mm3 = _mm_sub_epi16(mm3, mm4);
    mm0 = _mm_slli_epi16(mm0, 4);
    mm3 = _mm_slli_epi16(mm3, 4);

    __m128i mm2 = _mm_loadl_epi64((__m128i*)(mpeg_matrices + 256 + 4 * ecx)); // Load 64-bit data
    mm2 = _mm_srli_epi16(mm2, 1);
    mm0 = _mm_add_epi16(mm0, mm2);
    mm2 = _mm_loadl_epi64((__m128i*)(mpeg_matrices + 384 + 4 * ecx)); // Load next 64-bit data
    mm0 = _mm_mulhi_epi16(mm0, mm2);

    mm2 = _mm_loadl_epi64((__m128i*)(mpeg_matrices + 256 + 4 * ecx + 4)); // Load 64-bit data
    mm2 = _mm_srli_epi16(mm2, 1);
    mm3 = _mm_add_epi16(mm3, mm2);
    mm2 = _mm_loadl_epi64((__m128i*)(mpeg_matrices + 384 + 4 * ecx + 4)); // Load next 64-bit data
    mm3 = _mm_mulhi_epi16(mm3, mm2);

    mm0 = _mm_mulhi_epi16(mm0, divider);
    mm3 = _mm_mulhi_epi16(mm3, divider);
    mm0 = _mm_srli_epi16(mm0, 1);
    mm3 = _mm_srli_epi16(mm3, 1);

    sum = _mm_add_epi16(sum, mm0);
    mm0 = _mm_xor_si128(mm0, mm1);
    sum = _mm_add_epi16(sum, mm3);
    mm3 = _mm_xor_si128(mm3, mm4);
    mm0 = _mm_sub_epi16(mm0, mm1);
    mm3 = _mm_sub_epi16(mm3, mm4);

    _mm_storel_epi64((__m128i*)(coeff + 4 * ecx), mm0); // Store 64-bit data
    _mm_storel_epi64((__m128i*)(coeff + 4 * ecx + 4), mm3); // Store next 64-bit data
  }

  sum = _mm_madd_epi16(sum, xmm_one);
  __m128i mm0 = sum;
  sum = _mm_srli_si128(sum, 4);
  mm0 = _mm_add_epi32(mm0, sum);
  return _mm_cvtsi128_si32(mm0);
}

// this one utilize 64 bit inside, mimics the original asm code
uint32_t quant_mpeg_inter_q1_sse2(int16_t* coeff, const int16_t* data, const uint32_t quant, const uint16_t* mpeg_matrices) {
  __m128i xmm_one = _mm_set1_epi16(1);
  __m128i sum = _mm_setzero_si128();

  for (int ecx = 0; ecx < 16; ecx += 2) {
    __m128i mm0 = _mm_loadl_epi64((__m128i*)(data + 4 * ecx)); // Load 64-bit data
    __m128i mm3 = _mm_loadl_epi64((__m128i*)(data + 4 * ecx + 4)); // Load next 64-bit data
    __m128i mm1 = _mm_setzero_si128();
    __m128i mm4 = _mm_setzero_si128();

    mm1 = _mm_cmpgt_epi16(mm1, mm0);
    mm4 = _mm_cmpgt_epi16(mm4, mm3);
    mm0 = _mm_xor_si128(mm0, mm1);
    mm3 = _mm_xor_si128(mm3, mm4);
    mm0 = _mm_sub_epi16(mm0, mm1);
    mm3 = _mm_sub_epi16(mm3, mm4);
    mm0 = _mm_slli_epi16(mm0, 4);
    mm3 = _mm_slli_epi16(mm3, 4);

    __m128i mm2 = _mm_loadl_epi64((__m128i*)(mpeg_matrices + 256 + 4 * ecx)); // Load 64-bit data
    mm2 = _mm_srli_epi16(mm2, 1);
    mm0 = _mm_add_epi16(mm0, mm2);
    mm2 = _mm_loadl_epi64((__m128i*)(mpeg_matrices + 384 + 4 * ecx)); // Load next 64-bit data
    mm0 = _mm_mulhi_epi16(mm0, mm2);

    mm2 = _mm_loadl_epi64((__m128i*)(mpeg_matrices + 256 + 4 * ecx + 4)); // Load 64-bit data
    mm2 = _mm_srli_epi16(mm2, 1);
    mm3 = _mm_add_epi16(mm3, mm2);
    mm2 = _mm_loadl_epi64((__m128i*)(mpeg_matrices + 384 + 4 * ecx + 4)); // Load next 64-bit data
    mm3 = _mm_mulhi_epi16(mm3, mm2);

    mm0 = _mm_srli_epi16(mm0, 1); // mm0 >>= 1 (/2)
    mm3 = _mm_srli_epi16(mm3, 1);

    sum = _mm_add_epi16(sum, mm0);
    mm0 = _mm_xor_si128(mm0, mm1);
    sum = _mm_add_epi16(sum, mm3);
    mm3 = _mm_xor_si128(mm3, mm4);
    mm0 = _mm_sub_epi16(mm0, mm1);
    mm3 = _mm_sub_epi16(mm3, mm4);

    _mm_storel_epi64((__m128i*)(coeff + 4 * ecx), mm0); // Store 64-bit data
    _mm_storel_epi64((__m128i*)(coeff + 4 * ecx + 4), mm3); // Store next 64-bit data
  }

  sum = _mm_madd_epi16(sum, xmm_one);
  __m128i mm0 = sum;
  sum = _mm_srli_si128(sum, 4);
  mm0 = _mm_add_epi32(mm0, sum);
  return _mm_cvtsi128_si32(mm0);
}

// utilizing 64 bit side
uint32_t quant_mpeg_inter_q2_sse2(int16_t* coeff, const int16_t* data, const uint32_t quant, const uint16_t* mpeg_matrices) {
  __m128i xmm_one = _mm_set1_epi16(1);
  __m128i sum = _mm_setzero_si128();

  for (int ecx = 0; ecx < 16; ecx += 2) {
    __m128i mm0 = _mm_loadl_epi64((__m128i*)(data + 4 * ecx)); // Load 64-bit data
    __m128i mm3 = _mm_loadl_epi64((__m128i*)(data + 4 * ecx + 4)); // Load next 64-bit data
    __m128i mm1 = _mm_setzero_si128();
    __m128i mm4 = _mm_setzero_si128();

    mm1 = _mm_cmpgt_epi16(mm1, mm0);
    mm4 = _mm_cmpgt_epi16(mm4, mm3);
    mm0 = _mm_xor_si128(mm0, mm1);
    mm3 = _mm_xor_si128(mm3, mm4);
    mm0 = _mm_sub_epi16(mm0, mm1);
    mm3 = _mm_sub_epi16(mm3, mm4);
    mm0 = _mm_slli_epi16(mm0, 4);
    mm3 = _mm_slli_epi16(mm3, 4);

    __m128i mm2 = _mm_loadl_epi64((__m128i*)(mpeg_matrices + 256 + 4 * ecx)); // Load 64-bit data
    mm2 = _mm_srli_epi16(mm2, 1);
    mm0 = _mm_add_epi16(mm0, mm2);
    mm2 = _mm_loadl_epi64((__m128i*)(mpeg_matrices + 384 + 4 * ecx)); // Load next 64-bit data
    mm0 = _mm_mulhi_epi16(mm0, mm2);

    mm2 = _mm_loadl_epi64((__m128i*)(mpeg_matrices + 256 + 4 * ecx + 4)); // Load 64-bit data
    mm2 = _mm_srli_epi16(mm2, 1);
    mm3 = _mm_add_epi16(mm3, mm2);
    mm2 = _mm_loadl_epi64((__m128i*)(mpeg_matrices + 384 + 4 * ecx + 4)); // Load next 64-bit data
    mm3 = _mm_mulhi_epi16(mm3, mm2);

    mm0 = _mm_srli_epi16(mm0, 2); // mm0 >>= 2 (/4)
    mm3 = _mm_srli_epi16(mm3, 2);

    sum = _mm_add_epi16(sum, mm0);
    mm0 = _mm_xor_si128(mm0, mm1);
    sum = _mm_add_epi16(sum, mm3);
    mm3 = _mm_xor_si128(mm3, mm4);
    mm0 = _mm_sub_epi16(mm0, mm1);
    mm3 = _mm_sub_epi16(mm3, mm4);

    _mm_storel_epi64((__m128i*)(coeff + 4 * ecx), mm0); // Store 64-bit data
    _mm_storel_epi64((__m128i*)(coeff + 4 * ecx + 4), mm3); // Store next 64-bit data
  }

  sum = _mm_madd_epi16(sum, xmm_one);
  __m128i mm0 = sum;
  sum = _mm_srli_si128(sum, 4);
  mm0 = _mm_add_epi32(mm0, sum);
  return _mm_cvtsi128_si32(mm0);
}
#endif
