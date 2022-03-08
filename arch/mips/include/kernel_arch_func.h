/*
 * Copyright (c) 2020 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * based on arch/riscv/include/kernel_arch_func.h
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private kernel definitions
 *
 * This file contains private kernel function/macro definitions and various
 * other definitions for the MIPS processor architecture.
 */

#ifndef ZEPHYR_ARCH_MIPS_INCLUDE_KERNEL_ARCH_FUNC_H_
#define ZEPHYR_ARCH_MIPS_INCLUDE_KERNEL_ARCH_FUNC_H_

#include <kernel_arch_data.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE
static ALWAYS_INLINE void arch_kernel_init(void)
{
}

static ALWAYS_INLINE void
arch_thread_return_value_set(struct k_thread *thread, unsigned int value)
{
	thread->arch.swap_return_value = value;
}

FUNC_NORETURN void z_mips_fatal_error(unsigned int reason,
				       const z_arch_esf_t *esf);

static inline bool arch_is_in_isr(void)
{
	return _current_cpu->nested != 0U;
}

#ifdef CONFIG_IRQ_OFFLOAD
void z_irq_do_offload(void);
#endif

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_MIPS_INCLUDE_KERNEL_ARCH_FUNC_H_ */
