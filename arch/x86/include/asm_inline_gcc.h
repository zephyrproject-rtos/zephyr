/* Intel x86 GCC specific kernel inline assembler functions and macros */

/*
 * Copyright (c) 2015, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_X86_INCLUDE_ASM_INLINE_GCC_H_
#define ZEPHYR_ARCH_X86_INCLUDE_ASM_INLINE_GCC_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The file must not be included directly
 * Include asm_inline.h instead
 */

#ifndef _ASMLANGUAGE

/**
 *
 * @brief Return the current value of the EFLAGS register
 *
 * @return the EFLAGS register.
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
 * specified area.  If the specified thread supports SSE then
 * x87/MMX/SSEx thread info is saved, otherwise only x87/MMX thread is saved.
 * Function is invoked by _FpCtxSave(struct tcs *tcs)
 *
 * @return N/A
 */
static inline void _do_fp_regs_save(void *preemp_float_reg)
{
	__asm__ volatile("fnsave (%0);\n\t"
			 :
			 : "r"(preemp_float_reg)
			 : "memory");
}

#ifdef CONFIG_SSE
/**
 *
 * @brief Save non-integer context information
 *
 * This routine saves the system's "live" non-integer context into the
 * specified area.  If the specified thread supports SSE then
 * x87/MMX/SSEx thread info is saved, otherwise only x87/MMX thread is saved.
 * Function is invoked by _FpCtxSave(struct tcs *tcs)
 *
 * @return N/A
 */
static inline void _do_fp_and_sse_regs_save(void *preemp_float_reg)
{
	__asm__ volatile("fxsave (%0);\n\t"
			 :
			 : "r"(preemp_float_reg)
			 : "memory");
}
#endif /* CONFIG_SSE */

/**
 *
 * @brief Initialize floating point register context information.
 *
 * This routine initializes the system's "live" floating point registers.
 *
 * @return N/A
 */
static inline void _do_fp_regs_init(void)
{
	__asm__ volatile("fninit\n\t");
}

#ifdef CONFIG_SSE
/**
 *
 * @brief Initialize SSE register context information.
 *
 * This routine initializes the system's "live" SSE registers.
 *
 * @return N/A
 */
static inline void _do_sse_regs_init(void)
{
	__asm__ volatile("ldmxcsr _sse_mxcsr_default_value\n\t");
}
#endif /* CONFIG_SSE */

#endif /* CONFIG_FP_SHARING */

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_X86_INCLUDE_ASM_INLINE_GCC_H_ */
