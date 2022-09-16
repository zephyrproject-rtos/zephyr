/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/syscall_handler.h>
#include <zephyr/kernel_structs.h>

static struct z_object *validate_any_object(const void *obj)
{
	struct z_object *ko;
	int ret;

	ko = z_object_find(obj);

	/* This can be any kernel object and it doesn't have to be
	 * initialized
	 */
	ret = z_object_validate(ko, K_OBJ_ANY, _OBJ_INIT_ANY);
	if (ret != 0) {
#ifdef CONFIG_LOG
		z_dump_object_error(ret, obj, ko, K_OBJ_ANY);
#endif
		return NULL;
	}

	return ko;
}

/* Normally these would be included in userspace.c, but the way
 * syscall_dispatch.c declares weak handlers results in build errors if these
 * are located in userspace.c. Just put in a separate file.
 *
 * To avoid double z_object_find() lookups, we don't call the implementation
 * function, but call a level deeper.
 */
static inline void z_vrfy_k_object_access_grant(const void *object,
						struct k_thread *thread)
{
	struct z_object *ko;

	Z_OOPS(Z_SYSCALL_OBJ_INIT(thread, K_OBJ_THREAD));
	ko = validate_any_object(object);
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(ko != NULL, "object %p access denied",
				    object));
	z_thread_perms_set(ko, thread);
}
#include <syscalls/k_object_access_grant_mrsh.c>

static inline void z_vrfy_k_object_release(const void *object)
{
	struct z_object *ko;

	ko = validate_any_object((void *)object);
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(ko != NULL, "object %p access denied",
				    (void *)object));
	z_thread_perms_clear(ko, _current);
}
#include <syscalls/k_object_release_mrsh.c>

static inline void *z_vrfy_k_object_alloc(enum k_objects otype)
{
	return z_impl_k_object_alloc(otype);
}
#include <syscalls/k_object_alloc_mrsh.c>
