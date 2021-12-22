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

#ifdef CONFIG_RISCV_PMP
void z_riscv_configure_static_pmp_regions(void);
#endif

static ALWAYS_INLINE void arch_kernel_init(void)
{
#ifdef CONFIG_RISCV_PMP
	z_riscv_configure_static_pmp_regions();
#endif
}

void arch_switch(void *switch_to, void **switched_from);

FUNC_NORETURN void z_riscv_fatal_error(unsigned int reason,
				       const z_arch_esf_t *esf);

static inline bool arch_is_in_isr(void)
{
#ifdef CONFIG_SMP
	unsigned int key = arch_irq_lock();
	bool ret = arch_curr_cpu()->nested != 0U;

	arch_irq_unlock(key);
	return ret;
#else
	return _kernel.cpus[0].nested != 0U;
#endif
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
