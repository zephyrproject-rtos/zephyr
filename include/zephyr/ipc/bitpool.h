/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_SYS_bitpool_H_
#define ZEPHYR_INCLUDE_SYS_bitpool_H_

#include <zephyr/sys/atomic.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Define an array of atomic values variables.
 *
 * Macro symetrical to @ref ATOMIC_DEFINE but creates array of @ref atomic_val_t values.
 *
 * @sa ATOMIC_DEFINE
 *
 * @param name Name of array of atomic variables.
 * @param num_bits Number of bits needed.
 */
#define ATOMIC_VAL_DEFINE(name, num_bits) \
	atomic_val_t name[ATOMIC_BITMAP_SIZE(num_bits)]

/**
 * @brief Macro that simplifies atomic operation implementation
 *
 * The macro allows to write the code for atomic operation in a form of protected body.
 * Sample of usage:
 *
 * @code
 * BITPOOL_ATOMIC_OP(&bitmap, old, new, bitcnt)
 * {
 *    bitpool_copy(old, new, bitcnt);
 *    bitpool_set_inv_block(new, 0, 15, 1);
 * }
 * @endcode
 *
 * Is equivalent to:
 *
 * @code
 * bool status = false;
 * ATOMIC_VAL_DEFINE(old, bitcnt);
 * ATOMIC_VAL_DEFINE(new, bitcnt);
 *
 * while(!status) {
 *   bitpool_atomic_read(&bitmap, old, bitcnt);
 *   // Here goes the code from BITPOOL_ATOMIC_OP body
 *   bitpool_copy(old, new, bitcnt);
 *   bitpool_set_inv_block(new, 0, 15, 1);
 *   // End of the code from BITPOOL_ATOMIC_OP body
 *   status = bitpool_atomic_cas(&bitmap, old, new, bitcnt);
 * }
 * @endcode
 *
 * @param _ptr    Pointer to the bitmap
 * @param _old    The name of the variable that will hold the old value of the bitmap
 * @param _new    The name of the variable that will hold the new value of the bitmap
 * @param _bitcnt Number of bits in the bitmap
 *
 * @sa BITPOOL_ATOMIC_OP_break
 */
#define BITPOOL_ATOMIC_OP(_ptr, _old, _new, _bitcnt)                                       \
	for (bool __status = false, __first_run = true; __first_run; /* noop */)           \
		for (atomic_val_t _old[ATOMIC_BITMAP_SIZE(_bitcnt)],                       \
				  _new[ATOMIC_BITMAP_SIZE(_bitcnt)];                       \
		    __first_run; /* noop */)                                               \
			for (__first_run = false;                                          \
			    bitpool_atomic_read(_ptr, _old, _bitcnt), !__status;           \
			    __status = bitpool_atomic_cas(_ptr, _old, _new, _bitcnt))

/** @brief Exit from @ref BITPOOL_ATOMIC_OP loop body */
#define BITPOOL_ATOMIC_OP_break break

/**
 * @defgroup bitpool_apis Bitpools support for atomic bitmaps
 * @ingroup atomic_apis
 * @{
 *
 * The atomic bitpool is expansion to atomic bitfields api - @sa ATOMIC_DEFINE.
 * The main, but not the only, usage is in block memory allocator where it allows
 * to search and take number of free continuous blocks of memory.
 *
 * Find and allocate number of free bits that are directly next the others is complex process
 * and requires more computation.
 * To be able to make it in atomic manner it requires to read the atomic bitmap
 * (@ref bitpool_atomic_read),
 * then copying it (@ref bitpool_copy).
 * After that we can prepare allocation by the usage of @ref bitpool_find_first_block
 * and @ref bitpool_set_block_to.
 *
 * After the calculation is done the @ref bitpool_atomic_cas function should be used to store
 * new computed atomic bitmap.
 * Only if the operation succeeded we can continue with the computed values, in other
 * case the operation need to be repeated.
 */

