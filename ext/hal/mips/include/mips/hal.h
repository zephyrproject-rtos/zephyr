/*
 * Copyright 2014-2015, Imagination Technologies Limited and/or its
 *                      affiliated group companies.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _HAL_H
#define _HAL_H

#include <mips/asm.h>
#include <mips/m32c0.h>

#if _MIPS_SIM == _ABIO32
#define UHI_ABI 0
#elif _MIPS_SIM == _ABIN32
#define UHI_ABI 1
#elif _MIPS_SIM == _ABI64
#define UHI_ABI 2
#else
#error "UHI context structure is not defined for current ABI"
#endif

#define CTX_REG(REGNO)	((SZREG)*((REGNO)-1))

#define CTX_AT		((SZREG)*0)
#define CTX_V0		((SZREG)*1)
#define CTX_V1		((SZREG)*2)
#define CTX_A0		((SZREG)*3)
#define CTX_A1		((SZREG)*4)
#define CTX_A2		((SZREG)*5)
#define CTX_A3		((SZREG)*6)
#if _MIPS_SIM==_ABIN32 || _MIPS_SIM==_ABI64 || _MIPS_SIM==_ABIEABI
# define CTX_A4		((SZREG)*7)
# define CTX_A5		((SZREG)*8)
# define CTX_A6		((SZREG)*9)
# define CTX_A7		((SZREG)*10)
# define CTX_T0		((SZREG)*11)
# define CTX_T1		((SZREG)*12)
# define CTX_T2		((SZREG)*13)
# define CTX_T3		((SZREG)*14)
#else
# define CTX_T0		((SZREG)*7)
# define CTX_T1		((SZREG)*8)
# define CTX_T2		((SZREG)*9)
# define CTX_T3		((SZREG)*10)
# define CTX_T4		((SZREG)*11)
# define CTX_T5		((SZREG)*12)
# define CTX_T6		((SZREG)*13)
# define CTX_T7		((SZREG)*14)
#endif
#define CTX_S0		((SZREG)*15)
#define CTX_S1		((SZREG)*16)
#define CTX_S2		((SZREG)*17)
#define CTX_S3		((SZREG)*18)
#define CTX_S4		((SZREG)*19)
#define CTX_S5		((SZREG)*20)
#define CTX_S6		((SZREG)*21)
#define CTX_S7		((SZREG)*22)
#define CTX_T8		((SZREG)*23)
#define CTX_T9		((SZREG)*24)
#define CTX_K0		((SZREG)*25)
#define CTX_K1		((SZREG)*26)
#define CTX_GP		((SZREG)*27)
#define CTX_SP		((SZREG)*28)
#define CTX_FP		((SZREG)*29)
#define CTX_RA		((SZREG)*30)
#define CTX_EPC		((SZREG)*31)
#define CTX_BADVADDR	((SZREG)*32)
#define CTX_HI0		((SZREG)*33)
#define CTX_LO0		((SZREG)*34)
#define CTX_LINK	((SZREG)*35)
#define CTX_STATUS	(((SZREG)*35)+SZPTR)
#define CTX_CAUSE	(((SZREG)*35)+SZPTR+4)
#define CTX_BADINSTR	(((SZREG)*35)+SZPTR+8)
#define CTX_BADPINSTR	(((SZREG)*35)+SZPTR+12)
#define CTX_SIZE	(((SZREG)*35)+SZPTR+16)

#define DSPCTX_DSPC ((SZREG)*2)
#define DSPCTX_HI1  ((SZREG)*3)
#define DSPCTX_HI2  ((SZREG)*4)
#define DSPCTX_HI3  ((SZREG)*5)
#define DSPCTX_LO1  ((SZREG)*6)
#define DSPCTX_LO2  ((SZREG)*7)
#define DSPCTX_LO3  ((SZREG)*8)
#define DSPCTX_SIZE ((SZREG)*9)

#define FP32CTX_CSR	((SZREG)*2)
#define FP64CTX_CSR	((SZREG)*2)
#define MSACTX_FCSR	((SZREG)*2)
#define MSACTX_MSACSR	((SZREG)*3)

#define MSACTX_0	((SZREG)*4)
#define MSACTX_1	(MSACTX_0 + (1 * 16))
#define MSACTX_2	(MSACTX_0 + (2 * 16))
#define MSACTX_3	(MSACTX_0 + (3 * 16))
#define MSACTX_4	(MSACTX_0 + (4 * 16))
#define MSACTX_5	(MSACTX_0 + (5 * 16))
#define MSACTX_6	(MSACTX_0 + (6 * 16))
#define MSACTX_7	(MSACTX_0 + (7 * 16))
#define MSACTX_8	(MSACTX_0 + (8 * 16))
#define MSACTX_9	(MSACTX_0 + (9 * 16))
#define MSACTX_10	(MSACTX_0 + (10 * 16))
#define MSACTX_11	(MSACTX_0 + (11 * 16))
#define MSACTX_12	(MSACTX_0 + (12 * 16))
#define MSACTX_13	(MSACTX_0 + (13 * 16))
#define MSACTX_14	(MSACTX_0 + (14 * 16))
#define MSACTX_15	(MSACTX_0 + (15 * 16))
#define MSACTX_16	(MSACTX_0 + (16 * 16))
#define MSACTX_17	(MSACTX_0 + (17 * 16))
#define MSACTX_18	(MSACTX_0 + (18 * 16))
#define MSACTX_19	(MSACTX_0 + (19 * 16))
#define MSACTX_20	(MSACTX_0 + (20 * 16))
#define MSACTX_21	(MSACTX_0 + (21 * 16))
#define MSACTX_22	(MSACTX_0 + (22 * 16))
#define MSACTX_23	(MSACTX_0 + (23 * 16))
#define MSACTX_24	(MSACTX_0 + (24 * 16))
#define MSACTX_25	(MSACTX_0 + (25 * 16))
#define MSACTX_26	(MSACTX_0 + (26 * 16))
#define MSACTX_27	(MSACTX_0 + (27 * 16))
#define MSACTX_28	(MSACTX_0 + (28 * 16))
#define MSACTX_29	(MSACTX_0 + (29 * 16))
#define MSACTX_30	(MSACTX_0 + (30 * 16))
#define MSACTX_31	(MSACTX_0 + (31 * 16))
#define MSACTX_SIZE	(MSACTX_0 + (32 * 16))

#define FP32CTX_0	((SZREG)*4)
#define FP32CTX_2	(FP32CTX_0 + (1 * 8))
#define FP32CTX_4	(FP32CTX_0 + (2 * 8))
#define FP32CTX_6	(FP32CTX_0 + (3 * 8))
#define FP32CTX_8	(FP32CTX_0 + (4 * 8))
#define FP32CTX_10	(FP32CTX_0 + (5 * 8))
#define FP32CTX_12	(FP32CTX_0 + (6 * 8))
#define FP32CTX_14	(FP32CTX_0 + (7 * 8))
#define FP32CTX_16	(FP32CTX_0 + (8 * 8))
#define FP32CTX_18	(FP32CTX_0 + (9 * 8))
#define FP32CTX_20	(FP32CTX_0 + (10 * 8))
#define FP32CTX_22	(FP32CTX_0 + (11 * 8))
#define FP32CTX_24	(FP32CTX_0 + (12 * 8))
#define FP32CTX_26	(FP32CTX_0 + (13 * 8))
#define FP32CTX_28	(FP32CTX_0 + (14 * 8))
#define FP32CTX_30	(FP32CTX_0 + (15 * 8))
#define FP32CTX_SIZE	(FP32CTX_30 + (17 * 8))

#define FP64CTX_0	((SZREG)*4)
#define FP64CTX_2	(FP64CTX_0 + (1 * 8))
#define FP64CTX_4	(FP64CTX_0 + (2 * 8))
#define FP64CTX_6	(FP64CTX_0 + (3 * 8))
#define FP64CTX_8	(FP64CTX_0 + (4 * 8))
#define FP64CTX_10	(FP64CTX_0 + (5 * 8))
#define FP64CTX_12	(FP64CTX_0 + (6 * 8))
#define FP64CTX_14	(FP64CTX_0 + (7 * 8))
#define FP64CTX_16	(FP64CTX_0 + (8 * 8))
#define FP64CTX_18	(FP64CTX_0 + (9 * 8))
#define FP64CTX_20	(FP64CTX_0 + (10 * 8))
#define FP64CTX_22	(FP64CTX_0 + (11 * 8))
#define FP64CTX_24	(FP64CTX_0 + (12 * 8))
#define FP64CTX_26	(FP64CTX_0 + (13 * 8))
#define FP64CTX_28	(FP64CTX_0 + (14 * 8))
#define FP64CTX_30	(FP64CTX_0 + (15 * 8))
#define FP64CTX_1	(FP64CTX_30 + (1 * 8))
#define FP64CTX_3	(FP64CTX_30 + (2 * 8))
#define FP64CTX_5	(FP64CTX_30 + (3 * 8))
#define FP64CTX_7	(FP64CTX_30 + (4 * 8))
#define FP64CTX_9	(FP64CTX_30 + (5 * 8))
#define FP64CTX_11	(FP64CTX_30 + (6 * 8))
#define FP64CTX_13	(FP64CTX_30 + (7 * 8))
#define FP64CTX_15	(FP64CTX_30 + (8 * 8))
#define FP64CTX_17	(FP64CTX_30 + (9 * 8))
#define FP64CTX_19	(FP64CTX_30 + (10 * 8))
#define FP64CTX_21	(FP64CTX_30 + (11 * 8))
#define FP64CTX_23	(FP64CTX_30 + (12 * 8))
#define FP64CTX_25	(FP64CTX_30 + (13 * 8))
#define FP64CTX_27	(FP64CTX_30 + (14 * 8))
#define FP64CTX_29	(FP64CTX_30 + (15 * 8))
#define FP64CTX_31	(FP64CTX_30 + (16 * 8))
#define FP64CTX_SIZE	(FP64CTX_31 + (17 * 8))

#define FPCTX_SIZE()	(mips_getsr() & SR_FR ? FP64CTX_SIZE : FP32CTX_SIZE)

#define XPACTX_BADVADDR	((SZREG)*2)

#define LINKCTX_TYPE_MSA        0x004D5341
#define LINKCTX_TYPE_FP32       0x46503332
#define LINKCTX_TYPE_FP64       0x46503634
#define LINKCTX_TYPE_FMSA       0x463D5341
#define LINKCTX_TYPE_DSP        0x00445350
#define LINKCTX_TYPE_STKSWP     0x53574150
#define LINKCTX_TYPE_XPA	0x00585041

#define LINKCTX_ID      ((SZREG)*0)
#define LINKCTX_NEXT    ((SZREG)*1)

#define LINKCTX_TYPE(X) (((struct linkctx *)(X))->id)

#define SWAPCTX_OLDSP   ((SZREG)*2)
#define SWAPCTX_SIZE    ((SZREG)*3)

#ifndef __ASSEMBLER__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct linkctx;

struct gpctx
{
  union
  {
    struct
    {
      reg_t at;
      reg_t v[2];
# if _MIPS_SIM==_ABIN32 || _MIPS_SIM==_ABI64 || _MIPS_SIM==_ABIEABI
      reg_t a[8];
      reg_t t[4];
# else
      reg_t a[4];
      reg_t t[8];
# endif
      reg_t s[8];
      reg_t t2[2];
      reg_t k[2];
      reg_t gp;
      reg_t sp;
      reg_t fp;
      reg_t ra;
    };
    reg_t r[31];
  };

  reg_t epc;
  reg_t badvaddr;
  reg_t hi;
  reg_t lo;
  /* This field is for future extension */
  struct linkctx *link;
  /* Status at the point of the exception.  This may not be restored
     on return from exception if running under an RTOS */
  uint32_t status;
  /* These fields should be considered read-only */
  uint32_t cause;
  uint32_t badinstr;
  uint32_t badpinstr;
};

