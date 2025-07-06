/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_SYS_KOBJECT_H
#define ZEPHYR_INCLUDE_SYS_KOBJECT_H

#include <stdint.h>
#include <stddef.h>

#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/internal/kobject_internal.h>

#ifdef __cplusplus
extern "C" {
#endif

struct k_thread;
struct k_mutex;
struct z_futex_data;

/**
 * @brief Kernel Object Types
 *
 * This enumeration needs to be kept in sync with the lists of kernel objects
 * and subsystems in scripts/build/gen_kobject_list.py, as well as the otype_to_str()
 * function in kernel/userspace.c
 */
enum k_objects {
	K_OBJ_ANY,

	/** @cond
	 *  Doxygen should ignore this build-time generated include file
	 *  when generating API documentation.  Enumeration values are
	 *  generated during build by gen_kobject_list.py.  It includes
	 *  basic kernel objects (e.g.  pipes and mutexes) and driver types.
	 */
#include <zephyr/kobj-types-enum.h>
	/** @endcond
	 */

	K_OBJ_LAST
};
/**
 * @defgroup usermode_apis User Mode APIs
 * @ingroup kernel_apis
 * @{
 */

#ifdef CONFIG_USERSPACE

/**
 * @brief Grant a static thread access to a list of kernel objects
 *
 * For threads declared with K_THREAD_DEFINE(), grant the thread access to
 * a set of kernel objects. These objects do not need to be in an initialized
 * state. The permissions will be granted when the threads are initialized
 * in the early boot sequence.
 *
 * All arguments beyond the first must be pointers to kernel objects.
 *
 * @param name_ Name of the thread, as passed to K_THREAD_DEFINE()
 */
#define K_THREAD_ACCESS_GRANT(name_, ...) \
	static void * const _CONCAT(_object_list_, name_)[] = \
		{ __VA_ARGS__, NULL }; \
	static const STRUCT_SECTION_ITERABLE(k_object_assignment, \
					_CONCAT(_object_access_, name_)) = \
			{ (&_k_thread_obj_ ## name_), \
			  (_CONCAT(_object_list_, name_)) }

/** Object initialized */
#define K_OBJ_FLAG_INITIALIZED	BIT(0)
/** Object is Public */
#define K_OBJ_FLAG_PUBLIC	BIT(1)
/** Object allocated */
#define K_OBJ_FLAG_ALLOC	BIT(2)
/** Driver Object */
#define K_OBJ_FLAG_DRIVER	BIT(3)

/**
 * Grant a thread access to a kernel object
 *
 * The thread will be granted access to the object if the caller is from
 * supervisor mode, or the caller is from user mode AND has permissions
 * on both the object and the thread whose access is being granted.
 *
 * @param object Address of kernel object
 * @param thread Thread to grant access to the object
 */
__syscall void k_object_access_grant(const void *object,
				     struct k_thread *thread);

/**
 * Revoke a thread's access to a kernel object
 *
 * The thread will lose access to the object if the caller is from
 * supervisor mode, or the caller is from user mode AND has permissions
 * on both the object and the thread whose access is being revoked.
 *
 * @param object Address of kernel object
 * @param thread Thread to remove access to the object
 */
void k_object_access_revoke(const void *object, struct k_thread *thread);

/**
 * @brief Release an object
 *
 * Allows user threads to drop their own permission on an object
 * Their permissions are automatically cleared when a thread terminates.
 *
 * @param object The object to be released
 *
 */
__syscall void k_object_release(const void *object);

/**
 * Grant all present and future threads access to an object
 *
 * If the caller is from supervisor mode, or the caller is from user mode and
 * have sufficient permissions on the object, then that object will have
 * permissions granted to it for *all* current and future threads running in
 * the system, effectively becoming a public kernel object.
 *
 * Use of this API should be avoided on systems that are running untrusted code
 * as it is possible for such code to derive the addresses of kernel objects
 * and perform unwanted operations on them.
 *
 * It is not possible to revoke permissions on public objects; once public,
 * any thread may use it.
 *
 * @param object Address of kernel object
 */
void k_object_access_all_grant(const void *object);

/**
 * Check if a kernel object is of certain type and is valid.
 *
 * This checks if the kernel object exists, of certain type,
 * and has been initialized.
 *
 * @param obj Address of the kernel object
 * @param otype Object type (use K_OBJ_ANY for ignoring type checking)
 * @return True if kernel object (@a obj) exists, of certain type, and
 *         has been initialized. False otherwise.
 */
bool k_object_is_valid(const void *obj, enum k_objects otype);

#else
/* LCOV_EXCL_START */
#define K_THREAD_ACCESS_GRANT(thread, ...)

/**
 * @internal
 */
static inline void z_impl_k_object_access_grant(const void *object,
						struct k_thread *thread)
{
	ARG_UNUSED(object);
	ARG_UNUSED(thread);
}

/**
 * @internal
 */
static inline void k_object_access_revoke(const void *object,
					  struct k_thread *thread)
{
	ARG_UNUSED(object);
	ARG_UNUSED(thread);
}

/**
 * @internal
 */
static inline void z_impl_k_object_release(const void *object)
{
	ARG_UNUSED(object);
}

static inline void k_object_access_all_grant(const void *object)
{
	ARG_UNUSED(object);
}

static inline bool k_object_is_valid(const void *obj, enum k_objects otype)
{
	ARG_UNUSED(obj);
	ARG_UNUSED(otype);

	return true;
}

/* LCOV_EXCL_STOP */
#endif /* !CONFIG_USERSPACE */

#if defined(CONFIG_DYNAMIC_OBJECTS) || defined(__DOXYGEN__)
/**
 * Allocate a kernel object of a designated type
 *
 * This will instantiate at runtime a kernel object of the specified type,
 * returning a pointer to it. The object will be returned in an uninitialized
 * state, with the calling thread being granted permission on it. The memory
 * for the object will be allocated out of the calling thread's resource pool.
 *
 * @note This function is available only if @kconfig{CONFIG_DYNAMIC_OBJECTS}
 * is selected.
 *
 * @note Thread stack object has to use k_object_alloc_size() since stacks may
 * have different sizes.
 *
 * @param otype Requested kernel object type
 * @return A pointer to the allocated kernel object, or NULL if memory wasn't
 * available
 */
__syscall void *k_object_alloc(enum k_objects otype);

/**
 * Allocate a kernel object of a designated type and a given size
 *
 * This will instantiate at runtime a kernel object of the specified type,
 * returning a pointer to it. The object will be returned in an uninitialized
 * state, with the calling thread being granted permission on it. The memory
 * for the object will be allocated out of the calling thread's resource pool.
 *
 * This function is specially helpful for thread stack objects because
 * their sizes can vary. Other objects should probably look k_object_alloc().
 *
 * @note This function is available only if @kconfig{CONFIG_DYNAMIC_OBJECTS}
 * is selected.
 *
 * @param otype Requested kernel object type
 * @param size Requested kernel object size
 * @return A pointer to the allocated kernel object, or NULL if memory wasn't
 * available
 */
__syscall void *k_object_alloc_size(enum k_objects otype, size_t size);

/**
 * Free a kernel object previously allocated with k_object_alloc()
 *
 * This will return memory for a kernel object back to resource pool it was
 * allocated from.  Care must be exercised that the object will not be used
 * during or after when this call is made.
 *
 * @note This function is available only if @kconfig{CONFIG_DYNAMIC_OBJECTS}
 * is selected.
 *
 * @param obj Pointer to the kernel object memory address.
 */
void k_object_free(void *obj);
#else

/* LCOV_EXCL_START */
static inline void *z_impl_k_object_alloc(enum k_objects otype)
{
	ARG_UNUSED(otype);

	return NULL;
}

static inline void *z_impl_k_object_alloc_size(enum k_objects otype,
					size_t size)
{
	ARG_UNUSED(otype);
	ARG_UNUSED(size);

	return NULL;
}

/**
 * @brief Free an object
 *
 * @param obj
 */
static inline void k_object_free(void *obj)
{
	ARG_UNUSED(obj);
}
/* LCOV_EXCL_STOP */
#endif /* CONFIG_DYNAMIC_OBJECTS */

/** @} */

#include <zephyr/syscalls/kobject.h>
#ifdef __cplusplus
}
#endif

#endif
