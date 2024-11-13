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
	return csr_read(mhartid) & ((uintptr_t)CONFIG_RISCV_HART_MASK);
}

static ALWAYS_INLINE _cpu_t *arch_curr_cpu(void)
{
#ifdef CONFIG_VALIDATE_ARCH_CURR_CPU
	__ASSERT_NO_MSG(!z_smp_cpu_mobile());
#endif /* CONFIG_VALIDATE_ARCH_CURR_CPU */

#if defined(CONFIG_SMP) || defined(CONFIG_USERSPACE)
	return (_cpu_t *)csr_read(mscratch);
#else
	return &_kernel.cpus[0];
#endif /* defined(CONFIG_SMP) || defined(CONFIG_USERSPACE) */
}

static ALWAYS_INLINE unsigned int arch_num_cpus(void)
{
	return CONFIG_MP_MAX_NUM_CPUS;
}

#endif /* !_ASMLANGUAGE */
#endif /* ZEPHYR_INCLUDE_ARCH_RISCV_ARCH_INLINES_H_ */
