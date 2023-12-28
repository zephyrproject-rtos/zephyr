/*
 * Copyright (c) 2017, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_SYSCALL_HANDLER_H_
#define ZEPHYR_INCLUDE_SYSCALL_HANDLER_H_

/**
 * @brief User mode and Syscall APIs
 * @defgroup syscall_apis User mode and Syscall APIs
 * @ingroup internal_api
 * @{
 */

#if defined(CONFIG_USERSPACE) || defined(__DOXYGEN__)

#ifndef _ASMLANGUAGE
#include <zephyr/kernel.h>
#include <zephyr/sys/arch_interface.h>
#include <zephyr/sys/math_extras.h>
#include <stdbool.h>
#include <zephyr/logging/log.h>

extern const _k_syscall_handler_t _k_syscall_table[K_SYSCALL_LIMIT];

enum _obj_init_check {
	_OBJ_INIT_TRUE = 0,
	_OBJ_INIT_FALSE = -1,
	_OBJ_INIT_ANY = 1
};

/**
 * Return true if we are currently handling a system call from user mode
 *
 * Inside z_vrfy functions, we always know that we are handling
 * a system call invoked from user context.
 *
 * However, some checks that are only relevant to user mode must
 * instead be placed deeper within the implementation. This
 * API is useful to conditionally make these checks.
 *
 * For performance reasons, whenever possible, checks should be placed
 * in the relevant z_vrfy function since these are completely skipped
 * when a syscall is invoked.
 *
 * This will return true only if we are handling a syscall for a
 * user thread. If the system call was invoked from supervisor mode,
 * or we are not handling a system call, this will return false.
 *
 * @note This is an internal API. Do not use unless you are extending
 *       functionality in the Zephyr tree.
 *
 * @return whether the current context is handling a syscall for a user
 *         mode thread
 */
static inline bool k_is_in_user_syscall(void)
{
	/* This gets set on entry to the syscall's generasted z_mrsh
	 * function and then cleared on exit. This code path is only
	 * encountered when a syscall is made from user mode, system
	 * calls from supervisor mode bypass everything directly to
	 * the implementation function.
	 */
	return !k_is_in_isr() && _current->syscall_frame != NULL;
}

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
 * @param init Indicate whether the object needs to already be in initialized
 *             or uninitialized state, or that we don't care
 * @note This is an internal API. Do not use unless you are extending
 *       functionality in the Zephyr tree.
 *
 * @return 0 If the object is valid
 *         -EBADF if not a valid object of the specified type
 *         -EPERM If the caller does not have permissions
 *         -EINVAL Object is not initialized
 */
int k_object_validate(struct k_object *ko, enum k_objects otype,
		      enum _obj_init_check init);

/**
 * Dump out error information on failed k_object_validate() call
 *
 * @param retval Return value from k_object_validate()
 * @param obj Kernel object we were trying to verify
 * @param ko If retval=-EPERM, struct k_object * that was looked up, or NULL
 * @param otype Expected type of the kernel object
 * @note This is an internal API. Do not use unless you are extending
 *       functionality in the Zephyr tree.
 *
 */
void k_object_dump_error(int retval, const void *obj,
			 struct k_object *ko, enum k_objects otype);

/**
 * Kernel object validation function
 *
 * Retrieve metadata for a kernel object. This function is implemented in
 * the gperf script footer, see gen_kobject_list.py
 *
 * @param obj Address of kernel object to get metadata
 * @return Kernel object's metadata, or NULL if the parameter wasn't the
 * memory address of a kernel object
 * @note This is an internal API. Do not use unless you are extending
 *       functionality in the Zephyr tree.
 *
 */
struct k_object *k_object_find(const void *obj);

typedef void (*_wordlist_cb_func_t)(struct k_object *ko, void *context);

/**
 * Iterate over all the kernel object metadata in the system
 *
 * @param func function to run on each struct k_object
 * @param context Context pointer to pass to each invocation
 * @note This is an internal API. Do not use unless you are extending
 *       functionality in the Zephyr tree.
 *
 */
void k_object_wordlist_foreach(_wordlist_cb_func_t func, void *context);

/**
 * Copy all kernel object permissions from the parent to the child
 *
 * @param parent Parent thread, to get permissions from
 * @param child Child thread, to copy permissions to
 * @note This is an internal API. Do not use unless you are extending
 *       functionality in the Zephyr tree.
 *
 */
