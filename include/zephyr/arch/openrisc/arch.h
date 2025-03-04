/*
 * Copyright (c) 2025 NVIDIA Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_OPENRISC_ARCH_H_
#define ZEPHYR_INCLUDE_ARCH_OPENRISC_ARCH_H_

#include <zephyr/irq.h>

#include <zephyr/devicetree.h>
#if !defined(_ASMLANGUAGE) && !defined(__ASSEMBLER__)
#include <zephyr/arch/openrisc/exception.h>
#include <zephyr/arch/openrisc/thread.h>
#include <zephyr/arch/common/sys_bitops.h>
#include <zephyr/arch/common/sys_io.h>
#include <zephyr/arch/common/ffs.h>

#include <zephyr/sw_isr_table.h>

#include <openrisc/openriscregs.h>

#define ARCH_STACK_PTR_ALIGN 8

#define SPR_SR_IRQ_MASK		(SPR_SR_IEE | SPR_SR_TEE)

#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Configure a static interrupt.
 *
 * All arguments must be computable by the compiler at build time.
 *
 * @param irq_p IRQ line number
 * @param priority_p Interrupt priority
 * @param isr_p Interrupt service routine
 * @param isr_param_p ISR parameter
 * @param flags_p IRQ options
 *
 * @return The vector assigned to this interrupt
 */

#define ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p) \
	{								 \
		Z_ISR_DECLARE(irq_p, 0, isr_p, isr_param_p);		 \
	}

static ALWAYS_INLINE unsigned int arch_irq_lock(void)
{
	const uint32_t sr = openrisc_read_spr(SPR_SR);

	openrisc_write_spr(SPR_SR, sr & ~SPR_SR_IRQ_MASK);
	return (sr & SPR_SR_IRQ_MASK) ? 1 : 0;
}

static ALWAYS_INLINE void arch_irq_unlock(unsigned int key)
{
	const uint32_t sr = openrisc_read_spr(SPR_SR);

	openrisc_write_spr(SPR_SR, key ? (sr | SPR_SR_IRQ_MASK) : (sr & ~SPR_SR_IRQ_MASK));
}

static ALWAYS_INLINE bool arch_irq_unlocked(unsigned int key)
{
	return key != 0;
}

static ALWAYS_INLINE void arch_nop(void)
{
	__asm__ volatile ("l.nop");
}

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

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_OPENRISC_ARCH_H_ */
