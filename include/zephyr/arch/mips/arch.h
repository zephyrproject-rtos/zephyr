/*
 * Copyright (c) 2020 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * based on include/arch/sparc/arch.h
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_MIPS_ARCH_H_
#define ZEPHYR_INCLUDE_ARCH_MIPS_ARCH_H_

#include <zephyr/arch/mips/thread.h>
#include <zephyr/arch/mips/exp.h>
#include <zephyr/arch/common/sys_bitops.h>
#include <zephyr/arch/common/sys_io.h>
#include <zephyr/arch/common/ffs.h>

#include <zephyr/irq.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/devicetree.h>
#include <mips/mipsregs.h>

#define ARCH_STACK_PTR_ALIGN 16

#define OP_LOADREG lw
#define OP_STOREREG sw

#define CP0_STATUS_DEF_RESTORE (ST0_EXL | ST0_IE)

#ifndef _ASMLANGUAGE
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STACK_ROUND_UP(x) ROUND_UP(x, ARCH_STACK_PTR_ALIGN)

void arch_irq_enable(unsigned int irq);
void arch_irq_disable(unsigned int irq);
int arch_irq_is_enabled(unsigned int irq);
void z_irq_spurious(const void *unused);

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
	uint32_t status = read_c0_status();

	if (status & ST0_IE) {
		write_c0_status(status & ~ST0_IE);
		return 1;
	}
	return 0;
}

static ALWAYS_INLINE void arch_irq_unlock(unsigned int key)
{
	uint32_t status = read_c0_status();

	if (key) {
		status |= ST0_IE;
	} else {
		status &= ~ST0_IE;
	}

	write_c0_status(status);
}

static ALWAYS_INLINE bool arch_irq_unlocked(unsigned int key)
{
	return key != 0;
}

static ALWAYS_INLINE void arch_nop(void)
{
	__asm__ volatile ("nop");
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

#endif /* ZEPHYR_INCLUDE_ARCH_MIPS_ARCH_H_ */
