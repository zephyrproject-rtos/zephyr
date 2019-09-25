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
#include <sys/util.h>
#include <kernel_internal.h>
#include <sys/errno_private.h>
#include <sys/libc-hooks.h>
#include <syscall_handler.h>
#include <app_memory/app_memdomain.h>
#include <init.h>
#include <sys/sem.h>

#define LIBC_BSS	K_APP_BMEM(z_libc_partition)
#define LIBC_DATA	K_APP_DMEM(z_libc_partition)

#if CONFIG_NEWLIB_LIBC_ALIGNED_HEAP_SIZE
K_APPMEM_PARTITION_DEFINE(z_malloc_partition);
#define MALLOC_BSS	K_APP_BMEM(z_malloc_partition)

/* Compiler will throw an error if the provided value isn't a power of two */
MALLOC_BSS static unsigned char __aligned(CONFIG_NEWLIB_LIBC_ALIGNED_HEAP_SIZE)
	heap_base[CONFIG_NEWLIB_LIBC_ALIGNED_HEAP_SIZE];
#define MAX_HEAP_SIZE CONFIG_NEWLIB_LIBC_ALIGNED_HEAP_SIZE

#else /* CONFIG_NEWLIB_LIBC_ALIGNED_HEAP_SIZE */

/* Heap base and size are determined based on the available unused SRAM. */

#define USED_RAM_END_ADDR   POINTER_TO_UINT(&_end)

#if CONFIG_X86
#define USED_RAM_SIZE  (USED_RAM_END_ADDR - DT_PHYS_RAM_ADDR)
#define MAX_HEAP_SIZE ((KB(DT_RAM_SIZE)) - USED_RAM_SIZE)
#elif CONFIG_NIOS2
#include <layout.h>
#define USED_RAM_SIZE  (USED_RAM_END_ADDR - _RAM_ADDR)
#define MAX_HEAP_SIZE (_RAM_SIZE - USED_RAM_SIZE)
#elif CONFIG_RISCV
#include <soc.h>
#define USED_RAM_SIZE  (USED_RAM_END_ADDR - RISCV_RAM_BASE)
#define MAX_HEAP_SIZE  (RISCV_RAM_SIZE - USED_RAM_SIZE)
#elif CONFIG_ARM
#include <soc.h>
#if defined(CONFIG_USERSPACE)
/* MPU shall program the heap area as user-accessible; therefore, heap base
 * (and size) shall take into account the ARM MPU minimum region granularity.
 */
#define HEAP_BASE ((USED_RAM_END_ADDR + \
		CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE) & \
	(~(CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE - 1)))
#else
#define HEAP_BASE USED_RAM_END_ADDR
#endif /* CONFIG_USERSPACE*/
#define USED_RAM_SIZE  (HEAP_BASE - CONFIG_SRAM_BASE_ADDRESS)
#define MAX_HEAP_SIZE ((KB(CONFIG_SRAM_SIZE)) - USED_RAM_SIZE)
#elif CONFIG_XTENSA
extern void *_heap_sentry;
#define MAX_HEAP_SIZE  (POINTER_TO_UINT(&_heap_sentry) - USED_RAM_END_ADDR)
#else
#define USED_RAM_SIZE  (USED_RAM_END_ADDR - CONFIG_SRAM_BASE_ADDRESS)
#define MAX_HEAP_SIZE ((KB(CONFIG_SRAM_SIZE)) - USED_RAM_SIZE)
#endif

#ifndef HEAP_BASE
#define HEAP_BASE USED_RAM_END_ADDR
#endif

#ifdef CONFIG_USERSPACE
struct k_mem_partition z_malloc_partition;

static int malloc_prepare(struct device *unused)
{
	ARG_UNUSED(unused);

#if CONFIG_NEWLIB_LIBC_ALIGNED_HEAP_SIZE
	z_malloc_partition.start = (u32_t)heap_base;
#else
	z_malloc_partition.start = HEAP_BASE;
#endif
	z_malloc_partition.size = MAX_HEAP_SIZE;
	z_malloc_partition.attr = K_MEM_PARTITION_P_RW_U_RW;
	return 0;
}

SYS_INIT(malloc_prepare, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif /* CONFIG_USERSPACE */
#endif /* CONFIG_NEWLIB_LIBC_ALIGNED_HEAP_SIZE */

LIBC_BSS static unsigned int heap_sz;

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

int z_impl_zephyr_read_stdin(char *buf, int nbytes)
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
static inline int z_vrfy_z_zephyr_read_stdin(char *buf, int nbytes)
{
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(buf, nbytes));
	return z_impl_zephyr_read_stdin((char *)buf, nbytes);
}
#include <syscalls/z_zephyr_read_stdin_mrsh.c>
#endif

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
static inline int z_vrfy_z_zephyr_write_stdout(const void *buf, int nbytes)
{
	Z_OOPS(Z_SYSCALL_MEMORY_READ(buf, nbytes));
	return z_impl_zephyr_write_stdout((const void *)buf, nbytes);
}
#include <syscalls/z_zephyr_write_stdout_mrsh.c>
#endif

#ifndef CONFIG_POSIX_API
int _read(int fd, char *buf, int nbytes)
{
	ARG_UNUSED(fd);

	return z_impl_zephyr_read_stdin(buf, nbytes);
}
FUNC_ALIAS(_read, read, int);

int _write(int fd, const void *buf, int nbytes)
{
	ARG_UNUSED(fd);

	return z_impl_zephyr_write_stdout(buf, nbytes);
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
extern ssize_t write(int file, const char *buffer, size_t count);
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

static LIBC_DATA SYS_SEM_DEFINE(heap_sem, 1, 1);

void *_sbrk(int count)
{
	void *ret, *ptr;

	sys_sem_take(&heap_sem, K_FOREVER);

#if CONFIG_NEWLIB_LIBC_ALIGNED_HEAP_SIZE
	ptr = heap_base + heap_sz;
#else
	ptr = ((char *)HEAP_BASE) + heap_sz;
#endif

	if ((heap_sz + count) < MAX_HEAP_SIZE) {
		heap_sz += count;
		ret = ptr;
	} else {
		ret = (void *)-1;
	}

	sys_sem_give(&heap_sem);

	return ret;
}
FUNC_ALIAS(_sbrk, sbrk, void *);

int *__errno(void)
{
	return z_errno();
}
