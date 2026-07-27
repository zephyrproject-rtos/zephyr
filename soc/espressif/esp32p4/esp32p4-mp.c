/*
 * Copyright (c) 2026 Process Mission
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/arch_interface.h>
#include <zephyr/arch/riscv/arch.h>
#include <zephyr/arch/riscv/csr.h>
#include <kernel_internal.h>
#include <soc.h>
#include <esp_cpu.h>
#include <hal/cpu_utility_ll.h>
#include <hal/interrupt_clic_ll.h>
#include <riscv/interrupt.h>
#include <soc/interrupts.h>

extern volatile uintptr_t riscv_cpu_boot_flag;
extern volatile void *riscv_cpu_sp;

extern volatile struct {
	arch_cpustart_t fn;
	void *arg;
} riscv_cpu_init[CONFIG_MP_MAX_NUM_CPUS];

extern char _vector_table[];
extern char _mtvt_table[];

void arch_secondary_cpu_init(int hartid);

/* RISC-V CLIC machine trap-vector table CSR (mtvt). */
#define CSR_MTVT 0x307U

/*
 * Clear this core's CLIC interrupt routing matrix and set all CPU
 * interrupt lines to vectored mode, mirroring what
 * core_intr_matrix_clear() in soc.c does for the boot core (and what
 * ESP-IDF's cpu_start.c does on every core). Runs at SMP init time,
 * after the flash cache is up, so plain .text is fine.
 */
static void esp32p4_secondary_intr_matrix_init(void)
{
	for (int i = 0; i < ETS_MAX_INTR_SOURCE; i++) {
		interrupt_clic_ll_route(esp_cpu_get_core_id(), i, ETS_INVALID_INUM);
	}

	for (int i = 0; i < SOC_CPU_INTR_NUM; i++) {
		esprv_int_set_vectored(i, true);
	}
}

/*
 * C portion of the CPU1 bring-up. Reached from
 * esp32p4_zephyr_secondary_entry() with a valid stack and global pointer.
 * Runs on CPU1 in M-mode.
 */
void esp32p4_secondary_start(int hartid)
{
	/* Set up CLIC vectors for this core. */
	csr_write(mtvec, (uintptr_t)_vector_table);
	csr_write(CSR_MTVT, (uintptr_t)_mtvt_table);

	esp32p4_secondary_intr_matrix_init();

	/* Clear the boot address so a later reset does not re-enter here. */
	ets_set_appcpu_boot_addr(0);

	/* Signal the boot CPU that we have taken our stack and vectors. */
	riscv_cpu_boot_flag = 1U;

	/*
	 * Point this CPU's current thread at its idle thread, which the
	 * boot CPU initialized in z_init_cpu() before starting us. The
	 * scheduler only installs the dummy thread on the first context
	 * switch, and without a valid _current the irq_lock() taken by
	 * soc_per_core_init_hook() (esp_intr_alloc()) would dereference
	 * NULL. The hartid matches the kernel CPU index on this SoC.
	 */
#ifdef CONFIG_MULTITHREADING
	_kernel.cpus[hartid].current = &z_idle_threads[hartid];
#endif

	arch_secondary_cpu_init(hartid);
}

/*
 * CPU1 entry point. The ROM jumps here after arch_cpu_start() sets the
 * appcpu boot address and unstalled CPU1. Sets up the global pointer and
 * the per-CPU stack prepared by arch_cpu_start(), then continues in C.
 * Naked: this runs with an undefined stack pointer straight from the ROM,
 * so the compiler must not emit a prologue touching the stack.
 */
static void __attribute__((naked)) esp32p4_zephyr_secondary_entry(void)
{
	__asm__ volatile(
		".option push\n"
		".option norelax\n"
		"la gp, __global_pointer$\n"
		".option pop\n"

		"csrw mie, zero\n"
		"csrci mstatus, %[mie]\n"
		:
		: [mie] "i"(MSTATUS_MIE)
		: "gp", "memory");

#ifdef CONFIG_FPU
	/*
	 * This entry bypasses reset.S; enable the FPU now or the first ISR's
	 * FP context save traps (mirrors __esp_platform_app_start()).
	 */
	__asm__ volatile("li t0, %0\n"
			 "csrs mstatus, t0\n"
			 "fscsr zero\n" ::"i"(MSTATUS_FS_INIT)
			 : "t0");
#endif

	__asm__ volatile(
		/* Acquire fence paired with the release fence in arch_cpu_start(). */
		"fence rw, rw\n"

		"la t0, riscv_cpu_sp\n"
		"lw sp, 0(t0)\n"

		"csrr a0, mhartid\n"
		"tail esp32p4_secondary_start\n"
		:
		:
		: "t0", "a0", "memory");
}

void arch_cpu_start(int cpu_num, k_thread_stack_t *stack, int sz,
		    arch_cpustart_t fn, void *arg)
{
	riscv_cpu_init[cpu_num].fn = fn;
	riscv_cpu_init[cpu_num].arg = arg;

	riscv_cpu_sp = K_KERNEL_STACK_BUFFER(stack) + sz;
	riscv_cpu_boot_flag = 0U;

	/*
	 * Release fence paired with the acquire fence in the secondary entry;
	 * I/O is included so the stores above are ordered against the MMIO
	 * writes releasing CPU1.
	 */
	__asm__ volatile("fence iorw, iorw" ::: "memory");

	ets_set_appcpu_boot_addr((uint32_t)esp32p4_zephyr_secondary_entry);
	cpu_utility_ll_enable_clock_and_reset_app_cpu();
	esp_cpu_unstall(1);

	while (riscv_cpu_boot_flag == 0U) {
	}
}
