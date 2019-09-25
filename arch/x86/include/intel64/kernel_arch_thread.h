/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_X86_INCLUDE_INTEL64_KERNEL_ARCH_THREAD_H_
#define ZEPHYR_ARCH_X86_INCLUDE_INTEL64_KERNEL_ARCH_THREAD_H_

#define X86_THREAD_FLAG_ALL 0x01 /* _thread_arch.flags: entire state saved */

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>
#include <kernel_arch_data.h>

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

#endif /* ZEPHYR_ARCH_X86_INCLUDE_INTEL64_KERNEL_ARCH_THREAD_H_ */
