/*
 * Copyright (c) 1997-2015, Wind River Systems, Inc.
 * Copyright (c) 2021 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_ATOMIC_H_
#define ZEPHYR_INCLUDE_SYS_ATOMIC_H_

#include <stdbool.h>
#include <zephyr/toolchain.h>
#include <stddef.h>

#include <zephyr/sys/atomic_types.h> /* IWYU pragma: export */
#include <zephyr/types.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Low-level primitives come in several styles: */

#if defined(CONFIG_ATOMIC_OPERATIONS_C)
/* Generic-but-slow implementation based on kernel locking and syscalls */
#include <zephyr/sys/atomic_c.h>
#elif defined(CONFIG_ATOMIC_OPERATIONS_ARCH)
/* Some architectures need their own implementation */
# ifdef CONFIG_XTENSA
/* Not all Xtensa toolchains support GCC-style atomic intrinsics */
# include <zephyr/arch/xtensa/atomic_xtensa.h>
# else
/* Other arch specific implementation */
# include <zephyr/sys/atomic_arch.h>
# endif /* CONFIG_XTENSA */
#else
/* Default.  See this file for the Doxygen reference: */
#include <zephyr/sys/atomic_builtin.h>
#endif

/* Portable higher-level utilities: */

/**
 * @defgroup atomic_apis Atomic Services APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Initialize an atomic variable.
 *
 * This macro can be used to initialize an atomic variable. For example,
 * @code atomic_t my_var = ATOMIC_INIT(75); @endcode
 *
 * @param i Value to assign to atomic variable.
 */
#define ATOMIC_INIT(i) (i)

/**
 * @brief Initialize an atomic pointer variable.
 *
 * This macro can be used to initialize an atomic pointer variable. For
 * example,
 * @code atomic_ptr_t my_ptr = ATOMIC_PTR_INIT(&data); @endcode
 *
 * @param p Pointer value to assign to atomic pointer variable.
 */
#define ATOMIC_PTR_INIT(p) (p)

/**
 * @cond INTERNAL_HIDDEN
 */

#define ATOMIC_BITS (sizeof(atomic_val_t) * 8)
#define ATOMIC_MASK(bit) BIT((unsigned long)(bit) & (ATOMIC_BITS - 1U))
#define ATOMIC_ELEM(addr, bit) ((addr) + ((bit) / ATOMIC_BITS))

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @brief This macro computes the number of atomic variables necessary to
 * represent a bitmap with @a num_bits.
 *
 * @param num_bits Number of bits.
 */
#define ATOMIC_BITMAP_SIZE(num_bits) (ROUND_UP(num_bits, ATOMIC_BITS) / ATOMIC_BITS)

/**
 * @brief Define an array of atomic variables.
 *
 * This macro defines an array of atomic variables containing at least
 * @a num_bits bits.
 *
 * @note
 * If used from file scope, the bits of the array are initialized to zero;
 * if used from within a function, the bits are left uninitialized.
 *
 * @cond INTERNAL_HIDDEN
 * @note
 * This macro should be replicated in the PREDEFINED field of the documentation
 * Doxyfile.
 * @endcond
 *
 * @param name Name of array of atomic variables.
 * @param num_bits Number of bits needed.
 */
#define ATOMIC_DEFINE(name, num_bits) \
	atomic_t name[ATOMIC_BITMAP_SIZE(num_bits)]

/**
 * @brief Atomically get and test a bit.
 *
 * Atomically get a value and then test whether bit number @a bit of @a target is set or not.
 * The target may be a single atomic variable or an array of them.
 *
 * @note @atomic_api
 *
 * @param target Address of atomic variable or array.
 * @param bit Bit number (starting from 0).
 *
 * @return true if the bit was set, false if it wasn't.
 */
static inline bool atomic_test_bit(const atomic_t *target, int bit)
{
	atomic_val_t val = atomic_get(ATOMIC_ELEM(target, bit));

	return (1 & (val >> (bit & (ATOMIC_BITS - 1)))) != 0;
}

/**
 * @brief Atomically clear a bit and test it.
 *
 * Atomically clear bit number @a bit of @a target and return its old value.
 * The target may be a single atomic variable or an array of them.
 *
 * @note @atomic_api
 *
 * @param target Address of atomic variable or array.
 * @param bit Bit number (starting from 0).
 *
 * @return false if the bit was already cleared, true if it wasn't.
 */
