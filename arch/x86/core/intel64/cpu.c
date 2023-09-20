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
#ifdef CONFIG_ACPI
#include <zephyr/arch/x86/cpuid.h>
#include <zephyr/acpi/acpi.h>
#endif

#define CPUID_MASK_WORD  0xFF
#define CPUID_MASK_NIBLE 0x0F

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
static struct x86_cpu_info *cpu_info[CONFIG_MP_MAX_NUM_CPUS];
extern char x86_ap_start[]; /* AP entry point in locore.S */

LISTIFY(CONFIG_MP_MAX_NUM_CPUS, ACPI_CPU_INIT, (;));

Z_GENERIC_SECTION(.boot_arg)
x86_boot_arg_t x86_cpu_boot_arg;

struct x86_cpuboot x86_cpuboot[] = {
	LISTIFY(CONFIG_MP_MAX_NUM_CPUS, X86_CPU_BOOT_INIT, (,)),
};

int x86_update_cpu_info(uint8_t cpu_id)
{
	uint32_t eax, ebx, ecx, edx;
	struct _cpu *cpu;
	struct x86_cpu_info *info;

	__asm__ volatile("movq %%gs:(%c1), %0"
			 : "=r" (cpu)
			 : "i" (offsetof(x86_tss64_t, cpu)));

	info = &cpu->arch.info;

	if (!__get_cpuid(0x01, &eax, &ebx, &ecx, &edx)) {
		return -EIO;
	}
	info->family = (((eax >> 20u) & CPUID_MASK_WORD) << 4u) | ((eax >> 8u) & CPUID_MASK_NIBLE);
	info->model = (((eax >> 16u) & CPUID_MASK_NIBLE) << 4u) | ((eax >> 4u) & CPUID_MASK_NIBLE);
	info->stepping = eax & CPUID_MASK_NIBLE;
	info->apic_id = (ebx >> 24u) & CPUID_MASK_WORD;
	info->cpu_id = cpu_id;

	if (__get_cpuid(0x1A, &eax, &ebx, &ecx, &edx)) {
		info->type = (enum x86_cpu_type)(eax >> 24);
	} else {
		info->type = CPU_TYPE_UNKNOWN;
	}

	if (__get_cpuid_count(0x07, 0, &eax, &ebx, &ecx, &edx)) {
		info->hybrid = (edx >> 15) & 0x01;
	}

	cpu_info[cpu_id] = info;

	return 0;
}

/*
 * Send the INIT/STARTUP IPI sequence required to start up CPU 'cpu_num', which
 * will enter the kernel at fn(arg), running on the specified stack.
 */

void arch_start_cpu(int cpu_num, k_thread_stack_t *stack, int sz,
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

	x86_cpuboot[cpu_num].sp = (uint64_t) Z_KERNEL_STACK_BUFFER(stack) + sz;
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
#if defined(CONFIG_ACPI)
	__ASSERT(z_x86_cpuid_get_current_physical_apic_id() ==
		 x86_cpu_loapics[cpuboot->cpu_id], "APIC ID miss match!");
#endif
	x86_sse_init(NULL);

	if (cpuboot->cpu_id == 0U) {
		/* Only need to do these once per boot */
		z_bss_zero();
		z_data_copy();
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

	/* Cache AP info. */
	if (x86_update_cpu_info(cpuboot->cpu_id)) {
		__ASSERT(false, "Get AP CPU info failed");
	}

	/* Enter kernel, never return */
	cpuboot->ready++;
	cpuboot->fn(cpuboot->arg);
}

struct x86_cpu_info *z_x86_cpu_info_get(uint8_t cpu_id)
{
	if (cpu_id >= CONFIG_MP_MAX_NUM_CPUS) {
		return NULL;
	}

	return cpu_info[cpu_id];
}
