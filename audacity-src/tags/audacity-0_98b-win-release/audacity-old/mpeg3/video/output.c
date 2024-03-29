#include "../libmpeg3.h"
#include "mpeg3video.h"
#include <string.h>

#define CLIP(x)  ((x) >= 0 ? ((x) < 255 ? (x) : 255) : 0)

static long long mpeg3_MMX_0 = 0L;
static unsigned long  mpeg3_MMX_10w[]         = {0x00100010, 0x00100010};                     /*dd    00010 0010h, 000100010h */
static unsigned long  mpeg3_MMX_80w[]         = {0x00800080, 0x00800080};                     /*dd    00080 0080h, 000800080h */

static unsigned long  mpeg3_MMX_00FFw[]       = {0x00ff00ff, 0x00ff00ff};                     /*dd    000FF 00FFh, 000FF00FFh */

static unsigned short mpeg3_MMX_Ublucoeff[]   = {0x81, 0x81, 0x81, 0x81};                     /*dd    00081 0081h, 000810081h */
static unsigned short mpeg3_MMX_Vredcoeff[]   = {0x66, 0x66, 0x66, 0x66};                     /*dd    00066 0066h, 000660066h */

static unsigned short mpeg3_MMX_Ugrncoeff[]   = {0xffe8, 0xffe8, 0xffe8, 0xffe8};             /*dd    0FFE7 FFE7h, 0FFE7FFE7h */
static unsigned short mpeg3_MMX_Vgrncoeff[]   = {0xffcd, 0xffcd, 0xffcd, 0xffcd};             /*dd    0FFCC FFCCh, 0FFCCFFCCh */

static unsigned short mpeg3_MMX_Ycoeff[]      = {0x4a, 0x4a, 0x4a, 0x4a};                     /*dd    0004A 004Ah, 0004A004Ah */

static unsigned short mpeg3_MMX_redmask[]     = {0xf800, 0xf800, 0xf800, 0xf800};             /*dd    07c00 7c00h, 07c007c00h */

static unsigned short mpeg3_MMX_grnmask[]     = {0x7e0, 0x7e0, 0x7e0, 0x7e0};                 /*dd    003e0 03e0h, 003e003e0h */

static unsigned char mpeg3_601_to_rgb[256];

/* Algorithm */
/* 			r = (int)(*y + 1.371 * (*cr - 128)); */
/* 			g = (int)(*y - 0.698 * (*cr - 128) - 0.336 * (*cb - 128)); */
/* 			b = (int)(*y + 1.732 * (*cb - 128)); */