static inline bool atomic_test_and_clear_bit(atomic_t *target, int bit)
{
	atomic_val_t mask = ATOMIC_MASK(bit);
	atomic_val_t old;

	old = atomic_and(ATOMIC_ELEM(target, bit), ~mask);

	return (old & mask) != 0;
}

/**
 * @brief Atomically set a bit and test it.
 *
 * Atomically set bit number @a bit of @a target and return its old value.
 * The target may be a single atomic variable or an array of them.
 *
 * @note @atomic_api
 *
 * @param target Address of atomic variable or array.
 * @param bit Bit number (starting from 0).
 *
 * @return true if the bit was already set, false if it wasn't.
 */
static inline bool atomic_test_and_set_bit(atomic_t *target, int bit)
{
	atomic_val_t mask = ATOMIC_MASK(bit);
	atomic_val_t old;

	old = atomic_or(ATOMIC_ELEM(target, bit), mask);

	return (old & mask) != 0;
}

/**
 * @brief Atomically clear a bit.
 *
 * Atomically clear bit number @a bit of @a target.
 * The target may be a single atomic variable or an array of them.
 *
 * @note @atomic_api
 *
 * @param target Address of atomic variable or array.
 * @param bit Bit number (starting from 0).
 */
static inline void atomic_clear_bit(atomic_t *target, int bit)
{
	atomic_val_t mask = ATOMIC_MASK(bit);

	(void)atomic_and(ATOMIC_ELEM(target, bit), ~mask);
}

/**
 * @brief Atomically set a bit.
 *
 * Atomically set bit number @a bit of @a target.
 * The target may be a single atomic variable or an array of them.
 *
 * @note @atomic_api
 *
 * @param target Address of atomic variable or array.
 * @param bit Bit number (starting from 0).
 */
static inline void atomic_set_bit(atomic_t *target, int bit)
{
	atomic_val_t mask = ATOMIC_MASK(bit);

	(void)atomic_or(ATOMIC_ELEM(target, bit), mask);
}

/**
 * @brief Atomically set a bit to a given value.
 *
 * Atomically set bit number @a bit of @a target to value @a val.
 * The target may be a single atomic variable or an array of them.
 *
 * @note @atomic_api
 *
 * @param target Address of atomic variable or array.
 * @param bit Bit number (starting from 0).
 * @param val true for 1, false for 0.
 */
static inline void atomic_set_bit_to(atomic_t *target, int bit, bool val)
{
	atomic_val_t mask = ATOMIC_MASK(bit);

	if (val) {
		(void)atomic_or(ATOMIC_ELEM(target, bit), mask);
	} else {
		(void)atomic_and(ATOMIC_ELEM(target, bit), ~mask);
	}
}

/**
 * @brief Atomic compare-and-set.
 *
 * This routine performs an atomic compare-and-set on @a target. If the current
 * value of @a target equals @a old_value, @a target is set to @a new_value.
 * If the current value of @a target does not equal @a old_value, @a target
 * is left unchanged.
 *
 * @note @atomic_api
 *
 * @param target Address of atomic variable.
 * @param old_value Original value to compare against.
 * @param new_value New value to store.
 * @return true if @a new_value is written, false otherwise.
 */
bool atomic_cas(atomic_t *target, atomic_val_t old_value, atomic_val_t new_value);

/**
 * @brief Atomic compare-and-set with pointer values
 *
 * This routine performs an atomic compare-and-set on @a target. If the current
 * value of @a target equals @a old_value, @a target is set to @a new_value.
 * If the current value of @a target does not equal @a old_value, @a target
 * is left unchanged.
 *
 * @note @atomic_api
 *
 * @param target Address of atomic variable.
 * @param old_value Original value to compare against.
 * @param new_value New value to store.
 * @return true if @a new_value is written, false otherwise.
 */
bool atomic_ptr_cas(atomic_ptr_t *target, atomic_ptr_val_t old_value,
		    atomic_ptr_val_t new_value);

/**
 * @brief Atomic addition.
 *
 * This routine performs an atomic addition on @a target.
 *
 * @note @atomic_api
 *
 * @param target Address of atomic variable.
 * @param value Value to add.
 *
 * @return Previous value of @a target.
 */
atomic_val_t atomic_add(atomic_t *target, atomic_val_t value);

/**
 * @brief Atomic subtraction.
 *
 * This routine performs an atomic subtraction on @a target.
 *
 * @note @atomic_api
 *
 * @param target Address of atomic variable.
 * @param value Value to subtract.
 *
 * @return Previous value of @a target.
 */
atomic_val_t atomic_sub(atomic_t *target, atomic_val_t value);

