/*
 * Copyright © 2021, Keith Packard <keithp@keithp.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "picolibc-hooks.h"
#include "stdio-bufio.h"

static LIBC_DATA int (*_stdout_hook)(int);

int z_impl_zephyr_fputc(int a, FILE *out)
{
	(*_stdout_hook)(a);
	return 0;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_zephyr_fputc(int c, FILE *stream)
{
	return z_impl_zephyr_fputc(c, stream);
}
#include <zephyr/syscalls/zephyr_fputc_mrsh.c>
#endif

#ifndef CONFIG_ZVFS
static int picolibc_put(char a, FILE *f)
{
	zephyr_fputc(a, f);
	return 0;
}

static LIBC_DATA FILE __stdout = FDEV_SETUP_STREAM(picolibc_put, NULL, NULL, 0);
static LIBC_DATA FILE __stdin = FDEV_SETUP_STREAM(NULL, NULL, NULL, 0);
#endif

#ifdef __strong_reference
#define STDIO_ALIAS(x) __strong_reference(stdout, x);
#else
#define STDIO_ALIAS(x) FILE *const x = &__stdout;
#endif

#ifndef CONFIG_ZVFS
FILE *const stdin = &__stdin;
FILE *const stdout = &__stdout;
STDIO_ALIAS(stderr);
#endif

void __stdout_hook_install(int (*hook)(int))
{
#ifdef CONFIG_ZVFS
	struct __file_bufio *bp = (struct __file_bufio *)stdout;

	bp->ptr = INT_TO_POINTER(1 /* STDOUT_FILENO */);
	bp->bflags |= _FDEV_SETUP_WRITE;
#else
	__stdout.flags |= _FDEV_SETUP_WRITE;
#endif

	_stdout_hook = hook;
}

void __stdin_hook_install(unsigned char (*hook)(void))
{
#ifdef CONFIG_ZVFS
	struct __file_bufio *bp = (struct __file_bufio *)stdin;

	bp->bflags |= _FDEV_SETUP_READ;
	/* bp->get = (int (*)(FILE *))hook; */
	bp->ptr = INT_TO_POINTER(0 /* STDIN_FILENO */);
#else
	__stdin.get = (int (*)(FILE *)) hook;
	__stdin.flags |= _FDEV_SETUP_READ;
#endif
}
