/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief sys_heap release hook interface.
 */

#ifndef ZEPHYR_INCLUDE_SYS_HEAP_RELEASE_HOOK_H_
#define ZEPHYR_INCLUDE_SYS_HEAP_RELEASE_HOOK_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

/*
 * Called by sys_heap while a region of memory stops being owned by its
 * caller, when CONFIG_SYS_HEAP_RELEASE_HOOK is set. A consumer selects that
 * symbol and provides this entry point; the call sites in the heap carry no
 * consumer-specific conditionals. Exactly one implementation may exist in a
 * build: a second consumer collides with it at link time.
 *
 * The region is reported while it is still valid and just before it goes back
 * to the heap, both for the block handed to sys_heap_free() and for the
 * suffix that an in-place shrink splits off. The heap lock is held, so the
 * hook must not allocate from or free to the heap it is called for.
 *
 * With CONFIG_USERSPACE, free() on the common libc malloc arena runs in the
 * caller's context, so the hook can be entered from user mode and must
 * tolerate it.
 */
void heap_release_hook(void *mem, size_t bytes);

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_HEAP_RELEASE_HOOK_H_ */
