/*
 * Copyright (c) 2015, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <arch/cpu.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <linker/linker-defs.h>
#include <misc/util.h>
#include <kernel_internal.h>
#include <misc/errno_private.h>
#include <misc/libc-hooks.h>
#include <syscall_handler.h>

#define USED_RAM_END_ADDR   POINTER_TO_UINT(&_end)

#if CONFIG_NEWLIB_LIBC_ALIGNED_HEAP_SIZE
/* Compiler will throw an error if the provided value isn't a power of two */
static unsigned char __kernel __aligned(CONFIG_NEWLIB_LIBC_ALIGNED_HEAP_SIZE)
	heap_base[CONFIG_NEWLIB_LIBC_ALIGNED_HEAP_SIZE];
#define MAX_HEAP_SIZE CONFIG_NEWLIB_LIBC_ALIGNED_HEAP_SIZE
#else

#if CONFIG_X86
#define USED_RAM_SIZE  (USED_RAM_END_ADDR - CONFIG_PHYS_RAM_ADDR)
#define MAX_HEAP_SIZE ((KB(CONFIG_RAM_SIZE)) - USED_RAM_SIZE)
#elif CONFIG_NIOS2
#include <layout.h>
#define USED_RAM_SIZE  (USED_RAM_END_ADDR - _RAM_ADDR)
#define MAX_HEAP_SIZE (_RAM_SIZE - USED_RAM_SIZE)
#elif CONFIG_RISCV32
#include <soc.h>
#define USED_RAM_SIZE  (USED_RAM_END_ADDR - RISCV_RAM_BASE)
#define MAX_HEAP_SIZE  (RISCV_RAM_SIZE - USED_RAM_SIZE)
#elif CONFIG_ARM
#include <soc.h>
#define USED_RAM_SIZE  (USED_RAM_END_ADDR - CONFIG_SRAM_BASE_ADDRESS)
#define MAX_HEAP_SIZE ((KB(CONFIG_SRAM_SIZE)) - USED_RAM_SIZE)
#elif CONFIG_XTENSA
extern void *_heap_sentry;
#define MAX_HEAP_SIZE  (POINTER_TO_UINT(&_heap_sentry) - USED_RAM_END_ADDR)
#else
#define USED_RAM_SIZE  (USED_RAM_END_ADDR - CONFIG_SRAM_BASE_ADDRESS)
#define MAX_HEAP_SIZE ((KB(CONFIG_SRAM_SIZE)) - USED_RAM_SIZE)
#endif

static unsigned char *heap_base = UINT_TO_POINTER(USED_RAM_END_ADDR);
#endif /* CONFIG_NEWLIB_LIBC_ALIGNED_HEAP_SIZE */

static unsigned int heap_sz;

static int _stdout_hook_default(int c)
{
	(void)(c);  /* Prevent warning about unused argument */

	return EOF;
}

static int (*_stdout_hook)(int) = _stdout_hook_default;

void __stdout_hook_install(int (*hook)(int))
{
	_stdout_hook = hook;
}

static unsigned char _stdin_hook_default(void)
{
	return 0;
}

static unsigned char (*_stdin_hook)(void) = _stdin_hook_default;

void __stdin_hook_install(unsigned char (*hook)(void))
{
	_stdin_hook = hook;
}

int _impl__zephyr_read(char *buf, int nbytes)
{
	int i = 0;

	for (i = 0; i < nbytes; i++) {
		*(buf + i) = _stdin_hook();
		if ((*(buf + i) == '\n') || (*(buf + i) == '\r')) {
			i++;
			break;
		}
	}
	return i;
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER(_zephyr_read, buf, nbytes)
{
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(buf, nbytes));
	return _impl__zephyr_read((char *)buf, nbytes);
}
#endif

int _impl__zephyr_write(char *buf, int nbytes)
{
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
Z_SYSCALL_HANDLER(_zephyr_write, buf, nbytes)
{
	Z_OOPS(Z_SYSCALL_MEMORY_READ(buf, nbytes));
	return _impl__zephyr_write((char *)buf, nbytes);
}
#endif

#ifndef CONFIG_POSIX_FS
int _read(int fd, char *buf, int nbytes)
{
	ARG_UNUSED(fd);

	return _zephyr_read(buf, nbytes);
}
FUNC_ALIAS(_read, read, int);

int _write(int fd, char *buf, int nbytes)
{
	ARG_UNUSED(fd);

	return _zephyr_write(buf, nbytes);
}
FUNC_ALIAS(_write, write, int);

int _open(const char *name, int mode)
{
	return -1;
}
FUNC_ALIAS(_open, open, int);

int _close(int file)
{
	return -1;
}
FUNC_ALIAS(_close, close, int);

int _lseek(int file, int ptr, int dir)
{
	return 0;
}
FUNC_ALIAS(_lseek, lseek, int);
#else
extern ssize_t write(int file, char *buffer, unsigned int count);
#define _write	write
#endif

int _isatty(int file)
{
	return 1;
}
FUNC_ALIAS(_isatty, isatty, int);

int _kill(int i, int j)
{
	return 0;
}
FUNC_ALIAS(_kill, kill, int);

int _getpid(void)
{
	return 0;
}
FUNC_ALIAS(_getpid, getpid, int);

int _fstat(int file, struct stat *st)
{
	st->st_mode = S_IFCHR;
	return 0;
}
FUNC_ALIAS(_fstat, fstat, int);

void _exit(int status)
{
	_write(1, "exit\n", 5);
	while (1) {
		;
	}
}

void *_sbrk(int count)
{
	void *ptr = heap_base + heap_sz;

	if ((heap_sz + count) < MAX_HEAP_SIZE) {
		heap_sz += count;
		return ptr;
	} else {
		return (void *)-1;
	}
}
FUNC_ALIAS(_sbrk, sbrk, void *);

void z_newlib_get_heap_bounds(void **base, size_t *size)
{
	*base = heap_base;
	*size = MAX_HEAP_SIZE;
}

int *__errno(void)
{
	return z_errno();
}
