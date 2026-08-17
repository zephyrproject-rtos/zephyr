/*
 * Copyright (c) 2026 GlobalFoundries Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief MWDT implementation of the atomic operations API for ARC HS.
 *
 * The public documentation for these operations lives in <zephyr/sys/atomic.h>.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARC_ATOMIC_ARC_H_
#define ZEPHYR_INCLUDE_ARCH_ARC_ATOMIC_ARC_H_

#ifndef __CCAC__
/* ARC GNU GCC brackets every __ATOMIC_SEQ_CST operation with dmb 3 by itself,
 * so it keeps using <zephyr/sys/atomic_builtin.h>.
 */
#error "The ARC atomic backend is a MWDT-only workaround"
#endif

#include <stdbool.h>
#include <stddef.h>
#include <zephyr/toolchain.h>
#include <zephyr/sys/atomic_types.h>
#include <zephyr/sys/barrier.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Included from <atomic.h> */

/** @cond INTERNAL_HIDDEN */

/* MWDT lowers every __atomic_* operation to a barrier-free llock/scond library
 * routine (__Sync_*_4) and drops the memory-order argument on the way, so the
 * atomicity half of __ATOMIC_SEQ_CST is honoured but the ordering half is not.
 * Supply the missing ordering here.
 */
#define Z_ARC_SEQ_CST(expr)						\
	({								\
		__typeof__(expr) _z_arc_ret;				\
		barrier_dmem_fence_full();				\
		_z_arc_ret = (expr);					\
		barrier_dmem_fence_full();				\
		_z_arc_ret;						\
	})

static ALWAYS_INLINE bool atomic_cas(atomic_t *target, atomic_val_t old_value,
				     atomic_val_t new_value)
{
	return Z_ARC_SEQ_CST(__atomic_compare_exchange_n(target, &old_value,
							 new_value, 0,
							 __ATOMIC_SEQ_CST,
							 __ATOMIC_SEQ_CST));
}

static ALWAYS_INLINE bool atomic_ptr_cas(atomic_ptr_t *target,
					 atomic_ptr_val_t old_value,
					 atomic_ptr_val_t new_value)
{
	return Z_ARC_SEQ_CST(__atomic_compare_exchange_n(target, &old_value,
							 new_value, 0,
							 __ATOMIC_SEQ_CST,
							 __ATOMIC_SEQ_CST));
}

static ALWAYS_INLINE atomic_val_t atomic_add(atomic_t *target, atomic_val_t value)
{
	return Z_ARC_SEQ_CST(__atomic_fetch_add(target, value, __ATOMIC_SEQ_CST));
}

static ALWAYS_INLINE atomic_val_t atomic_sub(atomic_t *target, atomic_val_t value)
{
	return Z_ARC_SEQ_CST(__atomic_fetch_sub(target, value, __ATOMIC_SEQ_CST));
}

static ALWAYS_INLINE atomic_val_t atomic_inc(atomic_t *target)
{
	return atomic_add(target, 1);
}

static ALWAYS_INLINE atomic_val_t atomic_dec(atomic_t *target)
{
	return atomic_sub(target, 1);
}

static ALWAYS_INLINE atomic_val_t atomic_get(const atomic_t *target)
{
	return Z_ARC_SEQ_CST(__atomic_load_n(target, __ATOMIC_SEQ_CST));
}

static ALWAYS_INLINE atomic_ptr_val_t atomic_ptr_get(const atomic_ptr_t *target)
{
	return Z_ARC_SEQ_CST(__atomic_load_n(target, __ATOMIC_SEQ_CST));
}

static ALWAYS_INLINE atomic_val_t atomic_set(atomic_t *target, atomic_val_t value)
{
	return Z_ARC_SEQ_CST(__atomic_exchange_n(target, value, __ATOMIC_SEQ_CST));
}

static ALWAYS_INLINE atomic_ptr_val_t atomic_ptr_set(atomic_ptr_t *target,
						     atomic_ptr_val_t value)
{
	return Z_ARC_SEQ_CST(__atomic_exchange_n(target, value, __ATOMIC_SEQ_CST));
}

static ALWAYS_INLINE atomic_val_t atomic_clear(atomic_t *target)
{
	return atomic_set(target, 0);
}

static ALWAYS_INLINE atomic_ptr_val_t atomic_ptr_clear(atomic_ptr_t *target)
{
	return atomic_ptr_set(target, NULL);
}

static ALWAYS_INLINE atomic_val_t atomic_or(atomic_t *target, atomic_val_t value)
{
	return Z_ARC_SEQ_CST(__atomic_fetch_or(target, value, __ATOMIC_SEQ_CST));
}

static ALWAYS_INLINE atomic_val_t atomic_xor(atomic_t *target, atomic_val_t value)
{
	return Z_ARC_SEQ_CST(__atomic_fetch_xor(target, value, __ATOMIC_SEQ_CST));
}

static ALWAYS_INLINE atomic_val_t atomic_and(atomic_t *target, atomic_val_t value)
{
	return Z_ARC_SEQ_CST(__atomic_fetch_and(target, value, __ATOMIC_SEQ_CST));
}

static ALWAYS_INLINE atomic_val_t atomic_nand(atomic_t *target, atomic_val_t value)
{
	return Z_ARC_SEQ_CST(__atomic_fetch_nand(target, value, __ATOMIC_SEQ_CST));
}

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_ARC_ATOMIC_ARC_H_ */
