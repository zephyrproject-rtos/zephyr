/* atomic operations */

/*
 * Copyright (c) 1997-2015, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_ATOMIC_BUILTIN_H_
#define ZEPHYR_INCLUDE_SYS_ATOMIC_BUILTIN_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Included from <atomic.h> */

/**
 * @addtogroup atomic_apis Atomic Services APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Atomic compare-and-set.
 *
 * This routine performs an atomic compare-and-set on @a target. If the current
 * value of @a target equals @a old_value, @a target is set to @a new_value.
 * If the current value of @a target does not equal @a old_value, @a target
 * is left unchanged.
 *
 * @note As for all atomic APIs, includes a
 * full/sequentially-consistent memory barrier (where applicable).
 *
 * @param target Address of atomic variable.
 * @param old_value Original value to compare against.
 * @param new_value New value to store.
 * @return true if @a new_value is written, false otherwise.
 */
static inline bool atomic_cas(atomic_t *target, atomic_val_t old_value,
			  atomic_val_t new_value)
{
	return __atomic_compare_exchange_n(target, &old_value, new_value,
					   0, __ATOMIC_SEQ_CST,
					   __ATOMIC_SEQ_CST);
}

/**
 * @brief Atomic compare-and-set with pointer values
 *
 * This routine performs an atomic compare-and-set on @a target. If the current
 * value of @a target equals @a old_value, @a target is set to @a new_value.
 * If the current value of @a target does not equal @a old_value, @a target
 * is left unchanged.
 *
 * @note As for all atomic APIs, includes a
 * full/sequentially-consistent memory barrier (where applicable).
 *
 * @param target Address of atomic variable.
 * @param old_value Original value to compare against.
 * @param new_value New value to store.
 * @return true if @a new_value is written, false otherwise.
 */
static inline bool atomic_ptr_cas(atomic_ptr_t *target, atomic_ptr_val_t old_value,
				  atomic_ptr_val_t new_value)
{
	return __atomic_compare_exchange_n(target, &old_value, new_value,
					   0, __ATOMIC_SEQ_CST,
					   __ATOMIC_SEQ_CST);
}

/**
 *
 * @brief Atomic addition.
 *
 * This routine performs an atomic addition on @a target.
 *
 * @note As for all atomic APIs, includes a
 * full/sequentially-consistent memory barrier (where applicable).
 *
 * @param target Address of atomic variable.
 * @param value Value to add.
 *
 * @return Previous value of @a target.
 */
static inline atomic_val_t atomic_add(atomic_t *target, atomic_val_t value)
{
	return __atomic_fetch_add(target, value, __ATOMIC_SEQ_CST);
}

/**
 *
 * @brief Atomic subtraction.
 *
 * This routine performs an atomic subtraction on @a target.
 *
 * @note As for all atomic APIs, includes a
 * full/sequentially-consistent memory barrier (where applicable).
 *
 * @param target Address of atomic variable.
 * @param value Value to subtract.
 *
 * @return Previous value of @a target.
 */
static inline atomic_val_t atomic_sub(atomic_t *target, atomic_val_t value)
{
	return __atomic_fetch_sub(target, value, __ATOMIC_SEQ_CST);
}

/**
 *
 * @brief Atomic increment.
 *
 * This routine performs an atomic increment by 1 on @a target.
 *
 * @note As for all atomic APIs, includes a
 * full/sequentially-consistent memory barrier (where applicable).
 *
 * @param target Address of atomic variable.
 *
 * @return Previous value of @a target.
 */
static inline atomic_val_t atomic_inc(atomic_t *target)
{
	return atomic_add(target, 1);
}

/**
 *
 * @brief Atomic decrement.
 *
 * This routine performs an atomic decrement by 1 on @a target.
 *
 * @note As for all atomic APIs, includes a
 * full/sequentially-consistent memory barrier (where applicable).
 *
 * @param target Address of atomic variable.
 *
 * @return Previous value of @a target.
 */
static inline atomic_val_t atomic_dec(atomic_t *target)
{
	return atomic_sub(target, 1);
}

/**
 *
 * @brief Atomic get.
 *
 * This routine performs an atomic read on @a target.
 *
 * @note As for all atomic APIs, includes a
 * full/sequentially-consistent memory barrier (where applicable).
 *
 * @param target Address of atomic variable.
 *
 * @return Value of @a target.
 */
static inline atomic_val_t atomic_get(const atomic_t *target)
{
	return __atomic_load_n(target, __ATOMIC_SEQ_CST);
}

/**
 *
 * @brief Atomic get a pointer value
 *
 * This routine performs an atomic read on @a target.
 *
 * @note As for all atomic APIs, includes a
 * full/sequentially-consistent memory barrier (where applicable).
 *
 * @param target Address of pointer variable.
 *
 * @return Value of @a target.
 */
static inline atomic_ptr_val_t atomic_ptr_get(const atomic_ptr_t *target)
{
	return __atomic_load_n(target, __ATOMIC_SEQ_CST);
}

