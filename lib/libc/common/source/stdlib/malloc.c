/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <errno.h>
#include <zephyr/sys/math_extras.h>
#include <string.h>
#include <zephyr/app_memory/app_memdomain.h>
#ifdef CONFIG_MULTITHREADING
#include <zephyr/sys/mutex.h>
#endif
#include <zephyr/sys/sys_heap.h>
#include <zephyr/sys/libc-hooks.h>
#include <zephyr/types.h>
#ifdef CONFIG_MMU
#include <zephyr/sys/mem_manage.h>
#endif

#define LOG_LEVEL CONFIG_KERNEL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

#ifdef CONFIG_COMMON_LIBC_MALLOC

#if (CONFIG_COMMON_LIBC_MALLOC_ARENA_SIZE != 0)

/* Figure out where the malloc variables live */
# if Z_MALLOC_PARTITION_EXISTS
K_APPMEM_PARTITION_DEFINE(z_malloc_partition);
#  define POOL_SECTION Z_GENERIC_SECTION(K_APP_DMEM_SECTION(z_malloc_partition))
# else
#  define POOL_SECTION __noinit
# endif /* CONFIG_USERSPACE */

# if defined(CONFIG_MMU) && CONFIG_COMMON_LIBC_MALLOC_ARENA_SIZE < 0
#  define ALLOCATE_HEAP_AT_STARTUP
# endif

# ifndef ALLOCATE_HEAP_AT_STARTUP

/* Figure out alignment requirement */
#  ifdef Z_MALLOC_PARTITION_EXISTS

#   ifdef CONFIG_MMU
#    define HEAP_ALIGN CONFIG_MMU_PAGE_SIZE
#   elif defined(CONFIG_MPU)
#    if defined(CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT) && \
	(CONFIG_COMMON_LIBC_MALLOC_ARENA_SIZE > 0)
#     if (CONFIG_COMMON_LIBC_MALLOC_ARENA_SIZE & (CONFIG_COMMON_LIBC_MALLOC_ARENA_SIZE - 1)) != 0
#      error CONFIG_COMMON_LIBC_MALLOC_ARENA_SIZE must be power of two on this target
#     endif
#     define HEAP_ALIGN	CONFIG_COMMON_LIBC_MALLOC_ARENA_SIZE
#    elif defined(CONFIG_ARM) || defined(CONFIG_ARM64)
#     define HEAP_ALIGN	MAX(sizeof(double), CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE)
#    elif defined(CONFIG_ARC)
#     define HEAP_ALIGN	MAX(sizeof(double), Z_ARC_MPU_ALIGN)
#    elif defined(CONFIG_RISCV)
#     define HEAP_ALIGN	Z_POW2_CEIL(MAX(sizeof(double), Z_RISCV_STACK_GUARD_SIZE))
#    else
/* Default to 64-bytes; we'll get a run-time error if this doesn't work. */
#     define HEAP_ALIGN	64
#    endif /* CONFIG_<arch> */
#   endif /* elif CONFIG_MPU */

#  endif /* else Z_MALLOC_PARTITION_EXISTS */

#  ifndef HEAP_ALIGN
#   define HEAP_ALIGN	sizeof(double)
#  endif

#  if CONFIG_COMMON_LIBC_MALLOC_ARENA_SIZE > 0

#  define HEAP_STATIC

/* Static allocation of heap in BSS */

#   define HEAP_SIZE	ROUND_UP(CONFIG_COMMON_LIBC_MALLOC_ARENA_SIZE, HEAP_ALIGN)
#   define HEAP_BASE	POINTER_TO_UINT(malloc_arena)

static POOL_SECTION unsigned char __aligned(HEAP_ALIGN) malloc_arena[HEAP_SIZE];

#  else /* CONFIG_COMMON_LIBC_MALLOC_ARENA_SIZE > 0 */

/*
 * Heap base and size are determined based on the available unused SRAM, in the
 * interval from a properly aligned address after the linker symbol `_end`, to
 * the end of SRAM
 */

#   define USED_RAM_END_ADDR   POINTER_TO_UINT(&_end)

/*
 * No partition, heap can just start wherever _end is, with
 * suitable alignment
 */

#   define HEAP_BASE	ROUND_UP(USED_RAM_END_ADDR, HEAP_ALIGN)

#   if defined(CONFIG_XTENSA) && (defined(CONFIG_SOC_FAMILY_INTEL_ADSP) \
	|| defined(CONFIG_HAS_ESPRESSIF_HAL))
extern char _heap_sentry[];
#    define HEAP_SIZE  ROUND_DOWN((POINTER_TO_UINT(_heap_sentry) - HEAP_BASE), HEAP_ALIGN)
#   else
#    define HEAP_SIZE	ROUND_DOWN((KB((size_t) CONFIG_SRAM_SIZE) -	\
		((size_t) HEAP_BASE - (size_t) CONFIG_SRAM_BASE_ADDRESS)), HEAP_ALIGN)
#   endif /* else CONFIG_XTENSA */

#  endif /* else CONFIG_COMMON_LIBC_MALLOC_ARENA_SIZE > 0 */

# endif /* else ALLOCATE_HEAP_AT_STARTUP */

Z_LIBC_DATA static struct sys_heap z_malloc_heap;

#ifdef CONFIG_MULTITHREADING
Z_LIBC_DATA SYS_MUTEX_DEFINE(z_malloc_heap_mutex);

