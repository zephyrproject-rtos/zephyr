/*
 * Copyright (c) 2025 Google LLC
 * Copyright (c) 2026 SICK AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief C-based implementation of GCC __atomic built-ins.
 *
 * This file provides a fallback implementation for the atomic functions the
 * compiler emits (for example when using C++ std::atomic) on architectures
 * that do not have native atomic instructions and are using the generic C
 * implementation of atomics (CONFIG_ATOMIC_OPERATIONS_C), such as ARMv6-M
 * (Cortex-M0/M0+). All operations are made atomic by using a global interrupt
 * lock, which is valid on the single-core CPUs that lack lock-free atomics.
 */

#include <zephyr/kernel.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Note on memory ordering:
 * The `memorder` (and success/failure) parameters are ignored because
 * irq_lock() provides a full memory barrier, which is equivalent to the
 * strongest memory order, __ATOMIC_SEQ_CST. This is always safe.
 */

/* === Fixed-width operations (1, 2, 4 and 8 byte objects) ==================== */

#define DEFINE_ATOMIC_LOAD(n, type)                                                                \
	type __atomic_load_##n(const volatile void *ptr, int memorder)                             \
	{                                                                                          \
		unsigned int key = irq_lock();                                                     \
		type val = *(const volatile type *)ptr;                                            \
                                                                                                   \
		irq_unlock(key);                                                                   \
		return val;                                                                        \
	}

#define DEFINE_ATOMIC_STORE(n, type)                                                               \
	void __atomic_store_##n(volatile void *ptr, type val, int memorder)                        \
	{                                                                                          \
		unsigned int key = irq_lock();                                                     \
                                                                                                   \
		*(volatile type *)ptr = val;                                                       \
		irq_unlock(key);                                                                   \
	}

#define DEFINE_ATOMIC_EXCHANGE(n, type)                                                            \
	type __atomic_exchange_##n(volatile void *ptr, type val, int memorder)                     \
	{                                                                                          \
		unsigned int key = irq_lock();                                                     \
		type old = *(volatile type *)ptr;                                                  \
                                                                                                   \
		*(volatile type *)ptr = val;                                                       \
		irq_unlock(key);                                                                   \
		return old;                                                                        \
	}

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

#define DEFINE_ATOMIC_FETCH_OP(n, type, opname, op)                                                \
	type __atomic_fetch_##opname##_##n(volatile void *ptr, type val, int memorder)             \
	{                                                                                          \
		unsigned int key = irq_lock();                                                     \
		type old = *(volatile type *)ptr;                                                  \
                                                                                                   \
		*(volatile type *)ptr = (type)(old op val);                                        \
		irq_unlock(key);                                                                   \
		return old;                                                                        \
	}

#define DEFINE_ATOMIC_FETCH_NAND(n, type)                                                          \
	type __atomic_fetch_nand_##n(volatile void *ptr, type val, int memorder)                   \
	{                                                                                          \
		unsigned int key = irq_lock();                                                     \
		type old = *(volatile type *)ptr;                                                  \
                                                                                                   \
		*(volatile type *)ptr = (type) ~(old & val);                                       \
		irq_unlock(key);                                                                   \
		return old;                                                                        \
	}

#define DEFINE_ATOMIC_SIZED(n, type)                                                               \
	DEFINE_ATOMIC_LOAD(n, type)                                                                \
	DEFINE_ATOMIC_STORE(n, type)                                                               \
	DEFINE_ATOMIC_EXCHANGE(n, type)                                                            \
	DEFINE_ATOMIC_COMPARE_EXCHANGE(n, type)                                                    \
	DEFINE_ATOMIC_FETCH_OP(n, type, add, +)                                                    \
	DEFINE_ATOMIC_FETCH_OP(n, type, sub, -)                                                    \
	DEFINE_ATOMIC_FETCH_OP(n, type, and, &)                                                    \
	DEFINE_ATOMIC_FETCH_OP(n, type, or, |)                                                     \
	DEFINE_ATOMIC_FETCH_OP(n, type, xor, ^)                                                    \
	DEFINE_ATOMIC_FETCH_NAND(n, type)

DEFINE_ATOMIC_SIZED(1, uint8_t)
DEFINE_ATOMIC_SIZED(2, uint16_t)
DEFINE_ATOMIC_SIZED(4, uint32_t)
DEFINE_ATOMIC_SIZED(8, uint64_t)

/* === Generic variants for objects of arbitrary size ======================== */

void __atomic_load(size_t size, const volatile void *ptr, void *ret, int memorder)
{
	unsigned int key = irq_lock();

	__builtin_memcpy(ret, (const void *)ptr, size);
	irq_unlock(key);
}

void __atomic_store(size_t size, volatile void *ptr, void *val, int memorder)
{
	unsigned int key = irq_lock();

	__builtin_memcpy((void *)ptr, val, size);
	irq_unlock(key);
}

void __atomic_exchange(size_t size, volatile void *ptr, void *val, void *ret, int memorder)
{
	unsigned int key = irq_lock();

	__builtin_memcpy(ret, (void *)ptr, size);
	__builtin_memcpy((void *)ptr, val, size);
	irq_unlock(key);
}

bool __atomic_compare_exchange(size_t size, volatile void *ptr, void *expected, void *desired,
			       int success, int failure)
{
	bool ret;
	unsigned int key = irq_lock();

	if (__builtin_memcmp((void *)ptr, expected, size) == 0) {
		__builtin_memcpy((void *)ptr, desired, size);
		ret = true;
	} else {
		__builtin_memcpy(expected, (void *)ptr, size);
		ret = false;
	}
	irq_unlock(key);
	return ret;
}

bool __atomic_test_and_set(volatile void *ptr, int memorder)
{
	unsigned int key = irq_lock();
	volatile uint8_t *p = ptr;
	bool old = (*p != 0U);

	*p = 1U;
	irq_unlock(key);
	return old;
}

/*
 * None of the objects handled here are lock-free: this file is only compiled
 * for targets without native atomic instructions.
 */
bool __atomic_is_lock_free(size_t size, const volatile void *ptr)
{
	return false;
}
