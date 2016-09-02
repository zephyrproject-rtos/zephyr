/* atomic operations */

/*
 * Copyright (c) 1997-2015, Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __ATOMIC_H__
#define __ATOMIC_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef int atomic_t;
typedef atomic_t atomic_val_t;

#ifdef CONFIG_ATOMIC_OPERATIONS_BUILTIN

/**
 *
 * @brief Atomic compare-and-set primitive
 *
 * This routine provides the compare-and-set operator. If the original value at
 * <target> equals <oldValue>, then <newValue> is stored at <target> and the
 * function returns 1.
 *
 * If the original value at <target> does not equal <oldValue>, then the store
 * is not done and the function returns 0.
 *
 * The reading of the original value at <target>, the comparison,
 * and the write of the new value (if it occurs) all happen atomically with
 * respect to both interrupts and accesses of other processors to <target>.
 *
 * @param target address to be tested
 * @param old_value value to compare against
 * @param new_value value to compare against
 * @return Returns 1 if <new_value> is written, 0 otherwise.
 */
static inline int atomic_cas(atomic_t *target, atomic_val_t old_value,
			  atomic_val_t new_value)
{
	return __atomic_compare_exchange_n(target, &old_value, new_value,
					   0, __ATOMIC_SEQ_CST,
					   __ATOMIC_SEQ_CST);
}

/**
 *
 * @brief Atomic addition primitive
 *
 * This routine provides the atomic addition operator. The <value> is
 * atomically added to the value at <target>, placing the result at <target>,
 * and the old value from <target> is returned.
 *
 * @param target memory location to add to
 * @param value the value to add
 *
 * @return The previous value from <target>
 */
static inline atomic_val_t atomic_add(atomic_t *target, atomic_val_t value)
{
	return __atomic_fetch_add(target, value, __ATOMIC_SEQ_CST);
}

/**
 *
 * @brief Atomic subtraction primitive
 *
 * This routine provides the atomic subtraction operator. The <value> is
 * atomically subtracted from the value at <target>, placing the result at
 * <target>, and the old value from <target> is returned.
 *
 * @param target the memory location to subtract from
 * @param value the value to subtract
 *
 * @return The previous value from <target>
 */
static inline atomic_val_t atomic_sub(atomic_t *target, atomic_val_t value)
{
	return __atomic_fetch_sub(target, value, __ATOMIC_SEQ_CST);
}

/**
 *
 * @brief Atomic increment primitive
 *
 * @param target memory location to increment
 *
 * This routine provides the atomic increment operator. The value at <target>
 * is atomically incremented by 1, and the old value from <target> is returned.
 *
 * @return The value from <target> before the increment
 */
static inline atomic_val_t atomic_inc(atomic_t *target)
{
	return atomic_add(target, 1);
}

/**
 *
 * @brief Atomic decrement primitive
 *
 * @param target memory location to decrement
 *
 * This routine provides the atomic decrement operator. The value at <target>
 * is atomically decremented by 1, and the old value from <target> is returned.
 *
 * @return The value from <target> prior to the decrement
 */
static inline atomic_val_t atomic_dec(atomic_t *target)
{
	return atomic_sub(target, 1);
}

/**
 *
 * @brief Atomic get primitive
 *
 * @param target memory location to read from
 *
 * This routine provides the atomic get primitive to atomically read
 * a value from <target>. It simply does an ordinary load.  Note that <target>
 * is expected to be aligned to a 4-byte boundary.
 *
 * @return The value read from <target>
 */
static inline atomic_val_t atomic_get(const atomic_t *target)
{
	return __atomic_load_n(target, __ATOMIC_SEQ_CST);
}

/**
 *
 * @brief Atomic get-and-set primitive
 *
 * This routine provides the atomic set operator. The <value> is atomically
 * written at <target> and the previous value at <target> is returned.
 *
 * @param target the memory location to write to
 * @param value the value to write
 *
 * @return The previous value from <target>
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
 * @brief Atomic clear primitive
 *
 * This routine provides the atomic clear operator. The value of 0 is atomically
 * written at <target> and the previous value at <target> is returned. (Hence,
 * atomic_clear(pAtomicVar) is equivalent to atomic_set(pAtomicVar, 0).)
 *
 * @param target the memory location to write
 *
 * @return The previous value from <target>
 */
