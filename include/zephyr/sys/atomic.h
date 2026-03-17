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
#elif defined(CONFIG_ATOMIC_OPERATIONS_BUILTIN)
/* Default.  See this file for the Doxygen reference: */
#include <zephyr/sys/atomic_builtin.h>
#else
#error "CONFIG_ATOMIC_OPERATIONS_* not defined"
#endif

/*
 * Atomic unsigned char operations.
 *
 * Some architectures (RISC-V, Xtensa, ARC, RX, Cortex-M0/M0+) don't have
 * native byte-sized atomic instructions. On these platforms, atomic_uchar_t
 * is typedef'd to atomic_t and we simply use the existing word-sized
 * atomic functions.
 *
 * On architectures with native byte atomics (ARM Cortex-M3+, x86), we use
 * either C11 stdatomic (when available in C mode) or GCC __atomic_* builtins
 * (in C++ mode or when C11 is not available).
 */

#if defined(CONFIG_RISCV) || defined(CONFIG_XTENSA) || \
	defined(CONFIG_CPU_CORTEX_M0) || defined(CONFIG_CPU_CORTEX_M0PLUS) || \
	defined(CONFIG_RX) || defined(CONFIG_ARC)
/*
 * On platforms without byte-sized atomics, atomic_uchar_t is atomic_t,
 * so we just map to the existing atomic_* functions using inline functions
 * (not macros) to avoid unused-value warnings.
 */

/**
 * @brief Atomic unsigned char compare-and-set.
 */
static inline bool atomic_uchar_cas(atomic_uchar_t *target,
				    unsigned char old_value,
				    unsigned char new_value)
{
	return atomic_cas(target, (atomic_val_t)old_value,
			  (atomic_val_t)new_value);
}

/**
 * @brief Atomic unsigned char addition.
 */
static inline unsigned char atomic_uchar_add(atomic_uchar_t *target,
					     unsigned char value)
{
	return (unsigned char)atomic_add(target, (atomic_val_t)value);
}

/**
 * @brief Atomic unsigned char subtraction.
 */
static inline unsigned char atomic_uchar_sub(atomic_uchar_t *target,
					     unsigned char value)
{
	return (unsigned char)atomic_sub(target, (atomic_val_t)value);
}

/**
 * @brief Atomic unsigned char increment.
 */
static inline unsigned char atomic_uchar_inc(atomic_uchar_t *target)
{
	return (unsigned char)atomic_inc(target);
}

/**
 * @brief Atomic unsigned char decrement.
 */
static inline unsigned char atomic_uchar_dec(atomic_uchar_t *target)
{
	return (unsigned char)atomic_dec(target);
}

/**
 * @brief Atomic unsigned char get.
 */
static inline unsigned char atomic_uchar_get(const atomic_uchar_t *target)
{
	return (unsigned char)atomic_get(target);
}

/**
 * @brief Atomic unsigned char set.
 */
static inline unsigned char atomic_uchar_set(atomic_uchar_t *target,
					     unsigned char value)
{
	return (unsigned char)atomic_set(target, (atomic_val_t)value);
}

/**
 * @brief Atomic unsigned char clear.
 */
static inline unsigned char atomic_uchar_clear(atomic_uchar_t *target)
{
	return (unsigned char)atomic_clear(target);
}

/**
 * @brief Atomic unsigned char OR.
 */
static inline unsigned char atomic_uchar_or(atomic_uchar_t *target,
					    unsigned char value)
{
	return (unsigned char)atomic_or(target, (atomic_val_t)value);
}

/**
 * @brief Atomic unsigned char XOR.
 */
static inline unsigned char atomic_uchar_xor(atomic_uchar_t *target,
					     unsigned char value)
{
	return (unsigned char)atomic_xor(target, (atomic_val_t)value);
}

/**
 * @brief Atomic unsigned char AND.
 */
static inline unsigned char atomic_uchar_and(atomic_uchar_t *target,
					     unsigned char value)
{
	return (unsigned char)atomic_and(target, (atomic_val_t)value);
}

/**
 * @brief Atomic unsigned char NAND.
 */
static inline unsigned char atomic_uchar_nand(atomic_uchar_t *target,
					      unsigned char value)
{
	return (unsigned char)atomic_nand(target, (atomic_val_t)value);
}

#elif defined(ZEPHYR_HAS_C11_STDATOMIC)
/*
 * Tier 1: C11 stdatomic available. Use C11 stdatomic operations directly.
 * GCC __atomic_* builtins cannot operate on _Atomic qualified types in clang.
 */

/**
 * @brief Atomic unsigned char compare-and-set.
 */
static inline bool atomic_uchar_cas(atomic_uchar_t *target,
				    unsigned char old_value,
				    unsigned char new_value)
{
	return atomic_compare_exchange_strong(target, &old_value, new_value);
}

/**
 * @brief Atomic unsigned char addition.
 */
static inline unsigned char atomic_uchar_add(atomic_uchar_t *target,
					     unsigned char value)
{
	return atomic_fetch_add(target, value);
}

/**
 * @brief Atomic unsigned char subtraction.
 */
static inline unsigned char atomic_uchar_sub(atomic_uchar_t *target,
					     unsigned char value)
{
	return atomic_fetch_sub(target, value);
}

/**
 * @brief Atomic unsigned char increment.
 */
static inline unsigned char atomic_uchar_inc(atomic_uchar_t *target)
{
	return atomic_fetch_add(target, 1);
}