void k_thread_perms_inherit(struct k_thread *parent, struct k_thread *child);

/**
 * Grant a thread permission to a kernel object
 *
 * @param ko Kernel object metadata to update
 * @param thread The thread to grant permission
 * @note This is an internal API. Do not use unless you are extending
 *       functionality in the Zephyr tree.
 *
 */
void k_thread_perms_set(struct k_object *ko, struct k_thread *thread);

/**
 * Revoke a thread's permission to a kernel object
 *
 * @param ko Kernel object metadata to update
 * @param thread The thread to grant permission
 * @note This is an internal API. Do not use unless you are extending
 *       functionality in the Zephyr tree.
 *
 */
void k_thread_perms_clear(struct k_object *ko, struct k_thread *thread);

/**
 * Revoke access to all objects for the provided thread
 *
 * @note Unlike k_thread_perms_clear(), this function will not clear
 * permissions on public objects.
 *
 * @note This is an internal API. Do not use unless you are extending
 *       functionality in the Zephyr tree.
 *
 * @param thread Thread object to revoke access
 */
void k_thread_perms_all_clear(struct k_thread *thread);

/**
 * Clear initialization state of a kernel object
 *
 * Intended for thread objects upon thread exit, or for other kernel objects
 * that were released back to an object pool.
 *
 * @param obj Address of the kernel object
 *
 * @note This is an internal API. Do not use unless you are extending
 *       functionality in the Zephyr tree.
 */
void k_object_uninit(const void *obj);

/**
 * Initialize and reset permissions to only access by the caller
 *
 * Intended for scenarios where objects are fetched from slab pools
 * and may have had different permissions set during prior usage.
 *
 * This is only intended for pools of objects, where such objects are
 * acquired and released to the pool. If an object has already been used,
 * we do not want stale permission information hanging around, the object
 * should only have permissions on the caller. Objects which are not
 * managed by a pool-like mechanism should not use this API.
 *
 * The object will be marked as initialized and the calling thread
 * granted access to it.
 *
 * @param obj Address of the kernel object
 * @note This is an internal API. Do not use unless you are extending
 *       functionality in the Zephyr tree.
 */
void k_object_recycle(const void *obj);

/**
 * @brief Obtain the size of a C string passed from user mode
 *
 * Given a C string pointer and a maximum size, obtain the true
 * size of the string (not including the trailing NULL byte) just as
 * if calling strnlen() on it, with the same semantics of strnlen() with
 * respect to the return value and the maxlen parameter.
 *
 * Any memory protection faults triggered by the examination of the string
 * will be safely handled and an error code returned.
 *
 * NOTE: Doesn't guarantee that user mode has actual access to this
 * string, you will need to still do a K_SYSCALL_MEMORY_READ()
 * with the obtained size value to guarantee this.
 *
 * @param src String to measure size of
 * @param maxlen Maximum number of characters to examine
 * @param err Pointer to int, filled in with -1 on memory error, 0 on
 *	success
 * @return undefined on error, or strlen(src) if that is less than maxlen, or
 *	maxlen if there were no NULL terminating characters within the
 *	first maxlen bytes.
 * @note This is an internal API. Do not use unless you are extending
 *       functionality in the Zephyr tree.
 */
static inline size_t k_usermode_string_nlen(const char *src, size_t maxlen,
					int *err)
{
	return arch_user_string_nlen(src, maxlen, err);
}

/**
 * @brief Copy data from userspace into a resource pool allocation
 *
 * Given a pointer and a size, allocate a similarly sized buffer in the
 * caller's resource pool and copy all the data within it to the newly
 * allocated buffer. This will need to be freed later with k_free().
 *
 * Checks are done to ensure that the current thread would have read
 * access to the provided buffer.
 *
 * @param src Source memory address
 * @param size Size of the memory buffer
 * @return An allocated buffer with the data copied within it, or NULL
 *	if some error condition occurred
 * @note This is an internal API. Do not use unless you are extending
 *       functionality in the Zephyr tree.
 */
void *k_usermode_alloc_from_copy(const void *src, size_t size);