/**
 * @brief Copy between atomic value bitmaps
 *
 * @param[in]  source The source to copy from
 * @param[out] target The memory where to write the data
 * @param      bitcnt Number of bits in the bitfield
 */
static inline void bitpool_copy(const atomic_val_t *source,
				       atomic_val_t *target,
				       size_t bitcnt)
{
	size_t cnt = ATOMIC_BITMAP_SIZE(bitcnt);

	while (cnt--) {
		*target++ = *source++;
	}
}

/**
 * @brief AND operation on atomic values
 *
 * @param[out] target Target of the operation
 * @param[in]  s1 First operand
 * @param[in]  s2 Second operand
 * @param      bitcnt Number of bits in the bitfield
 *
 * @note Any of the @em target, @em s1 and @em s2 can be the same pointers.
 * @note This operation itself is not atomic.
 *       It is meant to be used together with @ref bitpool_atomic_cas.
 */
static inline void bitpool_and(atomic_val_t *target,
			       const atomic_val_t *s1,
			       const atomic_val_t *s2,
			       size_t bitcnt)
{
	size_t cnt = ATOMIC_BITMAP_SIZE(bitcnt);

	while (cnt--) {
		*target++ = *s1++ & *s2++;
	}
}

/**
 * @brief OR operation on atomic values
 *
 * @param[out] target Target of the operation
 * @param[in]  s1 First operand
 * @param[in]  s2 Second operand
 * @param      bitcnt Number of bits in the bitfield
 *
 * @note Any of the @em target, @em s1 and @em s2 can be the same pointers.
 * @note This operation itself is not atomic.
 *       It is meant to be used together with @ref bitpool_atomic_cas.
 */
static inline void bitpool_or(atomic_val_t *target,
			      const atomic_val_t *s1,
			      const atomic_val_t *s2,
			      size_t bitcnt)
{
	size_t cnt = ATOMIC_BITMAP_SIZE(bitcnt);

	while (cnt--) {
		*target++ = *s1++ | *s2++;
	}
}

/**
 * @brief XOR operation on atomic values
 *
 * @param[out] target Target of the operation
 * @param[in]  s1 First operand
 * @param[in]  s2 Second operand
 * @param      bitcnt Number of bits in the bitfield
 *
 * @note Any of the @em target, @em s1 and @em s2 can be the same pointers.
 * @note This operation itself is not atomic.
 *       It is meant to be used together with @ref bitpool_atomic_cas.
 */
static inline void bitpool_xor(atomic_val_t *target,
			       const atomic_val_t *s1,
			       const atomic_val_t *s2,
			       size_t bitcnt)
{
	size_t cnt = ATOMIC_BITMAP_SIZE(bitcnt);

	while (cnt--) {
		*target++ = *s1++ ^ *s2++;
	}
}

/**
 * @brief Bit negation operation on atomic values
 *
 * @param[out] target Target of the operation
 * @param[in]  source Source value to invert
 * @param      bitcnt Number of bits in the bitfield
 *
 * @note The @em target and @em source can be the same pointers.
 * @note This operation itself is not atomic.
 *       It is meant to be used together with @ref bitpool_atomic_cas.
 */
static inline void bitpool_not(atomic_val_t *target,
			       const atomic_val_t *source,
			       size_t bitcnt)
{
	size_t cnt = ATOMIC_BITMAP_SIZE(bitcnt) - 1;

	while (cnt--) {
		*target++ = ~*source++;
	}
	*target = (~*source & (ATOMIC_MASK(bitcnt) - 1U));
}

/**
 * @brief Set bit in atomic value
 *
 * @param[in,out] target Target of the operation
 * @param         bit Bit index to be set
 *
 * @note This operation itself is not atomic.
 *       It is meant to be used together with @ref bitpool_atomic_cas.
 */
