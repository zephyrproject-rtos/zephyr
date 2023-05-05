/*
 * Copyright © 2021, Keith Packard <keithp@keithp.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/posix/sys/stat.h>
#include <sys/time.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/errno_private.h>
#include <zephyr/sys/libc-hooks.h>
#include <zephyr/syscall_handler.h>
#include <zephyr/app_memory/app_memdomain.h>
#include <zephyr/init.h>
#include <zephyr/sys/sem.h>
#include <zephyr/logging/log.h>
#ifdef CONFIG_MMU
#include <zephyr/sys/mem_manage.h>
#endif

#define LIBC_BSS	K_APP_BMEM(z_libc_partition)
#define LIBC_DATA	K_APP_DMEM(z_libc_partition)

#ifdef CONFIG_MMU

/* When there is an MMU, allocate the heap at startup time */

# if Z_MALLOC_PARTITION_EXISTS
struct k_mem_partition z_malloc_partition;
# endif

static LIBC_BSS unsigned char *heap_base;
static LIBC_BSS size_t max_heap_size;

# define HEAP_BASE		((uintptr_t) heap_base)
# define MAX_HEAP_SIZE		max_heap_size

# define USE_MALLOC_PREPARE	1

#elif CONFIG_PICOLIBC_HEAP_SIZE == 0

/* No heap at all */
# define HEAP_BASE	0
# define MAX_HEAP_SIZE	0

#else /* CONFIG_PICOLIBC_HEAP_SIZE != 0 */

/* Figure out alignment requirement */
# ifdef Z_MALLOC_PARTITION_EXISTS

#  if defined(CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT)
#   if CONFIG_PICOLIBC_HEAP_SIZE < 0
#    error CONFIG_PICOLIBC_HEAP_SIZE must be defined on this target
#   endif
#   if (CONFIG_PICOLIBC_HEAP_SIZE & (CONFIG_PICOLIBC_HEAP_SIZE - 1)) != 0
#    error CONFIG_PICOLIBC_HEAP_SIZE must be power of two on this target
#   endif
#   define HEAP_ALIGN	CONFIG_PICOLIBC_HEAP_SIZE
#  elif defined(CONFIG_ARM) || defined(CONFIG_ARM64)
#   define HEAP_ALIGN	CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE
#  elif defined(CONFIG_ARC)
#   define HEAP_ALIGN	Z_ARC_MPU_ALIGN
#  elif defined(CONFIG_RISCV)
#   define HEAP_ALIGN	Z_RISCV_STACK_GUARD_SIZE
#  else
/*
 * Default to 64-bytes; we'll get a run-time error if this doesn't work.
 */
#   define HEAP_ALIGN	64
#  endif /* CONFIG_<arch> */

# else /* Z_MALLOC_PARTITION_EXISTS */

#  define HEAP_ALIGN	sizeof(double)

# endif /* else Z_MALLOC_PARTITION_EXISTS */

# if CONFIG_PICOLIBC_HEAP_SIZE > 0

/* Static allocation of heap in BSS */

#  ifdef Z_MALLOC_PARTITION_EXISTS
K_APPMEM_PARTITION_DEFINE(z_malloc_partition);
#   define MALLOC_BSS	K_APP_BMEM(z_malloc_partition)
#  else
#   define MALLOC_BSS	__noinit
#  endif

static MALLOC_BSS unsigned char __aligned(HEAP_ALIGN)
	heap_base[CONFIG_PICOLIBC_HEAP_SIZE];

#  define HEAP_BASE	((uintptr_t) heap_base)
#  define MAX_HEAP_SIZE	CONFIG_PICOLIBC_HEAP_SIZE

# else /* CONFIG_PICOLIBC_HEAP_SIZE > 0 */

/*
 * Heap base and size are determined based on the available unused SRAM, in the
 * interval from a properly aligned address after the linker symbol `_end`, to
 * the end of SRAM
 */

#  ifdef Z_MALLOC_PARTITION_EXISTS
/*
 * Need to be able to program a memory protection region from HEAP_BASE to the
 * end of RAM so that user threads can get at it.  Implies that the base address
 * needs to be suitably aligned since the bounds have to go in a
 * k_mem_partition.
 */
struct k_mem_partition z_malloc_partition;

#   define USE_MALLOC_PREPARE	1

#  endif /* Z_MALLOC_PARTITION_EXISTS */

#  define USED_RAM_END_ADDR   POINTER_TO_UINT(&_end)

