/* stdout_console.c */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/sys/libc-hooks.h>
#include <zephyr/internal/syscall_handler.h>
#include <string.h>

#if defined(CONFIG_ZVFS)
#include <zephyr/sys/zvfs.h>
#include <zephyr/sys/zvfs_libc.h>

#include "stdio_fd.h"
#endif

static int _stdout_hook_default(int c)
{
	(void)(c);  /* Prevent warning about unused argument */

	return EOF;
}

static int (*_stdout_hook)(int c) = _stdout_hook_default;

void __stdout_hook_install(int (*hook)(int c))
{
	_stdout_hook = hook;
}

static size_t minimal_console_write(const unsigned char *buf, size_t nbyte)
{
	size_t i;

	for (i = 0; i < nbyte; i++) {
		if (_stdout_hook((int)buf[i]) == EOF) {
			break;
		}
	}

	return i;
}

#if defined(CONFIG_ZVFS)
static size_t minimal_zvfs_stream_write(int fd, const void *ptr, size_t nbyte)
{
	const char *p = ptr;
	size_t written = 0;

	while (written < nbyte) {
		long ret = zvfs_write(fd, p + written, nbyte - written);

		if (ret <= 0) {
			break;
		}

		written += ret;
	}

	return written;
}

static size_t minimal_zvfs_stream_read(int fd, void *ptr, size_t nbyte)
{
	char *p = ptr;
	size_t nread = 0;

	while (nread < nbyte) {
		long ret = zvfs_read(fd, p + nread, nbyte - nread);

		if (ret <= 0) {
			break;
		}

		nread += ret;
	}

	return nread;
}
#endif

static size_t minimal_stream_read(void *ptr, size_t nbyte, FILE *stream)
{
#if defined(CONFIG_ZVFS)
	int fd = zvfs_libc_fileno(stream);

	if (fd < MINIMAL_STDIO_FD_MIN) {
		return 0;
	}

	return minimal_zvfs_stream_read(fd, ptr, nbyte);
#else
	ARG_UNUSED(ptr);
	ARG_UNUSED(nbyte);
	ARG_UNUSED(stream);
#endif

	return 0;
}

static size_t minimal_stream_write(const void *ptr, size_t nbyte, FILE *stream)
{
#if defined(CONFIG_ZVFS)
	int fd = zvfs_libc_fileno(stream);

#if !defined(CONFIG_ZVFS_RESERVE_STDIO_FDS)
	if (fd == 1 || fd == 2) {
		return minimal_console_write((const unsigned char *)ptr, nbyte);
	}
#endif
	if (fd >= MINIMAL_STDIO_FD_MIN) {
		return minimal_zvfs_stream_write(fd, ptr, nbyte);
	}
#else
	if ((stream == stdout) || (stream == stderr)) {
		return minimal_console_write((const unsigned char *)ptr, nbyte);
	}

	ARG_UNUSED(stream);
#endif

	return 0;
}

int z_impl_zephyr_fputc(int c, FILE *stream)
{
	unsigned char ch = (unsigned char)c;

	if (minimal_stream_write(&ch, 1, stream) == 1) {
		return c;
	}

	return EOF;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_zephyr_fputc(int c, FILE *stream)
{
	return z_impl_zephyr_fputc(c, stream);
}
#include <zephyr/syscalls/zephyr_fputc_mrsh.c>
#endif

int fputc(int c, FILE *stream)
{
	return zephyr_fputc(c, stream);
}

int fputs(const char *ZRESTRICT s, FILE *ZRESTRICT stream)
{
	size_t len;
	size_t n;

	if (s == NULL) {
		return EOF;
	}

	len = strlen(s);
	if (len == 0) {
		return 0;
	}

	n = minimal_stream_write(s, len, stream);

	return (n == len) ? 0 : EOF;
}

#undef putc
int putc(int c, FILE *stream)
{
	return zephyr_fputc(c, stream);
}

#undef putchar
int putchar(int c)
{
	return zephyr_fputc(c, stdout);
}

size_t z_impl_zephyr_fwrite(const void *ZRESTRICT ptr, size_t size,
			    size_t nitems, FILE *ZRESTRICT stream)
{
	size_t nbyte;
	size_t written;

	if ((nitems == 0) || (size == 0)) {
		return 0;
	}

	nbyte = size * nitems;
	written = minimal_stream_write(ptr, nbyte, stream);

	return written / size;
}

#ifdef CONFIG_USERSPACE
static inline size_t z_vrfy_zephyr_fwrite(const void *ZRESTRICT ptr,
					  size_t size, size_t nitems,
					  FILE *ZRESTRICT stream)
{

	K_OOPS(K_SYSCALL_MEMORY_ARRAY_READ(ptr, nitems, size));
	return z_impl_zephyr_fwrite(ptr, size, nitems, stream);
}
#include <zephyr/syscalls/zephyr_fwrite_mrsh.c>
#endif

size_t fwrite(const void *ZRESTRICT ptr, size_t size, size_t nitems,
			  FILE *ZRESTRICT stream)
{
	return zephyr_fwrite(ptr, size, nitems, stream);
}

size_t z_impl_zephyr_fread(void *ZRESTRICT ptr, size_t size, size_t nitems,
			   FILE *ZRESTRICT stream)
{
	size_t nbyte;
	size_t nread;

	if ((nitems == 0) || (size == 0)) {
		return 0;
	}

	nbyte = size * nitems;
	nread = minimal_stream_read(ptr, nbyte, stream);

	return nread / size;
}

#ifdef CONFIG_USERSPACE
static inline size_t z_vrfy_zephyr_fread(void *ZRESTRICT ptr, size_t size,
					 size_t nitems, FILE *ZRESTRICT stream)
{
	K_OOPS(K_SYSCALL_MEMORY_ARRAY_WRITE(ptr, nitems, size));
	return z_impl_zephyr_fread(ptr, size, nitems, stream);
}
#include <zephyr/syscalls/zephyr_fread_mrsh.c>
#endif

size_t fread(void *ZRESTRICT ptr, size_t size, size_t nitems, FILE *ZRESTRICT stream)
{
	return zephyr_fread(ptr, size, nitems, stream);
}

int fflush(FILE *stream)
{
	(void)stream;

	return 0;
}

int puts(const char *s)
{
	if (fputs(s, stdout) == EOF) {
		return EOF;
	}

	return (fputc('\n', stdout) == EOF) ? EOF : 0;
}

int z_impl_zephyr_write_stdout(const void *buffer, int nbytes)
{
	size_t n;

	if (buffer == NULL || nbytes < 0) {
		return 0;
	}

	n = minimal_console_write((const unsigned char *)buffer, (size_t)nbytes);

	return (int)n;
}
