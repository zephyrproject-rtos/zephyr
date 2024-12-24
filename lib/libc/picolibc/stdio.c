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
	return 0;
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

#include "stdio-bufio.h"

#include <fcntl.h>
#include <unistd.h>

#define FDEV_SETUP_POSIX(fd, buf, size, rwflags, bflags)                                           \
	FDEV_SETUP_BUFIO(fd, buf, size, read, write, lseek, close, rwflags, bflags)

int __posix_sflags(const char *mode, int *optr);

FILE *z_libc_file_alloc(int fd, const char *mode)
{
	int stdio_flags;
	int open_flags;
	struct __file_bufio *bf;
	char *buf;

	stdio_flags = __posix_sflags(mode, &open_flags);
	if (stdio_flags == 0) {
		return NULL;
	}

	/* Allocate file structure and necessary buffers */
	bf = calloc(1, sizeof(struct __file_bufio) + BUFSIZ);

	if (bf == NULL) {
		close(fd);
		return NULL;
	}
	buf = (char *)(bf + 1);

	*bf = (struct __file_bufio)FDEV_SETUP_POSIX(fd, buf, BUFSIZ, stdio_flags, __BFALL);

	__bufio_lock_init(&(bf->xfile.cfile.file));

	if (open_flags & O_APPEND) {
		(void)fseeko(&(bf->xfile.cfile.file), 0, SEEK_END);
	}

	return &(bf->xfile.cfile.file);
}
