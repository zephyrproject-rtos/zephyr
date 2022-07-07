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
};

typedef struct x86_cpuboot x86_cpuboot_t;

extern uint8_t x86_cpu_loapics[];	/* CPU logical ID -> local APIC ID */

#endif /* _ASMLANGUAGE */

#ifdef CONFIG_X86_KPTI
#define Z_X86_TRAMPOLINE_STACK_SIZE	128
#endif

#endif /* ZEPHYR_ARCH_X86_INCLUDE_INTEL64_KERNEL_ARCH_DATA_H_ */