static inline void
malloc_lock(void) {
	int lock_ret;

	lock_ret = sys_mutex_lock(&z_malloc_heap_mutex, K_FOREVER);
	__ASSERT_NO_MSG(lock_ret == 0);
}

static inline void
malloc_unlock(void)
{
	(void) sys_mutex_unlock(&z_malloc_heap_mutex);
}
#else
#define malloc_lock()
#define malloc_unlock()
#endif

void *malloc(size_t size)
{
	malloc_lock();

	void *ret = sys_heap_aligned_alloc(&z_malloc_heap,
					   __alignof__(z_max_align_t),
					   size);
	if (ret == NULL && size != 0) {
		errno = ENOMEM;
	}

	malloc_unlock();

	return ret;
}

void *aligned_alloc(size_t alignment, size_t size)
{
	malloc_lock();

	void *ret = sys_heap_aligned_alloc(&z_malloc_heap,
					   alignment,
					   size);
	if (ret == NULL && size != 0) {
		errno = ENOMEM;
	}

	malloc_unlock();

	return ret;
}

#ifdef CONFIG_GLIBCXX_LIBCPP

/*
 * GCC's libstdc++ may use this function instead of aligned_alloc due to a
 * bug in the configuration for "newlib" environments (which includes picolibc).
 * When toolchains including that bug fix can become a dependency for Zephyr,
 * this work-around can be removed.
 *
 * Note that aligned_alloc isn't defined to work as a replacement for
 * memalign as it requires that the size be a multiple of the alignment,
 * while memalign does not. However, the aligned_alloc implementation here
 * is just a wrapper around sys_heap_aligned_alloc which doesn't have that
 * requirement and so can be used by memalign.
 */

void *memalign(size_t alignment, size_t size)
{
	return aligned_alloc(alignment, size);
}
#endif

static int malloc_prepare(void)
{
	void *heap_base = NULL;
	size_t heap_size;

#ifdef ALLOCATE_HEAP_AT_STARTUP
	heap_size = k_mem_free_get();

	if (heap_size != 0) {
		heap_base = k_mem_map(heap_size, K_MEM_PERM_RW);
		__ASSERT(heap_base != NULL,
			 "failed to allocate heap of size %zu", heap_size);

	}
#elif defined(Z_MALLOC_PARTITION_EXISTS) && \
	defined(CONFIG_MPU) && \
	defined(CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT)

	/* Align size to power of two */
	heap_size = 1;
	while (heap_size * 2 <= HEAP_SIZE)
		heap_size *= 2;

	/* Search for an aligned heap that fits within the available space */
	while (heap_size >= HEAP_ALIGN) {
		heap_base = UINT_TO_POINTER(ROUND_UP(HEAP_BASE, heap_size));
		if (POINTER_TO_UINT(heap_base) + heap_size <= HEAP_BASE + HEAP_SIZE)
			break;
		heap_size >>= 1;
	}
#else
	heap_base = UINT_TO_POINTER(HEAP_BASE);
	heap_size = HEAP_SIZE;
#endif

#if Z_MALLOC_PARTITION_EXISTS && !defined(HEAP_STATIC)
	z_malloc_partition.start = POINTER_TO_UINT(heap_base);
	z_malloc_partition.size = heap_size;
	z_malloc_partition.attr = K_MEM_PARTITION_P_RW_U_RW;
#endif

	sys_heap_init(&z_malloc_heap, heap_base, heap_size);

	return 0;
}

void *realloc(void *ptr, size_t requested_size)
{
	malloc_lock();

	void *ret = sys_heap_aligned_realloc(&z_malloc_heap, ptr,
					     __alignof__(z_max_align_t),
					     requested_size);

	if (ret == NULL && requested_size != 0) {
		errno = ENOMEM;
	}

	malloc_unlock();

	return ret;
}

void free(void *ptr)
{
	malloc_lock();
	sys_heap_free(&z_malloc_heap, ptr);
	malloc_unlock();
}

SYS_INIT(malloc_prepare, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#else /* No malloc arena */
void *malloc(size_t size)
{
	ARG_UNUSED(size);

	LOG_ERR("CONFIG_COMMON_LIBC_MALLOC_ARENA_SIZE is 0");
	errno = ENOMEM;

	return NULL;
}

void free(void *ptr)
{
	ARG_UNUSED(ptr);
}

void *realloc(void *ptr, size_t size)
{
	ARG_UNUSED(ptr);
	return malloc(size);
}
#endif /* else no malloc arena */

#endif /* CONFIG_COMMON_LIBC_MALLOC */

#ifdef CONFIG_COMMON_LIBC_CALLOC
void *calloc(size_t nmemb, size_t size)
{
	void *ret;

	if (size_mul_overflow(nmemb, size, &size)) {
		errno = ENOMEM;
		return NULL;
	}

	ret = malloc(size);

	if (ret != NULL) {
		(void)memset(ret, 0, size);
	}

	return ret;
}
#endif /* CONFIG_COMMON_LIBC_CALLOC */

#ifdef CONFIG_COMMON_LIBC_REALLOCARRAY
void *reallocarray(void *ptr, size_t nmemb, size_t size)
{
	if (size_mul_overflow(nmemb, size, &size)) {
		errno = ENOMEM;
		return NULL;
	}
	return realloc(ptr, size);
}
#endif /* CONFIG_COMMON_LIBC_REALLOCARRAY */