/**
 * @brief Atomic unsigned char decrement.
 */
static inline unsigned char atomic_uchar_dec(atomic_uchar_t *target)
{
	return atomic_fetch_sub(target, 1);
}

/**
 * @brief Atomic unsigned char get.
 */
static inline unsigned char atomic_uchar_get(const atomic_uchar_t *target)
{
	return atomic_load(target);
}

/**
 * @brief Atomic unsigned char set.
 */
static inline unsigned char atomic_uchar_set(atomic_uchar_t *target,
					     unsigned char value)
{
	return atomic_exchange(target, value);
}

/**
 * @brief Atomic unsigned char clear.
 */
static inline unsigned char atomic_uchar_clear(atomic_uchar_t *target)
{
	return atomic_exchange(target, 0);
}

/**
 * @brief Atomic unsigned char OR.
 */
static inline unsigned char atomic_uchar_or(atomic_uchar_t *target,
					    unsigned char value)
{
	return atomic_fetch_or(target, value);
}

/**
 * @brief Atomic unsigned char XOR.
 */
static inline unsigned char atomic_uchar_xor(atomic_uchar_t *target,
					     unsigned char value)
{
	return atomic_fetch_xor(target, value);
}

/**
 * @brief Atomic unsigned char AND.
 */
static inline unsigned char atomic_uchar_and(atomic_uchar_t *target,
					     unsigned char value)
{
	return atomic_fetch_and(target, value);
}

/**
 * @brief Atomic unsigned char NAND.
 *
 * C11 stdatomic has no fetch_nand, so use a CAS loop.
 */
static inline unsigned char atomic_uchar_nand(atomic_uchar_t *target,
					      unsigned char value)
{
	unsigned char old_val = atomic_load(target);

	while (!atomic_compare_exchange_weak(target, &old_val,
					     (unsigned char)~(old_val & value))) {
		/* old_val is updated by compare_exchange_weak on failure */
	}
	return old_val;
}

#else /* Tier 2: C++ mode or no C11 - use GCC __atomic_* builtins */

/**
 * @brief Atomic unsigned char compare-and-set.
 */
static inline bool atomic_uchar_cas(atomic_uchar_t *target,
				    unsigned char old_value,
				    unsigned char new_value)
{
	return __atomic_compare_exchange_n(target, &old_value, new_value,
					   0, __ATOMIC_SEQ_CST,
					   __ATOMIC_SEQ_CST);
}

/**
 * @brief Atomic unsigned char addition.
 */
static inline unsigned char atomic_uchar_add(atomic_uchar_t *target,
					     unsigned char value)
{
	return __atomic_fetch_add(target, value, __ATOMIC_SEQ_CST);
}

/**
 * @brief Atomic unsigned char subtraction.
 */
static inline unsigned char atomic_uchar_sub(atomic_uchar_t *target,
					     unsigned char value)
{
	return __atomic_fetch_sub(target, value, __ATOMIC_SEQ_CST);
}

/**
 * @brief Atomic unsigned char increment.
 */
static inline unsigned char atomic_uchar_inc(atomic_uchar_t *target)
{
	return __atomic_fetch_add(target, 1, __ATOMIC_SEQ_CST);
}

/**
 * @brief Atomic unsigned char decrement.
 */
static inline unsigned char atomic_uchar_dec(atomic_uchar_t *target)
{
	return __atomic_fetch_sub(target, 1, __ATOMIC_SEQ_CST);
}

/**
 * @brief Atomic unsigned char get.
 */
static inline unsigned char atomic_uchar_get(const atomic_uchar_t *target)
{
	return __atomic_load_n(target, __ATOMIC_SEQ_CST);
}

/**
 * @brief Atomic unsigned char set.
 */
static inline unsigned char atomic_uchar_set(atomic_uchar_t *target,
					     unsigned char value)
{
	return __atomic_exchange_n(target, value, __ATOMIC_SEQ_CST);
}

/**
 * @brief Atomic unsigned char clear.
 */
static inline unsigned char atomic_uchar_clear(atomic_uchar_t *target)
{
	return __atomic_exchange_n(target, 0, __ATOMIC_SEQ_CST);
}

/**
 * @brief Atomic unsigned char OR.
 */
static inline unsigned char atomic_uchar_or(atomic_uchar_t *target,
					    unsigned char value)
{
	return __atomic_fetch_or(target, value, __ATOMIC_SEQ_CST);
}

/**
 * @brief Atomic unsigned char XOR.
 */
static inline unsigned char atomic_uchar_xor(atomic_uchar_t *target,
					     unsigned char value)
{
	return __atomic_fetch_xor(target, value, __ATOMIC_SEQ_CST);
}

/**
 * @brief Atomic unsigned char AND.
 */
static inline unsigned char atomic_uchar_and(atomic_uchar_t *target,
					     unsigned char value)
{
	return __atomic_fetch_and(target, value, __ATOMIC_SEQ_CST);
}

/**
 * @brief Atomic unsigned char NAND.
 */
static inline unsigned char atomic_uchar_nand(atomic_uchar_t *target,
					      unsigned char value)
{
	return __atomic_fetch_nand(target, value, __ATOMIC_SEQ_CST);
}

#endif /* CONFIG_RISCV || CONFIG_XTENSA */

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

#define ATOMIC_BITS            (sizeof(atomic_val_t) * BITS_PER_BYTE)
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
