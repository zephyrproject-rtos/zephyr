/* GCC specific test inline assembler functions and macros */

/*
 * Copyright (c) 2015, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TEST_ASM_INLINE_GCC_H
#define _TEST_ASM_INLINE_GCC_H

#if !defined(__GNUC__)
#error test_asm_inline_gcc.h goes only with GCC
#endif

#if defined(CONFIG_X86)
static inline void timestamp_serialize(void)
{
	__asm__ __volatile__ (/* serialize */
	"xorl %%eax,%%eax;\n\t"
	"cpuid;\n\t"
	:
	:
	: "%eax", "%ebx", "%ecx", "%edx");
}
#elif defined(CONFIG_CPU_CORTEX_M)
#include <arch/arm/cortex_m/cmsis.h>
static inline void timestamp_serialize(void)
{
	/* isb is available in all Cortex-M  */
	__ISB();
}
#elif defined(CONFIG_CPU_CORTEX_R)
#include <arch/arm/cortex_r/cpu.h>
static inline void timestamp_serialize(void)
{
	__ISB();
}
#elif defined(CONFIG_CPU_ARCV2)
#define timestamp_serialize()
#elif defined(CONFIG_ARCH_POSIX)
#define timestamp_serialize()
#elif defined(CONFIG_XTENSA)
#define timestamp_serialize()
#elif defined(CONFIG_NIOS2)
#define timestamp_serialize()
#elif defined(CONFIG_RISCV)
#define timestamp_serialize()
#else
#error implementation of timestamp_serialize() not provided for your CPU target
#endif

#endif /* _TEST_ASM_INLINE_GCC_H */
