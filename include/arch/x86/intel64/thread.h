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
#define X86_USER_CS_32		0x28	/* 32-bit user data (unused) */
#define X86_USER_DS		0x30	/* 64-bit user mode data */
#define X86_USER_CS		0x38	/* 64-bit user mode code */

/* Value programmed into bits 63:32 of STAR MSR with proper segment
 * descriptors for implementing user mode with syscall/sysret
 */
#define X86_STAR_UPPER		((X86_USER_CS_32 << 16) | X86_KERNEL_CS)

#define X86_KERNEL_CPU0_TR	0x40	/* 64-bit task state segment */
#define X86_KERNEL_CPU1_TR	0x50	/* 64-bit task state segment */
#define X86_KERNEL_CPU2_TR	0x60	/* 64-bit task state segment */
#define X86_KERNEL_CPU3_TR	0x70	/* 64-bit task state segment */

/*
 * Some SSE definitions. Ideally these will ultimately be shared with 32-bit.
 */

#define X86_FXSAVE_SIZE		512	/* size and alignment of buffer ... */
#define X86_FXSAVE_ALIGN	16	/* ... for FXSAVE/FXRSTOR ops */
#define X86_MXCSR_SANE		0x1dc0	/* enable division-by-zero exception */

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>
#include <arch/x86/mmustructs.h>

/*
 * 64-bit Task State Segment. One defined per CPU.
 */

struct x86_tss64 {
	/*
	 * Architecturally-defined portion. It is somewhat tedious to
	 * enumerate each member specifically (rather than using arrays)
	 * but we need to get (some of) their offsets from assembly.
	 */

	uint8_t reserved0[4];

	uint64_t rsp0;		/* privileged stacks */
	uint64_t rsp1;
	uint64_t rsp2;

	uint8_t reserved[8];

	uint64_t ist1;		/* interrupt stacks */
	uint64_t ist2;
	uint64_t ist3;
	uint64_t ist4;
	uint64_t ist5;
	uint64_t ist6;
	uint64_t ist7;

	uint8_t reserved1[10];

	uint16_t iomapb;		/* offset to I/O base */

	/*
	 * Zephyr specific portion. Stash per-CPU data here for convenience.
	 */

	struct _cpu *cpu;
#ifdef CONFIG_USERSPACE
	/* Privilege mode stack pointer value when doing a system call */
	char *psp;

	/* Storage area for user mode stack pointer when doing a syscall */
	char *usp;
#endif /* CONFIG_USERSPACE */
} __packed __aligned(8);

typedef struct x86_tss64 x86_tss64_t;

/*
 * The _callee_saved registers are unconditionally saved/restored across
 * context switches; the _thread_arch registers are only preserved when
 * the thread is interrupted. _arch_thread.flags tells __resume when to
 * cheat and only restore the first set. For more details see locore.S.
 */

struct _callee_saved {
	uint64_t rsp;
	uint64_t rbx;
	uint64_t rbp;
	uint64_t r12;
	uint64_t r13;
	uint64_t r14;
	uint64_t r15;
	uint64_t rip;
	uint64_t rflags;
};

typedef struct _callee_saved _callee_saved_t;

struct _thread_arch {
	uint8_t flags;

#ifdef CONFIG_USERSPACE
#ifndef CONFIG_X86_COMMON_PAGE_TABLE
	/* Physical address of the page tables used by this thread */
	uintptr_t ptables;
#endif /* CONFIG_X86_COMMON_PAGE_TABLE */

	/* Initial privilege mode stack pointer when doing a system call.
	 * Un-set for supervisor threads.
	 */
	char *psp;

	/* SS and CS selectors for this thread when restoring context */
	uint64_t ss;
	uint64_t cs;
#endif

	uint64_t rax;
	uint64_t rcx;
	uint64_t rdx;
	uint64_t rsi;
	uint64_t rdi;
	uint64_t r8;
	uint64_t r9;
	uint64_t r10;
	uint64_t r11;
	char __aligned(X86_FXSAVE_ALIGN) sse[X86_FXSAVE_SIZE];
};

typedef struct _thread_arch _thread_arch_t;

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_X86_INTEL64_THREAD_H_ */
