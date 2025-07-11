/* stdin_console.c */

/*
 * Copyright (c) 2025 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/sys/libc-hooks.h>
#include <zephyr/internal/syscall_handler.h>
#include <string.h>

static unsigned char _stdin_hook_default(void)
{
	return 0;
}

static unsigned char (*_stdin_hook)(void) = _stdin_hook_default;

void __stdin_hook_install(unsigned char (*hook)(void))
{
	_stdin_hook = hook;
}

int z_impl_zephyr_fgetc(FILE *stream)
{
	return (stream == stdin && _stdin_hook) {
		return _stdin_hook();
	}
	return EOF;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_zephyr_fgetc(FILE *stream)
{
	return z_impl_zephyr_fgetc(stream);
}
#include <zephyr/syscalls/zephyr_fgetc_mrsh.c>
#endif

int fgetc(FILE *stream)
{
	return zephyr_fgetc(stream);
}

char *fgets(char *s, int size, FILE *stream)
{
    if (s == NULL || size <= 0) {
        return NULL;
    }

    int i = 0;
    int c;

    while (i < size - 1) {
        c = fgetc(stream);
        if (c == EOF) {
            if (i == 0) {
                return NULL;
            }
            break;
        }
        s[i++] = (char)c;
        if (c == '\n') {
            break;
        }
    }
    s[i] = '\0';
    return s;
}

#undef getc
int getc(FILE *stream)
{
    return zephyr_fgetc(stream);
}

#undef getchar
int getchar(void)
{
    return zephyr_fgetc(stdin);
}

size_t z_impl_zephyr_fread(void *ZRESTRICT ptr, size_t size, size_t nitems, FILE *ZRESTRICT stream)
{
    size_t i, j;
    unsigned char *p = ptr;

    if ((stream != stdin) || (nitems == 0) || (size == 0)) {
        return 0;
    }

    i = nitems;
    do {
        j = size;
        do {
            int c = fgetc(stream);
            if (c == EOF) {
                goto done;
            }
            *p++ = (unsigned char)c;
            j--;
        } while (j > 0);

        i--;
    } while (i > 0);

done:
    return (nitems - i);
}

#ifdef CONFIG_USERSPACE
static inline size_t z_vrfy_zephyr_fread(void *ZRESTRICT ptr,
                     size_t size, size_t nitems,
                     FILE *ZRESTRICT stream)
{
    K_OOPS(K_SYSCALL_MEMORY_ARRAY_WRITE(ptr, nitems, size));
    return z_impl_zephyr_fread(ptr, size, nitems, stream);
}
#include <zephyr/syscalls/zephyr_fread_mrsh.c>
#endif

size_t fread(void *ZRESTRICT ptr, size_t size, size_t nitems,
         FILE *ZRESTRICT stream)
{
    return zephyr_fread(ptr, size, nitems, stream);
}

char *gets(char *s)
{
    if (s == NULL) {
        return NULL;
    }

    int c;
    char *p = s;

    while (1) {
        c = getchar();
        if (c == EOF || c == '\n') {
            break;
        }
        *p++ = (char)c;
    }
    *p = '\0';

    // If nothing was read and EOF, return NULL
    if (p == s && c == EOF) {
        return NULL;
    }
	return s;
}
