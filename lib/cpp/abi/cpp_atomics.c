/*
 * Copyright (c) 2025 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief C-based implementation of GCC __atomic built-ins.
 *
 * This file provides a fallback implementation for the C++ atomic functions
 * required by the compiler on architectures that do not have native atomic
 * instructions and are using the generic C implementation of atomics.
 * All operations are made atomic by using a global interrupt lock.
 */

#include <zephyr/kernel.h>
#include <stdint.h>

/*
 * Note on memory ordering:
 * The `memorder` parameters are ignored because the irq_lock() provides
 * a full memory barrier, which is equivalent to the strongest memory order,
 * __ATOMIC_SEQ_CST. This is always safe.
 */

/* === compare_exchange ======================================================= */

#define DEFINE_ATOMIC_COMPARE_EXCHANGE(n, type)                                                    \
	bool __atomic_compare_exchange_##n(volatile void *ptr, void *expected, type desired,       \
					   bool weak, int success, int failure)                    \
	{                                                                                          \
		bool ret = false;                                                                  \
		unsigned int key = irq_lock();                                                     \
		volatile type *p = ptr;                                                            \
		type *e = expected;                                                                \
                                                                                                   \
		if (*p == *e) {                                                                    \
			*p = desired;                                                              \
			ret = true;                                                                \
		} else {                                                                           \
			*e = *p;                                                                   \
			ret = false;                                                               \
		}                                                                                  \
		irq_unlock(key);                                                                   \
		return ret;                                                                        \
	}

DEFINE_ATOMIC_COMPARE_EXCHANGE(1, uint8_t)
DEFINE_ATOMIC_COMPARE_EXCHANGE(2, uint16_t)
DEFINE_ATOMIC_COMPARE_EXCHANGE(4, uint32_t)

/* === exchange =============================================================== */

#define DEFINE_ATOMIC_EXCHANGE(n, type)                                                            \
	type __atomic_exchange_##n(volatile void *ptr, type val, int memorder)                     \
	{                                                                                          \
		unsigned int key = irq_lock();                                                     \
		volatile type *p = ptr;                                                            \
		type old = *p;                                                                     \
                                                                                                   \
		*p = val;                                                                          \
		irq_unlock(key);                                                                   \
		return old;                                                                        \
	}

DEFINE_ATOMIC_EXCHANGE(1, uint8_t)
DEFINE_ATOMIC_EXCHANGE(2, uint16_t)
DEFINE_ATOMIC_EXCHANGE(4, uint32_t)

/* === fetch_<op> ============================================================= */

#define DEFINE_ATOMIC_FETCH_OP(n, type, name, expr)                                                \
	type __atomic_fetch_##name##_##n(volatile void *ptr, type val, int memorder)               \
	{                                                                                          \
		unsigned int key = irq_lock();                                                     \
		volatile type *p = ptr;                                                            \
		type old = *p;                                                                     \
                                                                                                   \
		*p = (expr);                                                                       \
		irq_unlock(key);                                                                   \
		return old;                                                                        \
	}

#define DEFINE_ATOMIC_FETCH_OPS(n, type)                                                           \
	DEFINE_ATOMIC_FETCH_OP(n, type, add, (old + val))                                          \
	DEFINE_ATOMIC_FETCH_OP(n, type, sub, (old - val))                                          \
	DEFINE_ATOMIC_FETCH_OP(n, type, and, (old & val))                                          \
	DEFINE_ATOMIC_FETCH_OP(n, type, or, (old | val))                                           \
	DEFINE_ATOMIC_FETCH_OP(n, type, xor, (old ^ val))                                          \
	DEFINE_ATOMIC_FETCH_OP(n, type, nand, ~(old & val))

DEFINE_ATOMIC_FETCH_OPS(1, uint8_t)
DEFINE_ATOMIC_FETCH_OPS(2, uint16_t)
DEFINE_ATOMIC_FETCH_OPS(4, uint32_t)
