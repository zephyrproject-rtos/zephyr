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
#include <sys/mempool.h>
#include <string.h>
#include <app_memory/app_memdomain.h>

#define LOG_LEVEL CONFIG_KERNEL_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_DECLARE(os);

#ifdef CONFIG_MINIMAL_LIBC_MALLOC

#if (CONFIG_MINIMAL_LIBC_MALLOC_ARENA_SIZE > 0)
#ifdef CONFIG_USERSPACE
K_APPMEM_PARTITION_DEFINE(z_malloc_partition);
#define POOL_SECTION K_APP_DMEM_SECTION(z_malloc_partition)
#else
#define POOL_SECTION .data
#endif /* CONFIG_USERSPACE */

SYS_MEM_POOL_DEFINE(z_malloc_mem_pool, NULL, 16,
		    CONFIG_MINIMAL_LIBC_MALLOC_ARENA_SIZE, 1, 4, POOL_SECTION);

void *malloc(size_t size)
{
	void *ret;

	ret = sys_mem_pool_alloc(&z_malloc_mem_pool, size);
	if (ret == NULL) {
		errno = ENOMEM;
	}

	return ret;
}

static int malloc_prepare(const struct device *unused)
{
	ARG_UNUSED(unused);

	sys_mem_pool_init(&z_malloc_mem_pool);

	return 0;
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
#endif

void *realloc(void *ptr, size_t requested_size)
{
	void *new_ptr;
	size_t copy_size;

	if (ptr == NULL) {
		return malloc(requested_size);
	}

	if (requested_size == 0) {
		free(ptr);
		return NULL;
	}

	copy_size = sys_mem_pool_try_expand_inplace(ptr, requested_size);
	if (copy_size == 0) {
		/* Existing block large enough, nothing else to do */
		return ptr;
	}

	new_ptr = malloc(requested_size);
	if (new_ptr == NULL) {
		return NULL;
	}

	memcpy(new_ptr, ptr, copy_size);
	free(ptr);

	return new_ptr;
}

void free(void *ptr)
{
	sys_mem_pool_free(ptr);
}
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
	if (size_mul_overflow(nmemb, size, &size)) {
		errno = ENOMEM;
		return NULL;
	}
	return realloc(ptr, size);
}
#endif /* CONFIG_MINIMAL_LIBC_REALLOCARRAY */