/**
 * @brief Copy data from user mode
 *
 * Given a userspace pointer and a size, copies data from it into a provided
 * destination buffer, performing checks to ensure that the caller would have
 * appropriate access when in user mode.
 *
 * @param dst Destination memory buffer
 * @param src Source memory buffer, in userspace
 * @param size Number of bytes to copy
 * @retval 0 On success
 * @retval EFAULT On memory access error
 * @note This is an internal API. Do not use unless you are extending
 *       functionality in the Zephyr tree.
 */
int k_usermode_from_copy(void *dst, const void *src, size_t size);

/**
 * @brief Copy data to user mode
 *
 * Given a userspace pointer and a size, copies data to it from a provided
 * source buffer, performing checks to ensure that the caller would have
 * appropriate access when in user mode.
 *
 * @param dst Destination memory buffer, in userspace
 * @param src Source memory buffer
 * @param size Number of bytes to copy
 * @retval 0 On success
 * @retval EFAULT On memory access error
 * @note This is an internal API. Do not use unless you are extending
 *       functionality in the Zephyr tree.
 */
int k_usermode_to_copy(void *dst, const void *src, size_t size);

/**
 * @brief Copy a C string from userspace into a resource pool allocation
 *
 * Given a C string and maximum length, duplicate the string using an
 * allocation from the calling thread's resource pool. This will need to be
 * freed later with k_free().
 *
 * Checks are performed to ensure that the string is valid memory and that
 * the caller has access to it in user mode.
 *
 * @param src Source string pointer, in userspace
 * @param maxlen Maximum size of the string including trailing NULL
 * @return The duplicated string, or NULL if an error occurred.
 * @note This is an internal API. Do not use unless you are extending
 *       functionality in the Zephyr tree.
 */
char *k_usermode_string_alloc_copy(const char *src, size_t maxlen);

/**
 * @brief Copy a C string from userspace into a provided buffer
 *
 * Given a C string and maximum length, copy the string into a buffer.
 *
 * Checks are performed to ensure that the string is valid memory and that
 * the caller has access to it in user mode.
 *
 * @param dst Destination buffer
 * @param src Source string pointer, in userspace
 * @param maxlen Maximum size of the string including trailing NULL
 * @retval 0 on success
 * @retval EINVAL if the source string is too long with respect
 *	to maxlen
 * @retval EFAULT On memory access error
 * @note This is an internal API. Do not use unless you are extending
 *       functionality in the Zephyr tree.
 */
int k_usermode_string_copy(char *dst, const char *src, size_t maxlen);

/**
 * @brief Induce a kernel oops
 *
 * This macro can be used to induce a kernel oops which will kill the
 * calling thread.
 *
 * @param expr Expression to be evaluated
 *
 * @note This is an internal API. Do not use unless you are extending
 *       functionality in the Zephyr tree.
 */
#define K_OOPS(expr) \
	do { \
		if (expr) { \
			arch_syscall_oops(_current->syscall_frame); \
		} \
	} while (false)

/**
 * @brief Runtime expression check for system call arguments
 *
 * Used in handler functions to perform various runtime checks on arguments,
 * and generate a kernel oops if anything is not expected, printing a custom
 * message.
 *
 * @param expr Boolean expression to verify, a false result will trigger an
 *             oops
 * @param fmt Printf-style format string (followed by appropriate variadic
 *            arguments) to print on verification failure
 * @return False on success, True on failure
 * @note This is an internal API. Do not use unless you are extending
 *       functionality in the Zephyr tree.
 */
#define K_SYSCALL_VERIFY_MSG(expr, fmt, ...) ({ \
	bool expr_copy = !(expr); \
	if (expr_copy) { \
		TOOLCHAIN_IGNORE_WSHADOW_BEGIN \
		LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL); \
		TOOLCHAIN_IGNORE_WSHADOW_END \
		LOG_ERR("syscall %s failed check: " fmt, \
			__func__, ##__VA_ARGS__); \
	} \
	expr_copy; })

/**
 * @brief Runtime expression check for system call arguments
 *
 * Used in handler functions to perform various runtime checks on arguments,
 * and generate a kernel oops if anything is not expected.
 *
 * @param expr Boolean expression to verify, a false result will trigger an
 *             oops. A stringified version of this expression will be printed.
 * @return 0 on success, nonzero on failure
 * @note This is an internal API. Do not use unless you are extending
 *       functionality in the Zephyr tree.
 */
