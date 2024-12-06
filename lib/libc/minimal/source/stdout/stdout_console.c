/* stdout_console.c */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/sys/libc-hooks.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/internal/syscall_handler.h>
#include <sys/_stdvtables.h>
#include <stdlib.h>

#include <string.h>

// Defined in stdio
FILE* _stdin;
FILE* _stdout;
FILE* _stderr;

int stdin_fd = 0;
int stdout_fd = 1;
int stderr_fd = 2;

// Alternative to setting FIle to int if fdtable is not used. Good for memory eh?
// #if !defined(CONFIG_FDTABLE)
// 	static struct fd_entry stdin_simple_entry = {.obj = (void*)&stdin_fd, .vtable = NULL};
//     static struct fd_entry stdout_simple_entry = {.obj = (void*)&stdout_fd, .vtable = NULL};
//     static struct fd_entry stderr_simple_entry = {.obj = (void*)&stderr_fd, .vtable = NULL};
// #endif

static int _stdout_hook_default(int c)
{
	(void)(c);  /* Prevent warning about unused argument */

	return EOF;
}

static int (*_stdout_hook)(int c) = _stdout_hook_default;

void __stdout_hook_install(int (*hook)(int c))
{
#if defined(CONFIG_FDTABLE)
	_stdin = zvfs_get_fd_entry(stdin_fd);
	_stdout = zvfs_get_fd_entry(stdout_fd);
	_stderr = zvfs_get_fd_entry(stderr_fd);
#else
	// fdtable not availabe fallback to primitive read and write.
    _stdin = &stdin_fd;
    _stdout = &stdout_fd;
    _stderr = &stderr_fd;
#endif

	_stdout_hook = hook;
}

#if defined(CONFIG_FDTABLE)

static ssize_t stdin_read_vmeth(void *obj, void *buffer, size_t count)
{
	return fputc('c', stdout);
}

static ssize_t stdout_write_vmeth(void *obj, const void *buffer, size_t count)
{
	int c = *((int *) buffer);
	return _stdout_hook(c);
}

static ssize_t stderr_write_vmeth(void *obj, const void *buffer, size_t count)
{
	return 10;
}

const struct fd_op_vtable stdin_fd_op_vtable = {
	.read = stdin_read_vmeth,
};

const struct fd_op_vtable stdout_fd_op_vtable = {
	.write = stdout_write_vmeth,
};

const struct fd_op_vtable stderr_fd_op_vtable = {
	.write = stderr_write_vmeth,
};

#endif

int z_impl_zephyr_fputc(int c, FILE *stream)
{
#if defined(CONFIG_FDTABLE)
	if(stream->vtable != NULL) {
		return stream->vtable->write(stream->obj, &c, 1);
	}
#else
	// usage of std streams without fdtable
	if(stream != NULL && (stream == stdout || stream == stderr))
		return _stdout_hook(c);
#endif
	// EOF for any other cases
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
	int len = strlen(s);
	int ret;

	ret = fwrite(s, 1, len, stream);

	return (len == ret) ? 0 : EOF;
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
	size_t i;
	size_t j;
	const unsigned char *p;

	if (((stream != stdout) && (stream != stderr)) ||
	    (nitems == 0) || (size == 0)) {
		return 0;
	}

	p = ptr;
	i = nitems;
	do {
		j = size;
		do {
			if (_stdout_hook((int) *p++) == EOF) {
				goto done;
			}
			j--;
		} while (j > 0);

		i--;
	} while (i > 0);

done:
	return (nitems - i);
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


int puts(const char *s)
{
	if (fputs(s, stdout) == EOF) {
		return EOF;
	}

	return (fputc('\n', stdout) == EOF) ? EOF : 0;
}
