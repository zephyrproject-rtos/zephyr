/*
 * Copyright (c) 2025, Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_DSPIC_ARCH_H_
#define ZEPHYR_INCLUDE_ARCH_DSPIC_ARCH_H_

#include <zephyr/arch/dspic/thread.h>
#include <zephyr/arch/dspic/exception.h>
#include <zephyr/arch/common/sys_bitops.h>
#include <zephyr/arch/common/sys_io.h>
#include <zephyr/arch/common/ffs.h>

#include <zephyr/irq.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/devicetree.h>
#include <stdbool.h>

#define ARCH_STACK_PTR_ALIGN 4

#define STATUS_CTX_MASK      0x00070000u
#define IRQ_KEY_ILR_IRQ_MASK 0x7u
#define DSPIC_PRIORITY_BITS  3u
#define DSPIC_PRIORITY_WIDTH DSPIC_PRIORITY_BITS + 1u
#define DSPIC_IRQ_PER_REG    8u
#define DSPIC_PRIORITY_MASK  ((1u << DSPIC_PRIORITY_BITS) - 1u)

#ifndef _ASMLANGUAGE
#include <xc.h>
#ifdef __cplusplus
extern "C" {
#endif

void arch_irq_enable(unsigned int irq);
void arch_irq_disable(unsigned int irq);
int arch_irq_is_enabled(unsigned int irq);
void z_irq_spurious(const void *unused);
bool arch_dspic_irq_isset(unsigned int irq);
void z_dspic_enter_irq(int irq);

/* dsPIC has no MMU, so device_map() is replaced with a direct assignment */
#define device_map(virt, phys, size, flags) *(virt) = (phys)

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
#define ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p)                           \
	{                                                                                          \
		Z_ISR_DECLARE(irq_p, 0, isr_p, isr_param_p);                                       \
		z_dspic_irq_priority_set(irq_p, priority_p, flags_p);                              \
	}

#define ARCH_IRQ_DIRECT_CONNECT(irq_p, priority_p, isr_p, flags_p)                                 \
	{                                                                                          \
		Z_ISR_DECLARE_DIRECT(irq_p, ISR_FLAG_DIRECT, isr_p);                               \
		z_dspic_irq_priority_set(irq_p, priority_p, flags_p);                              \
	}

/* helper function to set the priority of the interrupt */
static ALWAYS_INLINE void z_dspic_irq_priority_set(unsigned int irq, unsigned int prio,
						   uint32_t flags)
{
	ARG_UNUSED(flags);
	volatile unsigned int reg_index = irq / DSPIC_IRQ_PER_REG;
	volatile unsigned int bit_pos = (irq % DSPIC_IRQ_PER_REG) * (DSPIC_PRIORITY_WIDTH);
	volatile uint32_t *prior_reg = (volatile uint32_t *)(&IPC0 + reg_index);
	*prior_reg &= ~(DSPIC_PRIORITY_MASK << bit_pos);
	*prior_reg |= (prio & DSPIC_PRIORITY_MASK) << bit_pos;
}

static ALWAYS_INLINE void arch_irq_unlock(unsigned int key)
{
	if (key != 0U) {
		__builtin_enable_interrupts();
	}
}

static ALWAYS_INLINE bool arch_irq_unlocked(unsigned int key)
{
	return key;
}

static ALWAYS_INLINE unsigned int arch_irq_lock(void)
{
	volatile unsigned int key;

	key = INTCON1bits.GIE;
	__builtin_disable_interrupts();
	return key;
}

static ALWAYS_INLINE bool arch_is_in_isr(void)
{
	uint32_t status_reg;

	__asm__ volatile("mov.l sr, %0" : "=r"(status_reg)::);
	return ((status_reg & STATUS_CTX_MASK) ? (true) : (false));
}

static ALWAYS_INLINE void arch_nop(void)
{
	__asm__ volatile("nop");
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

#endif /* ZEPHYR_INCLUDE_ARCH_DSPIC_ARCH_H_ */
