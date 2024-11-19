/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/toolchain.h>

static struct k_object *validate_kernel_object(const void *obj,
					       enum k_objects otype,
					       enum _obj_init_check init)
{
	struct k_object *ko;
	int ret;

	ko = k_object_find(obj);

	/* This can be any kernel object and it doesn't have to be
	 * initialized
	 */
	ret = k_object_validate(ko, K_OBJ_ANY, _OBJ_INIT_ANY);
	if (ret != 0) {
#ifdef CONFIG_LOG
		k_object_dump_error(ret, obj, ko, otype);
#endif /* CONFIG_LOG */
		return NULL;
	}

	return ko;
}

static ALWAYS_INLINE struct k_object *validate_any_object(const void *obj)
{
	return validate_kernel_object(obj, K_OBJ_ANY, _OBJ_INIT_ANY);
}

bool k_object_is_valid(const void *obj, enum k_objects otype)
{
	struct k_object *ko;

	ko = validate_kernel_object(obj, otype, _OBJ_INIT_TRUE);

	return (ko != NULL);
}

/* Normally these would be included in userspace.c, but the way
 * syscall_dispatch.c declares weak handlers results in build errors if these
 * are located in userspace.c. Just put in a separate file.
 *
 * To avoid double k_object_find() lookups, we don't call the implementation
 * function, but call a level deeper.
 */
static inline void z_vrfy_k_object_access_grant(const void *object,
						struct k_thread *thread)
{
	struct k_object *ko;

	K_OOPS(K_SYSCALL_OBJ_INIT(thread, K_OBJ_THREAD));
	ko = validate_any_object(object);
	K_OOPS(K_SYSCALL_VERIFY_MSG(ko != NULL, "object %p access denied",
				    object));
	k_thread_perms_set(ko, thread);
}
#include <zephyr/syscalls/k_object_access_grant_mrsh.c>

static inline void z_vrfy_k_object_release(const void *object)
{
	struct k_object *ko;

	ko = validate_any_object(object);
	K_OOPS(K_SYSCALL_VERIFY_MSG(ko != NULL, "object %p access denied", object));
	k_thread_perms_clear(ko, arch_current_thread());
}
#include <zephyr/syscalls/k_object_release_mrsh.c>

static inline void *z_vrfy_k_object_alloc(enum k_objects otype)
{
	return z_impl_k_object_alloc(otype);
}
#include <zephyr/syscalls/k_object_alloc_mrsh.c>

static inline void *z_vrfy_k_object_alloc_size(enum k_objects otype, size_t size)
{
	return z_impl_k_object_alloc_size(otype, size);
}
#include <zephyr/syscalls/k_object_alloc_size_mrsh.c>
