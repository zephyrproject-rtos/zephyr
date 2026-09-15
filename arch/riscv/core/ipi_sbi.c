/*
 * Copyright (c) 2026 BeagleBoard.org Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/riscv/sbi.h>
#include <zephyr/kernel.h>
#include <kernel_arch_interface.h>
#include <ipi.h>

static atomic_val_t cpu_pending_ipi[CONFIG_MP_MAX_NUM_CPUS];
#define IPI_SCHED     0
#define IPI_FPU_FLUSH 1

static int send_ipi(unsigned long hart_mask, unsigned long hart_mask_base)
{
	register unsigned long a0 __asm__("a0") = hart_mask;
	register unsigned long a1 __asm__("a1") = hart_mask_base;
	register unsigned long a6 __asm__("a6") = SBI_FUNC_SEND_IPI;
	register unsigned long a7 __asm__("a7") = SBI_EXT_IPI;

	__asm__ volatile("ecall" : "+r"(a0), "+r"(a1) : "r"(a6), "r"(a7) : "memory");

	return sbi_err_to_errno(a0);
}

void arch_sched_directed_ipi(uint32_t cpu_bitmap)
{
	unsigned long hartid, hmask = 0, hbase = 0, htop = 0;
	unsigned int i;
	unsigned int key = arch_irq_lock();
	unsigned int id = _current_cpu->id;
	unsigned int num_cpus = arch_num_cpus();

	/* Inspired by Linux __sbi_rfence_v02 */
	for (i = 0; i < num_cpus; i++) {
		if ((i == id) || !IS_BIT_SET(cpu_bitmap, i) || !_kernel.cpus[i].arch.online) {
			continue;
		}

		atomic_set_bit(&cpu_pending_ipi[i], IPI_SCHED);
		hartid = _kernel.cpus[i].arch.hartid;

		if (hmask) {
			if (hartid + BITS_PER_LONG <= htop || hbase + BITS_PER_LONG <= hartid) {
				send_ipi(hmask, hbase);
				hmask = 0;
			} else if (hartid < hbase) {
				/* shift the mask to fit lower hartid */
				hmask <<= hbase - hartid;
				hbase = hartid;
			}
		}
		if (!hmask) {
			hbase = hartid;
			htop = hartid;
		} else if (hartid > htop) {
			htop = hartid;
		}
		WRITE_BIT(hmask, hartid - hbase, 1);
	}

	if (hmask) {
		send_ipi(hmask, hbase);
	}

	arch_irq_unlock(key);
}

#ifdef CONFIG_FPU_SHARING
void arch_flush_fpu_ipi(unsigned int cpu)
{
	atomic_set_bit(&cpu_pending_ipi[cpu], IPI_FPU_FLUSH);
	send_ipi(1UL, _kernel.cpus[cpu].arch.hartid);
}
#endif /* CONFIG_FPU_SHARING */

static void sched_ipi_handler(const void *unused)
{
	ARG_UNUSED(unused);

	csr_clear(sip, SIP_SSIP);

	atomic_val_t pending_ipi = atomic_clear(&cpu_pending_ipi[_current_cpu->id]);

	if (pending_ipi & ATOMIC_MASK(IPI_SCHED)) {
		z_sched_ipi();
	}
#ifdef CONFIG_FPU_SHARING
	if (pending_ipi & ATOMIC_MASK(IPI_FPU_FLUSH)) {
		/* disable IRQs */
		csr_clear(sstatus, SSTATUS_SIE);
		/* perform the flush */
		arch_flush_local_fpu();
		/*
		 * No need to re-enable IRQs here as long as
		 * this remains the last case.
		 */
	}
#endif /* CONFIG_FPU_SHARING */
}

#ifdef CONFIG_FPU_SHARING
/*
 * Make sure there is no pending FPU flush request for this CPU while
 * waiting for a contended spinlock to become available. This prevents
 * a deadlock when the lock we need is already taken by another CPU
 * that also wants its FPU content to be reinstated while such content
 * is still live in this CPU's FPU.
 */
void arch_spin_relax(void)
{
	atomic_val_t *pending_ipi = &cpu_pending_ipi[_current_cpu->id];

	if (atomic_test_and_clear_bit(pending_ipi, IPI_FPU_FLUSH)) {
		/*
		 * We may not be in IRQ context here hence cannot use
		 * arch_flush_local_fpu() directly. Use the helper that
		 * flushes the FPU context without altering thread options.
		 */
		z_riscv_fpu_flush_thread(_current_cpu->arch.fpu_owner);
	}
}
#endif /* CONFIG_FPU_SHARING */

int arch_smp_init(void)
{
	IRQ_CONNECT(RISCV_IRQ_SSOFT, 0, sched_ipi_handler, NULL, 0);
	irq_enable(RISCV_IRQ_SSOFT);

	return 0;
}
