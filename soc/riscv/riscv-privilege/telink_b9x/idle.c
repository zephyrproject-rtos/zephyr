/*
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 * Contributors: 2018 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/irq.h>
#include <zephyr/arch/cpu.h>

#include <zephyr/tracing/tracing.h>

static volatile bool __irq_pending;

static ALWAYS_INLINE void riscv_idle(unsigned int key)
{
	sys_trace_idle();

__irq_pending = true;

	/* unlock interrupts */
	irq_unlock(key);

	/* Due to silicon bug in B92 platform, the WFI emulation is implemented */
	while (__irq_pending) {
	}
}

/**
 * @brief Power save idle routine
 *
 * This function will be called by the kernel idle loop or possibly within
 * an implementation of _pm_save_idle in the kernel when the
 * '_pm_save_flag' variable is non-zero.
 */
void arch_cpu_idle(void)
{
	riscv_idle(MSTATUS_IEN);
}

/**
 * @brief Atomically re-enable interrupts and enter low power mode
 *
 * INTERNAL
 * The requirements for arch_cpu_atomic_idle() are as follows:
 * 1) The enablement of interrupts and entering a low-power mode needs to be
 *    atomic, i.e. there should be no period of time where interrupts are
 *    enabled before the processor enters a low-power mode.  See the comments
 *    in k_lifo_get(), for example, of the race condition that
 *    occurs if this requirement is not met.
 *
 * 2) After waking up from the low-power mode, the interrupt lockout state
 *    must be restored as indicated in the 'imask' input parameter.
 */
void arch_cpu_atomic_idle(unsigned int key)
{
	riscv_idle(key);
}

#ifdef CONFIG_SOC_RISCV_TELINK_B92
/**
 * @brief Telink B92 SOC handle IRQ implementation
 *
 * - __soc_handle_irq: handle SoC-specific details for a pending IRQ
 *   (e.g. clear a pending bit in a SoC-specific register)
 */
void __soc_handle_irq(unsigned long mcause)
{
	__irq_pending = false;
	__asm__ ("li t1, 1");
	__asm__ ("sll t0, t1, a0");
	__asm__ ("csrrc t1, mip, t0");
}
#endif