struct linkctx
{
  reg_t id;
  struct linkctx *next;
};

struct swapctx
{
  struct linkctx link;
  reg_t oldsp;
};

static inline void
_linkctx_append (struct gpctx *gp, struct linkctx *nu)
{
  struct linkctx **ctx = (struct linkctx **)&gp->link;
  while (*ctx)
    ctx = &(*ctx)->next;
  *ctx = nu;
}

struct dspctx
{
  struct linkctx link;
  reg_t dspc;
  reg_t hi[3];
  reg_t lo[3];
};

struct fpctx
{
  struct linkctx link;
  reg_t fcsr;
  reg_t reserved;
};

typedef char _msareg[16] __attribute__ ((aligned(16)));

struct msactx
{
  struct linkctx link;
  reg_t fcsr;
  reg_t msacsr;
  union
    {
      _msareg w[32];
      double d[64];
      float s[128];
    };
};

#define MSAMSACTX_D(CTX, N) (CTX)->w[(N)]
#define MSACTX_DBL(CTX, N) (CTX)->d[(N) << 1]
#ifdef __MIPSEL__
#define MSACTX_SGL(CTX, N) (CTX)->s[(N) << 2]
#else
#define MSACTX_SGL(CTX, N) (CTX)->s[((N) << 2) | 1]
#endif

