/*
 * Copyright (c) 2020 Katsuhiro Suzuki <katsuhiro@katsuster.net>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RISCV_ARCH_INLINES_H_
#define ZEPHYR_INCLUDE_ARCH_RISCV_ARCH_INLINES_H_

#ifndef _ASMLANGUAGE

#ifdef CONFIG_RISCV

#include <kernel_structs.h>

static inline uint32_t z_riscv_hart_id(void)
{
	uint32_t hartid;

	__asm__ volatile ("csrr %0, mhartid" : "=r"(hartid));

	return hartid;
}

static inline struct _cpu *arch_curr_cpu(void)
{
#ifdef CONFIG_SMP
	unsigned int k = arch_irq_lock();
	uint32_t hartid = z_riscv_hart_id();

	struct _cpu *ret = &_kernel.cpus[hartid];
	arch_irq_unlock(k);

	return ret;
#else
	return &_kernel.cpus[0];
#endif
}

#endif /* CONFIG_RISCV */

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_RISCV_ARCH_INLINES_H_ */