/*
 * No partition, heap can just start wherever _end is, with
 * suitable alignment
 */

#  define HEAP_BASE	ROUND_UP(USED_RAM_END_ADDR, HEAP_ALIGN)

#  ifdef CONFIG_XTENSA
extern char _heap_sentry[];
#   define MAX_HEAP_SIZE  (POINTER_TO_UINT(_heap_sentry) - HEAP_BASE)
#  else
#   define MAX_HEAP_SIZE	(KB(CONFIG_SRAM_SIZE) - \
			 (HEAP_BASE - CONFIG_SRAM_BASE_ADDRESS))
#  endif /* CONFIG_XTENSA */

# endif /* CONFIG_PICOLIBC_HEAP_SIZE < 0 */

#endif /* CONFIG_PICOLIBC_HEAP_SIZE == 0 */

#ifdef USE_MALLOC_PREPARE

static int malloc_prepare(void)
{

#ifdef CONFIG_MMU

	/* With an MMU, the heap is allocated at runtime */

# if CONFIG_PICOLIBC_HEAP_SIZE < 0
#  define MMU_MAX_HEAP_SIZE PTRDIFF_MAX
# else
#  define MMU_MAX_HEAP_SIZE CONFIG_PICOLIBC_HEAP_SIZE
# endif
	max_heap_size = MIN(MMU_MAX_HEAP_SIZE,
			    k_mem_free_get());

	max_heap_size &= ~(CONFIG_MMU_PAGE_SIZE-1);

	if (max_heap_size != 0) {
		heap_base = k_mem_map(max_heap_size, K_MEM_PERM_RW);
		__ASSERT(heap_base != NULL,
			 "failed to allocate heap of size %zu", max_heap_size);

	}
#endif

#if Z_MALLOC_PARTITION_EXISTS
	z_malloc_partition.start = HEAP_BASE;
	z_malloc_partition.size = MAX_HEAP_SIZE;
	z_malloc_partition.attr = K_MEM_PARTITION_P_RW_U_RW;
#endif
	return 0;
}

