/*
 * Copyright (c) 2024 Meta Platforms
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ipi.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/interrupt_controller/riscv_plic.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#ifdef CONFIG_FPU_SHARING
#define FPU_IPI_NODE DT_NODELABEL(fpu_ipi)
#define FPU_IPI_NUM_IRQS DT_NUM_IRQS(FPU_IPI_NODE)
#define FPU_IPI_IRQ(n) DT_IRQN_BY_IDX(FPU_IPI_NODE, n + CONFIG_MP_MAX_NUM_CPUS)
#define FPU_IPI_IRQS_FN(n, _) DT_IRQN_BY_IDX(FPU_IPI_NODE, n)
static const uint32_t fpu_ipi_irqs[FPU_IPI_NUM_IRQS] = {
	LISTIFY(FPU_IPI_NUM_IRQS, FPU_IPI_IRQS_FN, (,)),
};

static ALWAYS_INLINE void send_fpu_ipi(int cpu)
{
	riscv_plic_irq_set_pending(fpu_ipi_irqs[cpu]);
}

static ALWAYS_INLINE bool fpu_ipi_irq_is_pending(int cpu)
{
	return riscv_plic_irq_is_pending(fpu_ipi_irqs[cpu]);
}

static ALWAYS_INLINE void fpu_ipi_irq_clear_pending(int cpu)
{
	riscv_plic_irq_clear_pending(fpu_ipi_irqs[cpu]);
}

static void fpu_ipi_handler(const void *arg)
{
	ARG_UNUSED(arg);

	/* disable IRQs */
	csr_clear(mstatus, MSTATUS_IEN);
	/* perform the flush */
	arch_flush_local_fpu();
	/*
	 * No need to re-enable IRQs here as long as
	 * this remains the last case.
	 */
}

void arch_flush_fpu_ipi(unsigned int cpu)
{
	send_fpu_ipi(i);
}

/*
 * Make sure there is no pending FPU flush request for this CPU while
 * waiting for a contended spinlock to become available. This prevents
 * a deadlock when the lock we need is already taken by another CPU
 * that also wants its FPU content to be reinstated while such content
 * is still live in this CPU's FPU.
 */
void arch_spin_relax(void)
{
	int cpu = _current_cpu->id;

	if (fpu_ipi_irq_is_pending(cpu)) {
		fpu_ipi_irq_clear_pending(cpu);
		/*
		 * We may not be in IRQ context here hence cannot use
		 * arch_flush_local_fpu() directly.
		 */
		arch_float_disable(_current_cpu->arch.fpu_owner);
	}
}
#define FPU_IPI_IRQ_CONNECT(n, _)                                                                  \
	IRQ_CONNECT(FPU_IPI_IRQ(n), 1, fpu_ipi_handler, UINT_TO_POINTER(n), 0);                    \
	irq_enable(FPU_IPI_IRQ(n));                                                                \
	riscv_plic_irq_set_affinity(FPU_IPI_IRQ(n), BIT(n))

#define fpu_ipi_irqs_setup() LISTIFY(CONFIG_MP_MAX_NUM_CPUS, FPU_IPI_IRQ_CONNECT, (;))
#else
#define fpu_ipi_irqs_setup()
#endif /* CONFIG_FPU_SHARING */

#define SCHED_IPI_NODE DT_NODELABEL(sched_ipi)
#define SCHED_IPI_NUM_IRQS DT_NUM_IRQS(SCHED_IPI_NODE)
#define SCHED_IPI_IRQ(n) DT_IRQN_BY_IDX(SCHED_IPI_NODE, n)
#define SCHED_IPI_IRQS_FN(n, _) DT_IRQN_BY_IDX(SCHED_IPI_NODE, n)
static const uint32_t sched_ipi_irqs[SCHED_IPI_NUM_IRQS] = {
	LISTIFY(SCHED_IPI_NUM_IRQS, SCHED_IPI_IRQS_FN, (,)),
};

static ALWAYS_INLINE void send_sched_ipi(int cpu)
{
	riscv_plic_irq_set_pending(sched_ipi_irqs[cpu]);
}

void arch_sched_directed_ipi(uint32_t cpu_bitmap)
{
	unsigned int key = arch_irq_lock();
	unsigned int id = _current_cpu->id;
	unsigned int num_cpus = arch_num_cpus();

	for (unsigned int i = 0; i < num_cpus; i++) {
		if ((i != id) && _kernel.cpus[i].arch.online && ((cpu_bitmap & BIT(i)) != 0)) {
			send_sched_ipi(i);
		}
	}

	arch_irq_unlock(key);
}

static void sched_ipi_handler(const void *arg)
{
	ARG_UNUSED(arg);

	z_sched_ipi();
}

#define SCHED_IPI_IRQ_CONNECT(n, _)                                                                \
	IRQ_CONNECT(SCHED_IPI_IRQ(n), 1, sched_ipi_handler, UINT_TO_POINTER(n), 0);                \
	irq_enable(SCHED_IPI_IRQ(n));                                                              \
	riscv_plic_irq_set_affinity(SCHED_IPI_IRQ(n), BIT(n))

#define sched_ipi_irqs_setup() LISTIFY(CONFIG_MP_MAX_NUM_CPUS, SCHED_IPI_IRQ_CONNECT, (;))

int arch_smp_init(void)
{
	sched_ipi_irqs_setup();
	fpu_ipi_irqs_setup();

	return 0;
}

