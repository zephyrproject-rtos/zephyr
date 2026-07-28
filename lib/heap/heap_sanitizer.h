/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Internal sys_heap sanitizer hook interface.
 *
 * heap.c calls these three entry points at the public-API boundary when
 * CONFIG_SYS_HEAP_SANITIZER_HOOKS is set. A sanitizer backend selects that
 * symbol and implements the hooks; the call sites in heap.c carry no
 * backend-specific conditionals.
 */

#ifndef ZEPHYR_LIB_HEAP_HEAP_SANITIZER_H_
#define ZEPHYR_LIB_HEAP_HEAP_SANITIZER_H_

#include <zephyr/sys/sys_heap.h>

/* Called at the end of sys_heap_init(); backend poisons the heap buffer. */
void heap_sanitizer_on_init(struct sys_heap *heap, void *mem, size_t bytes);

/* Called after a successful allocation; backend unpoisons [mem, mem+bytes). */
void heap_sanitizer_on_alloc(struct sys_heap *heap, void *mem, size_t bytes);

/*
 * Called before a chunk is freed; backend re-poisons [mem, mem+bytes) where
 * bytes is the usable allocation size (sys_heap_usable_size).
 */
void heap_sanitizer_on_free(struct sys_heap *heap, void *mem, size_t bytes);

#endif /* ZEPHYR_LIB_HEAP_HEAP_SANITIZER_H_ */
