/*
 * Copyright (c) 2021, 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ipi.h>
#include <ksched.h>

#include <zephyr/kernel.h>

#define SWINT_NODE DT_NODELABEL(swint)
#if !DT_NODE_EXISTS(SWINT_NODE)
#error "Label 'swint' is not defined in the devicetree."
#endif
#define MSIP_BASE    DT_REG_ADDR_RAW(SWINT_NODE)
#define MSIP(hartid) ((volatile uint32_t *)MSIP_BASE)[hartid]

static atomic_val_t cpu_pending_ipi[CONFIG_MP_MAX_NUM_CPUS];
#define IPI_SCHED 0

void arch_sched_directed_ipi(uint32_t cpu_bitmap)
{
	unsigned int key = arch_irq_lock();
	unsigned int id = _current_cpu->id;
	unsigned int num_cpus = arch_num_cpus();

	for (unsigned int i = 0; i < num_cpus; i++) {
		if ((i != id) && _kernel.cpus[i].arch.online && ((cpu_bitmap & BIT(i)) != 0)) {
			atomic_set_bit(&cpu_pending_ipi[i], IPI_SCHED);
			MSIP(_kernel.cpus[i].arch.hartid) = 1;
		}
	}

	arch_irq_unlock(key);
}

static void sched_ipi_handler(const void *unused)
{
	ARG_UNUSED(unused);

	MSIP(csr_read(mhartid)) = 0;

	atomic_val_t pending_ipi = atomic_clear(&cpu_pending_ipi[_current_cpu->id]);

	if (pending_ipi & ATOMIC_MASK(IPI_SCHED)) {
		z_sched_ipi();
	}
}

int arch_smp_init(void)
{

	IRQ_CONNECT(RISCV_IRQ_MSOFT, 0, sched_ipi_handler, NULL, 0);
	irq_enable(RISCV_IRQ_MSOFT);

	return 0;
}
