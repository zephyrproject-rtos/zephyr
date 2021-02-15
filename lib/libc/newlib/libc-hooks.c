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
#include <sys/errno_private.h>
#include <sys/libc-hooks.h>
#include <syscall_handler.h>
#include <app_memory/app_memdomain.h>
#include <init.h>
#include <sys/sem.h>
#include <sys/mem_manage.h>

#define LIBC_BSS	K_APP_BMEM(z_libc_partition)
#define LIBC_DATA	K_APP_DMEM(z_libc_partition)

/*
 * End result of this thorny set of ifdefs is to define:
 *
 * - HEAP_BASE base address of the heap arena
 * - MAX_HEAP_SIZE size of the heap arena
 */

#ifdef CONFIG_MMU
	#ifdef CONFIG_USERSPACE
		struct k_mem_partition z_malloc_partition;
	#endif

	LIBC_BSS static unsigned char *heap_base;
	LIBC_BSS static size_t max_heap_size;

	#define HEAP_BASE		heap_base
	#define MAX_HEAP_SIZE		max_heap_size
	#define USE_MALLOC_PREPARE	1
#elif CONFIG_NEWLIB_LIBC_ALIGNED_HEAP_SIZE
	/* Arena size expressed in Kconfig, due to power-of-two size/align
	 * requirements of certain MPUs.
	 *
	 * We use an automatic memory partition instead of setting this up
	 * in malloc_prepare().
	 */
	K_APPMEM_PARTITION_DEFINE(z_malloc_partition);
	#define MALLOC_BSS	K_APP_BMEM(z_malloc_partition)

	/* Compiler will throw an error if the provided value isn't a
	 * power of two
	 */
	MALLOC_BSS static unsigned char
		__aligned(CONFIG_NEWLIB_LIBC_ALIGNED_HEAP_SIZE)
		heap_base[CONFIG_NEWLIB_LIBC_ALIGNED_HEAP_SIZE];
	#define MAX_HEAP_SIZE CONFIG_NEWLIB_LIBC_ALIGNED_HEAP_SIZE
	#define HEAP_BASE heap_base
#else /* Not MMU or CONFIG_NEWLIB_LIBC_ALIGNED_HEAP_SIZE */
	#define USED_RAM_END_ADDR   POINTER_TO_UINT(&_end)

	#ifdef Z_MALLOC_PARTITION_EXISTS
		/* Start of malloc arena needs to be aligned per MPU
		 * requirements
		 */
		struct k_mem_partition z_malloc_partition;

		#if defined(CONFIG_ARM)
			#define HEAP_BASE	ROUND_UP(USED_RAM_END_ADDR, \
				 CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE)
		#elif defined(CONFIG_ARC)
			#define HEAP_BASE	ROUND_UP(USED_RAM_END_ADDR, \
							  Z_ARC_MPU_ALIGN)
		#else
			#error "Unsupported platform"
		#endif /* CONFIG_<arch> */
		#define USE_MALLOC_PREPARE	1
	#else
		/* End of kernel image */
		#define HEAP_BASE		USED_RAM_END_ADDR
	#endif

	/* End of the malloc arena is the end of physical memory */
	#if defined(CONFIG_XTENSA)
		/* TODO: Why is xtensa a special case? */
		extern void *_heap_sentry;
		#define MAX_HEAP_SIZE	(POINTER_TO_UINT(&_heap_sentry) - \
					 HEAP_BASE)
	#else
		#define MAX_HEAP_SIZE	(KB(CONFIG_SRAM_SIZE) - (HEAP_BASE - \
					 CONFIG_SRAM_BASE_ADDRESS))
	#endif /* CONFIG_XTENSA */
#endif

#ifdef USE_MALLOC_PREPARE
static int malloc_prepare(const struct device *unused)
{
	ARG_UNUSED(unused);

#ifdef CONFIG_MMU
	max_heap_size = MIN(CONFIG_NEWLIB_LIBC_MAX_MAPPED_REGION_SIZE,
			    k_mem_free_get());

	if (max_heap_size != 0) {
		heap_base = k_mem_map(max_heap_size, K_MEM_PERM_RW);
		__ASSERT(heap_base != NULL,
			 "failed to allocate heap of size %zu", max_heap_size);

	}
#endif /* CONFIG_MMU */
#ifdef Z_MALLOC_PARTITION_EXISTS
	z_malloc_partition.start = (uintptr_t)HEAP_BASE;
	z_malloc_partition.size = (size_t)MAX_HEAP_SIZE;
	z_malloc_partition.attr = K_MEM_PARTITION_P_RW_U_RW;
#endif /* Z_MALLOC_PARTITION_EXISTS */
	return 0;
}