/**
 *
 * @brief Atomic get-and-set.
 *
 * This routine atomically sets @a target to @a value and returns
 * the previous value of @a target.
 *
 * @note As for all atomic APIs, includes a
 * full/sequentially-consistent memory barrier (where applicable).
 *
 * @param target Address of atomic variable.
 * @param value Value to write to @a target.
 *
 * @return Previous value of @a target.
 */
static inline atomic_val_t atomic_set(atomic_t *target, atomic_val_t value)
{
	/* This builtin, as described by Intel, is not a traditional
	 * test-and-set operation, but rather an atomic exchange operation. It
	 * writes value into *ptr, and returns the previous contents of *ptr.
	 */
	return __atomic_exchange_n(target, value, __ATOMIC_SEQ_CST);
}

/**
 *
 * @brief Atomic get-and-set for pointer values
 *
 * This routine atomically sets @a target to @a value and returns
 * the previous value of @a target.
 *
 * @note As for all atomic APIs, includes a
 * full/sequentially-consistent memory barrier (where applicable).
 *
 * @param target Address of atomic variable.
 * @param value Value to write to @a target.
 *
 * @return Previous value of @a target.
 */
static inline atomic_ptr_val_t atomic_ptr_set(atomic_ptr_t *target, atomic_ptr_val_t value)
{
	return __atomic_exchange_n(target, value, __ATOMIC_SEQ_CST);
}

/**
 *
 * @brief Atomic clear.
 *
 * This routine atomically sets @a target to zero and returns its previous
 * value. (Hence, it is equivalent to atomic_set(target, 0).)
 *
 * @note As for all atomic APIs, includes a
 * full/sequentially-consistent memory barrier (where applicable).
 *
 * @param target Address of atomic variable.
 *
 * @return Previous value of @a target.
 */
static inline atomic_val_t atomic_clear(atomic_t *target)
{
	return atomic_set(target, 0);
}

/**
 *
 * @brief Atomic clear of a pointer value
 *
 * This routine atomically sets @a target to zero and returns its previous
 * value. (Hence, it is equivalent to atomic_set(target, 0).)
 *
 * @note As for all atomic APIs, includes a
 * full/sequentially-consistent memory barrier (where applicable).
 *
 * @param target Address of atomic variable.
 *
 * @return Previous value of @a target.
 */
static inline atomic_ptr_val_t atomic_ptr_clear(atomic_ptr_t *target)
{
	return atomic_ptr_set(target, NULL);
}

/**
 *
 * @brief Atomic bitwise inclusive OR.
 *
 * This routine atomically sets @a target to the bitwise inclusive OR of
 * @a target and @a value.
 *
 * @note As for all atomic APIs, includes a
 * full/sequentially-consistent memory barrier (where applicable).
 *
 * @param target Address of atomic variable.
 * @param value Value to OR.
 *
 * @return Previous value of @a target.
 */
static inline atomic_val_t atomic_or(atomic_t *target, atomic_val_t value)
{
	return __atomic_fetch_or(target, value, __ATOMIC_SEQ_CST);
}

/**
 *
 * @brief Atomic bitwise exclusive OR (XOR).
 *
 * @note As for all atomic APIs, includes a
 * full/sequentially-consistent memory barrier (where applicable).
 *
 * This routine atomically sets @a target to the bitwise exclusive OR (XOR) of
 * @a target and @a value.
 *
 * @param target Address of atomic variable.
 * @param value Value to XOR
 *
 * @return Previous value of @a target.
 */
static inline atomic_val_t atomic_xor(atomic_t *target, atomic_val_t value)
{
	return __atomic_fetch_xor(target, value, __ATOMIC_SEQ_CST);
}

/**
 *
 * @brief Atomic bitwise AND.
 *
 * This routine atomically sets @a target to the bitwise AND of @a target
 * and @a value.
 *
 * @note As for all atomic APIs, includes a
 * full/sequentially-consistent memory barrier (where applicable).
 *
 * @param target Address of atomic variable.
 * @param value Value to AND.
 *
 * @return Previous value of @a target.
 */
static inline atomic_val_t atomic_and(atomic_t *target, atomic_val_t value)
{
	return __atomic_fetch_and(target, value, __ATOMIC_SEQ_CST);
}

/**
 *
 * @brief Atomic bitwise NAND.
 *
 * This routine atomically sets @a target to the bitwise NAND of @a target
 * and @a value. (This operation is equivalent to target = ~(target & value).)
 *
 * @note As for all atomic APIs, includes a
 * full/sequentially-consistent memory barrier (where applicable).
 *
 * @param target Address of atomic variable.
 * @param value Value to NAND.
 *
 * @return Previous value of @a target.
 */
static inline atomic_val_t atomic_nand(atomic_t *target, atomic_val_t value)
{
	return __atomic_fetch_nand(target, value, __ATOMIC_SEQ_CST);
}

/** @} */


#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_ATOMIC_BUILTIN_H_ */
