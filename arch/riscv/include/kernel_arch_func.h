/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private kernel definitions
 *
 * This file contains private kernel function/macro definitions and various
 * other definitions for the RISCV processor architecture.
 */

#ifndef ZEPHYR_ARCH_RISCV_INCLUDE_KERNEL_ARCH_FUNC_H_
#define ZEPHYR_ARCH_RISCV_INCLUDE_KERNEL_ARCH_FUNC_H_

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

FUNC_NORETURN void z_riscv_fatal_error(unsigned int reason,
				       const z_arch_esf_t *esf);

static inline bool arch_is_in_isr(void)
{
	return _kernel.cpus[0].nested != 0U;
}

extern FUNC_NORETURN void z_riscv_userspace_enter(k_thread_entry_t user_entry,
						 void *p1, void *p2, void *p3,
						 uint32_t stack_end,
						 uint32_t stack_start);

#ifdef CONFIG_IRQ_OFFLOAD
int z_irq_do_offload(void);
#endif

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_RISCV_INCLUDE_KERNEL_ARCH_FUNC_H_ */
