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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mips/cpu.h>
#include <mips/fpa.h>
#include <mips/hal.h>
#include <mips/uhi_syscalls.h>

/* Defined in .ld file */
extern char __use_excpt_boot[];
extern char __attribute__((weak)) __flush_to_zero[];

#ifdef VERBOSE_EXCEPTIONS
/*
 * Write a string, a formatted number, then a string.
 */
static void
putsnds (const char *pre, reg_t value, int digits, const char *post)
{
  char buf[digits];
  int shift;
  int idx = 0;

  if (pre != NULL)
    write (1, pre, strlen (pre));

  for (shift = ((digits - 1) * 4) ; shift >= 0 ; shift -= 4)
    buf[idx++] = "0123456789ABCDEF"[(value >> shift) & 0xf];
  write (1, buf, digits);

  if (post != NULL)
    write (1, post, strlen (post));
}

static void
putsns (const char *pre, reg_t value, const char *post)
{
  putsnds (pre, value, sizeof (reg_t) * 2, post);
}

# define WRITE(MSG) write (1, (MSG), strlen (MSG))
# define PUTSNDS(PRE, VALUE, DIGITS, POST) \
    putsnds ((PRE), (VALUE), (DIGITS), (POST))
# define PUTSNS(PRE, VALUE, POST) \
    putsns ((PRE), (VALUE), (POST))

#else

# define WRITE(MSG)
# define PUTSNDS(PRE, VALUE, DIGITS, POST)
# define PUTSNS(PRE, VALUE, POST)

#endif // !VERBOSE_EXCEPTIONS

