/*
 * Copyright (c) 2026 Takayuki Goto
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief CP15 accessors and CMSIS-style intrinsics for ARMv5 cores
 *
 * ARMv5 predates CMSIS-Core(A/R), so the handful of intrinsics the common
 * Zephyr ARM code expects (__DSB(), __WFI(), __enable_irq(), ...) are provided
 * here in terms of the ARMv5 CP15 operations.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_ARM9_LIB_HELPERS_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_ARM9_LIB_HELPERS_H_

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>
#include <zephyr/toolchain.h>
#include <zephyr/arch/arm/arm9/cpu.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MRC15(crn, op1, crm, op2)                                                                  \
	({                                                                                         \
		uint32_t _val;                                                                     \
		__asm__ volatile("mrc p15, " #op1 ", %0, " #crn ", " #crm ", " #op2                \
				 : "=r"(_val)                                                      \
				 : /* no inputs */                                                 \
				 : "memory");                                                      \
		_val;                                                                              \
	})

#define MCR15(crn, op1, crm, op2, val)                                                             \
	__asm__ volatile("mcr p15, " #op1 ", %0, " #crn ", " #crm ", " #op2                        \
			 : /* no outputs */                                                        \
			 : "r"(val)                                                                \
			 : "memory")

/* System Control Register */
static ALWAYS_INLINE uint32_t __get_SCTLR(void)
{
	return MRC15(c1, 0, c0, 0);
}

static ALWAYS_INLINE void __set_SCTLR(uint32_t val)
{
	MCR15(c1, 0, c0, 0, val);
}

/* Translation Table Base Register */
static ALWAYS_INLINE void __set_TTBR0(uint32_t val)
{
	MCR15(c2, 0, c0, 0, val);
}

/* Domain Access Control Register */
static ALWAYS_INLINE void __set_DACR(uint32_t val)
{
	MCR15(c3, 0, c0, 0, val);
}

/* Data / Instruction Fault Status and Address registers */
static ALWAYS_INLINE uint32_t __get_DFSR(void)
{
	return MRC15(c5, 0, c0, 0);
}

static ALWAYS_INLINE uint32_t __get_IFSR(void)
{
	return MRC15(c5, 0, c0, 1);
}

static ALWAYS_INLINE uint32_t __get_DFAR(void)
{
	return MRC15(c6, 0, c0, 0);
}

/*
 * ARMv5 has no IFAR; the faulting instruction address is the return address
 * recorded in the exception stack frame.
 */

static ALWAYS_INLINE uint32_t __get_MIDR(void)
{
	return MRC15(c0, 0, c0, 0);
}

static ALWAYS_INLINE uint32_t __get_CPSR(void)
{
	uint32_t val;

	__asm__ volatile("mrs %0, cpsr" : "=r"(val)::"memory");
	return val;
}

/*
 * Barriers.
 *
 * ARMv5 has neither DMB/DSB/ISB instructions nor a weakly ordered memory
 * model that would need them: "drain write buffer" is the strongest ordering
 * operation available and is what the architecture reference manual
 * prescribes before relying on a completed write.
 */
static ALWAYS_INLINE void __DSB(void)
{
	MCR15(c7, 0, c10, 4, 0);
}

static ALWAYS_INLINE void __DMB(void)
{
	MCR15(c7, 0, c10, 4, 0);
}

static ALWAYS_INLINE void __ISB(void)
{
	MCR15(c7, 0, c10, 4, 0);
	/* Flush the prefetch buffer by forcing a pipeline refill. */
	__asm__ volatile("mov r0, r0\n\t"
			 "mov r0, r0\n\t"
			 "mov r0, r0"
			 : /* no outputs */
			 : /* no inputs */
			 : "memory");
}

static ALWAYS_INLINE void __NOP(void)
{
	__asm__ volatile("mov r0, r0" ::: "memory");
}

/* Wait for interrupt: CP15 c7 c0 4. */
static ALWAYS_INLINE void __WFI(void)
{
	MCR15(c7, 0, c0, 4, 0);
}

/* ARMv5 has no WFE; the closest equivalent is WFI. */
static ALWAYS_INLINE void __WFE(void)
{
	__WFI();
}

static ALWAYS_INLINE void __enable_irq(void)
{
	uint32_t cpsr;

	__asm__ volatile("mrs %0, cpsr\n\t"
			 "bic %0, %0, %1\n\t"
			 "msr cpsr_c, %0"
			 : "=&r"(cpsr)
			 : "i"(I_BIT)
			 : "memory", "cc");
}

static ALWAYS_INLINE void __disable_irq(void)
{
	uint32_t cpsr;

	__asm__ volatile("mrs %0, cpsr\n\t"
			 "orr %0, %0, %1\n\t"
			 "msr cpsr_c, %0"
			 : "=&r"(cpsr)
			 : "i"(I_BIT)
			 : "memory", "cc");
}

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_ARM9_LIB_HELPERS_H_ */
