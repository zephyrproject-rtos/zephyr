/* stdout_console.c */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/sys/libc-hooks.h>
#include <zephyr/syscall_handler.h>
#include <string.h>

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

int z_impl_zephyr_fputc(int c, FILE *stream)
{
	return (stream == stdout || stream == stderr) ? _stdout_hook(c) : EOF;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_zephyr_fputc(int c, FILE *stream)
{
	return z_impl_zephyr_fputc(c, stream);
}
#include <syscalls/zephyr_fputc_mrsh.c>
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

	return len == ret ? 0 : EOF;
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

	if ((stream != stdout && stream != stderr) ||
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

	Z_OOPS(Z_SYSCALL_MEMORY_ARRAY_READ(ptr, nitems, size));
	return z_impl_zephyr_fwrite((const void *ZRESTRICT)ptr, size,
				    nitems, (FILE *ZRESTRICT)stream);
}
#include <syscalls/zephyr_fwrite_mrsh.c>
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

	return fputc('\n', stdout) == EOF ? EOF : 0;
}
