/* Intel x86 GCC specific kernel inline assembler functions and macros */

/*
 * Copyright (c) 2015, Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
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

#ifndef _ASM_INLINE_GCC_H
#define _ASM_INLINE_GCC_H

/*
 * The file must not be included directly
 * Include asm_inline.h instead
 */

#ifdef _ASMLANGUAGE

#define SYS_NANO_CPU_EXC_CONNECT(handler,vector) \
NANO_CPU_EXC_CONNECT_NO_ERR(handler,vector,0)

#else /* !_ASMLANGUAGE */

/*******************************************************************************
*
* EflagsGet - return the current value of the EFLAGS register
*
* RETURNS: the EFLAGS register.
*
* \NOMANUAL
*/

static inline unsigned int EflagsGet(void)
{
	unsigned int eflags; /* EFLAGS register contents */

	__asm__ volatile(
		"pushfl;\n\t"
		"popl  %0;\n\t"
		: "=r"(eflags)
		: );

	return eflags;
}


#ifdef CONFIG_FP_SHARING

/*******************************************************************************
*
* _FpAccessDisable - disallow use of floating point capabilities
*
* This routine sets CR0[TS] to 1, which disallows the use of FP instructions
* by the currently executing context.
*
* RETURNS: N/A
*/

static inline void _FpAccessDisable(void)
{
	void *tempReg;

	__asm__ volatile(
		"movl %%cr0, %0;\n\t"
		"orl $0x8, %0;\n\t"
		"movl %0, %%cr0;\n\t"
		: "=r"(tempReg)
		:
		: "memory");
}


/*******************************************************************************
*
* _do_fp_ctx_save - save non-integer context information
*
* This routine saves the system's "live" non-integer context into the
* specified area.  If the specified task or fiber supports SSE then
* x87/MMX/SSEx context info is saved, otherwise only x87/MMX context is saved.
* Function is invoked by _FpCtxSave(tCCS *ccs)
*
* RETURNS: N/A
*/

static inline void _do_fp_ctx_save(int flags, void *preemp_float_reg)
{
#ifdef CONFIG_SSE
	if (flags) {
		__asm__ volatile("fxsave (%0);\n\t"
				 :
				 : "r"(preemp_float_reg)
				 : "memory");
	} else
#else
	ARG_UNUSED(flags);
#endif /* CONFIG_SSE */
	{
		__asm__ volatile("fnsave (%0);\n\t"
				 :
				 : "r"(preemp_float_reg)
				 : "memory");
	}
}

/*******************************************************************************
*
* _do_fp_ctx_init - initialize non-integer context information
*
* This routine initializes the system's "live" non-integer context.
* Function is invoked by _FpCtxInit(tCCS *ccs)
*
* RETURNS: N/A
*/

static inline void _do_fp_ctx_init(int flags)
{
	/* initialize x87 FPU */
	__asm__ volatile("fninit\n\t");


#ifdef CONFIG_SSE
	if (flags) {
		/* initialize SSE (since context uses it) */
		__asm__ volatile("ldmxcsr _sse_mxcsr_default_value\n\t");

	}
#else
	ARG_UNUSED(flags);
#endif /* CONFIG_SSE */
}

#endif /* CONFIG_FP_SHARING */

#endif /* _ASMLANGUAGE */
#endif /* _ASM_INLINE_GCC_H */
