/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <kernel_arch_data.h>
#include <kernel_arch_func.h>
#include <kernel_structs.h>
#include <kernel_internal.h>
#include <arch/x86/multiboot.h>
#include <x86_mmu.h>
#include <drivers/interrupt_controller/loapic.h>
#include <arch/x86/acpi.h>

/*
 * Map of CPU logical IDs to CPU local APIC IDs. By default,
 * we assume this simple identity mapping, as found in QEMU.
 * The symbol is weak so that boards/SoC files can override.
 */

__weak uint8_t x86_cpu_loapics[] = { 0, 1, 2, 3 };

extern char x86_ap_start[];  /* AP entry point in locore.S */

extern uint8_t z_x86_exception_stack[];
extern uint8_t z_x86_exception_stack1[];
extern uint8_t z_x86_exception_stack2[];
extern uint8_t z_x86_exception_stack3[];

extern uint8_t z_x86_nmi_stack[];
extern uint8_t z_x86_nmi_stack1[];
extern uint8_t z_x86_nmi_stack2[];
extern uint8_t z_x86_nmi_stack3[];

#ifdef CONFIG_X86_KPTI
extern uint8_t z_x86_trampoline_stack[];
extern uint8_t z_x86_trampoline_stack1[];
extern uint8_t z_x86_trampoline_stack2[];
extern uint8_t z_x86_trampoline_stack3[];
#endif /* CONFIG_X86_KPTI */

Z_GENERIC_SECTION(.tss)
struct x86_tss64 tss0 = {
#ifdef CONFIG_X86_KPTI
	.ist2 = (uint64_t) z_x86_trampoline_stack + Z_X86_TRAMPOLINE_STACK_SIZE,
#endif
	.ist6 = (uint64_t) z_x86_nmi_stack + CONFIG_X86_EXCEPTION_STACK_SIZE,
	.ist7 = (uint64_t) z_x86_exception_stack + CONFIG_X86_EXCEPTION_STACK_SIZE,
	.iomapb = 0xFFFF,
	.cpu = &(_kernel.cpus[0])
};

#if CONFIG_MP_NUM_CPUS > 1
Z_GENERIC_SECTION(.tss)
struct x86_tss64 tss1 = {
#ifdef CONFIG_X86_KPTI
	.ist2 = (uint64_t) z_x86_trampoline_stack1 + Z_X86_TRAMPOLINE_STACK_SIZE,
#endif
	.ist6 = (uint64_t) z_x86_nmi_stack1 + CONFIG_X86_EXCEPTION_STACK_SIZE,
	.ist7 = (uint64_t) z_x86_exception_stack1 + CONFIG_X86_EXCEPTION_STACK_SIZE,
	.iomapb = 0xFFFF,
	.cpu = &(_kernel.cpus[1])
};
#endif

#if CONFIG_MP_NUM_CPUS > 2
Z_GENERIC_SECTION(.tss)
struct x86_tss64 tss2 = {
#ifdef CONFIG_X86_KPTI
	.ist2 = (uint64_t) z_x86_trampoline_stack2 + Z_X86_TRAMPOLINE_STACK_SIZE,
#endif
	.ist6 = (uint64_t) z_x86_nmi_stack2 + CONFIG_X86_EXCEPTION_STACK_SIZE,
	.ist7 = (uint64_t) z_x86_exception_stack2 + CONFIG_X86_EXCEPTION_STACK_SIZE,
	.iomapb = 0xFFFF,
	.cpu = &(_kernel.cpus[2])
};
#endif

#if CONFIG_MP_NUM_CPUS > 3
Z_GENERIC_SECTION(.tss)
struct x86_tss64 tss3 = {
#ifdef CONFIG_X86_KPTI
	.ist2 = (uint64_t) z_x86_trampoline_stack3 + Z_X86_TRAMPOLINE_STACK_SIZE,
#endif
	.ist6 = (uint64_t) z_x86_nmi_stack3 + CONFIG_X86_EXCEPTION_STACK_SIZE,
	.ist7 = (uint64_t) z_x86_exception_stack3 + CONFIG_X86_EXCEPTION_STACK_SIZE,
	.iomapb = 0xFFFF,
	.cpu = &(_kernel.cpus[3])
};
#endif

