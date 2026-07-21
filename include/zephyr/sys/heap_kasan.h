/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Lightweight heap write address sanitizer (Heap KASAN) API.
 */

#ifndef ZEPHYR_INCLUDE_SYS_HEAP_KASAN_H_
#define ZEPHYR_INCLUDE_SYS_HEAP_KASAN_H_

#include <stdint.h>
#include <stddef.h>
#include <zephyr/sys/sys_heap.h>
#include <zephyr/sys/bitarray.h>
#include <zephyr/toolchain.h>
#include <zephyr/init.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_SYS_HEAP_KASAN) || defined(__DOXYGEN__)

/**
 * @defgroup heap_kasan_apis Heap KASAN APIs
 * @ingroup heaps
 * @{
 */

/** @cond INTERNAL_HIDDEN */
/** Shadow granule size in bytes; configured via SYS_HEAP_KASAN_GRANULE_CHOICE. */
#define SYS_HEAP_KASAN_GRANULE             ((size_t)CONFIG_SYS_HEAP_KASAN_GRANULE)
/** @endcond */

/** Shadow bits required for a heap of @p _sz bytes (one bit per granule). */
#define SYS_HEAP_KASAN_SHADOW_BITS(_sz)    ((_sz) / SYS_HEAP_KASAN_GRANULE)
/** uint32_t bundle count for the shadow of a heap of @p _sz bytes. */
#define SYS_HEAP_KASAN_SHADOW_BUNDLES(_sz) DIV_ROUND_UP(SYS_HEAP_KASAN_SHADOW_BITS(_sz), 32)

/**
 * @brief Report a heap KASAN violation.
 *
 * Weak default calls k_panic(). Override in test code to intercept
 * violations without triggering a panic.
 *
 * @param addr  Violating address.
 * @param size  Write size in bytes.
 */
void heap_kasan_report(uintptr_t addr, size_t size);

/**
 * @brief Associate a shadow bitarray with a heap for KASAN tracking.
 *
 * Enables KASAN shadow tracking on a heap that was not registered at build
 * time via SYS_HEAP_KASAN_ENABLE() or K_HEAP_KASAN_ENABLE(). Useful for
 * dynamically created heaps.
 *
 * @warning Must be called before sys_heap_init(). Calling after
 *          sys_heap_init() has no effect and the heap will not be tracked.
 *
 * @param heap  sys_heap to track.
 * @param ba    Shadow bitarray; must have at least SYS_HEAP_KASAN_SHADOW_BITS(heap_size) bits
 *              backed by SYS_HEAP_KASAN_SHADOW_BUNDLES(heap_size) uint32_t bundles.
 */
void heap_kasan_register(struct sys_heap *heap, sys_bitarray_t *ba);

/**
 * @brief Enable KASAN shadow tracking on a struct sys_heap.
 *
 * Place at file scope after the sys_heap declaration. Allocates a static
 * shadow buffer and registers it with the heap at PRE_KERNEL_1 priority 0.
 * sys_heap_init() must be called after.
 *
 * @param _heap_name  sys_heap variable name.
 * @param _heap_sz    Heap buffer size in bytes (compile-time constant).
 *
 * Sample usage:
 * @code
 * static char heap_buf[4096];
 * static struct sys_heap my_heap;
 *
 * SYS_HEAP_KASAN_ENABLE(my_heap, sizeof(heap_buf));
 *
 * // call sys_heap_init(&my_heap, heap_buf, sizeof(heap_buf)) at runtime
 * @endcode
 */
#define SYS_HEAP_KASAN_ENABLE(_heap_name, _heap_sz) \
	static uint32_t \
		_heap_name##_kasan_bundles[SYS_HEAP_KASAN_SHADOW_BUNDLES(_heap_sz)] __noinit \
		__aligned(4); \
	static sys_bitarray_t _heap_name##_kasan_ba = { \
		.num_bits = SYS_HEAP_KASAN_SHADOW_BITS(_heap_sz), \
		.num_bundles = SYS_HEAP_KASAN_SHADOW_BUNDLES(_heap_sz), \
		.bundles = _heap_name##_kasan_bundles, \
	}; \
	static int _heap_name##_kasan_init_fn(void) \
	{ \
		heap_kasan_register(&(_heap_name), &_heap_name##_kasan_ba); \
		return 0; \
	} \
	SYS_INIT(_heap_name##_kasan_init_fn, PRE_KERNEL_1, 0)

/**
 * @brief Enable KASAN shadow tracking on a struct k_heap (K_HEAP_DEFINE).
 *
 * Place at file scope after the K_HEAP_DEFINE line. Allocates a static
 * shadow buffer and registers it with the heap at PRE_KERNEL_1 priority 0,
 * before static heap init at (PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS).
 *
 * @param _heap_name  k_heap variable name.
 * @param _heap_sz    Same size passed to K_HEAP_DEFINE (compile-time constant).
 *
 * Sample usage:
 * @code
 * K_HEAP_DEFINE(my_heap, 4096);
 * K_HEAP_KASAN_ENABLE(my_heap, 4096);
 * @endcode
 */
#define K_HEAP_KASAN_ENABLE(_heap_name, _heap_sz) \
	static uint32_t _heap_name##_kasan_bundles[SYS_HEAP_KASAN_SHADOW_BUNDLES( \
		MAX(_heap_sz, Z_HEAP_MIN_SIZE))] __noinit __aligned(4); \
	static sys_bitarray_t _heap_name##_kasan_ba = { \
		.num_bits = SYS_HEAP_KASAN_SHADOW_BITS(MAX(_heap_sz, Z_HEAP_MIN_SIZE)), \
		.num_bundles = SYS_HEAP_KASAN_SHADOW_BUNDLES(MAX(_heap_sz, Z_HEAP_MIN_SIZE)), \
		.bundles = _heap_name##_kasan_bundles, \
	}; \
	static int _heap_name##_kasan_init_fn(void) \
	{ \
		heap_kasan_register(&(_heap_name).heap, &_heap_name##_kasan_ba); \
		return 0; \
	} \
	SYS_INIT(_heap_name##_kasan_init_fn, PRE_KERNEL_1, 0)

/** @} */

#else                                              /* !CONFIG_SYS_HEAP_KASAN */

/** @cond INTERNAL_HIDDEN */
#define SYS_HEAP_KASAN_ENABLE(_heap_name, _heap_sz) /* no-op */
#define K_HEAP_KASAN_ENABLE(_heap_name, _heap_sz)   /* no-op */
/** @endcond */

#endif /* CONFIG_SYS_HEAP_KASAN */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_HEAP_KASAN_H_ */
