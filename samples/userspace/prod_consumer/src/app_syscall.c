/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/syscall_handler.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app_syscall);

/* Implementation of magic_syscall() which is a system call just for this
 * application, not part of the kernel codebase.
 *
 * It's trivial, update the supplied cookie value by 1, only if it is less than
 * 42. This is just for demonstration purposes to show how syscalls can be
 * defined in application code.
 */
int z_impl_magic_syscall(unsigned int *cookie)
{
	LOG_DBG("magic syscall: got a cookie %u", *cookie);

	if (*cookie > 42) {
		LOG_ERR("bad cookie :(");
		return -EINVAL;
	}

	*cookie = *cookie + 1;

	return 0;
}

static int z_vrfy_magic_syscall(unsigned int *cookie)
{
	unsigned int cookie_copy;
	int ret;

	/* Confirm that this user-supplied pointer is valid memory that
	 * can be accessed. If it's OK, copy into cookie_copy.
	 */
	if (z_user_from_copy(&cookie_copy, cookie, sizeof(*cookie)) != 0) {
		return -EPERM;
	}

	/* We pass to the implementation the *copy*, to prevent TOCTOU attacks
	 */
	ret = z_impl_magic_syscall(&cookie_copy);

	if (ret == 0 &&
	    z_user_to_copy(cookie, &cookie_copy, sizeof(*cookie)) != 0) {
		return -EPERM;
	}

	return ret;
}

#include <syscalls/magic_syscall_mrsh.c>