#ifdef HAVE_MMX
inline void mpeg3video_rgb16_mmx(unsigned char *lum, 
			unsigned char *cr, 
			unsigned char *cb,
            unsigned char *out, 
			int rows, 
			int cols, 
			int mod)
{
	unsigned short *row1;
	int x;
    unsigned char *y;
	int col1;

	row1 = (unsigned short *)out;
    col1 = cols + mod;
    mod += cols + mod;
    mod *= 2;
	y = lum + cols * rows;
    x = 0;

    __asm__ __volatile__(
        ".align 8\n"
        "1:\n"
            "movd           (%1),                   %%mm0\n"  /* 4 Cb	  0  0  0  0 u3 u2 u1 u0 */
            "pxor           %%mm7,                  %%mm7\n"
            "movd           (%0),                   %%mm1\n"  /* 4 Cr	  0  0  0  0 v3 v2 v1 v0 */
            "punpcklbw      %%mm7,                  %%mm0\n"  /* 4 W cb   0 u3  0 u2  0 u1  0 u0 */
            "punpcklbw      %%mm7,                  %%mm1\n"  /* 4 W cr   0 v3  0 v2  0 v1  0 v0 */

            "psubw          mpeg3_MMX_80w,          %%mm0\n"
            "psubw          mpeg3_MMX_80w,          %%mm1\n"
            "movq           %%mm0,                  %%mm2\n"  /* Cb 	  0 u3  0 u2  0 u1  0 u0 */
            "movq           %%mm1,                  %%mm3\n"  /* Cr */
            "pmullw         mpeg3_MMX_Ugrncoeff,    %%mm2\n"  /* Cb2green 0 R3  0 R2  0 R1  0 R0 */
            "movq           (%2),                   %%mm6\n"  /* L1 	 l7 L6 L5 L4 L3 L2 L1 L0 */
            "pmullw         mpeg3_MMX_Ublucoeff,    %%mm0\n"  /* Cb2blue */
            "pand           mpeg3_MMX_00FFw,        %%mm6\n"  /* L1 	 00 L6 00 L4 00 L2 00 L0 */
            "pmullw         mpeg3_MMX_Vgrncoeff,    %%mm3\n"  /* Cr2green */
            "movq           (%2),                   %%mm7\n"  /* L2 */
            "pmullw         mpeg3_MMX_Vredcoeff,    %%mm1\n"  /* Cr2red */
            "psrlw          $8,                     %%mm7\n"  /* L2 	 00 L7 00 L5 00 L3 00 L1 */
            "pmullw         mpeg3_MMX_Ycoeff,       %%mm6\n"  /* lum1 */
            "paddw          %%mm3,                  %%mm2\n"  /* Cb2green + Cr2green == green */
            "pmullw         mpeg3_MMX_Ycoeff,       %%mm7\n"  /* lum2 */

            "movq           %%mm6,                  %%mm4\n"  /* lum1 */
            "paddw          %%mm0,                  %%mm6\n"  /* lum1 +blue  00 B6 00 B4 00 B2 00 B0 */
            "movq           %%mm4,                  %%mm5\n"  /* lum1 */
            "paddw          %%mm1,                  %%mm4\n"  /* lum1 +red   00 R6 00 R4 00 R2 00 R0 */
            "paddw          %%mm2,                  %%mm5\n"  /* lum1 +green 00 G6 00 G4 00 G2 00 G0 */
            "psraw          $6,                     %%mm4\n"  /* R1 0 .. 64 */
            "movq           %%mm7,                  %%mm3\n"  /* lum2                       00 L7 00 L5 00 L3 00 L1 */
            "psraw          $6,                     %%mm5\n"  /* G1  - .. + */
            "paddw          %%mm0,                  %%mm7\n"  /* Lum2 +blue 00 B7 00 B5 00 B3 00 B1 */
            "psraw          $6,                     %%mm6\n"  /* B1         0 .. 64 */
            "packuswb       %%mm4,                  %%mm4\n"  /* R1 R1 */
            "packuswb       %%mm5,                  %%mm5\n"  /* G1 G1 */
            "packuswb       %%mm6,                  %%mm6\n"  /* B1 B1 */
            "punpcklbw      %%mm4,                  %%mm4\n"
            "punpcklbw      %%mm5,                  %%mm5\n"

            "pand           mpeg3_MMX_redmask,      %%mm4\n"
            "psllw          $3,                     %%mm5\n"  /* GREEN       1 */
            "punpcklbw      %%mm6,                  %%mm6\n"
            "pand           mpeg3_MMX_grnmask,      %%mm5\n"
            "pand           mpeg3_MMX_redmask,      %%mm6\n"
            "por            %%mm5,                  %%mm4\n"  /* */
            "psrlw          $11,                    %%mm6\n"  /* BLUE		 1 */
            "movq           %%mm3,                  %%mm5\n"  /* lum2 */
            "paddw          %%mm1,                  %%mm3\n"  /* lum2 +red	 00 R7 00 R5 00 R3 00 R1 */
            "paddw          %%mm2,                  %%mm5\n"  /* lum2 +green 00 G7 00 G5 00 G3 00 G1 */
            "psraw          $6,                     %%mm3\n"  /* R2 */
            "por            %%mm6,                  %%mm4\n"  /* MM4 */
            "psraw          $6,                     %%mm5\n"  /* G2 */
            "movq           (%2, %3),               %%mm6\n"  /* L3 */
            "psraw          $6,                     %%mm7\n"
            "packuswb       %%mm3,                  %%mm3\n"
            "packuswb       %%mm5,                  %%mm5\n"
            "packuswb       %%mm7,                  %%mm7\n"
            "pand           mpeg3_MMX_00FFw,        %%mm6\n"  /* L3 */
            "punpcklbw      %%mm3,                  %%mm3\n"
            "punpcklbw      %%mm5,                  %%mm5\n"
            "pmullw         mpeg3_MMX_Ycoeff,       %%mm6\n"  /* lum3 */
            "punpcklbw      %%mm7,                  %%mm7\n"
            "psllw          $3,                     %%mm5\n"  /* GREEN 2 */
            "pand           mpeg3_MMX_redmask,      %%mm7\n"
            "pand           mpeg3_MMX_redmask,      %%mm3\n"
            "psrlw          $11,                    %%mm7\n"  /* BLUE  2 */
            "pand           mpeg3_MMX_grnmask,      %%mm5\n"
            "por            %%mm7,  				%%mm3\n"
            "movq           (%2,%3),				%%mm7\n"  /* L4 */
            "por            %%mm5,  				%%mm3\n"	 /* */
            "psrlw          $8,                     %%mm7\n"    /* L4 */
            "movq           %%mm4,  				%%mm5\n"
            "punpcklwd      %%mm3,                  %%mm4\n"
            "pmullw         mpeg3_MMX_Ycoeff,       %%mm7\n"    /* lum4 */
            "punpckhwd      %%mm3,                  %%mm5\n"

            "movq           %%mm4,  				(%4)\n"
            "movq           %%mm5,  				8(%4)\n"

            "movq           %%mm6,  				%%mm4\n"		/* Lum3 */
            "paddw          %%mm0,                  %%mm6\n"                /* Lum3 +blue */

            "movq           %%mm4,  				%%mm5\n"						/* Lum3 */
            "paddw          %%mm1,                  %%mm4\n"       /* Lum3 +red */
            "paddw          %%mm2,                  %%mm5\n"                        /* Lum3 +green */
            "psraw          $6, 				    %%mm4\n"
            "movq           %%mm7,   			    %%mm3\n"	/* Lum4 */
            "psraw          $6, 				    %%mm5\n"
            "paddw          %%mm0,                  %%mm7\n"                   /* Lum4 +blue */
            "psraw          $6,                     %%mm6\n"                        /* Lum3 +blue */
            "movq           %%mm3,                  %%mm0\n"  /* Lum4 */
            "packuswb       %%mm4,                  %%mm4\n"
            "paddw          %%mm1,                  %%mm3\n"  /* Lum4 +red */
            "packuswb       %%mm5,                  %%mm5\n"
            "paddw          %%mm2,                  %%mm0\n"         /* Lum4 +green */
            "packuswb       %%mm6,                  %%mm6\n"
            "punpcklbw      %%mm4,                  %%mm4\n"
            "punpcklbw      %%mm5,                  %%mm5\n"
            "punpcklbw      %%mm6,                  %%mm6\n"
            "psllw          $3,                     %%mm5\n" /* GREEN 3 */
            "pand           mpeg3_MMX_redmask,      %%mm4\n"
            "psraw          $6,         			%%mm3\n" /* psr 6 */
            "psraw          $6,         			%%mm0\n"
            "pand           mpeg3_MMX_redmask,      %%mm6\n" /* BLUE */
            "pand           mpeg3_MMX_grnmask,      %%mm5\n"
            "psrlw          $11,                    %%mm6\n"  /* BLUE  3 */
            "por            %%mm5,  				%%mm4\n"
            "psraw          $6,                     %%mm7\n"
            "por            %%mm6,      			%%mm4\n"
            "packuswb       %%mm3,                  %%mm3\n"
            "packuswb       %%mm0,                  %%mm0\n"
            "packuswb       %%mm7,                  %%mm7\n"
            "punpcklbw      %%mm3,                  %%mm3\n"
            "punpcklbw      %%mm0,                  %%mm0\n"
            "punpcklbw      %%mm7,                  %%mm7\n"
            "pand           mpeg3_MMX_redmask,      %%mm3\n"
            "pand           mpeg3_MMX_redmask,      %%mm7\n" /* BLUE */
            "psllw          $3,                     %%mm0\n" /* GREEN 4 */
            "psrlw          $11,                    %%mm7\n"
            "pand           mpeg3_MMX_grnmask,      %%mm0\n"
            "por            %%mm7,                  %%mm3\n"
            "addl           $8,                             %6\n"
            "por            %%mm0,                  %%mm3\n"

            "movq           %%mm4,                  %%mm5\n"

            "punpcklwd      %%mm3,                  %%mm4\n"
            "punpckhwd      %%mm3,                  %%mm5\n"

            "movq           %%mm4,                  (%4,%5,2)\n"
            "movq           %%mm5,                  8(%4,%5,2)\n"

            "addl           $8,                     %2\n"
            "addl           $4,                     %0\n"
            "addl           $4,                     %1\n"
            "cmpl           %3,                     %6\n"
            "leal           16(%4),                 %4\n"
        "jl             1b\n"
        "addl           %3,     %2\n"                   /* lum += cols */
        "addl           %7,     %4\n"                   /* row1 += mod */
        "movl           $0,     %6\n"
        "cmpl           %8,     %2\n"
        "jl             1b\n"
        : : "r" (cr), 
			"r" (cb), 
			"r" (lum), 
			"r" (cols), 
			"r" (row1) ,
			"r" (col1), 
			"m" (x), 
			"m" (mod), 
			"m" (y)
		);
}

