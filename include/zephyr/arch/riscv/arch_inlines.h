/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RISCV_ARCH_INLINES_H_
#define ZEPHYR_INCLUDE_ARCH_RISCV_ARCH_INLINES_H_

#ifndef _ASMLANGUAGE

#include <zephyr/kernel_structs.h>

static ALWAYS_INLINE uint32_t arch_proc_id(void)
{
	uint32_t hartid;

#ifdef CONFIG_SMP
	__asm__ volatile("csrr %0, mhartid" : "=r" (hartid));
#else
	hartid = 0;
#endif

	return hartid;
}

static ALWAYS_INLINE _cpu_t *arch_curr_cpu(void)
{
	/* linear hartid enumeration space assumed */
	return &_kernel.cpus[arch_proc_id()];
}

#endif /* !_ASMLANGUAGE */
#endif /* ZEPHYR_INCLUDE_ARCH_RISCV_ARCH_INLINES_H_ */
