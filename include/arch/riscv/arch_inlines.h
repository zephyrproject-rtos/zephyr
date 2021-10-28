/*
 * Copyright (c) 2021 Microchip Inc.
 * Copyright (c) 2019 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RISCV_ARCH_INLINES_H_
#define ZEPHYR_INCLUDE_ARCH_RISCV_ARCH_INLINES_H_

#ifndef _ASMLANGUAGE

#if defined(CONFIG_RISCV)

#include <kernel_structs.h>

static inline struct _cpu *arch_curr_cpu(void)
{
#if defined(CONFIG_SMP)
	unsigned int hart_id;

	__asm__ volatile("csrr %0, mhartid" : "=r" (hart_id));

	return(&_kernel.cpus[hart_id - CONFIG_SMP_BASE_CPU]);
#else /* CONFIG_SMP */
	return(&_kernel.cpus[0]);
#endif /* CONFIG_SMP */
}

#endif /* CONFIG_RISCV */

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_RISCV_ARCH_INLINES_H_ */