struct fp32ctx
{
  struct fpctx fp;
  union
    {
      double d[16];	/* even doubles */
      float s[32];	/* even singles, padded */
    };
};

#define FP32CTX_DBL(CTX, N) (CTX)->d[(N)]
#ifdef __MIPSEL__
#define FP32CTX_SGL(CTX, N) (CTX)->s[(N)]
#else
#define FP32CTX_SGL(CTX, N) (CTX)->s[(N) ^ 1]
#endif

struct fp64ctx
{
  struct fpctx fp;
  union
    {
      double d[32];	/* even doubles, followed by odd doubles */
      float s[64];	/* even singles, followed by odd singles, padded */
    };
};

#define FP64CTX_DBL(CTX, N) (CTX)->d[((N) >> 1) + (((N) & 1) << 4)]
#ifdef __MIPSEL__
#define FP64CTX_SGL(CTX, N) (CTX)->s[((N) & ~1) + (((N) & 1) << 5)]
#else
#define FP64CTX_SGL(CTX, N) (CTX)->s[((N) | 1) + (((N) & 1) << 5)]
#endif

struct xpactx
{
  struct linkctx link;
  reg64_t badvaddr;
};

extern reg_t __attribute__((nomips16))
  _xpa_save (struct xpactx *ptr);

extern reg_t __attribute__((nomips16))
  _fpctx_save (struct fpctx *ptr);