static inline void bitpool_set_bit(atomic_val_t *target, size_t bit)
{
	*ATOMIC_ELEM(target, bit) |= ATOMIC_MASK(bit);
}

/**
 * @brief Clear bit in atomic value
 *
 * @param[in,out] target Target of the operation
 * @param         bit Bit index to be cleared
 *
 * @note This operation itself is not atomic.
 *       It is meant to be used together with @ref bitpool_atomic_cas.
 */
static inline void bitpool_clear_bit(atomic_val_t *target, size_t bit)
{
	*ATOMIC_ELEM(target, bit) &= ~ATOMIC_MASK(bit);
}

/**
 * @brief Get the bit value
 *
 * @param[in] source The array of values with the bit to set
 * @param     bit Bit index to be tested
 */
static inline bool bitpool_get_bit(const atomic_val_t *source, size_t bit)
{
	return (*ATOMIC_ELEM(source, bit)) & ATOMIC_MASK(bit);
}

void bitpool_atomic_read_internal(const atomic_t *source,
				  atomic_val_t *target,
				  size_t bitcnt);

/**
 * @brief Read atomic bitmap to atomic value copy
 *
 * @param[in]  source The source to copy from
 * @param[out] target The memory where to write the data
 * @param      bitcnt Number of bits in the bitfield
 */
static inline void bitpool_atomic_read(const atomic_t *source,
				       atomic_val_t *target,
				       size_t bitcnt)
{
	if (bitcnt <= ATOMIC_BITS) {
		*target = atomic_get(source);
	} else {
		bitpool_atomic_read_internal(source, target, bitcnt);
	}
}

void bitpool_atomic_write_internal(atomic_t *target,
				   const atomic_val_t *source,
				   size_t bitcnt);

/**
 * @brief Write atomic value to atomic bitmap
 *
 * @note For atomic computational operation use rather @ref bitpool_atomic_cas
 *
 * @param[out] target The target atomic bitmap to write to
 * @param[in]  source The source atomic value to write from
 * @param      bitcnt Number of bits in the bitfield
 */
static inline void bitpool_atomic_write(atomic_t *target,
					const atomic_val_t *source,
					size_t bitcnt)
{
	if (bitcnt <= ATOMIC_BITS) {
		atomic_set(target, *source);
	} else {
		bitpool_atomic_write_internal(target, source, bitcnt);
	}
}

bool bitpool_atomic_cas_internal(atomic_t *target,
				 const atomic_val_t *old_value,
				 const atomic_val_t *new_value,
				 size_t bitcnt);

/**
 * @brief Compare and store
 *
 * Compare current and old value and store new value only if target did not change.
 * Function together with @ref bitpool_atomic_read to atomically change the value.
 * @param[in,out] target    The target of the operation
 * @param[in]     old_value Old value before modification
 * @param[in]     new_value New value to be stored
 * @param         bitcnt    Number of bits in the bitfield
 */
static inline bool bitpool_atomic_cas(atomic_t *target,
				      const atomic_val_t *old_value,
				      const atomic_val_t *new_value,
				      size_t bitcnt)
{
	if (bitcnt <= ATOMIC_BITS) {
		return atomic_cas(target, *old_value, *new_value);
	} else {
		return bitpool_atomic_cas_internal(target, old_value, new_value, bitcnt);
	}
}

/**
 * @brief Find first block of bits set to given value and give its size
 *
 * @param[in]  source Bitpool to search for a block of given value
 * @param      val    The bit value to search for
 * @param[out] size   The size of the block found
 * @param      bitcnt Number of bits in the bitfield
 *
 * @return Index of the first bit in a block or negative error value
 */
int bitpool_find_first_block_any_size(const atomic_val_t *source,
				      bool val,
				      size_t *size,
				      size_t bitcnt);

