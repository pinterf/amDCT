#include <math.h>

#include "transfer_add.h"
#include <smmintrin.h> // SSE4.2

void test_copy_add_16to16_c(uint16_t* dst, uint16_t* src, uint32_t len) {
  uint32_t x;

  for (x = 0; x < len; x++) {
    dst[x] = (uint16_t)(src[x] + dst[x]);
  }

  return;
}

void test2_copy_add_16to16_c(uint16_t* dst, uint16_t* src, uint32_t len) {
  uint32_t x;
  uint32_t temp;

  for (x = 0; x < len; x++) {
    //temp = (uint16_t)(src[x] + dst[x]);
    temp = (uint32_t)(src[x] + dst[x]);

    if (temp < 0) {
      temp = 1;
    }

    if (temp > 254) {
      temp = 254;
    }

    dst[x] = (uint8_t)temp;
  }

  return;
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
#ifdef USE_NEW_INTRINSICS
  copy_add_16to16_sse2(dst, src, len);
#else
  copy_add_16to16_xmm(dst, src, len);
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

#ifndef ARCH_IS_X86_64
void copy_add_16to16_xmm(uint16_t* dst, uint16_t* const src, int32_t len) {
  __asm {
    ALIGN 16
    mov     edx, dst
    mov     esi, src
    mov     ecx, len

    MainLoop :
    movapd    xmm0, [EDX]
      movapd    xmm1, [ESI]
      movapd    xmm2, [EDX + 16]
      movapd    xmm3, [ESI + 16]
      movapd    xmm4, [EDX + 32]
      movapd    xmm5, [ESI + 32]
      movapd    xmm6, [EDX + 48]
      movapd    xmm7, [ESI + 48]

      paddw     xmm0, xmm1
      paddw     xmm2, xmm3
      paddw     xmm4, xmm5
      paddw     xmm6, xmm7

      movapd[EDX], xmm0
      movapd[EDX + 16], xmm2
      movapd[EDX + 32], xmm4
      movapd[EDX + 48], xmm6

      add       EDX, 64
      add       ESI, 64
      sub       ECX, 32
      jnz       MainLoop

      emms
  }

  return;
}
#endif


void copy_add3_16to16(uint16_t* dst, uint16_t* src1, uint16_t* src2, uint32_t len) {
#ifdef USE_NEW_INTRINSICS
  copy_add3_16to16_sse2(dst, src1, src2, len);
#else
  copy_add3_16to16_xmm(dst, src1, src2, len);
#endif
}

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

#ifndef ARCH_IS_X86_64
void copy_add3_16to16_xmm(uint16_t* dst, uint16_t* src1, uint16_t* src2, uint32_t len) {
  __asm {
    ALIGN 16
    mov     edx, dst
    mov     esi, src1
    mov     eax, src2
    mov     ecx, len
    movapd    xmm1, [ESI]

    MainLoop:
    movapd    xmm0, [EDX]
      movapd    xmm2, [EAX]

      movapd    xmm3, [EDX + 16]
      movapd    xmm4, [ESI + 16]
      movapd    xmm5, [EAX + 16]

      add       EAX, 32

      paddw     xmm0, xmm1
      paddw     xmm3, xmm4
      paddw     xmm0, xmm2
      paddw     xmm3, xmm5
      movapd[EDX], xmm0
      add       ESI, 32
      movapd    xmm1, [ESI]
      movapd[EDX + 16], xmm3

      add       EDX, 32
      sub       ECX, 16
      jnz       MainLoop

      emms
  }

  return;
}
#endif

#if 0
// not used
void copy_add4_16to16_xmm(uint16_t* dst, uint16_t* const src1, uint16_t* const src2, uint16_t* const src3, uint32_t len) {
  __asm {
    ALIGN 16
    push    ebx

    mov     edx, dst
    mov     esi, src1
    mov     ebx, src2
    mov     eax, src3
    mov     ecx, len

    MainLoop :
    movapd    xmm0, [EDX]
      movapd    xmm1, [ESI]
      movapd    xmm2, [EBX]
      movapd    xmm3, [EAX]

      movapd    xmm4, [EDX + 16]
      movapd    xmm5, [ESI + 16]
      movapd    xmm6, [EBX + 16]
      movapd    xmm7, [EAX + 16]

      paddw     xmm0, xmm1
      paddw     xmm0, xmm2
      paddw     xmm0, xmm3

      paddw     xmm4, xmm5
      movapd[EDX], xmm0
      paddw     xmm4, xmm6
      paddw     xmm4, xmm7

      add       EAX, 32
      add       ESI, 32
      add       EBX, 32
      movapd[EDX + 16], xmm4

      add       EDX, 32
      sub       ECX, 16
      jnz       MainLoop

      pop    ebx
      emms
  }

  return;
}
#endif

#if 0
// not used
void working_copy_add4_16to16_xmm(uint16_t* dst, const uint16_t* const src1, const uint16_t* const src2, const uint16_t* const src3, uint32_t len) {
  __asm {
    ALIGN 16
    mov     edx, dst
    mov     esi, src1
    mov     ebx, src2
    mov     eax, src3
    mov     ecx, len

    MainLoop :
    movapd       xmm0, [EDX]
      movapd       xmm1, [ESI]
      movapd       xmm2, [EBX]
      movapd       xmm3, [EAX]
      paddw     xmm0, xmm1
      paddw     xmm0, xmm2
      paddw     xmm0, xmm3
      movapd[EDX], xmm0
      add       EAX, 16
      add       EDX, 16
      add       ESI, 16
      add       EBX, 16
      sub       ECX, 8
      jnz       MainLoop
      emms
  }

  return;
}
#endif

#if 0
// not used
//void copy_add4_16to16_mmx(uint16_t *dst, const uint16_t * const src1, const uint16_t * const src2, const uint16_t * const src3, uint32_t len){
void copy_add4_16to16_mmx(uint16_t* dst, uint16_t* const src1, uint16_t* const src2, uint16_t* const src3, uint32_t len) {
  __asm {
    ALIGN 16
    //     push ebx
    mov     edx, dst
    mov     esi, src1
    mov     ebx, src2
    mov     eax, src3
    mov     ecx, len

    MainLoop :
    movq       mm0, [EDX]
      movq       mm1, [ESI]
      movq       mm2, [EBX]
      movq       mm3, [EAX]
      paddw     mm0, mm1
      paddw     mm0, mm2
      paddw     mm0, mm3
      movq[EDX], mm0
      add       EAX, 8
      add       EDX, 8
      add       ESI, 8
      add       EBX, 8
      sub       ECX, 4
      jnz       MainLoop

      //      pop ebx
      emms
  }

  return;
}
#endif

void copy_add4_16to16_c(uint16_t* dst, uint16_t* const src1, uint16_t* const src2, uint16_t* const src3, uint32_t len) {
  uint32_t x;

  for (x = 0; x < len; x++) {
    dst[x] = (uint16_t)(dst[x] + src1[x] + src2[x] + src3[x]);
  }

  return;
}