struct x86_cpuboot x86_cpuboot[] = {
	{
		.tr = X86_KERNEL_CPU0_TR,
		.gs_base = &tss0,
		.sp = (uint64_t) z_interrupt_stacks[0] +
			Z_KERNEL_STACK_SIZE_ADJUST(CONFIG_ISR_STACK_SIZE),
		.stack_size =
			Z_KERNEL_STACK_SIZE_ADJUST(CONFIG_ISR_STACK_SIZE),
		.fn = z_x86_prep_c,
	},
#if CONFIG_MP_NUM_CPUS > 1
	{
		.tr = X86_KERNEL_CPU1_TR,
		.gs_base = &tss1
	},
#endif
#if CONFIG_MP_NUM_CPUS > 2
	{
		.tr = X86_KERNEL_CPU2_TR,
		.gs_base = &tss2
	},
#endif
#if CONFIG_MP_NUM_CPUS > 3
	{
		.tr = X86_KERNEL_CPU3_TR,
		.gs_base = &tss3
	},
#endif
};

/*
 * Send the INIT/STARTUP IPI sequence required to start up CPU 'cpu_num', which
 * will enter the kernel at fn(arg), running on the specified stack.
 */

void arch_start_cpu(int cpu_num, k_thread_stack_t *stack, int sz,
		    arch_cpustart_t fn, void *arg)
{
	uint8_t vector = ((unsigned long) x86_ap_start) >> 12;
	uint8_t apic_id;

	if (IS_ENABLED(CONFIG_ACPI)) {
		struct acpi_cpu *cpu;

		cpu = z_acpi_get_cpu(cpu_num);
		if (cpu != NULL) {
			/* We update the apic_id, x86_ap_start will need it. */
			x86_cpu_loapics[cpu_num] = cpu->apic_id;
		}
	}

	apic_id = x86_cpu_loapics[cpu_num];

	x86_cpuboot[cpu_num].sp = (uint64_t) Z_KERNEL_STACK_BUFFER(stack) + sz;
	x86_cpuboot[cpu_num].stack_size = sz;
	x86_cpuboot[cpu_num].fn = fn;
	x86_cpuboot[cpu_num].arg = arg;

	z_loapic_ipi(apic_id, LOAPIC_ICR_IPI_INIT, 0);
	k_busy_wait(10000);
	z_loapic_ipi(apic_id, LOAPIC_ICR_IPI_STARTUP, vector);

	while (x86_cpuboot[cpu_num].ready == 0) {
	}
}

/* Per-CPU initialization, C domain. On the first CPU, z_x86_prep_c is the
 * next step. For other CPUs it is probably smp_init_top().
 */
FUNC_NORETURN void z_x86_cpu_init(struct x86_cpuboot *cpuboot)
{
	x86_sse_init(NULL);

	/* The internal cpu_number is the index to x86_cpuboot[] */
	z_loapic_enable((unsigned char)(cpuboot - x86_cpuboot));

#ifdef CONFIG_USERSPACE
	/* Set landing site for 'syscall' instruction */
	z_x86_msr_write(X86_LSTAR_MSR, (uint64_t)z_x86_syscall_entry_stub);

	/* Set segment descriptors for syscall privilege transitions */
	z_x86_msr_write(X86_STAR_MSR, (uint64_t)X86_STAR_UPPER << 32);

	/* Mask applied to RFLAGS when making a syscall */
	z_x86_msr_write(X86_FMASK_MSR, EFLAGS_SYSCALL);
#endif

	/* Enter kernel, never return */
	cpuboot->ready++;
	cpuboot->fn(cpuboot->arg);
}
