/*
 * Copyright (c) 2023 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/sys_heap.h>
#include <stdlib.h>

#ifdef CONFIG_TELINK_W91_MALLOC_FAILED_HOOK
static void sys_heap_alloc_failed(const char *function, struct sys_heap *heap, size_t bytes)
{
	printk("!!! %s failed, with size %u\n", function, bytes);
#if CONFIG_SYS_HEAP_INFO
	sys_heap_print_info(heap, false);
#endif /* CONFIG_SYS_HEAP_INFO */
	abort();
}
#endif /* CONFIG_TELINK_W91_MALLOC_FAILED_HOOK */

void *__wrap_sys_heap_alloc(struct sys_heap *heap, size_t bytes)
{
	extern void *__real_sys_heap_alloc(struct sys_heap *heap, size_t bytes);
	void *result = __real_sys_heap_alloc(heap, bytes);
#ifdef CONFIG_TELINK_W91_MALLOC_FAILED_HOOK
	if (!result) {
		sys_heap_alloc_failed("sys_heap_alloc()", heap, bytes);
	}
#endif /* CONFIG_TELINK_W91_MALLOC_FAILED_HOOK */
	return result;
}

void *__wrap_sys_heap_aligned_alloc(struct sys_heap *heap, size_t align, size_t bytes)
{
	extern void *__real_sys_heap_aligned_alloc(
		struct sys_heap *heap, size_t align, size_t bytes);
	void *result = __real_sys_heap_aligned_alloc(heap, align, bytes);
#ifdef CONFIG_TELINK_W91_MALLOC_FAILED_HOOK
	if (!result) {
		sys_heap_alloc_failed("sys_heap_aligned_alloc()", heap, bytes);
	}
#endif /* CONFIG_TELINK_W91_MALLOC_FAILED_HOOK */
	return result;
}

void *__wrap_sys_heap_aligned_realloc(struct sys_heap *heap, void *ptr, size_t align, size_t bytes)
{
	extern void *__real_sys_heap_aligned_realloc(
		struct sys_heap *heap, void *ptr, size_t align, size_t bytes);
	void *result = __real_sys_heap_aligned_realloc(heap, ptr, align, bytes);
#ifdef CONFIG_TELINK_W91_MALLOC_FAILED_HOOK
	if (!result) {
		sys_heap_alloc_failed("sys_heap_aligned_realloc()", heap, bytes);
	}
#endif /* CONFIG_TELINK_W91_MALLOC_FAILED_HOOK */
	return result;
}
