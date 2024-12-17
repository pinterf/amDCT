#include <math.h>

#include "transfer_add.h"
#include <smmintrin.h> // SSE4.2

void copy_add_16to16_c(uint16_t* dst, uint16_t* src, uint32_t len) {
  uint32_t x;
  for (x = 0; x < len; x++) {
    dst[x] = (uint16_t)(src[x] + dst[x]);
  }
}

// dst: block[64], 8x8 uint16_t
void copy_16to16_dctblk_c(int16_t* dst, int16_t* src, uint32_t stride)
{
  for (int j = 0; j < 8; j++) {
    for (int i = 0; i < 8; i++) {
      dst[j * 8 + i] = src[j * stride + i];
    }
  }
}

void copy_16to32_dctblk_c(int32_t* dst,
  int16_t* src,
  uint32_t stride)
{
  for (int j = 0; j < 8; j++) {
    for (int i = 0; i < 8; i++) {
      int pos = j * stride + i;
      dst[pos] = src[pos];
    }
  }
}

void test_transfer_16to16copy_c(uint16_t* dst,   // frame with stride width
  int16_t* src,    // block[64]
  uint32_t stride)
{
  int i, j;

  for (j = 0; j < 8; j++) {
    for (i = 0; i < 8; i++) {
      int16_t pixel = src[j * 8 + i];

      if (pixel < 0) {
        pixel = 0;
      }
      else if (pixel > 255) {
        pixel = 255;
      }

      dst[j * stride + i] = (uint16_t)pixel;
    }
  }
}

/*
 * SRC - the source buffer
 * DST - the destination buffer
 *
 * Then the function does the 8->16 bit transfer and this serie of operations :
 *
 *    SRC (16bit) = SRC
 *    DST (8bit)  = max(min(SRC, 255), 0)
 */
void test_transfer_16to8copy_c(uint8_t* dst,
  int16_t* src,
  uint32_t stride)
{
  int i, j;

  for (j = 0; j < 8; j++) {
    for (i = 0; i < 8; i++) {
      int16_t pixel = src[j * 8 + i];

      if (pixel < 0) {
        pixel = 0;
      }
      else if (pixel > 255) {
        pixel = 255;
      }

      dst[j * stride + i] = (uint8_t)pixel;
    }
  }
}

void copy_add_16to16_clpsrc_c(uint16_t* dst, int16_t* src, uint32_t stride) {
  int x, y;

  for (y = 0; y < 8; y++) {
    for (x = 0; x < 8; x++) {
      int      pos = y * stride + x;
      uint16_t i = *src++;

      if (i < 0)   i = 0;
      if (i > 255) i = 255;
      dst[pos] += i;
    }
  }

  return;
}

void copy_add_16to32_dctblk_c(int32_t* dst,   // 32bit frame.
  int16_t* src,   // 16bit 64 entry block
  uint32_t stride) {
  int x, y;

  for (y = 0; y < 8; y++) {
    for (x = 0; x < 8; x++) {
      dst[y * stride + x] += src[y * 8 + x];
    }
  }

  return;
}

void copy_add_16to16(uint16_t* dst, uint16_t* const src, int32_t len) {
#ifdef INTEL_INTRINSICS
  copy_add_16to16_sse2(dst, src, len);
#else
  copy_add_16to16_c(dst, src, len);
#endif
}


void copy_add_16to16_sse2(uint16_t* dst, uint16_t* const src, int32_t len) {
  __m128i* dst_ptr = (__m128i*)dst;
  __m128i* src_ptr = (__m128i*)src;
  int32_t num_iterations = len / 32;

  for (int i = 0; i < num_iterations; ++i) {
    __m128i xmm0 = _mm_load_si128(&dst_ptr[0]);
    __m128i xmm1 = _mm_load_si128(&src_ptr[0]);
    __m128i xmm2 = _mm_load_si128(&dst_ptr[1]);
    __m128i xmm3 = _mm_load_si128(&src_ptr[1]);
    __m128i xmm4 = _mm_load_si128(&dst_ptr[2]);
    __m128i xmm5 = _mm_load_si128(&src_ptr[2]);
    __m128i xmm6 = _mm_load_si128(&dst_ptr[3]);
    __m128i xmm7 = _mm_load_si128(&src_ptr[3]);

    xmm0 = _mm_add_epi16(xmm0, xmm1);
    xmm2 = _mm_add_epi16(xmm2, xmm3);
    xmm4 = _mm_add_epi16(xmm4, xmm5);
    xmm6 = _mm_add_epi16(xmm6, xmm7);

    _mm_store_si128(&dst_ptr[0], xmm0);
    _mm_store_si128(&dst_ptr[1], xmm2);
    _mm_store_si128(&dst_ptr[2], xmm4);
    _mm_store_si128(&dst_ptr[3], xmm6);

    dst_ptr += 4;
    src_ptr += 4;
  }

}


