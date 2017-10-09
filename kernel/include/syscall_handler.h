/*
 * Copyright (c) 2017, Intel Corporation
 *
 * SPDX-License-Identifier: Apache 2.0
 */


#ifndef _ZEPHYR_SYSCALL_HANDLER_H_
#define _ZEPHYR_SYSCALL_HANDLER_H_

#ifdef CONFIG_USERSPACE

#ifndef _ASMLANGUAGE
#include <kernel.h>
#include <misc/printk.h>
#include <nano_internal.h>

extern const _k_syscall_handler_t _k_syscall_table[K_SYSCALL_LIMIT];

/**
 * Ensure a system object is a valid object of the expected type
 *
 * Searches for the object and ensures that it is indeed an object
 * of the expected type, that the caller has the right permissions on it,
 * and that the object has been initialized.
 *
 * This function is intended to be called on the kernel-side system
 * call handlers to validate kernel object pointers passed in from
 * userspace.
 *
 * @param obj Address of the kernel object
 * @param otype Expected type of the kernel object
 * @param init If true, this is for an init function and we will not error
 *	   out if the object is not initialized
 * @return 0 If the object is valid
 *         -EBADF if not a valid object of the specified type
 *         -EPERM If the caller does not have permissions
 *         -EINVAL Object is not initialized
 */
int _k_object_validate(void *obj, enum k_objects otype, int init);

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
 * @brief Runtime check that a user thread has proper access to a memory area
 *
 * Checks that the particular memory area is readable or writable by the
 * currently running thread if the CPU was in user mode, and generates a kernel
 * oops if it wasn't. Prevents userspace from getting the kernel to read or
 * modify memory the thread does not have access to, or passing in garbage
 * pointers that would crash/pagefault the kernel if accessed.
 *
 * @param ptr Memory area to examine
 * @param size Size of the memory area
 * @param write If the thread should be able to write to this memory, not just
 *		read it
 * @param ssf Syscall stack frame argument passed to the handler function
 */
#define _SYSCALL_MEMORY(ptr, size, write, ssf) \
	_SYSCALL_VERIFY(!_arch_buffer_validate((void *)ptr, size, write), ssf)

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
			ARG_UNUSED(arg4); ARG_UNUSED(arg5); ARG_UNUSED(arg6)

#define _SYSCALL_ARG1	ARG_UNUSED(arg2); ARG_UNUSED(arg3); ARG_UNUSED(arg4); \
			ARG_UNUSED(arg5); ARG_UNUSED(arg6)

#define _SYSCALL_ARG2	ARG_UNUSED(arg3); ARG_UNUSED(arg4); ARG_UNUSED(arg5); \
			ARG_UNUSED(arg6)

#define _SYSCALL_ARG3	ARG_UNUSED(arg4); ARG_UNUSED(arg5); ARG_UNUSED(arg6)


#define _SYSCALL_ARG4	ARG_UNUSED(arg5); ARG_UNUSED(arg6)

#define _SYSCALL_ARG5	ARG_UNUSED(arg6)
#endif /* _ASMLANGUAGE */

#endif /* CONFIG_USERSPACE */

#endif /* _ZEPHYR_SYSCALL_H_ */
