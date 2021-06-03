/*
 * Copyright (c) 2015, 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <sys/libc-hooks.h>
#include <syscall_handler.h>
#include <string.h>
#include <sys/errno_private.h>
#include <errno.h>

static int _stdout_hook_default(int c)
{
	ARG_UNUSED(c);

	return EOF;
}

static int (*_stdout_hook)(int) = _stdout_hook_default;

void __stdout_hook_install(int (*hook)(int))
{
	_stdout_hook = hook;
}

int z_impl_zephyr_write_stdout(const void *buffer, int nbytes)
{
	const char *buf = buffer;
	int i;

	for (i = 0; i < nbytes; i++) {
		if (*(buf + i) == '\n') {
			_stdout_hook('\r');
		}
		_stdout_hook(*(buf + i));
	}
	return nbytes;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_z_zephyr_write_stdout(const void *buf, int nbytes)
{
	Z_OOPS(Z_SYSCALL_MEMORY_READ(buf, nbytes));
	return z_impl_zephyr_write_stdout(buf, nbytes);
}
#include <syscalls/z_zephyr_write_stdout_mrsh.c>
#endif

#ifndef CONFIG_POSIX_API
int _write(int fd, const char *buf, unsigned int nbytes)
{
	ARG_UNUSED(fd);

	return z_impl_zephyr_write_stdout(buf, nbytes);
}
#endif
