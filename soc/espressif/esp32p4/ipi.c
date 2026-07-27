/*
 * Copyright (c) 2026 Process Mission
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/arch_interface.h>
#include <zephyr/arch/riscv/csr.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>

#include <ipi.h>
#include <kernel_arch_interface.h>

#include <esp_cpu.h>
#include <soc/interrupts.h>
#include <hal/crosscore_int_ll.h>

/*
 * ESP32-P4 cross-core interrupt used as scheduler IPI.
 *
 * Writing HP_SYSTEM_CPU_INT_FROM_CPU_x raises a level interrupt on core x;
 * ETS_FROM_CPU_INTR0_SOURCE targets core 0, ETS_FROM_CPU_INTR1_SOURCE
 * targets core 1. As in ESP-IDF's crosscore_int.c, each core routes and
 * enables its own source through the CLIC interrupt matrix, which
 * esp_intr_alloc() does for the calling core.
 */
#ifdef CONFIG_FPU_SHARING
/*
 * Pending IPI reasons per CPU. The cross-core interrupt itself is the
 * scheduler IPI and needs no flag; FPU flush requests do.
 */
static atomic_val_t cpu_pending_ipi[CONFIG_MP_MAX_NUM_CPUS];
#define IPI_FPU_FLUSH 0
#endif /* CONFIG_FPU_SHARING */

static void esp32p4_crosscore_isr(void *arg)
{
	ARG_UNUSED(arg);

	/* Clear first so an IPI raised while handling is not lost. */
	crosscore_int_ll_clear_interrupt((int)esp_cpu_get_core_id());

	z_sched_ipi();

#ifdef CONFIG_FPU_SHARING
	if (atomic_test_and_clear_bit(&cpu_pending_ipi[_current_cpu->id], IPI_FPU_FLUSH)) {
		/* Disable IRQs, then perform the flush. */
		csr_clear(mstatus, MSTATUS_IEN);
		arch_flush_local_fpu();
	}
#endif /* CONFIG_FPU_SHARING */
}

static int esp32p4_crosscore_init(void)
{
	int source = (esp_cpu_get_core_id() == 0U) ? ETS_FROM_CPU_INTR0_SOURCE
						   : ETS_FROM_CPU_INTR1_SOURCE;

	return esp_intr_alloc(source, 0, esp32p4_crosscore_isr, NULL, NULL);
}

void arch_sched_directed_ipi(uint32_t cpu_bitmap)
{
	unsigned int key = arch_irq_lock();
	unsigned int id = _current_cpu->id;

	for (unsigned int cpu = 0; cpu < arch_num_cpus(); cpu++) {
		if ((cpu != id) && _kernel.cpus[cpu].arch.online &&
		    ((cpu_bitmap & BIT(cpu)) != 0U)) {
			crosscore_int_ll_trigger_interrupt((int)cpu);
		}
	}

	arch_irq_unlock(key);
}

int arch_smp_init(void)
{
	/* Runs on CPU0 during kernel init: install CPU0's IPI handler. */
	return esp32p4_crosscore_init();
}

void soc_per_core_init_hook(void)
{
	/*
	 * Runs on every core. CPU0 cannot use it: there it is invoked from
	 * arch_kernel_init(), before the kernel heap that esp_intr_alloc()
	 * needs is initialized, so CPU0 is handled by arch_smp_init()
	 * instead. Secondary cores install their handler here.
	 */
	if (esp_cpu_get_core_id() != 0U) {
		int ret = esp32p4_crosscore_init();

		__ASSERT(ret == 0, "CPU%u: failed to allocate cross-core interrupt (%d)",
			 (unsigned int)esp_cpu_get_core_id(), ret);
		ARG_UNUSED(ret);
	}
}

#ifdef CONFIG_FPU_SHARING
void arch_flush_fpu_ipi(unsigned int cpu)
{
	atomic_set_bit(&cpu_pending_ipi[cpu], IPI_FPU_FLUSH);
	crosscore_int_ll_trigger_interrupt((int)cpu);
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
	if (atomic_test_and_clear_bit(&cpu_pending_ipi[_current_cpu->id], IPI_FPU_FLUSH)) {
		/*
		 * We may not be in IRQ context here hence cannot use
		 * arch_flush_local_fpu() directly.
		 */
		arch_float_disable(_current_cpu->arch.fpu_owner);
	}
}
#endif /* CONFIG_FPU_SHARING */
