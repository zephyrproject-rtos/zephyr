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
 * @param ko Kernel object metadata pointer, or NULL
 * @param otype Expected type of the kernel object, or K_OBJ_ANY if type
 *	  doesn't matter
 * @param init If true, this is for an init function and we will not error
 *	   out if the object is not initialized
 * @return 0 If the object is valid
 *         -EBADF if not a valid object of the specified type
 *         -EPERM If the caller does not have permissions
 *         -EINVAL Object is not initialized
 */
int _k_object_validate(struct _k_object *ko, enum k_objects otype, int init);

/**
 * Dump out error information on failed _k_object_validate() call
 *
 * @param retval Return value from _k_object_validate()
 * @param obj Kernel object we were trying to verify
 * @param ko If retval=-EPERM, struct _k_object * that was looked up, or NULL
 * @param otype Expected type of the kernel object
 */
extern void _dump_object_error(int retval, void *obj, struct _k_object *ko,
			enum k_objects otype);

/**
 * Kernel object validation function
 *
 * Retrieve metadata for a kernel object. This function is implemented in
 * the gperf script footer, see gen_kobject_list.py
 *
 * @param obj Address of kernel object to get metadata
 * @return Kernel object's metadata, or NULL if the parameter wasn't the
 * memory address of a kernel object
 */
extern struct _k_object *_k_object_find(void *obj);

/**
 * Grant a thread permission to a kernel object
 *
 * @param ko Kernel object metadata to update
 * @param thread The thread to grant permission
 */
extern void _thread_perms_set(struct _k_object *ko, struct k_thread *thread);

/**
 * Grant all current and future threads access to a kernel object
 *
 * @param ko Kernel object metadata to update
 */
extern void _thread_perms_all_set(struct _k_object *ko);

/**
 * @brief Runtime expression check for system call arguments
 *
 * Used in handler functions to perform various runtime checks on arguments,
 * and generate a kernel oops if anything is not expected, printing a custom
 * message.
 *
 * @param expr Boolean expression to verify, a false result will trigger an
 *             oops
 * @param ssf Syscall stack frame argument passed to the handler function
 * @param fmt Printf-style format string (followed by appropriate variadic
 *            arguments) to print on verification failure
 */