#define K_SYSCALL_VERIFY(expr) K_SYSCALL_VERIFY_MSG(expr, #expr)

/**
 * @brief Macro to check if size is negative
 *
 * K_SYSCALL_MEMORY can be called with signed/unsigned types
 * and because of that if we check if size is greater or equal to
 * zero, many static analyzers complain about no effect expression.
 *
 * @param ptr Memory area to examine
 * @param size Size of the memory area
 * @return true if size is valid, false otherwise
 * @note This is an internal API. Do not use unless you are extending
 *       functionality in the Zephyr tree.
 */
#define K_SYSCALL_MEMORY_SIZE_CHECK(ptr, size) \
	(((uintptr_t)(ptr) + (size)) >= (uintptr_t)(ptr))

/**
 * @brief Runtime check that a user thread has read and/or write permission to
 *        a memory area
 *
 * Checks that the particular memory area is readable and/or writeable by the
 * currently running thread if the CPU was in user mode, and generates a kernel
 * oops if it wasn't. Prevents userspace from getting the kernel to read and/or
 * modify memory the thread does not have access to, or passing in garbage
 * pointers that would crash/pagefault the kernel if dereferenced.
 *
 * @param ptr Memory area to examine
 * @param size Size of the memory area
 * @param write If the thread should be able to write to this memory, not just
 *		read it
 * @return 0 on success, nonzero on failure
 * @note This is an internal API. Do not use unless you are extending
 *       functionality in the Zephyr tree.
 */
