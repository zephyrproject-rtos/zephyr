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
/* Heap base and size are determined based on the available unused SRAM,
 * in the interval from a properly aligned address after the linker symbol
 * `_end`, to the end of SRAM
 */
#define USED_RAM_END_ADDR   POINTER_TO_UINT(&_end)

#ifdef Z_MALLOC_PARTITION_EXISTS
/* Need to be able to program a memory protection region from HEAP_BASE
 * to the end of RAM so that user threads can get at it.
 * Implies that the base address needs to be suitably aligned since the
 * bounds have to go in a k_mem_partition.
 */
#ifdef CONFIG_MMU
/* Linker script may already have done this, but just to be safe */
#define HEAP_BASE	ROUND_UP(USED_RAM_END_ADDR, CONFIG_MMU_PAGE_SIZE)
#else /* MPU-based systems */
/* TODO: Need a generic Kconfig for the MPU region granularity */
#if defined(CONFIG_ARM)
#define HEAP_BASE	ROUND_UP(USED_RAM_END_ADDR, \
				 CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE)
#elif defined(CONFIG_ARC)
#define HEAP_BASE	ROUND_UP(USED_RAM_END_ADDR, Z_ARC_MPU_ALIGN)
#else
#error "Unsupported platform"
#endif /* CONFIG_<arch> */
#endif /* !CONFIG_MMU */
#else /* !Z_MALLOC_PARTITION_EXISTS */
/* No partition, heap can just start wherever _end is */
#define HEAP_BASE	USED_RAM_END_ADDR
#endif /* Z_MALLOC_PARTITION_EXISTS */

#ifdef CONFIG_MMU
/* Currently a placeholder, we're just setting up all unused RAM pages
 * past the end of the kernel as the heap arena. SRAM_BASE_ADDRESS is
 * a physical address so we can't use that.
 *
 * Later, we should do this much more like other VM-enabled operating systems:
 * - Set up a core kernel ontology of free physical pages
 * - Allow anonymous memoory mappings drawing from the pool of free physical
 *   pages
 * - Have _sbrk() map anonymous pages into the heap arena as needed
 * - See: https://github.com/zephyrproject-rtos/zephyr/issues/29526
 */
#define MAX_HEAP_SIZE	(CONFIG_KERNEL_RAM_SIZE - (HEAP_BASE - \
						   CONFIG_KERNEL_VM_BASE))
#elif defined(CONFIG_XTENSA)
extern void *_heap_sentry;
#define MAX_HEAP_SIZE	(POINTER_TO_UINT(&_heap_sentry) - HEAP_BASE)
#else
#define MAX_HEAP_SIZE	(KB(CONFIG_SRAM_SIZE) - (HEAP_BASE - \
						 CONFIG_SRAM_BASE_ADDRESS))
#endif /* CONFIG_MMU */

#if Z_MALLOC_PARTITION_EXISTS
struct k_mem_partition z_malloc_partition;

static int malloc_prepare(const struct device *unused)
{
	ARG_UNUSED(unused);

	z_malloc_partition.start = HEAP_BASE;
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

void *_sbrk(int count)
{
	void *ret, *ptr;

	/* coverity[CHECKED_RETURN] */
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
