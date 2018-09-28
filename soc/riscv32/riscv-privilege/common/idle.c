/*
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 * Contributors: 2018 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>
#include <irq.h>
#include <soc.h>

#include <tracing.h>

static ALWAYS_INLINE void riscv_idle(unsigned int key)
{
	z_sys_trace_idle();
	/* unlock interrupts */
	irq_unlock(key);

	/* Wait for interrupt */
	__asm__ volatile("wfi");
}

/**
 *
 * @brief Power save idle routine
 *
 * This function will be called by the kernel idle loop or possibly within
 * an implementation of _sys_power_save_idle in the kernel when the
 * '_sys_power_save_flag' variable is non-zero.
 *
 * @return N/A
 */
void k_cpu_idle(void)
{
	riscv_idle(SOC_MSTATUS_IEN);
}

/**
 *
 * @brief Atomically re-enable interrupts and enter low power mode
 *
 * INTERNAL
 * The requirements for k_cpu_atomic_idle() are as follows:
 * 1) The enablement of interrupts and entering a low-power mode needs to be
 *    atomic, i.e. there should be no period of time where interrupts are
 *    enabled before the processor enters a low-power mode.  See the comments
 *    in k_lifo_get(), for example, of the race condition that
 *    occurs if this requirement is not met.
 *
 * 2) After waking up from the low-power mode, the interrupt lockout state
 *    must be restored as indicated in the 'imask' input parameter.
 *
 * @return N/A
 */
void k_cpu_atomic_idle(unsigned int key)
{
	riscv_idle(key);
}
