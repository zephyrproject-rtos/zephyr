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
