/*
 * Copyright (c) 2017, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef ZEPHYR_INCLUDE_SYSCALL_H_
#define ZEPHYR_INCLUDE_SYSCALL_H_

#include <syscall_list.h>
#include <arch/syscall.h>
#include <stdbool.h>

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>
#include <syscall_macros.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * System Call Declaration macros
 *
 * These macros are used in public header files to declare system calls.
 * They generate inline functions which have different implementations
 * depending on the current compilation context:
 *
 * - Kernel-only code, or CONFIG_USERSPACE disabled, these inlines will
 *   directly call the implementation
 * - User-only code, these inlines will marshal parameters and elevate
 *   privileges
 * - Mixed or indeterminate code, these inlines will do a runtime check
 *   to determine what course of action is needed.
 *
 * All system calls require a verifier function and an implementation
 * function.  These must follow a naming convention. For a system call
 * named k_foo():
 *
 * - The handler function will be named z_vrfy_k_foo(). Handler
 *   functions have the same type signature as the wrapped call,
 *   verify arguments passed up from userspace, and call the
 *   implementation function. See documentation for that typedef for
 *   more information.  - The implementation function will be named
 *   z_impl_k_foo(). This is the actual implementation of the system
 *   call.
 */

/**
 * @typedef _k_syscall_handler_t
 * @brief System call handler function type
 *
 * These are kernel-side skeleton functions for system calls. They are
 * necessary to sanitize the arguments passed into the system call:
 *
 * - Any kernel object or device pointers are validated with _SYSCALL_IS_OBJ()
 * - Any memory buffers passed in are checked to ensure that the calling thread
 *   actually has access to them
 * - Many kernel calls do no sanity checking of parameters other than
 *   assertions. The handler must check all of these conditions using
 *   _SYSCALL_ASSERT()
 * - If the system call has more than 6 arguments, then arg6 will be a pointer
 *   to some struct containing arguments 6+. The struct itself needs to be
 *   validated like any other buffer passed in from userspace, and its members
 *   individually validated (if necessary) and then passed to the real
 *   implementation like normal arguments
 *
 * Even if the system call implementation has no return value, these always
 * return something, even 0, to prevent register leakage to userspace.
 *
 * Once everything has been validated, the real implementation will be executed.
 *
 * @param arg1 system call argument 1
 * @param arg2 system call argument 2
 * @param arg3 system call argument 3
 * @param arg4 system call argument 4
 * @param arg5 system call argument 5
 * @param arg6 system call argument 6
 * @param ssf System call stack frame pointer. Used to generate kernel oops
 *            via _arch_syscall_oops_at(). Contents are arch-specific.
 * @return system call return value, or 0 if the system call implementation
 *         return void
 *
 */
typedef u32_t (*_k_syscall_handler_t)(u32_t arg1, u32_t arg2, u32_t arg3,
				      u32_t arg4, u32_t arg5, u32_t arg6,
				      void *ssf);

/* True if a syscall function must trap to the kernel, usually a
 * compile-time decision.
 */
static ALWAYS_INLINE bool z_syscall_trap(void)
{
	bool ret = false;
#ifdef CONFIG_USERSPACE
#if defined(__ZEPHYR_SUPERVISOR__)
	ret = false;
#elif defined(__ZEPHYR_USER__)
	ret = true;
#else
	ret = z_arch_is_user_context();
#endif
#endif
	return ret;
}

/**
 * Indicate whether the CPU is currently in user mode
 *
 * @return true if the CPU is currently running with user permissions
 */
static inline bool _is_user_context(void)
{
#ifdef CONFIG_USERSPACE
	return z_arch_is_user_context();
#else
	return false;
#endif
}

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif

