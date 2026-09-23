/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/fatal.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <ipi.h>

#include <esp_cpu.h>
#include <esp_rom_sys.h>
#include <rom/ets_sys.h>
#include <hal/cpu_utility_ll.h>
#include <soc/system_reg.h>
#include <soc/soc.h>

#include "cache.h"
#include <esp32p4/rom/cache.h>

#include <esp_mp_stall.h>
#include <soc.h>

#ifdef CONFIG_FPU_SHARING
#include <zephyr/sys/atomic.h>
#include <kernel_arch_interface.h>
#endif

#define IPI_LINE_ALLOC(node, handler)                                                              \
	esp_intr_alloc(DT_IRQ_BY_IDX(DT_NODELABEL(node), 0, irq),                                  \
		ESP_PRIO_TO_FLAGS(DT_IRQ_BY_IDX(DT_NODELABEL(node), 0, priority)) |                \
			ESP_INT_FLAGS_CHECK(DT_IRQ_BY_IDX(DT_NODELABEL(node), 0, flags)) |         \
			ESP_INTR_FLAG_IRAM,                                                        \
		handler, NULL, NULL)

/* CPU1 entry point jumped to by the ROM. Sets up the per-core CSRs and
 * continues into the generic RISC-V __initialize.
 */
void IRAM_ATTR __attribute__((naked, noreturn)) esp_appcpu_entry(void)
{
	__asm__ volatile(
		/* Disable interrupts */
		"csrci mstatus, 0x8\n"

		/* mtvec = _vector_table */
		"la t0, _vector_table\n"
		"csrw mtvec, t0\n"

		/* mtvt = _mtvt_table (CLIC vectored table) */
		"la t0, _mtvt_table\n"
		"csrw 0x307, t0\n"

#ifdef CONFIG_FPU
		/* Enable FPU */
		"li t0, %0\n"
		"csrs mstatus, t0\n"
		"fscsr zero\n"
#endif

#ifdef CONFIG_RISCV_GP
		/* Set up the global pointer */
		".option push\n"
		".option norelax\n"
		"la gp, __global_pointer$\n"
		".option pop\n"
#endif

		"j __initialize\n"
#ifdef CONFIG_FPU
		::"i"(MSTATUS_FS_INIT)
#endif
	);
}

/* Reset and release CPU1 into esp_appcpu_entry(). CPU1 then waits in
 * boot_secondary_core until arch_cpu_start() wakes it.
 */
void IRAM_ATTR esp_appcpu_start(void)
{
	esp_cpu_stall(1);
	esp_cpu_reset(1);
	esp_cpu_unstall(1);

	cpu_utility_ll_enable_clock_and_reset_app_cpu();
	cpu_utility_ll_enable_clock_and_reset_app_cpu_int_matrix();

	ets_set_appcpu_boot_addr((uint32_t)esp_appcpu_entry);

	/* Drop the L1 instruction cache lines left stale by the CPU1 release. */
	Cache_Invalidate_All(CACHE_MAP_L1_ICACHE_MASK);
	__asm__ volatile("fence.i");
}

#ifdef CONFIG_SMP
#ifdef CONFIG_FPU_SHARING
/* The scheduler IPI also carries FPU flush requests. Pending reasons are
 * kept per CPU and set before the line is asserted.
 */
static atomic_t cpu_pending_ipi[CONFIG_MP_MAX_NUM_CPUS];

#define IPI_SCHED     0
#define IPI_FPU_FLUSH 1
#endif /* CONFIG_FPU_SHARING */

/* Assert the FROM_CPU line the target core listens on. */
static ALWAYS_INLINE void IRAM_ATTR raise_ipi(int target_cpu)
{
	if (target_cpu == 1) {
		WRITE_PERI_REG(SYSTEM_CPU_INTR_FROM_CPU_0_REG, SYSTEM_CPU_INTR_FROM_CPU_0);
	} else {
		WRITE_PERI_REG(SYSTEM_CPU_INTR_FROM_CPU_1_REG, SYSTEM_CPU_INTR_FROM_CPU_1);
	}
}