#define K_SYSCALL_MEMORY(ptr, size, write) \
	K_SYSCALL_VERIFY_MSG(K_SYSCALL_MEMORY_SIZE_CHECK(ptr, size) \
			     && !Z_DETECT_POINTER_OVERFLOW(ptr, size) \
			     && (arch_buffer_validate((void *)ptr, size, write) \
			     == 0), \
			     "Memory region %p (size %zu) %s access denied", \
			     (void *)(ptr), (size_t)(size), \
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
 * @return 0 on success, nonzero on failure
 * @note This is an internal API. Do not use unless you are extending
 *       functionality in the Zephyr tree.
 */
#define K_SYSCALL_MEMORY_READ(ptr, size) \
	K_SYSCALL_MEMORY(ptr, size, 0)

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
 * @return 0 on success, nonzero on failure
 *
 * @note This is an internal API. Do not use unless you are extending
 *       functionality in the Zephyr tree.
 */
#define K_SYSCALL_MEMORY_WRITE(ptr, size) \
	K_SYSCALL_MEMORY(ptr, size, 1)

#define K_SYSCALL_MEMORY_ARRAY(ptr, nmemb, size, write) \
	({ \
		size_t product; \
		K_SYSCALL_VERIFY_MSG(!size_mul_overflow((size_t)(nmemb), \
							(size_t)(size), \
							&product), \
				     "%zux%zu array is too large", \
				     (size_t)(nmemb), (size_t)(size)) ||  \
			K_SYSCALL_MEMORY(ptr, product, write); \
	})

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
 * @return 0 on success, nonzero on failure
 * @note This is an internal API. Do not use unless you are extending
 *       functionality in the Zephyr tree.
 */
#define K_SYSCALL_MEMORY_ARRAY_READ(ptr, nmemb, size) \
	K_SYSCALL_MEMORY_ARRAY(ptr, nmemb, size, 0)

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
 * @return 0 on success, nonzero on failure
 * @note This is an internal API. Do not use unless you are extending
 *       functionality in the Zephyr tree.
 */
#define K_SYSCALL_MEMORY_ARRAY_WRITE(ptr, nmemb, size) \
	K_SYSCALL_MEMORY_ARRAY(ptr, nmemb, size, 1)

static inline int k_object_validation_check(struct k_object *ko,
					 const void *obj,
					 enum k_objects otype,
					 enum _obj_init_check init)
{
	int ret;

	ret = k_object_validate(ko, otype, init);

#ifdef CONFIG_LOG
	if (ret != 0) {
		k_object_dump_error(ret, obj, ko, otype);
	}
#else
	ARG_UNUSED(obj);
#endif

	return ret;
}

#define K_SYSCALL_IS_OBJ(ptr, type, init) \
	K_SYSCALL_VERIFY_MSG(k_object_validation_check(			\
				     k_object_find((const void *)ptr),	\
				     (const void *)ptr,			\
				     type, init) == 0, "access denied")

/**
 * @brief Runtime check driver object pointer for presence of operation
 *
 * Validates if the driver object is capable of performing a certain operation.
 *
 * @param ptr Untrusted device instance object pointer
 * @param api_name Name of the driver API struct (e.g. gpio_driver_api)
 * @param op Driver operation (e.g. manage_callback)
 *
 * @return 0 on success, nonzero on failure
 *
 * @note This is an internal API. Do not use unless you are extending
 *       functionality in the Zephyr tree.
 */
#define K_SYSCALL_DRIVER_OP(ptr, api_name, op) \
	({ \
		struct api_name *__device__ = (struct api_name *) \
			((const struct device *)ptr)->api; \
		K_SYSCALL_VERIFY_MSG(__device__->op != NULL, \
				    "Operation %s not defined for driver " \
				    "instance %p", \
				    # op, __device__); \
	})

/**
 * @brief Runtime check that device object is of a specific driver type
 *
 * Checks that the driver object passed in is initialized, the caller has
 * correct permissions, and that it belongs to the specified driver
 * subsystems. Additionally, all devices store a structure pointer of the
 * driver's API. If this doesn't match the value provided, the check will fail.
 *
 * This provides an easy way to determine if a device object not only
 * belongs to a particular subsystem, but is of a specific device driver
 * implementation. Useful for defining out-of-subsystem system calls
 * which are implemented for only one driver.
 *
 * @param _device Untrusted device pointer
 * @param _dtype Expected kernel object type for the provided device pointer
 * @param _api Expected driver API structure memory address
 * @return 0 on success, nonzero on failure
 * @note This is an internal API. Do not use unless you are extending
 *       functionality in the Zephyr tree.
 */
#define K_SYSCALL_SPECIFIC_DRIVER(_device, _dtype, _api) \
	({ \
		const struct device *_dev = (const struct device *)_device; \
		K_SYSCALL_OBJ(_dev, _dtype) || \
			K_SYSCALL_VERIFY_MSG(_dev->api == _api, \
					     "API structure mismatch"); \
	})

/**
 * @brief Runtime check kernel object pointer for non-init functions
 *
 * Calls k_object_validate and triggers a kernel oops if the check fails.
 * For use in system call handlers which are not init functions; a fatal
 * error will occur if the object is not initialized.
 *
 * @param ptr Untrusted kernel object pointer
 * @param type Expected kernel object type
 * @return 0 on success, nonzero on failure
 * @note This is an internal API. Do not use unless you are extending
 *       functionality in the Zephyr tree.
 */
#define K_SYSCALL_OBJ(ptr, type) \
	K_SYSCALL_IS_OBJ(ptr, type, _OBJ_INIT_TRUE)

/**
 * @brief Runtime check kernel object pointer for non-init functions
 *
 * See description of _SYSCALL_IS_OBJ. No initialization checks are done.
 * Intended for init functions where objects may be re-initialized at will.
 *
 * @param ptr Untrusted kernel object pointer
 * @param type Expected kernel object type
 * @return 0 on success, nonzero on failure
 * @note This is an internal API. Do not use unless you are extending
 *       functionality in the Zephyr tree.
 */

#define K_SYSCALL_OBJ_INIT(ptr, type) \
	K_SYSCALL_IS_OBJ(ptr, type, _OBJ_INIT_ANY)

/**
 * @brief Runtime check kernel object pointer for non-init functions
 *
 * See description of _SYSCALL_IS_OBJ. Triggers a fatal error if the object is
 * initialized. Intended for init functions where objects, once initialized,
 * can only be re-used when their initialization state expires due to some
 * other mechanism.
 *
 * @param ptr Untrusted kernel object pointer
 * @param type Expected kernel object type
 * @return 0 on success, nonzero on failure
 * @note This is an internal API. Do not use unless you are extending
 *       functionality in the Zephyr tree.
 */

#define K_SYSCALL_OBJ_NEVER_INIT(ptr, type) \
	K_SYSCALL_IS_OBJ(ptr, type, _OBJ_INIT_FALSE)

#include <driver-validation.h>

#endif /* _ASMLANGUAGE */

#endif /* CONFIG_USERSPACE */
/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_SYSCALL_HANDLER_H_ */