static inline atomic_val_t atomic_clear(atomic_t *target)
{
	return atomic_set(target, 0);
}

/**
 *
 * @brief Atomic bitwise inclusive OR primitive
 *
 * This routine provides the atomic bitwise inclusive OR operator. The <value>
 * is atomically bitwise OR'ed with the value at <target>, placing the result
 * at <target>, and the previous value at <target> is returned.
 *
 * @param target the memory location to be modified
 * @param value the value to OR
 *
 * @return The previous value from <target>
 */
static inline atomic_val_t atomic_or(atomic_t *target, atomic_val_t value)
{
	return __atomic_fetch_or(target, value, __ATOMIC_SEQ_CST);
}

/**
 *
 * @brief Atomic bitwise exclusive OR (XOR) primitive
 *
 * This routine provides the atomic bitwise exclusive OR operator. The <value>
 * is atomically bitwise XOR'ed with the value at <target>, placing the result
 * at <target>, and the previous value at <target> is returned.
 *
 * @param target the memory location to be modified
 * @param value the value to XOR
 *
 * @return The previous value from <target>
 */
static inline atomic_val_t atomic_xor(atomic_t *target, atomic_val_t value)
{
	return __atomic_fetch_xor(target, value, __ATOMIC_SEQ_CST);
}
/**
 *
 * @brief Atomic bitwise AND primitive
 *
 * This routine provides the atomic bitwise AND operator. The <value> is
 * atomically bitwise AND'ed with the value at <target>, placing the result
 * at <target>, and the previous value at <target> is returned.
 *
 * @param target the memory location to be modified
 * @param value the value to AND
 *
 * @return The previous value from <target>
 */
static inline atomic_val_t atomic_and(atomic_t *target, atomic_val_t value)
{
	return __atomic_fetch_and(target, value, __ATOMIC_SEQ_CST);
}

/**
 *
 * @brief Atomic bitwise NAND primitive
 *
 * This routine provides the atomic bitwise NAND operator. The <value> is
 * atomically bitwise NAND'ed with the value at <target>, placing the result
 * at <target>, and the previous value at <target> is returned.
 *
 * The operation here is equivalent to *target = ~(tmp & value)
 *
 * @param target the memory location to be modified
 * @param value the value to NAND
 *
 * @return The previous value from <target>
 */
static inline atomic_val_t atomic_nand(atomic_t *target, atomic_val_t value)
{
	return __atomic_fetch_nand(target, value, __ATOMIC_SEQ_CST);
}
#else
extern atomic_val_t atomic_add(atomic_t *target, atomic_val_t value);
extern atomic_val_t atomic_and(atomic_t *target, atomic_val_t value);
extern atomic_val_t atomic_dec(atomic_t *target);
extern atomic_val_t atomic_inc(atomic_t *target);
extern atomic_val_t atomic_nand(atomic_t *target, atomic_val_t value);
extern atomic_val_t atomic_or(atomic_t *target, atomic_val_t value);
extern atomic_val_t atomic_sub(atomic_t *target, atomic_val_t value);
extern atomic_val_t atomic_xor(atomic_t *target, atomic_val_t value);
extern atomic_val_t atomic_clear(atomic_t *target);
extern atomic_val_t atomic_get(const atomic_t *target);
extern atomic_val_t atomic_set(atomic_t *target, atomic_val_t value);
extern int atomic_cas(atomic_t *target, atomic_val_t oldValue,
		      atomic_val_t newValue);
#endif /* CONFIG_ATOMIC_OPERATIONS_BUILTIN */


#define ATOMIC_INIT(i) (i)

