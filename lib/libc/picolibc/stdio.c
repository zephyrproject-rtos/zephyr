/*
 * Copyright © 2021, Keith Packard <keithp@keithp.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "picolibc-hooks.h"

static int _stdout_hook_default(int c)
{
	ARG_UNUSED(c);

	return EOF;
}

static LIBC_DATA int (*_stdout_hook)(int) = _stdout_hook_default;

int z_impl_zephyr_fputc(int a, FILE *out)
{
	return ((out == stdout) || (out == stderr)) ? _stdout_hook(a) : EOF;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_zephyr_fputc(int c, FILE *stream)
{
	return z_impl_zephyr_fputc(c, stream);
}
#include <zephyr/syscalls/zephyr_fputc_mrsh.c>
#endif

static int picolibc_put(char a, FILE *f)
{
	return zephyr_fputc(a, f);
}

static LIBC_DATA FILE __stdout = FDEV_SETUP_STREAM(picolibc_put, NULL, NULL, 0);
static LIBC_DATA FILE __stdin = FDEV_SETUP_STREAM(NULL, NULL, NULL, 0);

#ifdef __strong_reference
#define STDIO_ALIAS(x) __strong_reference(stdout, x);
#else
#define STDIO_ALIAS(x) FILE *const x = &__stdout;
#endif

FILE *const stdin = &__stdin;
FILE *const stdout = &__stdout;
STDIO_ALIAS(stderr);

void __stdout_hook_install(int (*hook)(int))
{
	_stdout_hook = hook;
	__stdout.flags |= _FDEV_SETUP_WRITE;
}

void __stdin_hook_install(unsigned char (*hook)(void))
{
	__stdin.get = (int (*)(FILE *)) hook;
	__stdin.flags |= _FDEV_SETUP_READ;
}

int z_impl_zephyr_write_stdout(const void *buffer, int nbytes)
{
	const char *buf = buffer;

	for (int i = 0; i < nbytes; i++) {
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
	K_OOPS(K_SYSCALL_MEMORY_READ(buf, nbytes));
	return z_impl_zephyr_write_stdout((const void *)buf, nbytes);
}
#include <zephyr/syscalls/zephyr_write_stdout_mrsh.c>
#endif
