/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <syscall_handler.h>

static struct _k_object *validate_any_object(void *obj)
{
	struct _k_object *ko;
	int ret;

	ko = _k_object_find(obj);

	/* This can be any kernel object and it doesn't have to be
	 * initialized
	 */
	ret = _k_object_validate(ko, K_OBJ_ANY, 1);
	if (ret) {
#ifdef CONFIG_PRINTK
		_dump_object_error(ret, obj, ko, K_OBJ_ANY);
#endif
		return NULL;
	}

	return ko;
}

/* Normally these would be included in userspace.c, but the way
 * syscall_dispatch.c declares weak handlers results in build errors if these
 * are located in userspace.c. Just put in a separate file.
 *
 * To avoid double _k_object_find() lookups, we don't call the implementation
 * function, but call a level deeper.
 */

u32_t _handler_k_object_access_grant(u32_t object, u32_t thread, u32_t arg3,
				     u32_t arg4, u32_t arg5, u32_t arg6,
				     void *ssf)
{
	_SYSCALL_ARG2;
	struct _k_object *ko;

	_SYSCALL_OBJ(thread, K_OBJ_THREAD, ssf);
	ko = validate_any_object((void *)object);
	_SYSCALL_VERIFY_MSG(ko, ssf, "object %p access denied", (void *)object);
	_thread_perms_set(ko, (struct k_thread *)thread);

	return 0;
}

u32_t _handler_k_object_access_all_grant(u32_t object, u32_t arg2, u32_t arg3,
					 u32_t arg4, u32_t arg5, u32_t arg6,
					 void *ssf)
{
	_SYSCALL_ARG1;
	struct _k_object *ko;

	ko = validate_any_object((void *)object);
	_SYSCALL_VERIFY_MSG(ko, ssf, "object %p access denied", (void *)object);
	_thread_perms_all_set(ko);

	return 0;
}
