/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MP_UTILS_H__
#define __MP_UTILS_H__

#include <ctype.h>
#include <stdbool.h>

/**
 * @brief Returns the sign of a number.
 *
 * @param x The input value to determine the sign
 * @return 1 if x is positive, -1 if x is negative, 0 if x is zero
 */
#define MP_SIGN(x) ((x > 0) - (x < 0))

/**
 * @brief Calculates the greatest common divisor (GCD) of two integers
 * using the Euclidean algorithm.
 *
 * @param a First integer
 * @param b Second integer
 * @return The greatest common divisor of a and b, always returns a positive value.
 *         If one parameter is 0, returns the absolute value of the other parameter.
 */
int mp_util_gcd(int a, int b);

/**
 * @brief Compute the least common multiple (LCM) of two integers.
 *
 * @param a First integer
 * @param b Second integer
 *
 * @retval The least common multiple of a and b.
 * @retval 0 if either input is 0.
 * @retval -1 if overflow occurs.
 */
int mp_util_lcm(int a, int b);

#endif /* __MP_UTILS_H__ */
