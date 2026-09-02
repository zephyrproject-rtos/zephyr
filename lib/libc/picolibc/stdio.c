/*
 * Copyright © 2021, Keith Packard <keithp@keithp.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "picolibc-hooks.h"

static LIBC_DATA int (*_stdout_hook)(int);

int z_impl_zephyr_fputc(int a, FILE *out)
{
	(*_stdout_hook)(a);
	ARG_UNUSED(out);
	return 0;
}

int z_impl_zephyr_write_stdout(const void *buffer, int nbytes)
{
	const char *buf = buffer;

	for (int i = 0; i < nbytes; i++) {
		if (*(buf + i) == '\n') {
			(*_stdout_hook)('\r');
		}
		(*_stdout_hook)(*(buf + i));
	}
	return nbytes;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_zephyr_fputc(int c, FILE *stream)
{
	return z_impl_zephyr_fputc(c, stream);
}
#include <zephyr/syscalls/zephyr_fputc_mrsh.c>
#endif

#ifdef CONFIG_ZVFS_RESERVE_STDIO_FDS
/*
 * The standard streams operate on the reserved descriptors 0..2 so that
 * dup2()'ed redirections (pipes, files) apply to stdio output as well; the
 * default descriptors refer to the console.
 */
#include <zephyr/sys/zvfs.h>

static int picolibc_put(char a, FILE *f)
{
	return (zvfs_write(1, &a, 1) == 1) ? 0 : _FDEV_ERR;
}

static int picolibc_put_err(char a, FILE *f)
{
	return (zvfs_write(2, &a, 1) == 1) ? 0 : _FDEV_ERR;
}

static int picolibc_get(FILE *f)
{
	char a;
	ssize_t ret = zvfs_read(0, &a, 1);

	return (ret == 1) ? (int)(unsigned char)a : ((ret == 0) ? _FDEV_EOF : _FDEV_ERR);
}

static LIBC_DATA FILE __stdout = FDEV_SETUP_STREAM(picolibc_put, NULL, NULL, _FDEV_SETUP_WRITE);
static LIBC_DATA FILE __stdin = FDEV_SETUP_STREAM(NULL, picolibc_get, NULL, _FDEV_SETUP_READ);
static LIBC_DATA FILE __stderr =
	FDEV_SETUP_STREAM(picolibc_put_err, NULL, NULL, _FDEV_SETUP_WRITE);

FILE *const stdin = &__stdin;
FILE *const stdout = &__stdout;
FILE *const stderr = &__stderr;
#else /* CONFIG_ZVFS_RESERVE_STDIO_FDS */
static int picolibc_put(char a, FILE *f)
{
	zephyr_fputc(a, f);
	return 0;
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
#endif /* CONFIG_ZVFS_RESERVE_STDIO_FDS */

void __stdout_hook_install(int (*hook)(int c))
{
	_stdout_hook = hook;
#ifndef CONFIG_ZVFS_RESERVE_STDIO_FDS
	__stdout.flags |= _FDEV_SETUP_WRITE;
#endif
}

void __stdin_hook_install(unsigned char (*hook)(void))
{
#ifndef CONFIG_ZVFS_RESERVE_STDIO_FDS
	__stdin.get = (int (*)(FILE *)) hook;
	__stdin.flags |= _FDEV_SETUP_READ;
#endif
	ARG_UNUSED(hook);
}
