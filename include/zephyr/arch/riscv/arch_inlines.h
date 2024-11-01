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
#include "reg.h"

static ALWAYS_INLINE uint32_t arch_proc_id(void)
{
	return csr_read(mhartid) & ((uintptr_t)CONFIG_RISCV_HART_MASK);
}

static ALWAYS_INLINE _cpu_t *arch_curr_cpu(void)
{
#if defined(CONFIG_SMP) || defined(CONFIG_USERSPACE)
	return (_cpu_t *)csr_read(mscratch);
#else
	return &_kernel.cpus[0];
#endif
}

#ifdef CONFIG_RISCV_CURRENT_VIA_GP
register struct k_thread *__arch_curr_thread __asm__("gp");

#define arch_curr_thread() __arch_curr_thread
#define arch_set_curr_thread(thread)                                                               \
	{                                                                                          \
		_current_cpu->current = thread;                                                    \
		__arch_curr_thread = (thread);                                                     \
	}
#endif /* CONFIG_RISCV_CURRENT_VIA_GP */

static ALWAYS_INLINE unsigned int arch_num_cpus(void)
{
	return CONFIG_MP_MAX_NUM_CPUS;
}

#endif /* !_ASMLANGUAGE */
#endif /* ZEPHYR_INCLUDE_ARCH_RISCV_ARCH_INLINES_H_ */