/**
 * @brief Atomic increment.
 *
 * This routine performs an atomic increment by 1 on @a target.
 *
 * @note @atomic_api
 *
 * @param target Address of atomic variable.
 *
 * @return Previous value of @a target.
 */
atomic_val_t atomic_inc(atomic_t *target);

/**
 * @brief Atomic decrement.
 *
 * This routine performs an atomic decrement by 1 on @a target.
 *
 * @note @atomic_api
 *
 * @param target Address of atomic variable.
 *
 * @return Previous value of @a target.
 */
atomic_val_t atomic_dec(atomic_t *target);

/**
 * @brief Atomic get.
 *
 * This routine performs an atomic read on @a target.
 *
 * @note @atomic_api
 *
 * @param target Address of atomic variable.
 *
 * @return Value of @a target.
 */
atomic_val_t atomic_get(const atomic_t *target);

/**
 * @brief Atomic get a pointer value
 *
 * This routine performs an atomic read on @a target.
 *
 * @note @atomic_api
 *
 * @param target Address of pointer variable.
 *
 * @return Value of @a target.
 */
atomic_ptr_val_t atomic_ptr_get(const atomic_ptr_t *target);

/**
 * @brief Atomic get-and-set.
 *
 * This routine atomically sets @a target to @a value and returns
 * the previous value of @a target.
 *
 * @note @atomic_api
 *
 * @param target Address of atomic variable.
 * @param value Value to write to @a target.
 *
 * @return Previous value of @a target.
 */
atomic_val_t atomic_set(atomic_t *target, atomic_val_t value);

/**
 * @brief Atomic get-and-set for pointer values
 *
 * This routine atomically sets @a target to @a value and returns
 * the previous value of @a target.
 *
 * @note @atomic_api
 *
 * @param target Address of atomic variable.
 * @param value Value to write to @a target.
 *
 * @return Previous value of @a target.
 */
atomic_ptr_val_t atomic_ptr_set(atomic_ptr_t *target, atomic_ptr_val_t value);

/**
 * @brief Atomic clear.
 *
 * This routine atomically sets @a target to zero and returns its previous
 * value. (Hence, it is equivalent to atomic_set(target, 0).)
 *
 * @note @atomic_api
 *
 * @param target Address of atomic variable.
 *
 * @return Previous value of @a target.
 */
atomic_val_t atomic_clear(atomic_t *target);

/**
 * @brief Atomic clear of a pointer value
 *
 * This routine atomically sets @a target to zero and returns its previous
 * value. (Hence, it is equivalent to atomic_set(target, 0).)
 *
 * @note @atomic_api
 *
 * @param target Address of atomic variable.
 *
 * @return Previous value of @a target.
 */
atomic_ptr_val_t atomic_ptr_clear(atomic_ptr_t *target);

/**
 * @brief Atomic bitwise inclusive OR.
 *
 * This routine atomically sets @a target to the bitwise inclusive OR of
 * @a target and @a value.
 *
 * @note @atomic_api
 *
 * @param target Address of atomic variable.
 * @param value Value to OR.
 *
 * @return Previous value of @a target.
 */
atomic_val_t atomic_or(atomic_t *target, atomic_val_t value);

/**
 * @brief Atomic bitwise exclusive OR (XOR).
 *
 * @note @atomic_api
 *
 * This routine atomically sets @a target to the bitwise exclusive OR (XOR) of
 * @a target and @a value.
 *
 * @param target Address of atomic variable.
 * @param value Value to XOR
 *
 * @return Previous value of @a target.
 */
atomic_val_t atomic_xor(atomic_t *target, atomic_val_t value);

/**
 * @brief Atomic bitwise AND.
 *
 * This routine atomically sets @a target to the bitwise AND of @a target
 * and @a value.
 *
 * @note @atomic_api
 *
 * @param target Address of atomic variable.
 * @param value Value to AND.
 *
 * @return Previous value of @a target.
 */
atomic_val_t atomic_and(atomic_t *target, atomic_val_t value);

/**
 * @brief Atomic bitwise NAND.
 *
 * This routine atomically sets @a target to the bitwise NAND of @a target
 * and @a value. (This operation is equivalent to target = ~(target & value).)
 *
 * @note @atomic_api
 *
 * @param target Address of atomic variable.
 * @param value Value to NAND.
 *
 * @return Previous value of @a target.
 */
atomic_val_t atomic_nand(atomic_t *target, atomic_val_t value);

/**
 * @}
 */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ZEPHYR_INCLUDE_SYS_ATOMIC_H_ */