/* Handle an exception */
#ifdef VERBOSE_EXCEPTIONS
void __attribute__((nomips16))
__exception_handle_verbose (struct gpctx *ctx, int exception)
#else
void __attribute__((nomips16))
__exception_handle_quiet (struct gpctx *ctx, int exception)
#endif
{
  switch (exception)
    {
   case EXC_MOD:
      WRITE ("TLB modification exception\n");
      break;
    case EXC_TLBL:
      PUTSNS ("TLB error on load from 0x", ctx->badvaddr, NULL);
      PUTSNS (" @0x", ctx->epc, "\n");
      break;
    case EXC_TLBS:
      PUTSNS ("TLB error on store to 0x", ctx->badvaddr, NULL);
      PUTSNS (" @0x", ctx->epc, "\n");
      break;
    case EXC_ADEL:
      PUTSNS ("Address error on load from 0x", ctx->badvaddr, NULL);
      PUTSNS (" @0x", ctx->epc, "\n");
      break;
    case EXC_ADES:
      PUTSNS ("Address error on store to 0x", ctx->badvaddr, NULL);
      PUTSNS (" @0x", ctx->epc, "\n");
      break;
    case EXC_IBE:
      WRITE ("Instruction bus error\n");
      break;
    case EXC_DBE:
      WRITE ("Data bus error\n");
      break;
    case EXC_SYS:
      /* Process a UHI SYSCALL, all other SYSCALLs should have been processed
	 by our caller.  __use_excpt_boot has following values:
	 0 = Do not use exception handler present in boot.
	 1 = Use exception handler present in boot if BEV
	     is 0 at startup.
	 2 = Always use exception handler present in boot.   */

      /* Special handling for boot/low level failures.  */
      if (ctx->t2[1] == __MIPS_UHI_BOOTFAIL)
	{
	  switch (ctx->a[0])
	    {
	    case __MIPS_UHI_BF_CACHE:
	      WRITE ("L2 cache configuration error\n");
	      break;
	    default:
	      WRITE ("Unknown boot failure error\n");
	      break;
	    }

	  /* These are unrecoverable.  Abort.  */
	  ctx->epc = (sreg_t)(long)&__exit;
	  /* Exit code of 255 */
	  ctx->a[0] = 0xff;
	  return;
	}

      if (((long) __use_excpt_boot == 2
	   || ((long) __use_excpt_boot == 1
	       && __get_startup_BEV
	       && __get_startup_BEV () == 0))
	  && __chain_uhi_excpt)
	/* This will not return.  */
	__chain_uhi_excpt (ctx);
      else
	__uhi_indirect (ctx);
      return;
    case EXC_BP:
      PUTSNS ("Breakpoint @0x", ctx->epc, "\n");
      break;
    case EXC_RI:
      PUTSNS ("Illegal instruction @0x", ctx->epc, "\n");
      break;
    case EXC_CPU:
      PUTSNS ("Coprocessor unusable @0x", ctx->epc, "\n");
      break;
    case EXC_OVF:
      WRITE ("Overflow\n");
      break;
    case EXC_TRAP:
      WRITE ("Trap\n");
      break;
    case EXC_MSAFPE:
#if !(__mips_isa_rev == 6 && defined (__mips_micromips))
      if (__flush_to_zero
	  && (msa_getsr () & FPA_CSR_UNI_X)
	  && (msa_getsr () & FPA_CSR_FS) == 0)
	{
	  unsigned int sr = msa_getsr ();
	  sr &= ~FPA_CSR_UNI_X;
	  sr |= FPA_CSR_FS;
	  msa_setsr (sr);
	  return;
	}
#endif
      WRITE ("MSA Floating point error\n");
      break;
    case EXC_FPE:
      /* Turn on flush to zero the first time we hit an unimplemented
	 operation.  If we hit it again then stop.  */
      if (__flush_to_zero
	  && (fpa_getsr () & FPA_CSR_UNI_X)
	  && (fpa_getsr () & FPA_CSR_FS) == 0)
	{
	  unsigned int sr = fpa_getsr ();
	  sr &= ~FPA_CSR_UNI_X;
	  sr |= FPA_CSR_FS;
	  fpa_setsr (sr);

	  return;
	}
      WRITE ("Floating point error\n");
      break;
    case EXC_IS1:
      WRITE ("Implementation specific exception (16)\n");
      break;
    case EXC_IS2:
      WRITE ("Implementation specific exception (17)\n");
      break;
    case EXC_C2E:
      WRITE ("Precise Coprocessor 2 exception\n");
      break;
    case EXC_TLBRI:
      WRITE ("TLB read inhibit exception\n");
      break;
    case EXC_TLBXI:
      WRITE ("TLB execute inhibit exception\n");
      break;
    case EXC_MSAU:
      PUTSNS ("MSA unusable @0x", ctx->epc, "\n");
      break;
    case EXC_MDMX:
      PUTSNS ("MDMX exception @0x", ctx->epc, "\n");
      break;
    case EXC_WATCH:
      PUTSNS ("Watchpoint @0x", ctx->epc, "\n");
      break;
    case EXC_MCHECK:
      WRITE ("Machine check error\n");
      break;
    case EXC_THREAD:
      WRITE ("Thread exception\n");
      break;
    case EXC_DSPU:
      WRITE ("DSP unusable\n");
      break;
    case EXC_RES30:
      WRITE ("Cache error\n");
      break;
    default:
      PUTSNS ("Unhandled exception ", exception, "\n");
    }

  /* Dump registers */
  PUTSNS (" 0:\t", 0, "\t");
  PUTSNS ("at:\t", ctx->at, "\t");
  PUTSNS ("v0:\t", ctx->v[0], "\t");
  PUTSNS ("v1:\t", ctx->v[1], "\n");

  PUTSNS ("a0:\t", ctx->a[0], "\t");
  PUTSNS ("a1:\t", ctx->a[1], "\t");
  PUTSNS ("a2:\t", ctx->a[2], "\t");
  PUTSNS ("a3:\t", ctx->a[3], "\n");

  PUTSNS ("t0:\t", ctx->t[0], "\t");
  PUTSNS ("t1:\t", ctx->t[1], "\t");
  PUTSNS ("t2:\t", ctx->t[2], "\t");
  PUTSNS ("t3:\t", ctx->t[3], "\n");

  PUTSNS ("t4:\t", ctx->t[4], "\t");
  PUTSNS ("t5:\t", ctx->t[5], "\t");
  PUTSNS ("t6:\t", ctx->t[6], "\t");
  PUTSNS ("t7:\t", ctx->t[7], "\n");

  PUTSNS ("s0:\t", ctx->s[0], "\t");
  PUTSNS ("s1:\t", ctx->s[1], "\t");
  PUTSNS ("s2:\t", ctx->s[2], "\t");
  PUTSNS ("s3:\t", ctx->s[3], "\n");

  PUTSNS ("s4:\t", ctx->s[4], "\t");
  PUTSNS ("s5:\t", ctx->s[5], "\t");
  PUTSNS ("s6:\t", ctx->s[6], "\t");
  PUTSNS ("s7:\t", ctx->s[7], "\n");

  PUTSNS ("t8:\t", ctx->t2[0], "\t");
  PUTSNS ("t9:\t", ctx->t2[1], "\t");
  PUTSNS ("k0:\t", ctx->k[0], "\t");
  PUTSNS ("k1:\t", ctx->k[1], "\n");

  PUTSNS ("gp:\t", ctx->gp, "\t");
  PUTSNS ("sp:\t", ctx->sp, "\t");
  PUTSNS ("fp:\t", ctx->fp, "\t");
  PUTSNS ("ra:\t", ctx->ra, "\n");

#if __mips_isa_rev < 6
  PUTSNS ("hi:\t", ctx->hi, "\t");
  PUTSNS ("lo:\t", ctx->lo, "\n");
#endif

  PUTSNS ("epc:     \t", ctx->epc, "\n");
  PUTSNS ("BadVAddr:\t", ctx->badvaddr, "\n");

  PUTSNDS ("Status:   \t", ctx->status, 8, "\n");
  PUTSNDS ("Cause:    \t", ctx->cause, 8, "\n");
  PUTSNDS ("BadInstr: \t", ctx->badinstr, 8, "\n");
  PUTSNDS ("BadPInstr:\t", ctx->badpinstr, 8, "\n");

  /* Raise UHI exception which may or may not return.  */
  if (__uhi_exception (ctx, UHI_ABI) != 0)
    {
      /* The exception was acknowledged but not handled.  Abort.  */
      ctx->epc = (sreg_t)(long)&__exit;
      /* Exit code of 255 */
      ctx->a[0] = 0xff;
    }
}