#define _SYSCALL_VERIFY_MSG(expr, ssf, fmt, ...) \
	do { \
		if (!(expr)) { \
			printk("FATAL: syscall %s failed check: " fmt "\n", \
			       __func__, ##__VA_ARGS__); \
			_arch_syscall_oops(ssf); \
		} \
	} while (0)

/**
 * @brief Runtime expression check for system call arguments
 *
 * Used in handler functions to perform various runtime checks on arguments,
 * and generate a kernel oops if anything is not expected.
 *
 * @param expr Boolean expression to verify, a false result will trigger an
 *             oops. A stringified version of this expression will be printed.
 * @param ssf Syscall stack frame argument passed to the handler function
 *            arguments) to print on verification failure
 */
#define _SYSCALL_VERIFY(expr, ssf) _SYSCALL_VERIFY_MSG(expr, ssf, #expr)

#define _SYSCALL_MEMORY(ptr, size, write, ssf) \
	_SYSCALL_VERIFY_MSG(!_arch_buffer_validate((void *)ptr, size, write), \
			    ssf, "Memory region %p (size %u) %s access denied", \
			    (void *)(ptr), (u32_t)(size), \
			    write ? "write" : "read")

/**
 * @brief Runtime check that a user thread has read permission to a memory area
 *
 * Checks that the particular memory area is readable by the currently running
 * thread if the CPU was in user mode, and generates a kernel oops if it
 * wasn't. Prevents userspace from getting the kernel to read memory the thread
 * does not have access to, or passing in garbage pointers that would
 * crash/pagefault the kernel if dereferenced.
 *
 * @param ptr Memory area to examine
 * @param size Size of the memory area
 * @param write If the thread should be able to write to this memory, not just
 *		read it
 * @param ssf Syscall stack frame argument passed to the handler function
 */
#define _SYSCALL_MEMORY_READ(ptr, size, ssf) \
	_SYSCALL_MEMORY(ptr, size, 0, ssf)

/**
 * @brief Runtime check that a user thread has write permission to a memory area
 *
 * Checks that the particular memory area is readable and writable by the
 * currently running thread if the CPU was in user mode, and generates a kernel
 * oops if it wasn't. Prevents userspace from getting the kernel to read or
 * modify memory the thread does not have access to, or passing in garbage
 * pointers that would crash/pagefault the kernel if dereferenced.
 *
 * @param ptr Memory area to examine
 * @param size Size of the memory area
 * @param write If the thread should be able to write to this memory, not just
 *		read it
 * @param ssf Syscall stack frame argument passed to the handler function
 */
#define _SYSCALL_MEMORY_WRITE(ptr, size, ssf) \
	_SYSCALL_MEMORY(ptr, size, 1, ssf)

#define _SYSCALL_MEMORY_ARRAY(ptr, nmemb, size, write, ssf) \
	do { \
		u32_t product; \
		_SYSCALL_VERIFY_MSG(!__builtin_umul_overflow((u32_t)(nmemb), \
							     (u32_t)(size), \
							     &product), ssf, \
				    "%ux%u array is too large", \
				    (u32_t)(nmemb), (u32_t)(size)); \
		_SYSCALL_MEMORY(ptr, product, write, ssf); \
	} while (0)

/**
 * @brief Validate user thread has read permission for sized array
 *
 * Used when the memory region is expressed in terms of number of elements and
 * each element size, handles any overflow issues with computing the total
 * array bounds. Otherwise see _SYSCALL_MEMORY_READ.
 *
 * @param ptr Memory area to examine
 * @param nmemb Number of elements in the array
 * @param size Size of each array element
 * @param ssf Syscall stack frame argument passed to the handler function
 */
#define _SYSCALL_MEMORY_ARRAY_READ(ptr, nmemb, size, ssf) \
	_SYSCALL_MEMORY_ARRAY(ptr, nmemb, size, 0, ssf)

/**
 * @brief Validate user thread has read/write permission for sized array
 *
 * Used when the memory region is expressed in terms of number of elements and
 * each element size, handles any overflow issues with computing the total
 * array bounds. Otherwise see _SYSCALL_MEMORY_WRITE.
 *
 * @param ptr Memory area to examine
 * @param nmemb Number of elements in the array
 * @param size Size of each array element
 * @param ssf Syscall stack frame argument passed to the handler function
 */
#define _SYSCALL_MEMORY_ARRAY_WRITE(ptr, nmemb, size, ssf) \
	_SYSCALL_MEMORY_ARRAY(ptr, nmemb, size, 1, ssf)

static inline int _obj_validation_check(void *obj, enum k_objects otype,
					int init)
{
	struct _k_object *ko;
	int ret;

	ko = _k_object_find(obj);
	ret = _k_object_validate(ko, otype, init);

#ifdef CONFIG_PRINTK
	if (ret) {
		_dump_object_error(ret, obj, ko, otype);
	}
#endif

	return ret;
}


#define _SYSCALL_IS_OBJ(ptr, type, init, ssf) \
	_SYSCALL_VERIFY_MSG(!_obj_validation_check((void *)ptr, type, init), \
			    ssf, "object %p access denied", (void *)(ptr))

/**
 * @brief Runtime check kernel object pointer for non-init functions
 *
 * Calls _k_object_validate and triggers a kernel oops if the check files.
 * For use in system call handlers which are not init functions; a check
 * enforcing that an object is initialized* will not occur.
 *
 * @param ptr Untrusted kernel object pointer
 * @param type Expected kernel object type
 * @param ssf Syscall stack frame argument passed to the handler function
 */
#define _SYSCALL_OBJ(ptr, type, ssf) \
	_SYSCALL_IS_OBJ(ptr, type, 0, ssf)

/**
 * @brief Runtime check kernel object pointer for non-init functions
 *
 * See description of _SYSCALL_IS_OBJ. For use in system call handlers which
 * are not init functions; a check enforcing that an object is initialized
 * will not occur.
 *
 * @param ptr Untrusted kernel object pointer
 * @param type Expected kernel object type
 * @param ssf Syscall stack frame argument passed to the handler function
 */

#define _SYSCALL_OBJ_INIT(ptr, type, ssf) \
	_SYSCALL_IS_OBJ(ptr, type, 1, ssf)

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