static unsigned long long  mpeg3_MMX_U_80 = 0x0000008000800000;
static unsigned long long  mpeg3_MMX_V_80 = 0x0000000000800080;
static long long  mpeg3_MMX_U_COEF        = 0x00000058ffd30000;
static long long  mpeg3_MMX_V_COEF        = 0x00000000ffea006f;
static long long  mpeg3_MMX_601_Y_COEF    = 0x0000004800480048;
static long long  mpeg3_MMX_601_Y_DIFF    = 0x0000000000000010;

inline void mpeg3_bgra32_mmx(unsigned long y, 
		unsigned long u, 
		unsigned long v, 
		unsigned long *output)
{
asm("
/* Output will be 0x00rrggbb with the 00 trailing so this can also be used */
/* for bgr24. */
	movd (%0), %%mm0;          /* Load y   0x00000000000000yy */
	movd (%1), %%mm1;          /* Load u    0x00000000000000cr */
	movq %%mm0, %%mm3;         /* Copy y to temp */
	psllq $16, %%mm1;          /* Shift u   0x0000000000cr0000 */
	movd (%2), %%mm2;          /* Load v    0x00000000000000cb */
	psllq $16, %%mm3;          /* Shift y */
	movq %%mm1, %%mm4;         /* Copy u to temp */
	por %%mm3, %%mm0;          /* Overlay new y byte 0x0000000000yy00yy */
	psllq $16, %%mm4;          /* Shift u */
	movq %%mm2, %%mm5;         /* Copy v to temp */
	psllq $16, %%mm3;          /* Shift y  */
	por %%mm4, %%mm1;          /* Overlay new u byte 0x000000cr00cr0000 */
	psllq $16, %%mm5;          /* Shift v  */
	por %%mm3, %%mm0;          /* Overlay new y byte 0x000000yy00yy00yy */
	por %%mm5, %%mm2;          /* Overlay new v byte 0x0000000000cb00cb */

/* mm0: 0x000000yy00yy00yy mm1: 0x000000uu00uu0000 mm2: 0x0000000000vv00vv */
 	psubw mpeg3_MMX_U_80, %%mm1;    /* Subtract 128 from u 0x000000uu00uu0000 */
 	pmullw mpeg3_MMX_U_COEF, %%mm1; /* Multiply u coeffs 0x0000uuuuuuuu0000 */
 	psllw $6, %%mm0;                /* Shift y coeffs 0x0000yyy0yyy0yyy0 */
 	psubw mpeg3_MMX_V_80, %%mm2;    /* Subtract 128 from v 0x0000000000cb00cb */
 	pmullw mpeg3_MMX_V_COEF, %%mm2; /* Multiply v coeffs 0x0000crcrcrcrcrcr */

/* mm0: 0x000000yy00yy00yy mm1: 0x0000uuuuuuuu0000 mm2: 0x00000000vvvvvvvv */
	paddsw %%mm1, %%mm0;        /* Add u to result */
	paddsw %%mm2, %%mm0;        /* Add v to result 0x0000rrrrggggbbbb */
	psraw $6, %%mm0;           /* Demote precision */
	packuswb %%mm0, %%mm0;     /* Pack into ARGB 0x0000000000rrggbb */
	movd %%mm0, (%3);          /* Store output */
	"
:
: "r" (&y), "r" (&u), "r" (&v), "r" (output));
}

