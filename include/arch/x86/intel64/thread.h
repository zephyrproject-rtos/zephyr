/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_INTEL64_THREAD_H_
#define ZEPHYR_INCLUDE_ARCH_X86_INTEL64_THREAD_H_

#define X86_THREAD_FLAG_ALL 0x01 /* _thread_arch.flags: entire state saved */

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

/*
 * Some SSE definitions. Ideally these will ultimately be shared with 32-bit.
 */

#define X86_FXSAVE_SIZE		512	/* size and alignment of buffer ... */
#define X86_FXSAVE_ALIGN	16	/* ... for FXSAVE/FXRSTOR ops */
#define X86_MXCSR_SANE		0x1dc0	/* enable division-by-zero exception */

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>

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
 * The _callee_saved registers are unconditionally saved/restored across
 * context switches; the _thread_arch registers are only preserved when
 * the thread is interrupted. _arch_thread.flags tells __resume when to
 * cheat and only restore the first set. For more details see locore.S.
 */

struct _callee_saved {
	u64_t rsp;
	u64_t rbx;
	u64_t rbp;
	u64_t r12;
	u64_t r13;
	u64_t r14;
	u64_t r15;
	u64_t rip;
	u64_t rflags;
};

typedef struct _callee_saved _callee_saved_t;

struct _thread_arch {
	u8_t flags;

	u64_t rax;
	u64_t rcx;
	u64_t rdx;
	u64_t rsi;
	u64_t rdi;
	u64_t r8;
	u64_t r9;
	u64_t r10;
	u64_t r11;
	char __aligned(X86_FXSAVE_ALIGN) sse[X86_FXSAVE_SIZE];
};

typedef struct _thread_arch _thread_arch_t;

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_X86_INTEL64_THREAD_H_ */
