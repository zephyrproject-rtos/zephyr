/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RISCV_ARCH_INLINES_H_
#define ZEPHYR_INCLUDE_ARCH_RISCV_ARCH_INLINES_H_

#ifndef _ASMLANGUAGE

#include <zephyr/kernel_structs.h>
#include "csr.h"

static ALWAYS_INLINE uint32_t arch_proc_id(void)
{
#ifdef CONFIG_RISCV_S_MODE
	/* mhartid is M-mode only; S-mode cannot read it.
	 * For single-hart systems the hart ID is always 0.
	 * SMP S-mode would need to retrieve this from per-CPU data.
	 */
	return 0;
#else
	return csr_read(mhartid) & ((uintptr_t)CONFIG_RISCV_HART_MASK);
#endif
}

static ALWAYS_INLINE _cpu_t *arch_curr_cpu(void)
{
#if defined(CONFIG_SMP) || defined(CONFIG_USERSPACE)
#ifdef CONFIG_RISCV_S_MODE
	return (_cpu_t *)csr_read(sscratch);
#else
	return (_cpu_t *)csr_read(mscratch);
#endif
#else
	return &_kernel.cpus[0];
#endif
}

#ifdef CONFIG_RISCV_CURRENT_VIA_GP

register struct k_thread *__arch_current_thread __asm__("gp");

#define arch_current_thread() __arch_current_thread
#define arch_current_thread_set(thread) ({ __arch_current_thread = (thread); })

#endif /* CONFIG_RISCV_CURRENT_VIA_GP */

static ALWAYS_INLINE unsigned int arch_num_cpus(void)
{
	return CONFIG_MP_MAX_NUM_CPUS;
}

#endif /* !_ASMLANGUAGE */
#endif /* ZEPHYR_INCLUDE_ARCH_RISCV_ARCH_INLINES_H_ */
