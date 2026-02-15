/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_IA32_EXCEPTION_H_
#define ZEPHYR_INCLUDE_ARCH_X86_IA32_EXCEPTION_H_

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Exception Stack Frame
 *
 * A pointer to an "exception stack frame" (ESF) is passed as an argument
 * to exception handlers registered via nanoCpuExcConnect().  As the system
 * always operates at ring 0, only the EIP, CS and EFLAGS registers are pushed
 * onto the stack when an exception occurs.
 *
 * The exception stack frame includes the volatile registers (EAX, ECX, and
 * EDX) as well as the 5 non-volatile registers (EDI, ESI, EBX, EBP and ESP).
 * Those registers are pushed onto the stack by _ExcEnt().
 */

struct arch_esf {
#ifdef CONFIG_GDBSTUB
	unsigned int ss;
	unsigned int gs;
	unsigned int fs;
	unsigned int es;
	unsigned int ds;
#endif
	unsigned int esp;
	unsigned int ebp;
	unsigned int ebx;
	unsigned int esi;
	unsigned int edi;
	unsigned int edx;
	unsigned int eax;
	unsigned int ecx;
	unsigned int errorCode;
	unsigned int eip;
	unsigned int cs;
	unsigned int eflags;
};

extern unsigned int z_x86_exception_vector;

struct _x86_syscall_stack_frame {
	uint32_t eip;
	uint32_t cs;
	uint32_t eflags;

	/* These are only present if cs = USER_CODE_SEG */
	uint32_t esp;
	uint32_t ss;
};

#if defined(CONFIG_SIGNAL) && defined(CONFIG_USERSPACE)
struct arch_sig_ctx {
	unsigned long eip;
	unsigned long eax;
	unsigned long esp;
	unsigned long eflags;
	/*
	 * Registers the interrupted context expects preserved across a syscall: the syscall
	 * gate restores EDX, ECX, EBX, and EDI from its privileged-stack argument slots, and
	 * EBP and ESI survive in-register per the ABI. The signal trampoline chain clobbers
	 * them, and k_sig_return() resumes the interrupted context without unwinding the
	 * trampoline's frames, so they are captured at redirection and restored on the
	 * k_sig_return() exit path.
	 */
	unsigned long s[6]; /* edx, ecx, ebx, edi, ebp, esi */
};
#endif

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_X86_IA32_EXCEPTION_H_ */
