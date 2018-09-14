/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Kernel event logger support for RISCV32
 */

#ifndef ZEPHYR_ARCH_RISCV32_INCLUDE_TRACING_ARCH_H_
#define ZEPHYR_ARCH_RISCV32_INCLUDE_TRACING_ARCH_H_

#include <arch/cpu.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the identification of the current interrupt.
 *
 * This routine obtain the key of the interrupt that is currently processed
 * if it is called from an IRQ context.
 *
 * @return The key of the interrupt that is currently being processed.
 */
static inline int _sys_current_irq_key_get(void)
{
	u32_t mcause;

	__asm__ volatile("csrr %0, mcause" : "=r" (mcause));

	mcause &= SOC_MCAUSE_EXP_MASK;

	return mcause;
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_RISCV32_INCLUDE_TRACING_ARCH_H_ */
