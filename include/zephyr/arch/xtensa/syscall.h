/* Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_ARCH_XTENSA_SYSCALL_H_
#define ZEPHYR_INCLUDE_ARCH_XTENSA_SYSCALL_H_

/* The Xtensa syscall interface is a straightforward extension of the
 * CALL0 function ABI convention, the only difference being placement
 * of the system call ID (the C declarations for Zephyr put it in the
 * last argument, which is variable, where the handler wants to see it
 * in A11 always.  Therefore the system calls are expressed as tiny
 * linkable functions in xtensa-call0.S.
 *
 * Alas the declarations for the Zephyr API have these functions
 * tagged inline, so we need to do a little preprocessor work to avoid
 * redeclaration warnings.
 */

uintptr_t xtensa_syscall6(uintptr_t a1, uintptr_t a2, uintptr_t a3,
			  uintptr_t a4, uintptr_t a5, uintptr_t a6, uintptr_t id);
uintptr_t xtensa_syscall5(uintptr_t a1, uintptr_t a2, uintptr_t a3,
			  uintptr_t a4, uintptr_t a5, uintptr_t id);
uintptr_t xtensa_syscall4(uintptr_t a1, uintptr_t a2, uintptr_t a3,
			  uintptr_t a4, uintptr_t id);
uintptr_t xtensa_syscall3(uintptr_t a1, uintptr_t a2, uintptr_t a3,
			  uintptr_t id);
uintptr_t xtensa_syscall2(uintptr_t a1, uintptr_t a2, uintptr_t id);
uintptr_t xtensa_syscall1(uintptr_t a1, uintptr_t id);

#define arch_syscall_invoke6 xtensa_syscall6
#define arch_syscall_invoke5 xtensa_syscall5
#define arch_syscall_invoke4 xtensa_syscall4
#define arch_syscall_invoke3 xtensa_syscall3
#define arch_syscall_invoke2 xtensa_syscall2
#define arch_syscall_invoke1 xtensa_syscall1

#endif /* ZEPHYR_INCLUDE_ARCH_XTENSA_SYSCALL_H_ */
