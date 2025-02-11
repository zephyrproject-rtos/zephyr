/*
 * Copyright (c) 2019 Intel Corp.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_INTEL64_EXPCEPTION_H_
#define ZEPHYR_INCLUDE_ARCH_X86_INTEL64_EXPCEPTION_H_

#ifndef _ASMLANGUAGE
#include <zephyr/arch/x86/intel64/thread.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * the exception stack frame
 */

struct arch_esf {
#ifdef CONFIG_EXCEPTION_DEBUG
	/* callee-saved */
	unsigned long rbx;
	unsigned long r12;
	unsigned long r13;
	unsigned long r14;
	unsigned long r15;
#endif /* CONFIG_EXCEPTION_DEBUG */
	unsigned long rbp;

	/* Caller-saved regs */
	unsigned long rax;
	unsigned long rcx;
	unsigned long rdx;
	unsigned long rsi;
	unsigned long rdi;
	unsigned long r8;
	unsigned long r9;
	unsigned long r10;
	/* Must be aligned 16 bytes from the end of this struct due to
	 * requirements of 'fxsave (%rsp)'
	 */
	char fxsave[X86_FXSAVE_SIZE];
	unsigned long r11;

	/* Pushed by CPU or assembly stub */
	unsigned long vector;
	unsigned long code;
	unsigned long rip;
	unsigned long cs;
	unsigned long rflags;
	unsigned long rsp;
	unsigned long ss;
};

struct x86_ssf {
	unsigned long rip;
	unsigned long rflags;
	unsigned long r10;
	unsigned long r9;
	unsigned long r8;
	unsigned long rdx;
	unsigned long rsi;
	char fxsave[X86_FXSAVE_SIZE];
	unsigned long rdi;
	unsigned long rsp;
};

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_X86_INTEL64_EXPCEPTION_H_ */
