/*
 * Copyright (c) 2019 Intel Corp.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_X86_INCLUDE_INTEL64_KERNEL_ARCH_DATA_H_
#define ZEPHYR_ARCH_X86_INCLUDE_INTEL64_KERNEL_ARCH_DATA_H_

#include <zephyr/arch/x86/mmustructs.h>

#ifndef _ASMLANGUAGE

/* linker symbols defining the bounds of the kernel part loaded in locore */

extern char _locore_start[], _locore_end[];

/*
 * Per-CPU bootstrapping parameters. See locore.S and cpu.c.
 */

struct x86_cpuboot {
	volatile int ready;	/* CPU has started */
	uint16_t tr;		/* selector for task register */
	struct x86_tss64 *gs_base; /* Base address for GS segment */
	uint64_t sp;		/* initial stack pointer */
	size_t stack_size;	/* size of stack */
	arch_cpustart_t fn;	/* kernel entry function */
	void *arg;		/* argument for above function */
	uint8_t cpu_id;		/* CPU ID */
};

typedef struct x86_cpuboot x86_cpuboot_t;

extern uint8_t x86_cpu_loapics[];	/* CPU logical ID -> local APIC ID */

#endif /* _ASMLANGUAGE */

#ifdef CONFIG_X86_KPTI
#define Z_X86_TRAMPOLINE_STACK_SIZE	128
#endif

#ifdef CONFIG_X86_KPTI
#define TRAMPOLINE_STACK(n)									\
	uint8_t z_x86_trampoline_stack##n[Z_X86_TRAMPOLINE_STACK_SIZE]				\
		__attribute__ ((section(".trampolines")));

#define TRAMPOLINE_INIT(n)									\
	.ist2 = (uint64_t)z_x86_trampoline_stack##n + Z_X86_TRAMPOLINE_STACK_SIZE,
#else
#define TRAMPOLINE_STACK(n)
#define TRAMPOLINE_INIT(n)
#endif /* CONFIG_X86_KPTI */

#define ACPI_CPU_INIT(n, _)									\
	uint8_t z_x86_exception_stack##n[CONFIG_X86_EXCEPTION_STACK_SIZE] __aligned(16);	\
	uint8_t z_x86_nmi_stack##n[CONFIG_X86_EXCEPTION_STACK_SIZE] __aligned(16);		\
	TRAMPOLINE_STACK(n);									\
	Z_GENERIC_SECTION(.tss)									\
	struct x86_tss64 tss##n = {								\
		TRAMPOLINE_INIT(n)								\
		.ist6 =	(uint64_t)z_x86_nmi_stack##n + CONFIG_X86_EXCEPTION_STACK_SIZE,		\
		.ist7 = (uint64_t)z_x86_exception_stack##n + CONFIG_X86_EXCEPTION_STACK_SIZE,	\
		.iomapb = 0xFFFF, .cpu = &(_kernel.cpus[n])	\
	}

#define X86_CPU_BOOT_INIT(n, _)									\
	{											\
		.tr = (0x40 + (16 * n)),							\
		.gs_base = &tss##n,								\
		.sp = (uint64_t)z_interrupt_stacks[n] +						\
		      K_KERNEL_STACK_LEN(CONFIG_ISR_STACK_SIZE),				\
		.stack_size = K_KERNEL_STACK_LEN(CONFIG_ISR_STACK_SIZE),			\
		.fn = z_prep_c,									\
		.arg = &x86_cpu_boot_arg,							\
	}

#define STACK_ARRAY_IDX(n, _) n

#define DEFINE_STACK_ARRAY_IDX\
	LISTIFY(CONFIG_MP_MAX_NUM_CPUS, STACK_ARRAY_IDX, (,))

#endif /* ZEPHYR_ARCH_X86_INCLUDE_INTEL64_KERNEL_ARCH_DATA_H_ */
