/*
 * Copyright (c) 2019 Intel Corp.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_X86_INCLUDE_INTEL64_KERNEL_ARCH_DATA_H_
#define ZEPHYR_ARCH_X86_INCLUDE_INTEL64_KERNEL_ARCH_DATA_H_

#include <arch/x86/mmustructs.h>

/*
 * Some SSE definitions. Ideally these will ultimately be shared with 32-bit.
 */

#define X86_FXSAVE_SIZE		512	/* size and alignment of buffer ... */
#define X86_FXSAVE_ALIGN	16	/* ... for FXSAVE/FXRSTOR ops */
#define X86_MXCSR_SANE		0x1dc0	/* enable division-by-zero exception */

/*
 * GDT selectors - these must agree with the GDT layout in locore.S.
 */

#define X86_KERNEL_CS_32	0x08	/* 32-bit kernel code */
#define X86_KERNEL_DS_32	0x10	/* 32-bit kernel data */
#define X86_KERNEL_CS		0x18	/* 64-bit kernel code */
#define X86_KERNEL_DS		0x20	/* 64-bit kernel data */

#define X86_KERNEL_CPU0_GS	0x30	/* data selector covering TSS */
#define X86_KERNEL_CPU0_TR	0x40	/* 64-bit task state segment */
#define X86_KERNEL_CPU1_GS	0x50	/* data selector covering TSS */
#define X86_KERNEL_CPU1_TR	0x60	/* 64-bit task state segment */
#define X86_KERNEL_CPU2_GS	0x70	/* data selector covering TSS */
#define X86_KERNEL_CPU2_TR	0x80	/* 64-bit task state segment */
#define X86_KERNEL_CPU3_GS	0x90	/* data selector covering TSS */
#define X86_KERNEL_CPU3_TR	0xA0	/* 64-bit task state segment */

#ifndef _ASMLANGUAGE

/* linker symbols defining the bounds of the kernel part loaded in locore */

extern char _locore_start[], _locore_end[];

/*
 * 64-bit Task State Segment. One defined per CPU.
 */

struct x86_tss64 {
	/*
	 * Architecturally-defined portion. It is somewhat tedious to
	 * enumerate each member specifically (rather than using arrays)
	 * but we need to get (some of) their offsets from assembly.
	 */

	u8_t reserved0[4];

	u64_t rsp0;		/* privileged stacks */
	u64_t rsp1;
	u64_t rsp2;

	u8_t reserved[8];

	u64_t ist1;		/* interrupt stacks */
	u64_t ist2;
	u64_t ist3;
	u64_t ist4;
	u64_t ist5;
	u64_t ist6;
	u64_t ist7;

	u8_t reserved1[10];

	u16_t iomapb;		/* offset to I/O base */

	/*
	 * Zephyr specific portion. Stash per-CPU data here for convenience.
	 */

	struct _cpu *cpu;
} __packed __aligned(8);

typedef struct x86_tss64 x86_tss64_t;

/*
 * Per-CPU bootstrapping parameters. See locore.S and cpu.c.
 */

struct x86_cpuboot {
	volatile int ready;	/* CPU has started */
	u16_t tr;		/* selector for task register */
	u16_t gs;		/* selector for GS */
	u64_t sp;		/* initial stack pointer */
	void *fn;		/* kernel entry function */
	void *arg;		/* argument for above function */
#ifdef CONFIG_X86_MMU
	struct x86_page_tables *ptables; /* Runtime page tables to install */
#endif /* CONFIG_X86_MMU */
};

typedef struct x86_cpuboot x86_cpuboot_t;

extern u8_t x86_cpu_loapics[];	/* CPU logical ID -> local APIC ID */

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_X86_INCLUDE_INTEL64_KERNEL_ARCH_DATA_H_ */