#define ATOMIC_BITS (sizeof(atomic_val_t) * 8)
#define ATOMIC_MASK(bit) (1 << ((bit) & (ATOMIC_BITS - 1)))
#define ATOMIC_ELEM(addr, bit) ((addr) + ((bit) / ATOMIC_BITS))

/** @def ATOMIC_DEFINE
 *  @brief Helper to declare an atomic_t array.
 *
 *  A helper to define an atomic_t array based on the number of needed
 *  bits, e.g. any bit count of 32 or less will produce a single-element
 *  array.
 *
 *  @param name Name of atomic_t array.
 *  @param num_bits Maximum number of bits needed.
 *
 *  @return n/a
 */
#define ATOMIC_DEFINE(name, num_bits) \
	atomic_t name[1 + ((num_bits) - 1) / ATOMIC_BITS]

/** @brief Test whether a bit is set
 *
 *  Test whether bit number bit is set or not.
 *
 *  Also works for an array of multiple atomic_t variables, in which
 *  case the bit number may go beyond the number of bits in a single
 *  atomic_t variable.
 *
 *  @param addr base address to start counting from
 *  @param bit bit number counted from the base address
 *
 *  @return 1 if the bit was set, 0 if it wasn't
 */
static inline int atomic_test_bit(const atomic_t *addr, int bit)
{
	atomic_val_t val = atomic_get(ATOMIC_ELEM(addr, bit));

	return (1 & (val >> (bit & (ATOMIC_BITS - 1))));
}

/** @brief Clear a bit and return its old value
 *
 *  Atomically clear a bit and return its old value.
 *
 *  Also works for an array of multiple atomic_t variables, in which
 *  case the bit number may go beyond the number of bits in a single
 *  atomic_t variable.
 *
 *  @param addr base address to start counting from
 *  @param bit bit number counted from the base address
 *
 *  @return 1 if the bit was set, 0 if it wasn't
 */
static inline int atomic_test_and_clear_bit(atomic_t *addr, int bit)
{
	atomic_val_t mask = ATOMIC_MASK(bit);
	atomic_val_t old;

	old = atomic_and(ATOMIC_ELEM(addr, bit), ~mask);

	return (old & mask) != 0;
}

/** @brief Set a bit and return its old value
 *
 *  Atomically set a bit and return its old value.
 *
 *  Also works for an array of multiple atomic_t variables, in which
 *  case the bit number may go beyond the number of bits in a single
 *  atomic_t variable.
 *
 *  @param addr base address to start counting from
 *  @param bit bit number counted from the base address
 *
 *  @return 1 if the bit was set, 0 if it wasn't
 */
static inline int atomic_test_and_set_bit(atomic_t *addr, int bit)
{
	atomic_val_t mask = ATOMIC_MASK(bit);
	atomic_val_t old;

	old = atomic_or(ATOMIC_ELEM(addr, bit), mask);

	return (old & mask) != 0;
}

/** @brief Clear a bit
 *
 *  Atomically clear a bit.
 *
 *  Also works for an array of multiple atomic_t variables, in which
 *  case the bit number may go beyond the number of bits in a single
 *  atomic_t variable.
 *
 *  @param addr base address to start counting from
 *  @param bit bit number counted from the base address
 */
static inline void atomic_clear_bit(atomic_t *addr, int bit)
{
	atomic_val_t mask = ATOMIC_MASK(bit);

	atomic_and(ATOMIC_ELEM(addr, bit), ~mask);
}

/** @brief Set a bit
 *
 *  Atomically set a bit.
 *
 *  Also works for an array of multiple atomic_t variables, in which
 *  case the bit number may go beyond the number of bits in a single
 *  atomic_t variable.
 *
 *  @param addr base address to start counting from
 *  @param bit bit number counted from the base address
 */
static inline void atomic_set_bit(atomic_t *addr, int bit)
{
	atomic_val_t mask = ATOMIC_MASK(bit);

	atomic_or(ATOMIC_ELEM(addr, bit), mask);
}

#ifdef __cplusplus
}
#endif

#endif /* __ATOMIC_H__ */
