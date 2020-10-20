/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr.h>
#include <init.h>
#include <errno.h>
#include <sys/math_extras.h>
#include <string.h>
#include <app_memory/app_memdomain.h>
#include <sys/sys_heap.h>

#define LOG_LEVEL CONFIG_KERNEL_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

#ifdef CONFIG_MINIMAL_LIBC_MALLOC

#if (CONFIG_MINIMAL_LIBC_MALLOC_ARENA_SIZE > 0)
#ifdef CONFIG_USERSPACE
K_APPMEM_PARTITION_DEFINE(z_malloc_partition);
#define POOL_SECTION K_APP_DMEM_SECTION(z_malloc_partition)
#else
#define POOL_SECTION .data
#endif /* CONFIG_USERSPACE */

#define HEAP_BYTES CONFIG_MINIMAL_LIBC_MALLOC_ARENA_SIZE

Z_GENERIC_SECTION(POOL_SECTION) static struct sys_heap z_malloc_heap;
Z_GENERIC_SECTION(POOL_SECTION) static char z_malloc_heap_mem[HEAP_BYTES];

void *malloc(size_t size)
{
	void *ret;
	ret = sys_heap_alloc(&z_malloc_heap, size);
	if (ret == NULL) {
		errno = ENOMEM;
	}

	return ret;
}

static int malloc_prepare(const struct device *unused)
{
	ARG_UNUSED(unused);

	sys_heap_init(&z_malloc_heap, z_malloc_heap_mem, HEAP_BYTES);

	return 0;
}

void *realloc(void *ptr, size_t requested_size)
{
	void *ret = sys_heap_realloc(&z_malloc_heap, ptr, requested_size);

	return ret == NULL ? ptr : ret;
}

void free(void *ptr)
{
	sys_heap_free(&z_malloc_heap, ptr);
}

SYS_INIT(malloc_prepare, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#else /* No malloc arena */
void *malloc(size_t size)
{
	ARG_UNUSED(size);

	LOG_ERR("CONFIG_MINIMAL_LIBC_MALLOC_ARENA_SIZE is 0");
	errno = ENOMEM;

	return NULL;
}

void free(void *ptr)
{
	ARG_UNUSED(ptr);
}

void *realloc(void *ptr, size_t requested_size)
{
	ARG_UNUSED(ptr);
	return malloc(requested_size);
}
#endif

#endif /* CONFIG_MINIMAL_LIBC_MALLOC */

#ifdef CONFIG_MINIMAL_LIBC_CALLOC
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
#endif /* CONFIG_MINIMAL_LIBC_CALLOC */

#ifdef CONFIG_MINIMAL_LIBC_REALLOCARRAY
void *reallocarray(void *ptr, size_t nmemb, size_t size)
{
#if (CONFIG_MINIMAL_LIBC_MALLOC_ARENA_SIZE > 0)
	if (size_mul_overflow(nmemb, size, &size)) {
		errno = ENOMEM;
		return NULL;
	}
	return sys_heap_realloc(&z_malloc_heap, ptr, size);
#else
	return NULL;
#endif
}
#endif /* CONFIG_MINIMAL_LIBC_REALLOCARRAY */