SYS_INIT(malloc_prepare, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif /* USE_MALLOC_PREPARE */

static LIBC_BSS uintptr_t heap_sz;

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
#include <syscalls/zephyr_fputc_mrsh.c>
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

int z_impl_zephyr_read_stdin(char *buf, int nbytes)
{
	int i = 0;

	for (i = 0; i < nbytes; i++) {
		*(buf + i) = getchar();
		if ((*(buf + i) == '\n') || (*(buf + i) == '\r')) {
			i++;
			break;
		}
	}
	return i;
}

int z_impl_zephyr_write_stdout(const void *buffer, int nbytes)
{
	const char *buf = buffer;
	int i;

	for (i = 0; i < nbytes; i++) {
		if (*(buf + i) == '\n') {
			putchar('\r');
		}
		putchar(*(buf + i));
	}
	return nbytes;
}

#include <zephyr/sys/cbprintf.h>

struct cb_bits {
	FILE f;
	cbprintf_cb out;
	void	*ctx;
};

static int cbputc(char c, FILE *_s)
{
	struct cb_bits *s = (struct cb_bits *) _s;

	(*s->out) (c, s->ctx);
	return 0;
}

int cbvprintf(cbprintf_cb out, void *ctx, const char *fp, va_list ap)
{
	struct cb_bits	s = {
		.f = FDEV_SETUP_STREAM(cbputc, NULL, NULL, _FDEV_SETUP_WRITE),
		.out = out,
		.ctx = ctx,
	};
	return vfprintf(&s.f, fp, ap);
}

__weak void _exit(int status)
{
	printk("exit\n");
	while (1) {
		;
	}
}

static LIBC_DATA SYS_SEM_DEFINE(heap_sem, 1, 1);

void *sbrk(intptr_t incr)
{
	void *ret = (void *) -1;
	char *brk;
	char *heap_start = (char *) HEAP_BASE;
	char *heap_end = (char *) (HEAP_BASE + MAX_HEAP_SIZE);

	sys_sem_take(&heap_sem, K_FOREVER);

	brk = ((char *)HEAP_BASE) + heap_sz;

	if (incr < 0) {
		if (brk - heap_start < -incr) {
			goto out;
		}
	} else {
		if (heap_end - brk < incr) {
			goto out;
		}
	}

	ret = brk;
	heap_sz += incr;

out:
	/* coverity[CHECKED_RETURN] */
	sys_sem_give(&heap_sem);

	return ret;
}

#ifdef CONFIG_MULTITHREADING
#define _LOCK_T void *
K_MUTEX_DEFINE(__lock___libc_recursive_mutex);

#ifdef CONFIG_USERSPACE
/* Grant public access to picolibc lock after boot */
static int picolibc_locks_prepare(void)
{

	/* Initialise recursive locks */
	k_object_access_all_grant(&__lock___libc_recursive_mutex);

	return 0;
}

SYS_INIT(picolibc_locks_prepare, POST_KERNEL,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif /* CONFIG_USERSPACE */

/* Create a new dynamic non-recursive lock */
void __retarget_lock_init(_LOCK_T *lock)
{
	__ASSERT_NO_MSG(lock != NULL);

	/* Allocate semaphore object */
#ifndef CONFIG_USERSPACE
	*lock = malloc(sizeof(struct k_sem));
#else
	*lock = k_object_alloc(K_OBJ_SEM);
#endif /* !CONFIG_USERSPACE */
	__ASSERT(*lock != NULL, "non-recursive lock allocation failed");

	k_sem_init((struct k_sem *)*lock, 1, 1);
}

/* Create a new dynamic recursive lock */
void __retarget_lock_init_recursive(_LOCK_T *lock)
{
	__ASSERT_NO_MSG(lock != NULL);

	/* Allocate mutex object */
#ifndef CONFIG_USERSPACE
	*lock = malloc(sizeof(struct k_mutex));
#else
	*lock = k_object_alloc(K_OBJ_MUTEX);
#endif /* !CONFIG_USERSPACE */
	__ASSERT(*lock != NULL, "recursive lock allocation failed");

	k_mutex_init((struct k_mutex *)*lock);
}

/* Close dynamic non-recursive lock */
void __retarget_lock_close(_LOCK_T lock)
{
	__ASSERT_NO_MSG(lock != NULL);
#ifndef CONFIG_USERSPACE
	free(lock);
#else
	k_object_release(lock);
#endif /* !CONFIG_USERSPACE */
}

/* Close dynamic recursive lock */
void __retarget_lock_close_recursive(_LOCK_T lock)
{
	__ASSERT_NO_MSG(lock != NULL);
#ifndef CONFIG_USERSPACE
	free(lock);
#else
	k_object_release(lock);
#endif /* !CONFIG_USERSPACE */
}

/* Acquiure non-recursive lock */
void __retarget_lock_acquire(_LOCK_T lock)
{
	__ASSERT_NO_MSG(lock != NULL);
	k_sem_take((struct k_sem *)lock, K_FOREVER);
}

/* Acquiure recursive lock */
void __retarget_lock_acquire_recursive(_LOCK_T lock)
{
	__ASSERT_NO_MSG(lock != NULL);
	k_mutex_lock((struct k_mutex *)lock, K_FOREVER);
}

/* Try acquiring non-recursive lock */
int __retarget_lock_try_acquire(_LOCK_T lock)
{
	__ASSERT_NO_MSG(lock != NULL);
	return !k_sem_take((struct k_sem *)lock, K_NO_WAIT);
}

/* Try acquiring recursive lock */
int __retarget_lock_try_acquire_recursive(_LOCK_T lock)
{
	__ASSERT_NO_MSG(lock != NULL);
	return !k_mutex_lock((struct k_mutex *)lock, K_NO_WAIT);
}

/* Release non-recursive lock */
void __retarget_lock_release(_LOCK_T lock)
{
	__ASSERT_NO_MSG(lock != NULL);
	k_sem_give((struct k_sem *)lock);
}

/* Release recursive lock */
void __retarget_lock_release_recursive(_LOCK_T lock)
{
	__ASSERT_NO_MSG(lock != NULL);
	k_mutex_unlock((struct k_mutex *)lock);
}

#endif /* CONFIG_MULTITHREADING */

/* This function gets called if static buffer overflow detection is enabled on
 * stdlib side (Picolibc here), in case such an overflow is detected. Picolibc
 * provides an implementation not suitable for us, so we override it here.
 */
__weak FUNC_NORETURN void __chk_fail(void)
{
	printk("* buffer overflow detected *\n");
	z_except_reason(K_ERR_STACK_CHK_FAIL);
	CODE_UNREACHABLE;
}

#ifndef CONFIG_LIBC_ERRNO

/*
 * Picolibc needs to be able to declare this itself so that the library
 * doesn't end up needing zephyr header files. That means using a regular
 * function instead of an inline.
 */
int *z_errno_wrap(void)
{
	return z_errno();
}

#endif
