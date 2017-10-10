/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <syscall_handler.h>

/* Normally these would be included in userspace.c, but the way
 * syscall_dispatch.c declares weak handlers results in build errors if these
 * are located in userspace.c. Just put in a separate file.
 */

u32_t _handler_k_object_access_grant(u32_t object, u32_t thread, u32_t arg3,
				     u32_t arg4, u32_t arg5, u32_t arg6,
				     void *ssf)
{
	_SYSCALL_ARG2;

	_SYSCALL_OBJ(thread, K_OBJ_THREAD, ssf);
	_impl_k_object_access_grant((void *)object, (struct k_thread *)thread);
	return 0;
}

u32_t _handler_k_object_access_all_grant(u32_t object, u32_t arg2, u32_t arg3,
					 u32_t arg4, u32_t arg5, u32_t arg6,
					 void *ssf)
{
	_SYSCALL_ARG1;

	_impl_k_object_access_all_grant((void *)object);
	return 0;
}