static void IRAM_ATTR crosscore_isr(void *arg)
{
	ARG_UNUSED(arg);

	if (esp_cpu_get_core_id() == 0) {
		WRITE_PERI_REG(SYSTEM_CPU_INTR_FROM_CPU_1_REG, 0);
	} else {
		WRITE_PERI_REG(SYSTEM_CPU_INTR_FROM_CPU_0_REG, 0);
	}

#ifdef CONFIG_FPU_SHARING
	atomic_val_t pending = atomic_clear(&cpu_pending_ipi[esp_cpu_get_core_id()]);

	if (pending & BIT(IPI_SCHED)) {
		z_sched_ipi();
	}

	if (pending & BIT(IPI_FPU_FLUSH)) {
		/* arch_flush_local_fpu() needs interrupts disabled. */
		unsigned int key = arch_irq_lock();

		arch_flush_local_fpu();
		arch_irq_unlock(key);
	}
#else
	z_sched_ipi();
#endif /* CONFIG_FPU_SHARING */
}

void arch_sched_directed_ipi(uint32_t cpu_bitmap)
{
	const int core_id = esp_cpu_get_core_id();
	const int target = (core_id == 0) ? 1 : 0;

	if ((cpu_bitmap & BIT(target)) == 0) {
		return;
	}

#ifdef CONFIG_FPU_SHARING
	atomic_set_bit(&cpu_pending_ipi[target], IPI_SCHED);
#endif
	raise_ipi(target);
}

#ifdef CONFIG_FPU_SHARING
void arch_flush_fpu_ipi(unsigned int cpu)
{
	atomic_set_bit(&cpu_pending_ipi[cpu], IPI_FPU_FLUSH);
	raise_ipi(cpu);
}

/* Service a pending FPU flush request while spinning on a lock. */
void arch_spin_relax(void)
{
	atomic_t *pending = &cpu_pending_ipi[_current_cpu->id];

	if (atomic_test_and_clear_bit(pending, IPI_FPU_FLUSH)) {
		z_riscv_fpu_flush_thread(_current_cpu->arch.fpu_owner);
	}
}
#endif /* CONFIG_FPU_SHARING */

int esp_mp_stall_isr_register(int core_id)
{
	/* Clear any pending request before enabling the line. */
	if (core_id == 0) {
		WRITE_PERI_REG(SYSTEM_CPU_INTR_FROM_CPU_2_REG, 0);
		return IPI_LINE_ALLOC(ipi2, (intr_handler_t)esp_mp_stall_isr);
	}

	WRITE_PERI_REG(SYSTEM_CPU_INTR_FROM_CPU_3_REG, 0);
	return IPI_LINE_ALLOC(ipi3, (intr_handler_t)esp_mp_stall_isr);
}

/* Register the scheduler and stall IPIs of the calling core. */
static void register_ipi(void)
{
	int core_id = esp_cpu_get_core_id();
	int ret;

	/* Scheduler IPI: CPU0 listens on ipi1 (driven by CPU1), CPU1 on ipi0. */
	if (core_id == 0) {
		ret = IPI_LINE_ALLOC(ipi1, crosscore_isr);
	} else {
		ret = IPI_LINE_ALLOC(ipi0, crosscore_isr);
	}

	if (ret == 0) {
		ret = esp_mp_stall_isr_register(core_id);
	}

	if (ret != 0) {
		esp_rom_printf("CPU%d: IPI setup failed: %d\n", core_id, ret);
		k_fatal_halt(K_ERR_KERNEL_PANIC);
	}

	/* Mark this core online before the stall gate is armed. */
	esp_mp_set_cpu_online(core_id, true);

	/* CPU1 registers last, so both stall ISRs are installed here. */
	if (core_id == 1) {
		esp_mp_stall_enable();
	}
}

#endif /* CONFIG_SMP */

/* CPU0 runs this before the kernel heap exists and registers its IPIs
 * from arch_smp_init(). CPU1 registers them here.
 */
void soc_per_core_init_hook(void)
{
	if (esp_cpu_get_core_id() == 0) {
		return;
	}
	esp_core_intr_matrix_clear();
#ifdef CONFIG_SMP
	register_ipi();
#endif

#if defined(CONFIG_NOCACHE_MEMORY)
	/* Program the nocache PMA entry on this core too. */
	nocache_region_init();
#endif
}

#ifdef CONFIG_SMP
int arch_smp_init(void)
{
	register_ipi();
	return 0;
}
#endif /* CONFIG_SMP */
