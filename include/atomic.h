/* atomic operations */

/*
 * Copyright (c) 1997-2015, Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __ATOMIC_H__
#define __ATOMIC_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef int atomic_t;
typedef atomic_t atomic_val_t;

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
extern int atomic_cas(atomic_t *target,
					  atomic_val_t oldValue, atomic_val_t newValue);


#define ATOMIC_INIT(i) {(i)}

#define ATOMIC_BITS (sizeof(atomic_val_t) * 8)
#define ATOMIC_MASK(bit) (1 << ((bit) & (ATOMIC_BITS - 1)))
#define ATOMIC_ELEM(addr, bit) ((addr) + ((bit) / ATOMIC_BITS))

/*! @brief Test whether a bit is set
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

/*! @brief Clear a bit and return its old value
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

/*! @brief Set a bit and return its old value
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

/*! @brief Clear a bit
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

/*! @brief Set a bit
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
