/*
 * Copyright (c) 2024, Joel Hirsbrunner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @cond INTERNAL_HIDDEN
 */

#ifndef ZEPHYR_INCLUDE_SYS_UTIL_INTERNAL_MATH_H_
#define ZEPHYR_INCLUDE_SYS_UTIL_INTERNAL_MATH_H_

#ifndef ZEPHYR_INCLUDE_SYS_UTIL_INTERNAL_H_
#error "This header should not be used directly, please include util_internal.h instead"
#endif /* ZEPHYR_INCLUDE_SYS_UTIL_INTERNAL_H_ */

#include "util_internal_number_to_bits.h"

#define Z_DO_UTIL_INTERNAL_BIT_OP_12(fn, v1, v2, c, ...) (__VA_ARGS__), c
#define Z_UTIL_INTERNAL_BIT_OP_12(fn, v1, v2, ...) \
	Z_DO_UTIL_INTERNAL_BIT_OP_12(fn, v1, v2, __VA_ARGS__)
#define Z_DO_UTIL_INTERNAL_BIT_OP_11(fn, v1, v2, c, ...) \
	Z_UTIL_INTERNAL_BIT_OP_12(fn, v1, v2, fn(Z_GET_BIT_N(11, v1), Z_GET_BIT_N(11, v2), c), ## __VA_ARGS__)
#define Z_UTIL_INTERNAL_BIT_OP_11(fn, v1, v2, ...) \
	Z_DO_UTIL_INTERNAL_BIT_OP_11(fn, v1, v2, __VA_ARGS__)
#define Z_DO_UTIL_INTERNAL_BIT_OP_10(fn, v1, v2, c, ...) \
	Z_UTIL_INTERNAL_BIT_OP_11(fn, v1, v2, fn(Z_GET_BIT_N(10, v1), Z_GET_BIT_N(10, v2), c), ## __VA_ARGS__)
#define Z_UTIL_INTERNAL_BIT_OP_10(fn, v1, v2, ...) \
	Z_DO_UTIL_INTERNAL_BIT_OP_10(fn, v1, v2, __VA_ARGS__)
#define Z_DO_UTIL_INTERNAL_BIT_OP_9(fn, v1, v2, c, ...) \
	Z_UTIL_INTERNAL_BIT_OP_10(fn, v1, v2, fn(Z_GET_BIT_N(9, v1), Z_GET_BIT_N(9, v2), c), ## __VA_ARGS__)
#define Z_UTIL_INTERNAL_BIT_OP_9(fn, v1, v2, ...) \
	Z_DO_UTIL_INTERNAL_BIT_OP_9(fn, v1, v2, __VA_ARGS__)
#define Z_DO_UTIL_INTERNAL_BIT_OP_8(fn, v1, v2, c, ...) \
	Z_UTIL_INTERNAL_BIT_OP_9(fn, v1, v2, fn(Z_GET_BIT_N(8, v1), Z_GET_BIT_N(8, v2), c), ## __VA_ARGS__)
#define Z_UTIL_INTERNAL_BIT_OP_8(fn, v1, v2, ...) \
	Z_DO_UTIL_INTERNAL_BIT_OP_8(fn, v1, v2, __VA_ARGS__)
#define Z_DO_UTIL_INTERNAL_BIT_OP_7(fn, v1, v2, c, ...) \
	Z_UTIL_INTERNAL_BIT_OP_8(fn, v1, v2, fn(Z_GET_BIT_N(7, v1), Z_GET_BIT_N(7, v2), c), ## __VA_ARGS__)
#define Z_UTIL_INTERNAL_BIT_OP_7(fn, v1, v2, ...) \
	Z_DO_UTIL_INTERNAL_BIT_OP_7(fn, v1, v2, __VA_ARGS__)
#define Z_DO_UTIL_INTERNAL_BIT_OP_6(fn, v1, v2, c, ...) \
	Z_UTIL_INTERNAL_BIT_OP_7(fn, v1, v2, fn(Z_GET_BIT_N(6, v1), Z_GET_BIT_N(6, v2), c), ## __VA_ARGS__)
#define Z_UTIL_INTERNAL_BIT_OP_6(fn, v1, v2, ...) \
	Z_DO_UTIL_INTERNAL_BIT_OP_6(fn, v1, v2, __VA_ARGS__)
#define Z_DO_UTIL_INTERNAL_BIT_OP_5(fn, v1, v2, c, ...) \
	Z_UTIL_INTERNAL_BIT_OP_6(fn, v1, v2, fn(Z_GET_BIT_N(5, v1), Z_GET_BIT_N(5, v2), c), ## __VA_ARGS__)
#define Z_UTIL_INTERNAL_BIT_OP_5(fn, v1, v2, ...) \
	Z_DO_UTIL_INTERNAL_BIT_OP_5(fn, v1, v2, __VA_ARGS__)
#define Z_DO_UTIL_INTERNAL_BIT_OP_4(fn, v1, v2, c, ...) \
	Z_UTIL_INTERNAL_BIT_OP_5(fn, v1, v2, fn(Z_GET_BIT_N(4, v1), Z_GET_BIT_N(4, v2), c), ## __VA_ARGS__)
#define Z_UTIL_INTERNAL_BIT_OP_4(fn, v1, v2, ...) \
	Z_DO_UTIL_INTERNAL_BIT_OP_4(fn, v1, v2, __VA_ARGS__)
#define Z_DO_UTIL_INTERNAL_BIT_OP_3(fn, v1, v2, c, ...) \
	Z_UTIL_INTERNAL_BIT_OP_4(fn, v1, v2, fn(Z_GET_BIT_N(3, v1), Z_GET_BIT_N(3, v2), c), ## __VA_ARGS__)
#define Z_UTIL_INTERNAL_BIT_OP_3(fn, v1, v2, ...) \
	Z_DO_UTIL_INTERNAL_BIT_OP_3(fn, v1, v2, __VA_ARGS__)
#define Z_DO_UTIL_INTERNAL_BIT_OP_2(fn, v1, v2, c, ...) \
	Z_UTIL_INTERNAL_BIT_OP_3(fn, v1, v2, fn(Z_GET_BIT_N(2, v1), Z_GET_BIT_N(2, v2), c), ## __VA_ARGS__)
#define Z_UTIL_INTERNAL_BIT_OP_2(fn, v1, v2, ...) \
	Z_DO_UTIL_INTERNAL_BIT_OP_2(fn, v1, v2, __VA_ARGS__)
#define Z_DO_UTIL_INTERNAL_BIT_OP_1(fn, v1, v2, c, ...) \
	Z_UTIL_INTERNAL_BIT_OP_2(fn, v1, v2, fn(Z_GET_BIT_N(1, v1), Z_GET_BIT_N(1, v2), c), ## __VA_ARGS__)
#define Z_UTIL_INTERNAL_BIT_OP_1(fn, v1, v2, ...) \
	Z_DO_UTIL_INTERNAL_BIT_OP_1(fn, v1, v2, __VA_ARGS__)
#define Z_DO_UTIL_INTERNAL_BIT_OP_0(fn, v1, v2, c, ...) \
	Z_UTIL_INTERNAL_BIT_OP_1(fn, v1, v2, fn(Z_GET_BIT_N(0, v1), Z_GET_BIT_N(0, v2), c), ## __VA_ARGS__)
#define Z_UTIL_INTERNAL_BIT_OP_0(fn, v1, v2, ...) \
	Z_DO_UTIL_INTERNAL_BIT_OP_0(fn, v1, v2, __VA_ARGS__)

/**
 * @brief Execute `fn` for every 12 bit in `v1` and `v2`.
 * @param fn Macro to evoke on every iteration. It takes a bit of `v1` and
 *           `v2`, and a carry bit `c` as its input arguments. The return
 *           must be the result `r` and the carry bit `c` of the operation.
 *           The order must be `c`, `r`.
 * @param v1, v2 These are 12 bit numbers on which a bitwise operation is
 *               performed. Use Z_NUMBER_TO_BITS to convert from a literal
 *               integer to a 12 bit number.
 * @returns
 * A 12 bit number representing the result `r` of the bitwise operation.
 * Use Z_BITS_TO_NUMBER to convert from a 12 bit number to an integer
 * literal. Additionally, the last carry bit `c` is also returned.
 * The order is `r`, `c`.
 */
#define Z_UTIL_INTERNAL_BIT_OP(fn, v1, v2) Z_UTIL_INTERNAL_BIT_OP_0(fn, v1, v2, 0)

#define Z_UTIL_INTERNAL_MATH_GET_CARRY(r, c, ...)  c
#define Z_UTIL_INTERNAL_MATH_GET_RESULT(r, c, ...) r

#define Z_UTIL_INTERNAL_MATH_CARRY(...)  Z_UTIL_INTERNAL_MATH_GET_CARRY(__VA_ARGS__)
#define Z_UTIL_INTERNAL_MATH_RESULT(...) Z_UTIL_INTERNAL_MATH_GET_RESULT(__VA_ARGS__)

/* Z_UTIL_INTERNAL_<OP>_<v1> c, r (c and v2 are ignored) */
#define Z_UTIL_INTERNAL_BIT_INV_0 0, 1
#define Z_UTIL_INTERNAL_BIT_INV_1 0, 0

/* Z_UTIL_INTERNAL_<OP>_<v1><v2> c, r (c is ignored) */
#define Z_UTIL_INTERNAL_BIT_AND_00 0, 0
#define Z_UTIL_INTERNAL_BIT_AND_01 0, 0
#define Z_UTIL_INTERNAL_BIT_AND_10 0, 0
#define Z_UTIL_INTERNAL_BIT_AND_11 0, 1
#define Z_UTIL_INTERNAL_BIT_OR_00  0, 0
#define Z_UTIL_INTERNAL_BIT_OR_01  0, 1
#define Z_UTIL_INTERNAL_BIT_OR_10  0, 1
#define Z_UTIL_INTERNAL_BIT_OR_11  0, 1
#define Z_UTIL_INTERNAL_BIT_XOR_00 0, 0
#define Z_UTIL_INTERNAL_BIT_XOR_01 0, 1
#define Z_UTIL_INTERNAL_BIT_XOR_10 0, 1
#define Z_UTIL_INTERNAL_BIT_XOR_11 0, 0

/* Z_UTIL_INTERNAL_<OP>_<v1><v2><c>  c, r */
#define Z_UTIL_INTERNAL_ADD_000 0, 0
#define Z_UTIL_INTERNAL_ADD_001 0, 1
#define Z_UTIL_INTERNAL_ADD_010 0, 1
#define Z_UTIL_INTERNAL_ADD_011 1, 0
#define Z_UTIL_INTERNAL_ADD_100 0, 1
#define Z_UTIL_INTERNAL_ADD_101 1, 0
#define Z_UTIL_INTERNAL_ADD_110 1, 0
#define Z_UTIL_INTERNAL_ADD_111 1, 1

#define Z_UTIL_INTERNAL_SUB_000 0, 0
#define Z_UTIL_INTERNAL_SUB_001 1, 1
#define Z_UTIL_INTERNAL_SUB_010 1, 1
#define Z_UTIL_INTERNAL_SUB_011 1, 0
#define Z_UTIL_INTERNAL_SUB_100 0, 1
#define Z_UTIL_INTERNAL_SUB_101 0, 0
#define Z_UTIL_INTERNAL_SUB_110 0, 0
#define Z_UTIL_INTERNAL_SUB_111 1, 1

/* Shift operation */
#define Z_UTIL_INTERNAL_MATH_LSH(N, a) Z_UTIL_INTERNAL_MATH_LSH_##N a
#define Z_UTIL_INTERNAL_MATH_LSH_0(_11, _10, _9, _8, _7, _6, _5, _4, _3, _2, _1, _0)  (_11, _10, _9, _8, _7, _6, _5, _4, _3, _2, _1, _0)
#define Z_UTIL_INTERNAL_MATH_LSH_1(_11, _10, _9, _8, _7, _6, _5, _4, _3, _2, _1, _0)  (_10,  _9, _8, _7, _6, _5, _4, _3, _2, _1, _0,  0)
#define Z_UTIL_INTERNAL_MATH_LSH_2(_11, _10, _9, _8, _7, _6, _5, _4, _3, _2, _1, _0)  ( _9,  _8, _7, _6, _5, _4, _3, _2, _1, _0,  0,  0)
#define Z_UTIL_INTERNAL_MATH_LSH_3(_11, _10, _9, _8, _7, _6, _5, _4, _3, _2, _1, _0)  ( _8,  _7, _6, _5, _4, _3, _2, _1, _0,  0,  0,  0)
#define Z_UTIL_INTERNAL_MATH_LSH_4(_11, _10, _9, _8, _7, _6, _5, _4, _3, _2, _1, _0)  ( _7,  _6, _5, _4, _3, _2, _1, _0,  0,  0,  0,  0)
#define Z_UTIL_INTERNAL_MATH_LSH_5(_11, _10, _9, _8, _7, _6, _5, _4, _3, _2, _1, _0)  ( _6,  _5, _4, _3, _2, _1, _0,  0,  0,  0,  0,  0)
#define Z_UTIL_INTERNAL_MATH_LSH_6(_11, _10, _9, _8, _7, _6, _5, _4, _3, _2, _1, _0)  ( _5,  _4, _3, _2, _1, _0,  0,  0,  0,  0,  0,  0)
#define Z_UTIL_INTERNAL_MATH_LSH_7(_11, _10, _9, _8, _7, _6, _5, _4, _3, _2, _1, _0)  ( _4,  _3, _2, _1, _0,  0,  0,  0,  0,  0,  0,  0)
#define Z_UTIL_INTERNAL_MATH_LSH_8(_11, _10, _9, _8, _7, _6, _5, _4, _3, _2, _1, _0)  ( _3,  _2, _1, _0,  0,  0,  0,  0,  0,  0,  0,  0)
#define Z_UTIL_INTERNAL_MATH_LSH_9(_11, _10, _9, _8, _7, _6, _5, _4, _3, _2, _1, _0)  ( _2,  _1, _0,  0,  0,  0,  0,  0,  0,  0,  0,  0)
#define Z_UTIL_INTERNAL_MATH_LSH_10(_11, _10, _9, _8, _7, _6, _5, _4, _3, _2, _1, _0) ( _1,  _0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0)
#define Z_UTIL_INTERNAL_MATH_LSH_11(_11, _10, _9, _8, _7, _6, _5, _4, _3, _2, _1, _0) ( _0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0)

#define Z_DO_UTIL_INTERNAL_MATH_BIT_INV(v1)     Z_UTIL_INTERNAL_BIT_INV_##v1
#define Z_UTIL_INTERNAL_MATH_BIT_INV(v1, v2, c) Z_DO_UTIL_INTERNAL_MATH_BIT_INV(v1)

#define Z_DO_UTIL_INTERNAL_MATH_BIT_AND(v1, v2) Z_UTIL_INTERNAL_BIT_AND_##v1##v2
#define Z_UTIL_INTERNAL_MATH_BIT_AND(v1, v2, c) Z_DO_UTIL_INTERNAL_MATH_BIT_AND(v1, v2)
#define Z_DO_UTIL_INTERNAL_MATH_BIT_OR(v1, v2)  Z_UTIL_INTERNAL_BIT_OR_##v1##v2
#define Z_UTIL_INTERNAL_MATH_BIT_OR(v1, v2, c)  Z_DO_UTIL_INTERNAL_MATH_BIT_OR(v1, v2)
#define Z_DO_UTIL_INTERNAL_MATH_BIT_XOR(v1, v2) Z_UTIL_INTERNAL_BIT_XOR_##v1##v2
#define Z_UTIL_INTERNAL_MATH_BIT_XOR(v1, v2, c) Z_DO_UTIL_INTERNAL_MATH_BIT_XOR(v1, v2)

#define Z_DO_UTIL_INTERNAL_MATH_ADD(v1, v2, c) Z_UTIL_INTERNAL_ADD_##v1##v2##c
#define Z_UTIL_INTERNAL_MATH_ADD(v1, v2, c)    Z_DO_UTIL_INTERNAL_MATH_ADD(v1, v2, c)
#define Z_DO_UTIL_INTERNAL_MATH_SUB(v1, v2, c) Z_UTIL_INTERNAL_SUB_##v1##v2##c
#define Z_UTIL_INTERNAL_MATH_SUB(v1, v2, c)    Z_DO_UTIL_INTERNAL_MATH_SUB(v1, v2, c)

#define Z_DO_UTIL_BIT_INV(a) Z_UTIL_INTERNAL_BIT_OP(Z_UTIL_INTERNAL_MATH_BIT_INV, Z_NUMBER_TO_BITS(a), Z_NUMBER_TO_BITS(a))
#define Z_UTIL_BIT_INV(a)    Z_BITS_TO_NUMBER(Z_UTIL_INTERNAL_MATH_RESULT(Z_DO_UTIL_BIT_INV(a)))

#define Z_DO_UTIL_BIT_AND(a, b) Z_UTIL_INTERNAL_BIT_OP(Z_UTIL_INTERNAL_MATH_BIT_AND, Z_NUMBER_TO_BITS(a), Z_NUMBER_TO_BITS(b))
#define Z_UTIL_BIT_AND(a, b)    Z_BITS_TO_NUMBER(Z_UTIL_INTERNAL_MATH_RESULT(Z_DO_UTIL_BIT_AND(a, b)))
#define Z_DO_UTIL_BIT_OR(a, b)  Z_UTIL_INTERNAL_BIT_OP(Z_UTIL_INTERNAL_MATH_BIT_OR, Z_NUMBER_TO_BITS(a), Z_NUMBER_TO_BITS(b))
#define Z_UTIL_BIT_OR(a, b)     Z_BITS_TO_NUMBER(Z_UTIL_INTERNAL_MATH_RESULT(Z_DO_UTIL_BIT_OR(a, b)))
#define Z_DO_UTIL_BIT_XOR(a, b) Z_UTIL_INTERNAL_BIT_OP(Z_UTIL_INTERNAL_MATH_BIT_XOR, Z_NUMBER_TO_BITS(a), Z_NUMBER_TO_BITS(b))
#define Z_UTIL_BIT_XOR(a, b)    Z_BITS_TO_NUMBER(Z_UTIL_INTERNAL_MATH_RESULT(Z_DO_UTIL_BIT_XOR(a, b)))

#define Z_DO_UTIL_ADD(a, b) Z_UTIL_INTERNAL_BIT_OP(Z_UTIL_INTERNAL_MATH_ADD, Z_NUMBER_TO_BITS(a), Z_NUMBER_TO_BITS(b))
#define Z_UTIL_ADD(a, b)    Z_BITS_TO_NUMBER(Z_UTIL_INTERNAL_MATH_RESULT(Z_DO_UTIL_ADD(a, b)))
#define Z_DO_UTIL_SUB(a, b) Z_UTIL_INTERNAL_BIT_OP(Z_UTIL_INTERNAL_MATH_SUB, Z_NUMBER_TO_BITS(a), Z_NUMBER_TO_BITS(b))
#define Z_UTIL_SUB(a, b)    Z_BITS_TO_NUMBER(Z_UTIL_INTERNAL_MATH_RESULT(Z_DO_UTIL_SUB(a, b)))

#define Z_IS_LT(a, b) Z_UTIL_INTERNAL_MATH_CARRY(Z_DO_UTIL_SUB(a, b))

#define Z_DO_UTIL_LSH(a, s) Z_UTIL_INTERNAL_MATH_LSH(s, Z_NUMBER_TO_BITS(a))
#define Z_UTIL_LSH(a, s)    COND_CODE_1(Z_IS_LT(s, 12), (Z_BITS_TO_NUMBER(Z_DO_UTIL_LSH(a, s))), (0))
#define Z_DO_UTIL_RSH(a, s) Z_REVERSE_BITS(Z_UTIL_INTERNAL_MATH_LSH(s, Z_REVERSE_BITS(Z_NUMBER_TO_BITS(a))))
#define Z_UTIL_RSH(a, s)    COND_CODE_1(Z_IS_LT(s, 12), (Z_BITS_TO_NUMBER(Z_DO_UTIL_RSH(a, s))), (0))

#define Z_DO_UTIL_INTERNAL_MAC_0(fn, acc, add, i, v1, v2) \
	Z_UTIL_ADD(acc, add)
#define Z_UTIL_INTERNAL_MAC_0(fn, acc, ...) \
	Z_DO_UTIL_INTERNAL_MAC_0(fn, acc, __VA_ARGS__)
#define Z_DO_UTIL_INTERNAL_MAC_1(fn, acc, add, i, v1, v2) \
	Z_UTIL_INTERNAL_MAC_0(fn, Z_UTIL_ADD(acc, add), fn(i, v1, v2))
#define Z_UTIL_INTERNAL_MAC_1(fn, acc, ...) \
	Z_DO_UTIL_INTERNAL_MAC_1(fn, acc, __VA_ARGS__)
#define Z_DO_UTIL_INTERNAL_MAC_2(fn, acc, add, i, v1, v2) \
	Z_UTIL_INTERNAL_MAC_1(fn, Z_UTIL_ADD(acc, add), fn(i, v1, v2))
#define Z_UTIL_INTERNAL_MAC_2(fn, acc, ...) \
	Z_DO_UTIL_INTERNAL_MAC_2(fn, acc, __VA_ARGS__)
#define Z_DO_UTIL_INTERNAL_MAC_3(fn, acc, add, i, v1, v2) \
	Z_UTIL_INTERNAL_MAC_2(fn, Z_UTIL_ADD(acc, add), fn(i, v1, v2))
#define Z_UTIL_INTERNAL_MAC_3(fn, acc, ...) \
	Z_DO_UTIL_INTERNAL_MAC_3(fn, acc, __VA_ARGS__)
#define Z_DO_UTIL_INTERNAL_MAC_4(fn, acc, add, i, v1, v2) \
	Z_UTIL_INTERNAL_MAC_3(fn, Z_UTIL_ADD(acc, add), fn(i, v1, v2))
#define Z_UTIL_INTERNAL_MAC_4(fn, acc, ...) \
	Z_DO_UTIL_INTERNAL_MAC_4(fn, acc, __VA_ARGS__)
#define Z_DO_UTIL_INTERNAL_MAC_5(fn, acc, add, i, v1, v2) \
	Z_UTIL_INTERNAL_MAC_4(fn, Z_UTIL_ADD(acc, add), fn(i, v1, v2))
#define Z_UTIL_INTERNAL_MAC_5(fn, acc, ...) \
	Z_DO_UTIL_INTERNAL_MAC_5(fn, acc, __VA_ARGS__)
#define Z_DO_UTIL_INTERNAL_MAC_6(fn, acc, add, i, v1, v2) \
	Z_UTIL_INTERNAL_MAC_5(fn, Z_UTIL_ADD(acc, add), fn(i, v1, v2))
#define Z_UTIL_INTERNAL_MAC_6(fn, acc, ...) \
	Z_DO_UTIL_INTERNAL_MAC_6(fn, acc, __VA_ARGS__)
#define Z_DO_UTIL_INTERNAL_MAC_7(fn, acc, add, i, v1, v2) \
	Z_UTIL_INTERNAL_MAC_6(fn, Z_UTIL_ADD(acc, add), fn(i, v1, v2))
#define Z_UTIL_INTERNAL_MAC_7(fn, acc, ...) \
	Z_DO_UTIL_INTERNAL_MAC_7(fn, acc, __VA_ARGS__)
#define Z_DO_UTIL_INTERNAL_MAC_8(fn, acc, add, i, v1, v2) \
	Z_UTIL_INTERNAL_MAC_7(fn, Z_UTIL_ADD(acc, add), fn(i, v1, v2))
#define Z_UTIL_INTERNAL_MAC_8(fn, acc, ...) \
	Z_DO_UTIL_INTERNAL_MAC_8(fn, acc, __VA_ARGS__)
#define Z_DO_UTIL_INTERNAL_MAC_9(fn, acc, add, i, v1, v2) \
	Z_UTIL_INTERNAL_MAC_8(fn, Z_UTIL_ADD(acc, add), fn(i, v1, v2))
#define Z_UTIL_INTERNAL_MAC_9(fn, acc, ...) \
	Z_DO_UTIL_INTERNAL_MAC_9(fn, acc, __VA_ARGS__)
#define Z_DO_UTIL_INTERNAL_MAC_10(fn, acc, add, i, v1, v2) \
	Z_UTIL_INTERNAL_MAC_9(fn, Z_UTIL_ADD(acc, add), fn(i, v1, v2))
#define Z_UTIL_INTERNAL_MAC_10(fn, acc, ...) \
	Z_DO_UTIL_INTERNAL_MAC_10(fn, acc, __VA_ARGS__)
#define Z_DO_UTIL_INTERNAL_MAC_11(fn, acc, add, i, v1, v2) \
	Z_UTIL_INTERNAL_MAC_10(fn, Z_UTIL_ADD(acc, add), fn(i, v1, v2))
#define Z_UTIL_INTERNAL_MAC_11(fn, acc, ...) \
	Z_DO_UTIL_INTERNAL_MAC_11(fn, acc, __VA_ARGS__)

/**
 * @brief Mathematical Accumulator for multiplication and division.
 *
 * @param fn Takes the current index, value `v1`, and value `v2` as an
 *           input argument. The macro must return a value that is added
 *           to the accumulator, the next index for the next iteration,
 *           and the values for `v1` and `v2`. `v1` and `v2` may be
 *           changed in `fn`.
 * @param v1, v2 These are integer literals, the values to process in `fn`.
 * @param i_start This is the start index that is given to `fn`.
 *
 * @returns
 * Returns  the result of the performed operation as an integer literal.
 */
#define Z_UTIL_INTERNAL_MAC(fn, v1, v2, i_start) Z_UTIL_INTERNAL_MAC_11(fn, 0, fn(i_start, v1, v2))

/**
 * Multiplication
 *
 * A bitwise multiplication is fairly simple. In terms of c-code, it is
 * implemented as follows.
 *
 * static uint16_t mult_12bit(uint16_t v1, uint16_t v2)
 * {
 *     uint16_t res = 0;
 *     for (int i = 0; i < 12; i++) {
 *         if (0 == (v1 & (1 << i))) {
 *             continue;
 *         }
 *         res += (v2 << i);
 *     }
 *     return res;
 * }
 *
 * @returns
 * `add`, `i`, `v1`, `v2`
 */
#define Z_UTIL_INTERNAL_MATH_MUL(i, v1, v2) \
	COND_CODE_1(Z_GET_BIT_N(i, Z_NUMBER_TO_BITS(v1)), \
		    (Z_UTIL_LSH(v2, i)), \
		    (0)), \
	UTIL_INC(i), v1, v2
/**
 * Division
 *
 * A bitwise division is a bit more complicated. However, in the essence,
 * it's the same as with the multiplication. Following the c-code equialent.
 *
 * static uint16_t div_12bit(uint16_t v1, uint16_t v2)
 * {
 *     uint32_t res = 0;
 *     for (int i = 11; i >= 0; i--) {
 *         if ((v1 >> i) < v2) {
 *             continue;
 *         }
 *         v1 -= v2 << i;
 *         res |= 1 << i;
 *     }
 *     return res;
 * }
 *
 * @returns
 * `add`, `i`, `v1`, `v2`
 */
#define Z_UTIL_INTERNAL_MATH_DIV(i, v1, v2) \
	COND_CODE_1(Z_IS_LT(Z_UTIL_RSH(v1, i), v2), \
		    (0, UTIL_DEC(i), v1, v2), \
		    (Z_UTIL_LSH(1, i), UTIL_DEC(i), Z_UTIL_SUB(v1, Z_UTIL_LSH(v2, i)), v2))

#define Z_UTIL_MUL(a, b) Z_UTIL_INTERNAL_MAC(Z_UTIL_INTERNAL_MATH_MUL, a, b, 0)
#define Z_UTIL_DIV(a, b) Z_UTIL_INTERNAL_MAC(Z_UTIL_INTERNAL_MATH_DIV, a, b, 11)

#endif /* ZEPHYR_INCLUDE_SYS_UTIL_INTERNAL_MATH_H_ */

/**
 * INTERNAL_HIDDEN @endcond
 */