void copy_add3_16to16_c(uint16_t* dst, uint16_t* src1, uint16_t* src2, uint32_t len) {
  uint32_t num_iterations = len / 16;

  for (uint32_t i = 0; i < num_iterations; ++i) {
    for (uint32_t j = 0; j < 16; ++j) {
      dst[i * 16 + j] = dst[i * 16 + j] + src1[i * 16 + j] + src2[i * 16 + j];
    }
  }
}

void copy_add3_16to16(uint16_t* dst, uint16_t* src1, uint16_t* src2, uint32_t len) {
#ifdef INTEL_INTRINSICS
  copy_add3_16to16_sse2(dst, src1, src2, len);
#else
  copy_add3_16to16_c(dst, src1, src2, len);
#endif
}

#ifdef INTEL_INTRINSICS
void copy_add3_16to16_sse2(uint16_t* dst, uint16_t* src1, uint16_t* src2, uint32_t len) {
  __m128i* dst_ptr = (__m128i*)dst;
  __m128i* src1_ptr = (__m128i*)src1;
  __m128i* src2_ptr = (__m128i*)src2;
  uint32_t num_iterations = len / 16;

  for (uint32_t i = 0; i < num_iterations; ++i) {
    __m128i xmm0 = _mm_load_si128(&dst_ptr[0]);
    __m128i xmm1 = _mm_load_si128(&src1_ptr[0]);
    __m128i xmm2 = _mm_load_si128(&src2_ptr[0]);

    __m128i xmm3 = _mm_load_si128(&dst_ptr[1]);
    __m128i xmm4 = _mm_load_si128(&src1_ptr[1]);
    __m128i xmm5 = _mm_load_si128(&src2_ptr[1]);

    xmm0 = _mm_add_epi16(xmm0, xmm1);
    xmm3 = _mm_add_epi16(xmm3, xmm4);
    xmm0 = _mm_add_epi16(xmm0, xmm2);
    xmm3 = _mm_add_epi16(xmm3, xmm5);

    _mm_store_si128(&dst_ptr[0], xmm0);
    _mm_store_si128(&dst_ptr[1], xmm3);

    dst_ptr += 2;
    src1_ptr += 2;
    src2_ptr += 2;
  }

}
#endif



void copy_add4_16to16(uint16_t* dst, uint16_t* const src1, uint16_t* const src2, uint16_t* const src3, uint32_t len) {
#ifdef INTEL_INTRINSICS
  copy_add4_16to16_sse2(dst, src1, src2, src3, len);
#else
  copy_add4_16to16_c(dst, src1, src2, src3, len);
#endif
}

#ifdef INTEL_INTRINSICS
void copy_add4_16to16_sse2(uint16_t* dst, uint16_t* const src1, uint16_t* const src2, uint16_t* const src3, uint32_t len) {
  __m128i* dst_ptr = (__m128i*)dst;
  __m128i* src1_ptr = (__m128i*)src1;
  __m128i* src2_ptr = (__m128i*)src2;
  __m128i* src3_ptr = (__m128i*)src3;
  uint32_t num_iterations = len / 8;

  // Each __m128i can hold 8 uint16_t values
  // len is mod8 ?? if so, the final copy can be omitted

  for (uint32_t i = 0; i < num_iterations; ++i) {
    __m128i xmm0 = _mm_load_si128(&dst_ptr[i]);
    __m128i xmm1 = _mm_load_si128(&src1_ptr[i]);
    __m128i xmm2 = _mm_load_si128(&src2_ptr[i]);
    __m128i xmm3 = _mm_load_si128(&src3_ptr[i]);

    xmm0 = _mm_add_epi16(xmm0, xmm1);
    xmm0 = _mm_add_epi16(xmm0, xmm2);
    xmm0 = _mm_add_epi16(xmm0, xmm3);

    _mm_store_si128(&dst_ptr[i], xmm0);
  }

  // Handle any remaining elements
  for (uint32_t i = num_iterations * 8; i < len; ++i) {
    dst[i] = (uint16_t)(dst[i] + src1[i] + src2[i] + src3[i]);
  }
}
#endif

void copy_add4_16to16_c(uint16_t* dst, uint16_t* const src1, uint16_t* const src2, uint16_t* const src3, uint32_t len) {
  for (uint32_t x = 0; x < len; x++) {
    dst[x] = (uint16_t)(dst[x] + src1[x] + src2[x] + src3[x]);
  }
}