inline void mpeg3_601_bgra32_mmx(unsigned long y, 
		unsigned long u, 
		unsigned long v, 
		unsigned long *output)
{
asm("
/* Output will be 0x00rrggbb with the 00 trailing so this can also be used */
/* for bgr24. */
	movd (%0), %%mm0;          /* Load y   0x00000000000000yy */
	psubsw mpeg3_MMX_601_Y_DIFF, %%mm0;      /* Subtract 16 from y */
	movd (%1), %%mm1;          /* Load u    0x00000000000000cr */
	movq %%mm0, %%mm3;         /* Copy y to temp */
	psllq $16, %%mm1;          /* Shift u   0x0000000000cr0000 */
	movd (%2), %%mm2;          /* Load v    0x00000000000000cb */
	psllq $16, %%mm3;          /* Shift y */
	movq %%mm1, %%mm4;         /* Copy u to temp */
	por %%mm3, %%mm0;          /* Overlay new y byte 0x0000000000yy00yy */
	psllq $16, %%mm4;          /* Shift u */
	movq %%mm2, %%mm5;         /* Copy v to temp */
	psllq $16, %%mm3;          /* Shift y  */
	por %%mm4, %%mm1;          /* Overlay new u byte 0x000000cr00cr0000 */
	psllq $16, %%mm5;          /* Shift v  */
	por %%mm3, %%mm0;          /* Overlay new y byte 0x000000yy00yy00yy */
	por %%mm5, %%mm2;          /* Overlay new v byte 0x0000000000cb00cb */

/* mm0: 0x000000yy00yy00yy mm1: 0x000000uu00uu0000 mm2: 0x0000000000vv00vv */
	pmullw mpeg3_MMX_601_Y_COEF, %%mm0; /* Scale and shift y coeffs */
	psubw mpeg3_MMX_U_80, %%mm1;     /* Subtract 128 from u 0x000000uu00uu0000 */
 	pmullw mpeg3_MMX_U_COEF, %%mm1;  /* Multiply u coeffs 0x0000uuuuuuuu0000 */
	psubw mpeg3_MMX_V_80, %%mm2;     /* Subtract 128 from v 0x0000000000cb00cb */
 	pmullw mpeg3_MMX_V_COEF, %%mm2;  /* Multiply v coeffs 0x0000crcrcrcrcrcr */

/* mm0: 0x000000yy00yy00yy mm1: 0x0000uuuuuuuu0000 mm2: 0x00000000vvvvvvvv */
	paddsw %%mm1, %%mm0;        /* Add u to result */
	paddsw %%mm2, %%mm0;        /* Add v to result 0x0000rrrrggggbbbb */
	psraw $6, %%mm0;           /* Demote precision */
	packuswb %%mm0, %%mm0;     /* Pack into ARGB 0x0000000000rrggbb */
	movd %%mm0, (%3);          /* Store output */
	"
:
: "r" (&y), "r" (&u), "r" (&v), "r" (output));
}

static unsigned long long  mpeg3_MMX_U_80_RGB    = 0x0000000000800080;
static unsigned long long  mpeg3_MMX_V_80_RGB    = 0x0000008000800000;
static long long  mpeg3_MMX_U_COEF_RGB    = 0x00000000ffd30058;
static long long  mpeg3_MMX_V_COEF_RGB    = 0x0000006fffea0000;

inline void mpeg3_rgba32_mmx(unsigned long y, 
		unsigned long u, 
		unsigned long v, 
		unsigned long *output)
{
asm("
/* Output will be 0x00bbggrr with the 00 trailing so this can also be used */
/* for rgb24. */
	movd (%0), %%mm0;          /* Load y   0x00000000000000yy */
	movd (%1), %%mm1;          /* Load v    0x00000000000000vv */
	movq %%mm0, %%mm3;         /* Copy y to temp */
	psllq $16, %%mm1;          /* Shift v   0x0000000000vv0000 */
	movd (%2), %%mm2;          /* Load u    0x00000000000000uu */
	psllq $16, %%mm3;          /* Shift y */
	movq %%mm1, %%mm4;         /* Copy v to temp */
	por %%mm3, %%mm0;          /* Overlay new y byte 0x0000000000yy00yy */
	psllq $16, %%mm4;          /* Shift v */
	movq %%mm2, %%mm5;         /* Copy u to temp */
	psllq $16, %%mm3;          /* Shift y  */
	por %%mm4, %%mm1;          /* Overlay new v byte 0x000000vv00vv0000 */
	psllq $16, %%mm5;          /* Shift u  */
	por %%mm3, %%mm0;          /* Overlay new y byte 0x000000yy00yy00yy */
	por %%mm5, %%mm2;          /* Overlay new u byte 0x0000000000uu00uu */

/* mm0: 0x000000yy00yy00yy mm1: 0x000000vv00vv0000 mm2: 0x0000000000uu00uu */
 	psubw mpeg3_MMX_V_80_RGB, %%mm1;    /* Subtract 128 from v 0x000000vv00vv0000 */
 	pmullw mpeg3_MMX_V_COEF_RGB, %%mm1; /* Multiply v coeffs 0x0000vvvvvvvv0000 */
 	psllw $6, %%mm0;                /* Shift y coeffs 0x0000yyy0yyy0yyy0 */
 	psubw mpeg3_MMX_U_80_RGB, %%mm2;    /* Subtract 128 from u 0x0000000000uu00uu */
 	pmullw mpeg3_MMX_U_COEF_RGB, %%mm2; /* Multiply u coeffs 0x0000uuuuuuuuuuuu */

/* mm0: 0x000000yy00yy00yy mm1: 0x0000vvvvvvvv0000 mm2: 0x00000000uuuuuuuu */
	paddsw %%mm1, %%mm0;        /* Add v to result */
	paddsw %%mm2, %%mm0;        /* Add u to result 0x0000bbbbggggrrrr */
	psraw $6, %%mm0;           /* Demote precision */
	packuswb %%mm0, %%mm0;     /* Pack into RGBA 0x0000000000bbggrr */
	movd %%mm0, (%3);          /* Store output */
	"
:
: "r" (&y), "r" (&v), "r" (&u), "r" (output));
}