/**
 * @brief Find first block with given minimal size
 *
 * @param[in] source         Bitpool to search for a block of given value
 * @param     val            The bit value to search for
 * @param     requested_size Minimum number of continuous bits of given value
 * @param     bitcnt         Number of bits in the bitfield
 *
 * @return Index of the first bit in a block or negative error value
 */
int bitpool_find_first_block(const atomic_val_t *source,
			     bool val,
			     size_t requested_size,
			     size_t bitcnt);

/**
 * @brief Set a block of bits in the bitpool to 1.
 *
 * @param target Pointer to the bitpool.
 * @param first_bit Index of the first bit to set.
 * @param size Number of bits to set.
 */
void bitpool_set_block(atomic_val_t *target, size_t first_bit, size_t size);

/**
 * @brief Clear a block of bits in the bitpool to 0.
 *
 * @param target Pointer to the bitpool.
 * @param first_bit Index of the first bit to clear.
 * @param size Number of bits to clear.
 */
void bitpool_clear_block(atomic_val_t *target, size_t first_bit, size_t size);

/**
 * @brief Set a block of bits in a bitpool to given value
 *
 * @param[out] target    Bitpool where to set the bits
 * @param      first_bit First bit in a block to set
 * @param      size      Number of bits to set
 * @param      val       The value of the bits to set
 */
static inline void bitpool_set_block_to(atomic_val_t *target,
			  size_t first_bit,
			  size_t size,
			  bool val)
{
	if (val) {
		bitpool_set_block(target, first_bit, size);
	} else {
		bitpool_clear_block(target, first_bit, size);
	}
}

/**
 * @brief Conditionally set a block of bits in the bitpool to 1.
 *
 * This function sets a block of bits starting from the specified bit index
 * to 1 only if the current state of the bits is 0. If any bit in the block
 * is already set to 1, the function returns false and no bits are changed.
 *
 * @param target Pointer to the bitpool.
 * @param first_bit Index of the first bit to set.
 * @param size Number of bits to set.
 *
 * @return true if the bits were set successfully, false otherwise.
 */
bool bitpool_set_block_cond(atomic_val_t *target, size_t first_bit, size_t size);

/**
 * @brief Conditionally clear a block of bits in the bitpool to 0.
 *
 * This function clears a block of bits starting from the specified bit index
 * to 0 only if the current state of the bits is 1. If any bit in the block
 * is already cleared to 0, the function returns false and no bits are changed.
 *
 * @param target Pointer to the bitpool.
 * @param first_bit Index of the first bit to clear.
 * @param size Number of bits to clear.
 *
 * @return true if the bits were cleared successfully, false otherwise.
 */
bool bitpool_clear_block_cond(atomic_val_t *target, size_t first_bit, size_t size);

/**
 * @brief Conditionally set a block of bits in a bitpool to a given value.
 *
 * This function sets a specified number of bits starting from a given index
 * in the target bitmap to a given value only if the current state is opposite.
 * If the current state is not opposite, the function returns false.
 * If the function succeeds, it returns true.
 *
 * @param[out] target    Bitpool where to set the bits.
 * @param      first_bit First bit in a block to set.
 * @param      size      Number of bits to set.
 * @param      val       The value of the bits to set (0 or 1).
 *
 * @return true if the bits were set successfully, false otherwise.
 */
static inline bool bitpool_set_block_to_cond(atomic_val_t *target,
			       size_t first_bit,
			       size_t size,
			       bool val)
{
	if (val) {
		return bitpool_set_block_cond(target, first_bit, size);
	} else {
		return bitpool_clear_block_cond(target, first_bit, size);
	}
}

/**
 * @brief Invert block of bits in a bitpool
 *
 * @param[in,out] target    Bitpool where to invert bits
 * @param         first_bit First bit in a block to invert
 * @param         size      Number of bits to invert
 */
void bitpool_inv_block(atomic_val_t *target,
		       size_t first_bit,
		       size_t size);

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ZEPHYR_INCLUDE_SYS_bitpool_H_ */