extern reg_t __attribute__((nomips16))
  _fpctx_load (struct fpctx *ptr);
extern reg_t __attribute__((nomips16))
  _msactx_save (struct msactx *ptr);
extern reg_t __attribute__((nomips16))
  _msactx_load (struct msactx *ptr);

/* Fall back exception handlers:
   _mips_handle_exception - May be implemented by a user but is aliased
			    to __exception_handle by default.
   __exception_handle	  - Toolchain provided fallback handler.
*/
extern void __attribute__((nomips16))
  _mips_handle_exception (struct gpctx *ctx, int exception);
extern void __attribute__((nomips16))
  __exception_handle (struct gpctx *ctx, int exception);

/* Obtain the largest available region of RAM */
extern void __attribute__((nomips16))
  _get_ram_range (void **ram_base, void **ram_extent);

/* Invoke a UHI operation via SDBBP using the provided context */
extern int __attribute__((nomips16))
  __uhi_indirect (struct gpctx *);

/* Report an unhandled exception */
extern int32_t __attribute__((nomips16))
  __uhi_exception (struct gpctx *, int32_t);

/* Print a message to a logger.  Minimal formatting support for one
   integer argument.  */
extern int32_t __attribute__((nomips16))
  __plog (int8_t *, int32_t);

/* Boot context support functions */
extern int __attribute__((nomips16))
  __get_startup_BEV (void) __attribute__((weak));
extern int __attribute__((nomips16))
  __chain_uhi_excpt (struct gpctx *) __attribute__((weak));

/* Emergency exit, use it when unrecoverable errors occur */
extern int __attribute__((nomips16))
  __exit (int);

/* UHI assert support.  This function can return.  */
extern void __attribute__((nomips16))
  __uhi_assert (const char *, const char *, int32_t);

#ifdef NDEBUG           /* required by ANSI standard */
# define uhi_assert(__e) ((void)0)
#else
# define uhi_assert(__e) ((__e) ? (void)0 : \
			   __uhi_assert (#__e, __FILE__, __LINE__))
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif // _HAL_H
