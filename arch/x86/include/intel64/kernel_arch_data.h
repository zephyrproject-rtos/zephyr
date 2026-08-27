/*
 * Copyright (c) 2019 Intel Corp.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_X86_INCLUDE_INTEL64_KERNEL_ARCH_DATA_H_
#define ZEPHYR_ARCH_X86_INCLUDE_INTEL64_KERNEL_ARCH_DATA_H_

#include <zephyr/arch/x86/mmustructs.h>
#include <zephyr/arch/x86/cet.h>

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

struct x86_interrupt_ssp_table {
	uintptr_t not_used;
	uintptr_t ist1;
	uintptr_t ist2;
	uintptr_t ist3;
	uintptr_t ist4;
	uintptr_t ist5;
	uintptr_t ist6;
	uintptr_t ist7;
};

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

#define SHSTK_TOKEN_ENTRY(i, name, size)							\
		  [size * (i + 1) / sizeof(arch_thread_hw_shadow_stack_t) - 1] =		\
			(uintptr_t)name + size * (i + 1) - 1 *					\
			sizeof(arch_thread_hw_shadow_stack_t)

#define X86_EXCEPTION_SHADOW_STACK_SIZE \
	K_THREAD_HW_SHADOW_STACK_SIZE(CONFIG_X86_EXCEPTION_STACK_SIZE)

#define X86_IRQ_SHADOW_STACK_DEFINE(name, size)							\
	arch_thread_hw_shadow_stack_t Z_GENERIC_SECTION(.x86shadowstack)			\
	__aligned(CONFIG_X86_CET_SHADOW_STACK_ALIGNMENT)					\
	name[CONFIG_ISR_DEPTH * size / sizeof(arch_thread_hw_shadow_stack_t)] =			\
		{										\
			LISTIFY(CONFIG_ISR_DEPTH, SHSTK_TOKEN_ENTRY, (,), name, size)		\
		}

#define X86_INTERRUPT_SHADOW_STACK_DEFINE(n)							\
	X86_IRQ_SHADOW_STACK_DEFINE(z_x86_nmi_shadow_stack##n,					\
				     X86_EXCEPTION_SHADOW_STACK_SIZE);				\
	X86_IRQ_SHADOW_STACK_DEFINE(z_x86_exception_shadow_stack##n,				\
				     X86_EXCEPTION_SHADOW_STACK_SIZE)

#define SHSTK_LAST_ENTRY (X86_EXCEPTION_SHADOW_STACK_SIZE *					\
			  CONFIG_ISR_DEPTH / sizeof(arch_thread_hw_shadow_stack_t) - 1)

#define IRQ_SHSTK_LAST_ENTRY sizeof(__z_interrupt_stacks_shstk_arr[0]) /			\
	sizeof(arch_thread_hw_shadow_stack_t) - 1

#define X86_INTERRUPT_SSP_TABLE_INIT(n, _)							\
	{											\
		.ist1 = (uintptr_t)&__z_interrupt_stacks_shstk_arr[n][IRQ_SHSTK_LAST_ENTRY],	\
		.ist6 = (uintptr_t)&z_x86_nmi_shadow_stack##n[SHSTK_LAST_ENTRY],		\
		.ist7 = (uintptr_t)&z_x86_exception_shadow_stack##n[SHSTK_LAST_ENTRY],	\
	}

#define STACK_ARRAY_IDX(n, _) n

#define DEFINE_STACK_ARRAY_IDX\
	LISTIFY(CONFIG_MP_MAX_NUM_CPUS, STACK_ARRAY_IDX, (,))

#endif /* ZEPHYR_ARCH_X86_INCLUDE_INTEL64_KERNEL_ARCH_DATA_H_ */
