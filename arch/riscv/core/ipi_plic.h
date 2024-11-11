/*
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_RISCV_CORE_IPI_PLIC_H_
#define ZEPHYR_ARCH_RISCV_CORE_IPI_PLIC_H_

#include <zephyr/devicetree.h>
#include <zephyr/drivers/interrupt_controller/riscv_plic.h>
#include <zephyr/irq_multilevel.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#define DT_DRV_COMPAT zephyr_ipi_plic

#define IPI_PLIC_IRQS(n, _) DT_INST_IRQN_BY_IDX(0, n)

/* Should get this from the devicetree, placeholder now */
static const uint32_t ipi_irqs[CONFIG_MP_MAX_NUM_CPUS] = {
	LISTIFY(CONFIG_MP_MAX_NUM_CPUS, IPI_PLIC_IRQS, (,)),
};

static ALWAYS_INLINE void z_riscv_ipi_send(unsigned int cpu)
{
	riscv_plic_irq_set_pending(ipi_irqs[cpu]);
}

static ALWAYS_INLINE void z_riscv_ipi_clear(unsigned int cpu)
{
	ARG_UNUSED(cpu);
	/* IRQ will be cleared by PLIC */
}

static void sched_ipi_handler(const void *arg)
{
	unsigned int cpu_id = POINTER_TO_UINT(arg);

	z_riscv_sched_ipi_handler(cpu_id);
}

#define IPI_PLIC_IRQ_CONNECT(n, _)                                                                 \
	IRQ_CONNECT(DT_INST_IRQN_BY_IDX(0, n), 1, sched_ipi_handler, UINT_TO_POINTER(n), 0);       \
	irq_enable(n);                                                                             \
	riscv_plic_irq_set_affinity(n, BIT(n))

int arch_smp_init(void)
{
	LISTIFY(CONFIG_MP_MAX_NUM_CPUS, IPI_PLIC_IRQ_CONNECT, (;));

	return 0;
}

#endif /* ZEPHYR_ARCH_RISCV_CORE_IPI_PLIC_H_ */
