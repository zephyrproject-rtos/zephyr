/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. (AMD)
 * Copyright (c) 2023 Alp Sayin <alpsayin@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#ifndef ZEPHYR_INCLUDE_ARCH_MICROBLAZE_ARCH_H_
#define ZEPHYR_INCLUDE_ARCH_MICROBLAZE_ARCH_H_

#include <zephyr/arch/common/ffs.h>
#include <zephyr/arch/microblaze/exp.h>
#include <zephyr/arch/microblaze/sys_bitops.h>
#include <zephyr/arch/microblaze/sys_io.h>
#include <zephyr/arch/microblaze/thread.h>
#include <zephyr/devicetree.h>
#include <zephyr/irq.h>
#include <zephyr/sw_isr_table.h>

#include <microblaze/emulate_isr.h>
#include <microblaze/mb_interface.h>
#include <microblaze/microblaze_asm.h>
#include <microblaze/microblaze_regs.h>

#define ARCH_STACK_PTR_ALIGN 16

#ifndef _ASMLANGUAGE
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STACK_ROUND_UP(x) ROUND_UP(x, ARCH_STACK_PTR_ALIGN)

uint32_t arch_irq_pending(void);
void arch_irq_enable(unsigned int irq);
void arch_irq_disable(unsigned int irq);
int arch_irq_is_enabled(unsigned int irq);
uint32_t arch_irq_set_emulated_pending(uint32_t irq);
uint32_t arch_irq_pending_vector(uint32_t irq_pending);
void z_irq_spurious(const void *unused);

/**
 * Normally used to configure a static interrupt.
 * Barebones microblaze has 1 interrupt to offer so we connect
 * whatever isr & param supplied to that. SoCs should use this
 * macro to connect a single device (can be the AXI interrupt controller)
 * to the microblaze's only ISR to eventually make it call XIntc_DeviceInterruptHandler.
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
	}

static ALWAYS_INLINE unsigned int arch_irq_lock(void)
{
	const uint32_t unshifted_msr_ie_status = mfmsr() & MSR_IE_MASK;

	if (unshifted_msr_ie_status) {
		extern void microblaze_disable_interrupts(void);
		microblaze_disable_interrupts();
		return 1;
	}
	return 0;
}

static ALWAYS_INLINE void arch_irq_unlock(unsigned int key)
{
	if (key) {
		extern void microblaze_enable_interrupts(void);
		microblaze_enable_interrupts();
	}
}

static ALWAYS_INLINE bool arch_irq_unlocked(unsigned int key)
{
	return key != 0;
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

#endif /* ZEPHYR_INCLUDE_ARCH_MICROBLAZE_ARCH_H_ */
