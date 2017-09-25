/*
 * Copyright (c) 2017, Intel Corporation
 *
 * SPDX-License-Identifier: Apache 2.0
 */


#ifndef _ZEPHYR_SYSCALL_H_
#define _ZEPHYR_SYSCALL_H_

/* Fixed system call IDs. We use #defines instead of enumeration so that if
 * system calls are retired it does not shift the IDs of other system calls.
 */
#define K_SYSCALL_BAD 0

#define K_SYSCALL_LIMIT	1

#ifndef _ASMLANGUAGE
#include <misc/printk.h>

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


extern const _k_syscall_handler_t _k_syscall_table[K_SYSCALL_LIMIT];


/**
 * @brief Runtime expression check for system call arguments
 *
 * Used in handler functions to perform various runtime checks on arguments,
 * and generate a kernel oops if anything is not expected
 *
 * @param expr Boolean expression to verify, a false result will trigger an
 *             oops
 * @param ssf Syscall stack frame argument passed to the handler function
 */
#define _SYSCALL_VERIFY(expr, ssf) \
	do { \
		if (!(expr)) { \
			printk("FATAL: syscall failed check: " #expr "\n"); \
			_arch_syscall_oops(ssf); \
		} \
	} while (0)

/**
 * @brief Runtime check that a pointer is a kernel object of expected type
 *
 * Passes along arguments to _k_object_validate() and triggers a kernel oops
 * if the object wasn't valid or had incorrect permissions.
 *
 * @param ptr Untrusted kernel object pointer
 * @param type Expected kernel object type
 * @param init Whether this is an init function handler
 * @param ssf Syscall stack frame argument passed to the handler function
 */
#define _SYSCALL_IS_OBJ(ptr, type, init, ssf) \
	_SYSCALL_VERIFY(!_k_object_validate((void *)ptr, type, init), ssf)

/* Convenience macros for handler implementations */
#define _SYSCALL_ARG0	ARG_UNUSED(arg1); ARG_UNUSED(arg2); ARG_UNUSED(arg3); \
			ARG_UNUSED(arg4); ARH_UNUSED(arg5); ARG_UNUSED(arg6)

#define _SYSCALL_ARG1	ARG_UNUSED(arg2); ARG_UNUSED(arg3); ARG_UNUSED(arg4); \
			ARG_UNUSED(arg5); ARG_UNUSED(arg6)

#define _SYSCALL_ARG2	ARG_UNUSED(arg3); ARG_UNUSED(arg4); ARG_UNUSED(arg5); \
			ARG_UNUSED(arg6)

#define _SYSCALL_ARG3	ARG_UNUSED(arg4); ARG_UNUSED(arg5); ARG_UNUSED(arg6)


#define _SYSCALL_ARG4	ARG_UNUSED(arg5); ARG_UNUSED(arg6)

#define _SYSCALL_ARG5	ARG_UNUSED(arg6)
#endif /* _ASMLANGUAGE */

#endif /* _ZEPHYR_SYSCALL_H_ */