inline void mpeg3_601_rgba32_mmx(unsigned long y, 
		unsigned long u, 
		unsigned long v, 
		unsigned long *output)
{
asm("
/* Output will be 0x00bbggrr with the 00 trailing so this can also be used */
/* for rgb24. */
	movd (%0), %%mm0;          /* Load y   0x00000000000000yy */
	psubsw mpeg3_MMX_601_Y_DIFF, %%mm0;      /* Subtract 16 from y */
	movd (%1), %%mm1;          /* Load v    0x00000000000000vv */
	movq %%mm0, %%mm3;         /* Copy y to temp */
	psllq $16, %%mm1;          /* Shift v   0x0000000000vv0000 */
	movd (%2), %%mm2;          /* Load u    0x00000000000000uu */
	psllq $16, %%mm3;          /* Shift y */
	movq %%mm1, %%mm4;         /* Copy v to temp */
	por %%mm3, %%mm0;          /* Overlay new y byte 0x0000000000yy00yy */
	psllq $16, %%mm4;          /* Shift v */
	movq %%mm2, %%mm5;         /* Copy u to temp */
	psllq $16, %%mm3;          /* Shift y  */
	por %%mm4, %%mm1;          /* Overlay new v byte 0x000000vv00vv0000 */
	psllq $16, %%mm5;          /* Shift u  */
	por %%mm3, %%mm0;          /* Overlay new y byte 0x000000yy00yy00yy */
	por %%mm5, %%mm2;          /* Overlay new u byte 0x0000000000uu00uu */

/* mm0: 0x000000yy00yy00yy     mm1: 0x000000vv00vv0000     mm2: 0x0000000000uu00uu */
	pmullw mpeg3_MMX_601_Y_COEF, %%mm0;     /* Scale y coeffs */
 	psubw mpeg3_MMX_V_80_RGB, %%mm1;    /* Subtract 128 from v 0x000000vv00vv0000 */
 	pmullw mpeg3_MMX_V_COEF_RGB, %%mm1; /* Multiply v coeffs 0x0000vvvvvvvv0000 */
 	psubw mpeg3_MMX_U_80_RGB, %%mm2;    /* Subtract 128 from u 0x0000000000uu00uu */
 	pmullw mpeg3_MMX_U_COEF_RGB, %%mm2; /* Multiply u coeffs 0x0000uuuuuuuuuuuu */

/* mm0: 0x000000yy00yy00yy mm1: 0x0000vvvvvvvv0000 mm2: 0x00000000uuuuuuuu */
	paddsw %%mm1, %%mm0;        /* Add v to result */
	paddsw %%mm2, %%mm0;        /* Add u to result 0x0000bbbbggggrrrr */
	psraw $6, %%mm0;           /* Demote precision */
	packuswb %%mm0, %%mm0;     /* Pack into RGBA 0x0000000000bbggrr */
	movd %%mm0, (%3);          /* Store output */
	"
:
: "r" (&y), "r" (&v), "r" (&u), "r" (output));
}

#endif

#define DITHER_ROW_HEAD \
	for(h = 0; h < video->out_h; h++) \
	{ \
		y_in = &src[0][(video->y_table[h] + video->in_y) * video->coded_picture_width] + video->in_x; \
		cb_in = &src[1][((video->y_table[h] + video->in_y) >> 1) * video->chrom_width] + (video->in_x >> 2); \
		cr_in = &src[2][((video->y_table[h] + video->in_y) >> 1) * video->chrom_width] + (video->in_x >> 1); \
		data = output_rows[h];

#define DITHER_ROW_TAIL \
	}