SYS_INIT(malloc_prepare, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif /* USE_MALLOC_PREPARE */

/* Current offset from HEAP_BASE of unused memory */
LIBC_BSS static size_t heap_sz;

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
__weak FUNC_ALIAS(_read, read, int);

int _write(int fd, const void *buf, int nbytes)
{
	ARG_UNUSED(fd);

	return z_impl_zephyr_write_stdout(buf, nbytes);
}
__weak FUNC_ALIAS(_write, write, int);

int _open(const char *name, int mode)
{
	return -1;
}
__weak FUNC_ALIAS(_open, open, int);

int _close(int file)
{
	return -1;
}
__weak FUNC_ALIAS(_close, close, int);

int _lseek(int file, int ptr, int dir)
{
	return 0;
}
__weak FUNC_ALIAS(_lseek, lseek, int);
#else
extern ssize_t write(int file, const char *buffer, size_t count);
#define _write	write
#endif

int _isatty(int file)
{
	return file <= 2;
}
__weak FUNC_ALIAS(_isatty, isatty, int);

int _kill(int i, int j)
{
	return 0;
}
__weak FUNC_ALIAS(_kill, kill, int);

int _getpid(void)
{
	return 0;
}
__weak FUNC_ALIAS(_getpid, getpid, int);

int _fstat(int file, struct stat *st)
{
	st->st_mode = S_IFCHR;
	return 0;
}
__weak FUNC_ALIAS(_fstat, fstat, int);

__weak void _exit(int status)
{
	_write(1, "exit\n", 5);
	while (1) {
		;
	}
}

static LIBC_DATA SYS_SEM_DEFINE(heap_sem, 1, 1);

void *_sbrk(intptr_t count)
{
	void *ret, *ptr;

	/* coverity[CHECKED_RETURN] */
	sys_sem_take(&heap_sem, K_FOREVER);
	ptr = ((char *)HEAP_BASE) + heap_sz;

	if ((heap_sz + count) < MAX_HEAP_SIZE) {
		heap_sz += count;
		ret = ptr;
	} else {
		ret = (void *)-1;
	}

	/* coverity[CHECKED_RETURN] */
	sys_sem_give(&heap_sem);

	return ret;
}
__weak FUNC_ALIAS(_sbrk, sbrk, void *);

__weak int *__errno(void)
{
	return z_errno();
}

/* This function gets called if static buffer overflow detection is enabled
 * on stdlib side (Newlib here), in case such an overflow is detected. Newlib
 * provides an implementation not suitable for us, so we override it here.
 */
__weak FUNC_NORETURN void __chk_fail(void)
{
	static const char chk_fail_msg[] = "* buffer overflow detected *\n";
	_write(2, chk_fail_msg, sizeof(chk_fail_msg) - 1);
	k_oops();
	CODE_UNREACHABLE;
}

#if CONFIG_XTENSA
extern int _read(int fd, char *buf, int nbytes);
extern int _open(const char *name, int mode);
extern int _close(int file);
extern int _lseek(int file, int ptr, int dir);

/* The Newlib in xtensa toolchain has a few missing functions for the
 * reentrant versions of the syscalls.
 */
_ssize_t _read_r(struct _reent *r, int fd, void *buf, size_t nbytes)
{
	ARG_UNUSED(r);

	return _read(fd, (char *)buf, nbytes);
}

_ssize_t _write_r(struct _reent *r, int fd, const void *buf, size_t nbytes)
{
	ARG_UNUSED(r);

	return _write(fd, buf, nbytes);
}

int _open_r(struct _reent *r, const char *name, int flags, int mode)
{
	ARG_UNUSED(r);
	ARG_UNUSED(flags);

	return _open(name, mode);
}

int _close_r(struct _reent *r, int file)
{
	ARG_UNUSED(r);

	return _close(file);
}

_off_t _lseek_r(struct _reent *r, int file, _off_t ptr, int dir)
{
	ARG_UNUSED(r);

	return _lseek(file, ptr, dir);
}

int _isatty_r(struct _reent *r, int file)
{
	ARG_UNUSED(r);

	return _isatty(file);
}

int _kill_r(struct _reent *r, int i, int j)
{
	ARG_UNUSED(r);

	return _kill(i, j);
}

int _getpid_r(struct _reent *r)
{
	ARG_UNUSED(r);

	return _getpid();
}

int _fstat_r(struct _reent *r, int file, struct stat *st)
{
	ARG_UNUSED(r);

	return _fstat(file, st);
}

void _exit_r(struct _reent *r, int status)
{
	ARG_UNUSED(r);

	_exit(status);
}

void *_sbrk_r(struct _reent *r, int count)
{
	ARG_UNUSED(r);

	return _sbrk(count);
}
#endif /* CONFIG_XTENSA */

struct timeval;

int _gettimeofday(struct timeval *__tp, void *__tzp)
{
	ARG_UNUSED(__tp);
	ARG_UNUSED(__tzp);

	return -1;
}
