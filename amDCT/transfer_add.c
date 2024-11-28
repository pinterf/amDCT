#include <math.h>

#include "transfer_add.h"

/*
void copy_add_8to16_len_c_2(uint16_t *dst, uint8_t *src, uint32_t len) {
	uint32_t x;

	for (x=0; x<len; x++){
		dst[x] += *src++;
	}

	return;
}

*/
void test_copy_add_16to16_c(uint16_t *dst, uint16_t *src, uint32_t len) {
	uint32_t x;

	for (x = 0; x < len; x++) {
		dst[x] = (uint16_t)(src[x] + dst[x]);
	}

	return;
}

void test2_copy_add_16to16_c(uint16_t *dst, uint16_t *src, uint32_t len) {
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

void copy_16to16_clpsrc_c(uint16_t *dst, int16_t *src, uint32_t len) {
	uint32_t x;

	for (x = 0; x < len; x++) {
		int16_t pixel = src[x];
		if (pixel < 0) {
			pixel = 0;
		}
		else if (pixel > 255) {
			pixel = 255;
		}
		dst[x] = *src++;
	}

	return;
}

void
copy_16to16_dctblk_c(int16_t * dst,   // block[64]
	int16_t * src,   // frame with stride width
	uint32_t stride)
{
	int i, j;
	int pos;
	int d, s;

	for (j = 0; j < 8; j++) {
		for (i = 0; i < 8; i++) {
			pos = j*stride + i;

			s = src[pos];
			d = dst[j * 8 + i];

			dst[j * 8 + i] = src[pos];
			d = dst[j * 8 + i];
		}
	}
}

void
copy_16to32_dctblk_c(int32_t * dst,
	int16_t * src,
	uint32_t stride)
{
	int i, j;

	for (j = 0; j < 8; j++) {
		for (i = 0; i < 8; i++) {
			int16_t pos = (int16_t)(j*stride + i);

			dst[pos] = src[pos];
		}
	}
}

void
test_transfer_16to16copy_c(uint16_t * dst,   // frame with stride width
	int16_t * src,    // block[64]
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
void
test_transfer_16to8copy_c(uint8_t * dst,
	int16_t * src,
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

// /*
void copy_add_16to16_clpsrc_c(uint16_t *dst, int16_t *src, uint32_t stride) {
	int x, y;

	for (y = 0; y < 8; y++) {
		for (x = 0; x < 8; x++) {
			int      pos = y*stride + x;
			uint16_t i = *src++;

			if (i < 0)   i = 0;
			if (i > 255) i = 255;
			dst[pos] += i;
		}
	}

	return;
}
// */

/*
void copy_add_16to16_dctblk_c(int16_t *dst,     // 16bit frame.
							  int16_t *src,     // 16bit 64 entry block
							  uint32_t stride) {
	int x,y;

	for (y=0; y<8; y++){
		for (x=0; x<8; x++){
			dst[y*stride+x] += src[y * 8 + x];
		}
	}

	return;
}
*/

void copy_add_16to32_dctblk_c(int32_t *dst,   // 32bit frame.
	int16_t *src,   // 16bit 64 entry block
	uint32_t stride) {
	int x, y;

	for (y = 0; y < 8; y++) {
		for (x = 0; x < 8; x++) {
			dst[y*stride + x] += src[y * 8 + x];
		}
	}

	return;
}

void copy_add_8to16_mmx(uint16_t *dst, const uint8_t * const src, uint32_t len) {
	__asm {
		ALIGN 16
		mov     edx, dst
		mov     esi, src
		mov     ecx, len
		pxor    mm7, mm7

		MainLoop :
		movq   		mm0, [EDX]
			movd   		mm1, [ESI]
			punpcklbw	mm1, mm7
			paddw  		mm0, mm1
			movq[EDX], mm0
			add    		EDX, 8
			add    		ESI, 4
			sub    		ECX, 4
			jnz    MainLoop
			emms
	}

	return;
}

void copy_add_16to16_xmm(uint16_t *dst, uint16_t * const src, int32_t len) {
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

void copy_add_16to16_mmx(uint16_t *dst, const uint16_t * const src, uint32_t len) {
	__asm {
		ALIGN 16
		mov     edx, dst
		mov     esi, src
		mov     ecx, len

		MainLoop :
		movq       mm0, [EDX]
			movq       mm1, [ESI]
			paddw     mm0, mm1
			movq[EDX], mm0
			add       EDX, 8
			add       ESI, 8
			sub       ECX, 4
			jnz       MainLoop
			emms
	}

	return;
}

void copy_add3_16to16_xmm(uint16_t *dst, uint16_t *src1, uint16_t *src2, uint32_t len) {
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

void copy_add3_16to16_mmx(uint16_t *dst, const uint16_t * const src1, const uint16_t * const src2, uint32_t len) {
	__asm {
		ALIGN 16
		mov     edx, dst
		mov     esi, src1
		mov     eax, src2
		mov     ecx, len

		MainLoop :
		movq       mm0, [EDX]
			movq       mm1, [ESI]
			movq       mm2, [EAX]
			paddw     mm0, mm1
			paddw     mm0, mm2
			movq[EDX], mm0
			add       EDX, 8
			add       ESI, 8
			add       EBX, 8
			sub       ECX, 4
			jnz       MainLoop
			emms
	}

	return;
}

//void copy_add4_16to16_xmm(uint16_t *dst, const uint16_t * const src1, const uint16_t * const src2, const uint16_t * const src3, uint32_t len){
void copy_add4_16to16_xmm(uint16_t *dst, uint16_t * const src1, uint16_t * const src2, uint16_t * const src3, uint32_t len) {
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

void working_copy_add4_16to16_xmm(uint16_t *dst, const uint16_t * const src1, const uint16_t * const src2, const uint16_t * const src3, uint32_t len) {
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

/*
void copy_add4_16to16_xmm(uint16_t *dst, const uint16_t * const src1, const uint16_t * const src2, const uint16_t * const src3, uint32_t len){
	__asm {
	   ALIGN 16
	   mov     edx, dst
	   mov     esi, src1
	   //mov     ebx, src2
	   //mov     eax, src3
	   mov     eax, src2
	   mov     ecx, len

	MainLoop:
		movapd       xmm0, [EDX]
		movapd       xmm1, [ESI]
		//movapd       xmm2, [EBX]
		movapd       xmm2, [EAX]
	mov     esi, src3
		movapd       xmm3, [ESI]
		paddw     xmm0, xmm1
		paddw     xmm0, xmm2
		paddw     xmm0, xmm3
		movapd      [EDX], xmm0
		add       EAX, 16
		add       EDX, 16
		add       ESI, 16
		//add       EBX, 16
		sub       ECX, 8
		jnz       MainLoop
		emms
	}

	return;
}
*/

//void copy_add4_16to16_mmx(uint16_t *dst, const uint16_t * const src1, const uint16_t * const src2, const uint16_t * const src3, uint32_t len){
void copy_add4_16to16_mmx(uint16_t *dst, uint16_t * const src1, uint16_t * const src2, uint16_t * const src3, uint32_t len) {
	__asm {
		ALIGN 16
		//	   push ebx
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

			//	    pop ebx
			emms
	}

	return;
}

void copy_add4_16to16_c(uint16_t *dst, uint16_t * const src1, uint16_t * const src2, uint16_t * const src3, uint32_t len) {
	uint32_t x;

	for (x = 0; x < len; x++) {
		dst[x] = (uint16_t)(dst[x] + src1[x] + src2[x] + src3[x]);
	}

	return;
}