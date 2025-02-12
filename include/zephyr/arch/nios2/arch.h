/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Nios II specific kernel interface header
 * This header contains the Nios II specific kernel interface.  It is
 * included by the generic kernel interface header (include/arch/cpu.h)
 */

#ifndef ZEPHYR_INCLUDE_ARCH_NIOS2_ARCH_H_
#define ZEPHYR_INCLUDE_ARCH_NIOS2_ARCH_H_

#include <system.h>

#include <zephyr/arch/nios2/thread.h>
#include <zephyr/arch/nios2/exception.h>
#include <zephyr/arch/nios2/asm_inline.h>
#include <zephyr/arch/common/addr_types.h>
#include <zephyr/devicetree.h>
#include <zephyr/arch/nios2/nios2.h>
#include <zephyr/arch/common/sys_bitops.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/arch/common/ffs.h>

#define ARCH_STACK_PTR_ALIGN  4

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>
#include <zephyr/irq.h>
#include <zephyr/sw_isr_table.h>

#ifdef __cplusplus
extern "C" {
#endif

/* There is no notion of priority with the Nios II internal interrupt
 * controller and no flags are currently supported.
 */
#define ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p) \
{ \
	Z_ISR_DECLARE(irq_p, 0, isr_p, isr_param_p); \
}

static ALWAYS_INLINE unsigned int arch_irq_lock(void)
{
	unsigned int key, tmp;

	__asm__ volatile (
	    "rdctl %[key], status\n\t"
	    "movi %[tmp], -2\n\t"
	    "and %[tmp], %[key], %[tmp]\n\t"
	    "wrctl status, %[tmp]\n\t"
	    : [key] "=r" (key), [tmp] "=r" (tmp)
	    : : "memory");

	return key;
}

static ALWAYS_INLINE void arch_irq_unlock(unsigned int key)
{
	/* If the CPU is built without certain features, then
	 * the only writable bit in the status register is PIE
	 * in which case we can just write the value stored in key,
	 * all the other writable bits will be the same.
	 *
	 * If not, other stuff could have changed and we need to
	 * specifically flip just that bit.
	 */

#if (ALT_CPU_NUM_OF_SHADOW_REG_SETS > 0) || \
		(defined ALT_CPU_EIC_PRESENT) || \
		(defined ALT_CPU_MMU_PRESENT) || \
		(defined ALT_CPU_MPU_PRESENT)
	__asm__ volatile (
	    "andi %[key], %[key], 1\n\t"
	    "beq %[key], zero, 1f\n\t"
	    "rdctl %[key], status\n\t"
	    "ori %[key], %[key], 1\n\t"
	    "wrctl status, %[key]\n\t"
	    "1:\n\t"
	    : [key] "+r" (key)
	    : : "memory");
#else
	__asm__ volatile (
	    "wrctl status, %[key]"
	    : : [key] "r" (key)
	    : "memory");
#endif
}

static ALWAYS_INLINE bool arch_irq_unlocked(unsigned int key)
{
	return key & 1;
}

void arch_irq_enable(unsigned int irq);
void arch_irq_disable(unsigned int irq);

FUNC_NORETURN void z_SysFatalErrorHandler(unsigned int reason,
					 const struct arch_esf *esf);

FUNC_NORETURN void z_NanoFatalErrorHandler(unsigned int reason,
					  const struct arch_esf *esf);

enum nios2_exception_cause {
	NIOS2_EXCEPTION_UNKNOWN                      = -1,
	NIOS2_EXCEPTION_RESET                        = 0,
	NIOS2_EXCEPTION_CPU_ONLY_RESET_REQUEST       = 1,
	NIOS2_EXCEPTION_INTERRUPT                    = 2,
	NIOS2_EXCEPTION_TRAP_INST                    = 3,
	NIOS2_EXCEPTION_UNIMPLEMENTED_INST           = 4,
	NIOS2_EXCEPTION_ILLEGAL_INST                 = 5,
	NIOS2_EXCEPTION_MISALIGNED_DATA_ADDR         = 6,
	NIOS2_EXCEPTION_MISALIGNED_TARGET_PC         = 7,
	NIOS2_EXCEPTION_DIVISION_ERROR               = 8,
	NIOS2_EXCEPTION_SUPERVISOR_ONLY_INST_ADDR    = 9,
	NIOS2_EXCEPTION_SUPERVISOR_ONLY_INST         = 10,
	NIOS2_EXCEPTION_SUPERVISOR_ONLY_DATA_ADDR    = 11,
	NIOS2_EXCEPTION_TLB_MISS                     = 12,
	NIOS2_EXCEPTION_TLB_EXECUTE_PERM_VIOLATION   = 13,
	NIOS2_EXCEPTION_TLB_READ_PERM_VIOLATION      = 14,
	NIOS2_EXCEPTION_TLB_WRITE_PERM_VIOLATION     = 15,
	NIOS2_EXCEPTION_MPU_INST_REGION_VIOLATION    = 16,
	NIOS2_EXCEPTION_MPU_DATA_REGION_VIOLATION    = 17,
	NIOS2_EXCEPTION_ECC_TLB_ERR                  = 18,
	NIOS2_EXCEPTION_ECC_FETCH_ERR                = 19,
	NIOS2_EXCEPTION_ECC_REGISTER_FILE_ERR        = 20,
	NIOS2_EXCEPTION_ECC_DATA_ERR                 = 21,
	NIOS2_EXCEPTION_ECC_DATA_CACHE_WRITEBACK_ERR = 22
};

/* Bitfield indicating which exception cause codes report a valid
 * badaddr register. NIOS2_EXCEPTION_TLB_MISS and NIOS2_EXCEPTION_ECC_TLB_ERR
 * are deliberately not included here, you need to check if TLBMISC.D=1
 */
#define NIOS2_BADADDR_CAUSE_MASK \
	(BIT(NIOS2_EXCEPTION_SUPERVISOR_ONLY_DATA_ADDR) | \
	 BIT(NIOS2_EXCEPTION_MISALIGNED_DATA_ADDR) | \
	 BIT(NIOS2_EXCEPTION_MISALIGNED_TARGET_PC) | \
	 BIT(NIOS2_EXCEPTION_TLB_READ_PERM_VIOLATION) | \
	 BIT(NIOS2_EXCEPTION_TLB_WRITE_PERM_VIOLATION) | \
	 BIT(NIOS2_EXCEPTION_MPU_DATA_REGION_VIOLATION) | \
	 BIT(NIOS2_EXCEPTION_ECC_DATA_ERR))


extern uint32_t sys_clock_cycle_get_32(void);

static inline uint32_t arch_k_cycle_get_32(void)
{
	return sys_clock_cycle_get_32();
}

extern uint64_t sys_clock_cycle_get_64(void);

static inline uint64_t arch_k_cycle_get_64(void)
{
	return sys_clock_cycle_get_64();
}

static ALWAYS_INLINE void arch_nop(void)
{
	__asm__ volatile("nop");
}

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_NIOS2_ARCH_H_ */
