/*
 * Copyright (c) 2021 Intel Corporation
 * Copyright (c) 2026 Andes Technology Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <kernel_arch_interface.h>
#include <ipi.h>
#include <zephyr/drivers/interrupt_controller/riscv_plic.h>

#define PLIC_NODE DT_NODELABEL(plic_sw)
#if !DT_NODE_EXISTS(PLIC_NODE)
#error "Label 'plic_sw' is not defined in the devicetree."
#endif

#define IPI_IRQN(irq) (IRQ_TO_L2((irq) + 1) | DT_IRQN(PLIC_NODE))

static atomic_val_t cpu_pending_ipi[CONFIG_MP_MAX_NUM_CPUS];
#define IPI_SCHED     0
#define IPI_FPU_FLUSH 1

void arch_sched_directed_ipi(uint32_t cpu_bitmap)
{
	unsigned int key = arch_irq_lock();
	unsigned int id = _current_cpu->id;
	unsigned int num_cpus = arch_num_cpus();

	for (unsigned int i = 0; i < num_cpus; i++) {
		if ((i != id) && _kernel.cpus[i].arch.online && ((cpu_bitmap & BIT(i)) != 0)) {
			atomic_set_bit(&cpu_pending_ipi[i], IPI_SCHED);
			riscv_plic_irq_set_pending(IPI_IRQN(i));
		}
	}

	arch_irq_unlock(key);
}

#ifdef CONFIG_FPU_SHARING
void arch_flush_fpu_ipi(unsigned int cpu)
{
	atomic_set_bit(&cpu_pending_ipi[cpu], IPI_FPU_FLUSH);
	riscv_plic_irq_set_pending(IPI_IRQN(cpu));
}
#endif /* CONFIG_FPU_SHARING */

static void sched_ipi_handler(const void *unused)
{
	ARG_UNUSED(unused);

	atomic_val_t pending_ipi = atomic_clear(&cpu_pending_ipi[_current_cpu->id]);

	if (pending_ipi & ATOMIC_MASK(IPI_SCHED)) {
		z_sched_ipi();
	}
#ifdef CONFIG_FPU_SHARING
	if (pending_ipi & ATOMIC_MASK(IPI_FPU_FLUSH)) {
		/* disable IRQs */
		csr_clear(mstatus, MSTATUS_IEN);
		/* perform the flush */
		arch_flush_local_fpu();
		/*
		 * No need to re-enable IRQs here as long as
		 * this remains the last case.
		 */
	}
#endif /* CONFIG_FPU_SHARING */
}

#if defined(CONFIG_FPU_SHARING) || defined(CONFIG_RISCV_CUSTOM_ANDES_MORE_SPIN_RELAX_NOPS)
void arch_spin_relax(void)
{
#ifdef CONFIG_FPU_SHARING
	/*
	 * Make sure there is no pending FPU flush request for this CPU while
	 * waiting for a contended spinlock to become available. This prevents
	 * a deadlock when the lock we need is already taken by another CPU
	 * that also wants its FPU content to be reinstated while such content
	 * is still live in this CPU's FPU.
	 */
	atomic_val_t *pending_ipi = &cpu_pending_ipi[_current_cpu->id];

	if (atomic_test_and_clear_bit(pending_ipi, IPI_FPU_FLUSH)) {
		/*
		 * We may not be in IRQ context here hence cannot use
		 * arch_flush_local_fpu() directly. Use the helper that
		 * flushes the FPU context without altering thread options.
		 */
		z_riscv_fpu_flush_thread(_current_cpu->arch.fpu_owner);
	}
#endif /* CONFIG_FPU_SHARING */

#ifdef CONFIG_RISCV_CUSTOM_ANDES_MORE_SPIN_RELAX_NOPS
	uint32_t remaining = CONFIG_RISCV_CUSTOM_ANDES_NUM_SPIN_RELAX_NOPS;

#ifdef CONFIG_RISCV_CUSTOM_ANDES_SPIN_RELAX_NOPS_JITTER
	const uint32_t jitter_step = 64U / CONFIG_MP_MAX_NUM_CPUS;

	/* Add bounded jitter to prevent CPUs from retrying RCW at the same time. */
	remaining += jitter_step * (csr_read(mcycle) % CONFIG_MP_MAX_NUM_CPUS);
#endif /* CONFIG_RISCV_CUSTOM_ANDES_SPIN_RELAX_NOPS_JITTER */

	while (remaining > 0U) {
		arch_nop();
		remaining--;
	}
#endif /* CONFIG_RISCV_CUSTOM_ANDES_MORE_SPIN_RELAX_NOPS */
}
#endif /* CONFIG_FPU_SHARING || CONFIG_RISCV_CUSTOM_ANDES_MORE_SPIN_RELAX_NOPS */

#define PER_CPU_IRQ_CONNECT(idx, _) IRQ_CONNECT(IPI_IRQN(idx), 1, sched_ipi_handler, NULL, 0)

int arch_smp_init(void)
{
	LISTIFY(CONFIG_MP_MAX_NUM_CPUS, PER_CPU_IRQ_CONNECT, (;));

	for (unsigned int id = 0; id < arch_num_cpus(); id++) {
#ifdef CONFIG_PLIC_IRQ_AFFINITY
		riscv_plic_irq_set_affinity(IPI_IRQN(id), BIT(id));
#endif /* CONFIG_PLIC_IRQ_AFFINITY */
		riscv_plic_irq_enable(IPI_IRQN(id));
	}

	irq_enable(RISCV_IRQ_MSOFT);

	return 0;
}
