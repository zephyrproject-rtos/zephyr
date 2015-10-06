/* Intel x86 GCC specific kernel inline assembler functions and macros */

/*
 * Copyright (c) 2015, Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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

/**
 *
 * @brief Return the current value of the EFLAGS register
 *
 * @return the EFLAGS register.
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

/**
 *
 * @brief Disallow use of floating point capabilities
 *
 * This routine sets CR0[TS] to 1, which disallows the use of FP instructions
 * by the currently executing thread.
 *
 * @return N/A
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


/**
 *
 * @brief Save non-integer context information
 *
 * This routine saves the system's "live" non-integer context into the
 * specified area.  If the specified task or fiber supports SSE then
 * x87/MMX/SSEx thread info is saved, otherwise only x87/MMX thread is saved.
 * Function is invoked by _FpCtxSave(struct tcs *tcs)
 *
 * @return N/A
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

/**
 *
 * @brief Initialize non-integer context information
 *
 * This routine initializes the system's "live" non-integer context.
 * Function is invoked by _FpCtxInit(struct tcs *tcs)
 *
 * @return N/A
 */

static inline void _do_fp_ctx_init(int flags)
{
	/* initialize x87 FPU */
	__asm__ volatile("fninit\n\t");


#ifdef CONFIG_SSE
	if (flags) {
		/* initialize SSE (since thread uses it) */
		__asm__ volatile("ldmxcsr _sse_mxcsr_default_value\n\t");

	}
#else
	ARG_UNUSED(flags);
#endif /* CONFIG_SSE */
}

#endif /* CONFIG_FP_SHARING */

#endif /* _ASMLANGUAGE */
#endif /* _ASM_INLINE_GCC_H */