#define DITHER_SCALE_HEAD \
	for(w = 0; w < video->out_w; w++) \
	{ \
		uv_subscript = video->x_table[w] / 2; \
		y_l = y_in[video->x_table[w]]; \
		y_l <<= 16; \
		r_l = (y_l + video->cr_to_r[cr_in[uv_subscript]]) >> 16; \
		g_l = (y_l + video->cr_to_g[cr_in[uv_subscript]] + video->cb_to_g[cb_in[uv_subscript]]) >> 16; \
		b_l = (y_l + video->cb_to_b[cb_in[uv_subscript]]) >> 16;

#define DITHER_SCALE_601_HEAD \
	for(w = 0; w < video->out_w; w++) \
	{ \
		uv_subscript = video->x_table[w] / 2; \
		y_l = mpeg3_601_to_rgb[y_in[video->x_table[w]]]; \
		y_l <<= 16; \
		r_l = (y_l + video->cr_to_r[cr_in[uv_subscript]]) >> 16; \
		g_l = (y_l + video->cr_to_g[cr_in[uv_subscript]] + video->cb_to_g[cb_in[uv_subscript]]) >> 16; \
		b_l = (y_l + video->cb_to_b[cb_in[uv_subscript]]) >> 16;

#define DITHER_SCALE_TAIL \
	}

#define DITHER_MMX_SCALE_HEAD \
	for(w = 0; w < video->out_w; w++) \
	{ \
		uv_subscript = video->x_table[w] / 2;

#define DITHER_MMX_SCALE_TAIL \
		data += step; \
	}

#define DITHER_MMX_HEAD \
	for(w = 0; w < video->out_w; w += 2) \
	{

#define DITHER_MMX_TAIL \
		data += step; \
        cr_in++; \
        cb_in++; \
	}

#define DITHER_HEAD \
    for(w = 0; w < video->horizontal_size; w++) \
	{ \
		y_l = *y_in++; \
		y_l <<= 16; \
		r_l = (y_l + video->cr_to_r[*cr_in]) >> 16; \
		g_l = (y_l + video->cr_to_g[*cr_in] + video->cb_to_g[*cb_in]) >> 16; \
		b_l = (y_l + video->cb_to_b[*cb_in]) >> 16;

#define DITHER_601_HEAD \
    for(w = 0; w < video->horizontal_size; w++) \
	{ \
		y_l = mpeg3_601_to_rgb[*y_in++]; \
		y_l <<= 16; \
		r_l = (y_l + video->cr_to_r[*cr_in]) >> 16; \
		g_l = (y_l + video->cr_to_g[*cr_in] + video->cb_to_g[*cb_in]) >> 16; \
		b_l = (y_l + video->cb_to_b[*cb_in]) >> 16;

#define DITHER_TAIL \
    	if(w & 1) \
		{ \
        	cr_in++; \
        	cb_in++; \
    	} \
    }


#define STORE_PIXEL_BGR888 \
	*data++ = CLIP(b_l); \
	*data++ = CLIP(g_l); \
	*data++ = CLIP(r_l);

#define STORE_PIXEL_BGRA8888 \
	*data++ = CLIP(b_l); \
	*data++ = CLIP(g_l); \
	*data++ = CLIP(r_l); \
	*data++ = 0;

#define STORE_PIXEL_RGB565 \
	*((unsigned short*)data)++ = \
		((CLIP(r_l) & 0xf8) << 8) | \
		((CLIP(g_l) & 0xfc) << 3) | \
		((CLIP(b_l) & 0xf8) >> 3);

#define STORE_PIXEL_RGB888 \
	*data++ = CLIP(r_l); \
	*data++ = CLIP(g_l); \
	*data++ = CLIP(b_l);

#define STORE_PIXEL_RGBA8888 \
	*data++ = CLIP(r_l); \
	*data++ = CLIP(g_l); \
	*data++ = CLIP(b_l); \
	*data++ = 0;

#define STORE_PIXEL_RGBA16161616 \
	*data_s++ = CLIP(r_l); \
	*data_s++ = CLIP(g_l); \
	*data_s++ = CLIP(b_l); \
	*data_s++ = 0;



/* Only good for YUV 4:2:0 */
int mpeg3video_ditherframe(mpeg3video_t *video, unsigned char **src, unsigned char **output_rows)
{
	int h = 0;
	register unsigned char *y_in, *cb_in, *cr_in;
	long y_l, r_l, b_l, g_l;
	register unsigned char *data;
	register int uv_subscript, step, w = -1;

#ifdef HAVE_MMX
/* =================================== MMX ===================================== */
	if(video->have_mmx &&
		video->out_w == video->horizontal_size &&
		video->out_h == video->vertical_size &&
		video->in_w == video->out_w &&
		video->in_h == video->out_h &&
		video->in_x == 0 &&
		video->in_y == 0 &&
		(video->color_model == MPEG3_RGB565 || video->color_model == MPEG3_601_RGB565))
	{
/* Unscaled 16 bit */
		mpeg3video_rgb16_mmx(src[0], 
			src[2], 
			src[1], 
			output_rows[0], 
			video->out_h, 
			video->out_w, 
			(output_rows[1] - output_rows[0]) / 2 - video->out_w);
	}
	else
	if(video->have_mmx && 
		(video->color_model == MPEG3_BGRA8888 || 
		video->color_model == MPEG3_BGR888 ||
/*		video->color_model == MPEG3_RGB888 || */
		video->color_model == MPEG3_RGBA8888 ||
		video->color_model == MPEG3_601_BGR888 ||
		video->color_model == MPEG3_601_BGRA8888 ||
		video->color_model == MPEG3_601_RGB888 ||
		video->color_model == MPEG3_601_RGBA8888))
	{
/* Original MMX */
		if(video->color_model == MPEG3_BGRA8888 ||
			video->color_model == MPEG3_RGBA8888 ||
			video->color_model == MPEG3_601_BGRA8888 ||
			video->color_model == MPEG3_601_RGBA8888) step = 4;
		else
		if(video->color_model == MPEG3_BGR888 ||
			video->color_model == MPEG3_RGB888 ||
			video->color_model == MPEG3_601_BGR888 ||
			video->color_model == MPEG3_601_RGB888) step = 3;

		DITHER_ROW_HEAD
/* Transfer row with scaling */
			if(video->out_w != video->horizontal_size)
			{
				switch(video->color_model)
				{
					case MPEG3_BGRA8888:
					case MPEG3_BGR888:
						DITHER_MMX_SCALE_HEAD
							mpeg3_bgra32_mmx(y_in[video->x_table[w]], 
								cr_in[uv_subscript], 
								cb_in[uv_subscript], 
								(unsigned long*)data);
						DITHER_MMX_SCALE_TAIL
						break;
					
					case MPEG3_601_BGRA8888:
					case MPEG3_601_BGR888:
						DITHER_MMX_SCALE_HEAD
							mpeg3_601_bgra32_mmx(y_in[video->x_table[w]], 
								cr_in[uv_subscript], 
								cb_in[uv_subscript], 
								(unsigned long*)data);
						DITHER_MMX_SCALE_TAIL
						break;

					case MPEG3_RGBA8888:
					case MPEG3_RGB888:
						DITHER_MMX_SCALE_HEAD
							mpeg3_rgba32_mmx(y_in[video->x_table[w]], 
								cr_in[uv_subscript], 
								cb_in[uv_subscript], 
								(unsigned long*)data);
						DITHER_MMX_SCALE_TAIL
						break;

					case MPEG3_601_RGBA8888:
					case MPEG3_601_RGB888:
						DITHER_MMX_SCALE_HEAD
							mpeg3_601_rgba32_mmx(y_in[video->x_table[w]], 
								cr_in[uv_subscript], 
								cb_in[uv_subscript], 
								(unsigned long*)data);
						DITHER_MMX_SCALE_TAIL
						break;
				}
			}
			else
/* Transfer row unscaled */
			{
				switch(video->color_model)
				{
/* MMX byte swap 24 and 32 bit */
					case MPEG3_BGRA8888:
					case MPEG3_BGR888:
						DITHER_MMX_HEAD
							mpeg3_bgra32_mmx(*y_in++, 
								*cr_in, 
								*cb_in, 
								(unsigned long*)data);
							data += step;
							mpeg3_bgra32_mmx(*y_in++, 
								*cr_in, 
								*cb_in, 
								(unsigned long*)data);
						DITHER_MMX_TAIL
						break;

/* MMX 601 byte swap 24 and 32 bit */
					case MPEG3_601_BGRA8888:
					case MPEG3_601_BGR888:
						DITHER_MMX_HEAD
							mpeg3_601_bgra32_mmx(*y_in++, 
								*cr_in, 
								*cb_in, 
								(unsigned long*)data);
							data += step;
							mpeg3_601_bgra32_mmx(*y_in++, 
								*cr_in, 
								*cb_in, 
								(unsigned long*)data);
						DITHER_MMX_TAIL
						break;

/* MMX 24 and 32 bit no byte swap */
					case MPEG3_RGBA8888:
					case MPEG3_RGB888:
						DITHER_MMX_HEAD
							mpeg3_rgba32_mmx(*y_in++, 
								*cr_in, 
								*cb_in, 
								(unsigned long*)data);
							data += step;
							mpeg3_rgba32_mmx(*y_in++, 
								*cr_in, 
								*cb_in, 
								(unsigned long*)data);
						DITHER_MMX_TAIL
						break;

/* MMX 601 24 and 32 bit no byte swap */
					case MPEG3_601_RGBA8888:
					case MPEG3_601_RGB888:
						DITHER_MMX_HEAD
							mpeg3_601_rgba32_mmx(*y_in++, 
								*cr_in, 
								*cb_in, 
								(unsigned long*)data);
							data += step;
							mpeg3_601_rgba32_mmx(*y_in++, 
								*cr_in, 
								*cb_in, 
								(unsigned long*)data);
						DITHER_MMX_TAIL
						break;
				}
			}
		DITHER_ROW_TAIL
	}
	else
#endif
/* ================================== NO MMX ==================================== */
	{
		DITHER_ROW_HEAD
/* Transfer row with scaling */
			if(video->out_w != video->horizontal_size)
			{
				switch(video->color_model)
				{
					case MPEG3_BGR888:
						DITHER_SCALE_HEAD
						STORE_PIXEL_BGR888
						DITHER_SCALE_TAIL
						break;
					case MPEG3_BGRA8888:
						DITHER_SCALE_HEAD
						STORE_PIXEL_BGRA8888
						DITHER_SCALE_TAIL
						break;
					case MPEG3_RGB565:
						DITHER_SCALE_HEAD
						STORE_PIXEL_RGB565
						DITHER_SCALE_TAIL
						break;
					case MPEG3_RGB888:
						DITHER_SCALE_HEAD
						STORE_PIXEL_RGB888
						DITHER_SCALE_TAIL
						break;
					case MPEG3_RGBA8888:
						DITHER_SCALE_HEAD
						STORE_PIXEL_RGBA8888
						DITHER_SCALE_TAIL
						break;
					case MPEG3_601_BGR888:
						DITHER_SCALE_601_HEAD
						STORE_PIXEL_BGR888
						DITHER_SCALE_TAIL
						break;
					case MPEG3_601_BGRA8888:
						DITHER_SCALE_601_HEAD
						STORE_PIXEL_BGRA8888
						DITHER_SCALE_TAIL
						break;
					case MPEG3_601_RGB565:
						DITHER_SCALE_601_HEAD
						STORE_PIXEL_RGB565
						DITHER_SCALE_TAIL
						break;
					case MPEG3_601_RGB888:
						DITHER_SCALE_601_HEAD
						STORE_PIXEL_RGB888
						DITHER_SCALE_TAIL
						break;
					case MPEG3_601_RGBA8888:
						DITHER_SCALE_601_HEAD
						STORE_PIXEL_RGBA8888
						DITHER_SCALE_TAIL
						break;
					case MPEG3_RGBA16161616:
					{
						register unsigned short *data_s = (unsigned short*)data;
						DITHER_SCALE_HEAD
						STORE_PIXEL_RGBA16161616
						DITHER_SCALE_TAIL
					}
						break;
				}
			}
			else
			{
/* Transfer row unscaled */
				switch(video->color_model)
				{
					case MPEG3_BGR888:
						DITHER_HEAD
						STORE_PIXEL_BGR888
						DITHER_TAIL
						break;
					case MPEG3_BGRA8888:
						DITHER_HEAD
						STORE_PIXEL_BGRA8888
						DITHER_TAIL
						break;
					case MPEG3_RGB565:
						DITHER_HEAD
						STORE_PIXEL_RGB565
						DITHER_TAIL
						break;
					case MPEG3_RGB888:
						DITHER_HEAD
						STORE_PIXEL_RGB888
						DITHER_TAIL
						break;
					case MPEG3_RGBA8888:
						DITHER_HEAD
						STORE_PIXEL_RGBA8888
						DITHER_TAIL
						break;
					case MPEG3_601_BGR888:
						DITHER_601_HEAD
						STORE_PIXEL_BGR888
						DITHER_TAIL
						break;
					case MPEG3_601_BGRA8888:
						DITHER_601_HEAD
						STORE_PIXEL_BGRA8888
						DITHER_TAIL
						break;
					case MPEG3_601_RGB565:
						DITHER_601_HEAD
						STORE_PIXEL_RGB565
						DITHER_TAIL
						break;
					case MPEG3_601_RGB888:
						DITHER_601_HEAD
						STORE_PIXEL_RGB888
						DITHER_TAIL
						break;
					case MPEG3_601_RGBA8888:
						DITHER_601_HEAD
						STORE_PIXEL_RGBA8888
						DITHER_TAIL
						break;
					case MPEG3_RGBA16161616:
					{
						register unsigned short *data_s = (unsigned short*)data;
						DITHER_HEAD
						STORE_PIXEL_RGBA16161616
						DITHER_TAIL
					}
						break;
				}
			}
		DITHER_ROW_TAIL
	} /* End of non-MMX */

#ifdef HAVE_MMX
	if(video->have_mmx)
		__asm__ __volatile__ ("emms");
#endif
	return 0;
}

int mpeg3video_ditherframe444(mpeg3video_t *video, unsigned char *src[])
{
	return 0;
}

int mpeg3video_dithertop(mpeg3video_t *video, unsigned char *src[])
{
	return mpeg3video_ditherframe(video, src, video->output_rows);
}

int mpeg3video_dithertop444(mpeg3video_t *video, unsigned char *src[])
{
	return 0;
}

int mpeg3video_ditherbot(mpeg3video_t *video, unsigned char *src[])
{
	return 0;
}

int mpeg3video_ditherbot444(mpeg3video_t *video, unsigned char *src[])
{
	return 0;
}

void memcpy_fast(unsigned char *output, unsigned char *input, long len)
{
	int i, len2;
/* 8 byte alignment */
/*
 * 	if(!((long)input & 0x7))
 * 	{
 * 		len2 = len >> 4;
 * 		for(i = 0; i < len2; )
 * 		{
 * 			((int64_t*)output)[i] = ((int64_t*)input)[i];
 * 			i++;
 * 			((int64_t*)output)[i] = ((int64_t*)input)[i];
 * 			i++;
 * 		}
 * 
 * 		for(i *= 16; i < len; i++)
 * 		{
 * 			output[i] = input[i];
 * 		}
 * 	}
 * 	else
 */
		memcpy(output, input, len);
}

int mpeg3video_init_output()
{
	int i, value;
	for(i = 0; i < 256; i++)
	{
		value = (int)(1.1644 * i - 255 * 0.0627 + 0.5);
		if(value < 0) value = 0;
		else
		if(value > 255) value = 255;
		mpeg3_601_to_rgb[i] = value;
	}
	return 0;
}

int mpeg3video_present_frame(mpeg3video_t *video)
{
	int i, j, k, l;
	unsigned char **src = video->output_src;

/* Copy YUV buffers */
	if(video->want_yvu)
	{
		long size[2];
		long offset[2];

/* Drop a frame */
		if(!video->y_output) return 0;

/* Copy a frame */
		if(video->in_x == 0 && 
			video->in_w >= video->coded_picture_width)
		{
			size[0] = video->coded_picture_width * video->in_h;
			size[1] = video->chrom_width * (int)((float)video->in_h / 2 + 0.5);
			offset[0] = video->coded_picture_width * video->in_y;
			offset[1] = video->chrom_width * (int)((float)video->in_y / 2 + 0.5);

/*
 * 			if(video->in_y > 0)
 * 			{
 * 				offset[1] += video->chrom_width / 2;
 * 				size[1] += video->chrom_width / 2;
 * 			}
 */

			memcpy(video->y_output, src[0] + offset[0], size[0]);
			memcpy(video->u_output, src[1] + offset[1], size[1]);
			memcpy(video->v_output, src[2] + offset[1], size[1]);
		}
		else
		{
			for(i = 0, j = video->in_y; i < video->in_h; i++, j++)
			{
				memcpy(video->y_output + i * video->in_w, 
					src[0] + j * video->coded_picture_width + video->in_x, 
					video->in_w);
				memcpy(video->u_output + i * video->in_w / 4, 
					src[1] + j * video->chrom_width / 2 + video->in_x / 4, 
					video->in_w / 4);
				memcpy(video->v_output + i * video->in_w / 4, 
					src[2] + j * video->chrom_width / 2 + video->in_x / 4, 
					video->in_w / 4);
			}
		}

		return 0;
	}

/* Want RGB buffer */
/* Copy the frame to the output with YUV to RGB conversion */
  	if(video->prog_seq)
	{
    	if(video->chroma_format != CHROMA444)
		{
    		mpeg3video_ditherframe(video, src, video->output_rows);
    	}
    	else
    	  	mpeg3video_ditherframe444(video, src);
  	}
	else
	{
   		if((video->pict_struct == FRAME_PICTURE && video->topfirst) || 
			video->pict_struct == BOTTOM_FIELD)
		{
/* top field first */
    		if(video->chroma_format != CHROMA444)
			{
        		mpeg3video_dithertop(video, src);
        		mpeg3video_ditherbot(video, src);
    		}
    		else 
			{
        		mpeg3video_dithertop444(video, src);
        		mpeg3video_ditherbot444(video, src);
    		}
    	}
    	else 
		{
/* bottom field first */
    		if(video->chroma_format != CHROMA444)
			{
        		mpeg3video_ditherbot(video, src);
        		mpeg3video_dithertop(video, src);
    		}
    		else 
			{
        		mpeg3video_ditherbot444(video, src);
        		mpeg3video_dithertop444(video, src);
    		}
    	}
  	}
	return 0;
}

int mpeg3video_display_second_field(mpeg3video_t *video)
{
/* Not used */
	return 0;
}
