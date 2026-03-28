/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cpuid.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <kernel_arch_data.h>
#include <kernel_arch_func.h>
#include <zephyr/kernel_structs.h>
#include <kernel_internal.h>
#include <zephyr/arch/x86/multiboot.h>
#include <x86_mmu.h>
#include <zephyr/drivers/interrupt_controller/loapic.h>
#include <cet.h>
#include <zephyr/arch/common/xip.h>
#include <zephyr/arch/common/init.h>
#ifdef CONFIG_ACPI
#include <zephyr/arch/x86/cpuid.h>
#include <zephyr/acpi/acpi.h>
#endif

/*
 * Map of CPU logical IDs to CPU local APIC IDs. By default,
 * we assume this simple identity mapping, as found in QEMU.
 * The symbol is weak so that boards/SoC files can override.
 */

#if defined(CONFIG_ACPI)
__weak uint8_t x86_cpu_loapics[CONFIG_MP_MAX_NUM_CPUS];
#else
#define INIT_CPUID(n, _) n
__weak uint8_t x86_cpu_loapics[] = {
	LISTIFY(CONFIG_MP_MAX_NUM_CPUS, INIT_CPUID, (,)),};
#endif
extern char x86_ap_start[]; /* AP entry point in locore.S */

LISTIFY(CONFIG_MP_MAX_NUM_CPUS, ACPI_CPU_INIT, (;));

Z_GENERIC_SECTION(.boot_arg)
x86_boot_arg_t x86_cpu_boot_arg;

struct x86_cpuboot x86_cpuboot[] = {
	LISTIFY(CONFIG_MP_MAX_NUM_CPUS, X86_CPU_BOOT_INIT, (,)),
};

#ifdef CONFIG_HW_SHADOW_STACK
#define _CPU_IDX(n, _) n
FOR_EACH(X86_INTERRUPT_SHADOW_STACK_DEFINE, (;),
	 LISTIFY(CONFIG_MP_MAX_NUM_CPUS, _CPU_IDX, (,)));

struct x86_interrupt_ssp_table issp_table[] = {
	LISTIFY(CONFIG_MP_MAX_NUM_CPUS, X86_INTERRUPT_SSP_TABLE_INIT, (,)),
};
#endif

/*
 * Send the INIT/STARTUP IPI sequence required to start up CPU 'cpu_num', which
 * will enter the kernel at fn(arg), running on the specified stack.
 */

void arch_cpu_start(int cpu_num, k_thread_stack_t *stack, int sz,
		    arch_cpustart_t fn, void *arg)
{
#if CONFIG_MP_MAX_NUM_CPUS > 1
	uint8_t vector = ((unsigned long) x86_ap_start) >> 12;
	uint8_t apic_id;

	IF_ENABLED(CONFIG_ACPI, ({
		ACPI_MADT_LOCAL_APIC *lapic = acpi_local_apic_get(cpu_num);

		if (lapic != NULL) {
			/* We update the apic_id, __start will need it. */
			x86_cpu_loapics[cpu_num] = lapic->Id;
		} else {
			/* TODO: kernel need to handle the error scenario if someone config
			 * CONFIG_MP_MAX_NUM_CPUS more than what platform supported.
			 */
			__ASSERT(false, "CPU reached more than maximum supported!");
			return;
		}
	}));

	apic_id = x86_cpu_loapics[cpu_num];

	x86_cpuboot[cpu_num].sp = (uint64_t) K_KERNEL_STACK_BUFFER(stack) + sz;
	x86_cpuboot[cpu_num].stack_size = sz;
	x86_cpuboot[cpu_num].fn = fn;
	x86_cpuboot[cpu_num].arg = arg;
	x86_cpuboot[cpu_num].cpu_id = cpu_num;

	z_loapic_ipi(apic_id, LOAPIC_ICR_IPI_INIT, 0);
	k_busy_wait(10000);
	z_loapic_ipi(apic_id, LOAPIC_ICR_IPI_STARTUP, vector);

	while (x86_cpuboot[cpu_num].ready == 0) {
	}
#else
	ARG_UNUSED(cpu_num);
	ARG_UNUSED(stack);
	ARG_UNUSED(sz);
	ARG_UNUSED(fn);
	ARG_UNUSED(arg);
#endif
}

/* Per-CPU initialization, C domain. On the first CPU, z_prep_c is the
 * next step. For other CPUs it is probably smp_init_top().
 */
FUNC_NORETURN void z_x86_cpu_init(struct x86_cpuboot *cpuboot)
{
#if defined(CONFIG_ACPI) && !defined(CONFIG_ACRN_COMMON)
	__ASSERT(z_x86_cpuid_get_current_physical_apic_id() ==
		 x86_cpu_loapics[cpuboot->cpu_id], "APIC ID miss match!");
#endif
	x86_sse_init(NULL);

	if (cpuboot->cpu_id == 0U) {
		/* Only need to do these once per boot */
		arch_bss_zero();
		arch_data_copy();
	}

	z_loapic_enable(cpuboot->cpu_id);

#ifdef CONFIG_USERSPACE
	/* Set landing site for 'syscall' instruction */
	z_x86_msr_write(X86_LSTAR_MSR, (uint64_t)z_x86_syscall_entry_stub);

	/* Set segment descriptors for syscall privilege transitions */
	z_x86_msr_write(X86_STAR_MSR, (uint64_t)X86_STAR_UPPER << 32);

	/* Mask applied to RFLAGS when making a syscall */
	z_x86_msr_write(X86_FMASK_MSR, EFLAGS_SYSCALL);
#endif

#ifdef CONFIG_X86_CET
	z_x86_cet_enable();
#ifdef CONFIG_X86_CET_IBT
	z_x86_ibt_enable();
#endif /* CONFIG_X86_CET_IBT */
#ifdef CONFIG_HW_SHADOW_STACK
	z_x86_setup_interrupt_ssp_table((uintptr_t)&issp_table[cpuboot->cpu_id]);
	cpuboot->gs_base->shstk_addr = &issp_table[cpuboot->cpu_id].ist1;
	cpuboot->gs_base->exception_shstk_addr = issp_table[cpuboot->cpu_id].ist7;
#endif /* CONFIG_HW_SHADOW_STACK */

#endif /* CONFIG_X86_CET */

	/* Enter kernel, never return */
	cpuboot->ready++;
	cpuboot->fn(cpuboot->arg);

	CODE_UNREACHABLE; /* LCOV_EXCL_LINE */
}
