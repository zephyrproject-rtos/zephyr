/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_RISCV_CORE_IPI_CLINT_H_
#define ZEPHYR_ARCH_RISCV_CORE_IPI_CLINT_H_

#include <zephyr/kernel.h>

#define MSIP_BASE 0x2000000UL
#define MSIP(hartid) ((volatile uint32_t *)MSIP_BASE)[hartid]

static ALWAYS_INLINE void z_riscv_ipi_send(unsigned int cpu)
{
	MSIP(_kernel.cpus[cpu].arch.hartid) = 1;
}

static ALWAYS_INLINE void z_riscv_ipi_clear(unsigned int cpu)
{
	MSIP(_kernel.cpus[cpu].arch.hartid) = 0;
}

static void sched_ipi_handler(const void *unused)
{
	ARG_UNUSED(unused);

	z_riscv_sched_ipi_handler(_current_cpu->id);
}

int arch_smp_init(void)
{
	IRQ_CONNECT(RISCV_IRQ_MSOFT, 0, sched_ipi_handler, NULL, 0);
	irq_enable(RISCV_IRQ_MSOFT);

	return 0;
}

#endif /* ZEPHYR_ARCH_RISCV_CORE_IPI_CLINT_H_ */
