/*
 * Copyright (c) 2015, 2021 Intel Corporation.
 * Copyright (c) 2021 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/sys/libc-hooks.h>
#include <zephyr/syscall_handler.h>
#include <string.h>
#include <zephyr/sys/errno_private.h>
#include <unistd.h>
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
static inline int z_vrfy_zephyr_write_stdout(const void *buf, int nbytes)
{
	Z_OOPS(Z_SYSCALL_MEMORY_READ(buf, nbytes));
	return z_impl_zephyr_write_stdout(buf, nbytes);
}
#include <syscalls/zephyr_write_stdout_mrsh.c>
#endif

#ifndef CONFIG_POSIX_API
int _write(int fd, const char *buf, unsigned int nbytes)
{
	ARG_UNUSED(fd);

	return zephyr_write_stdout(buf, nbytes);
}
#endif

/*
 * It's require to implement _isatty to have STDIN/STDOUT/STDERR buffered
 * properly.
 */
int _isatty(int file)
{
	if (file == STDIN_FILENO || file == STDOUT_FILENO || file == STDERR_FILENO) {
		return 1;
	}

	return 0;
}

int *___errno(void)
{
	return z_errno();
}

__weak void _exit(int status)
{
	while (1) {
	}
}
