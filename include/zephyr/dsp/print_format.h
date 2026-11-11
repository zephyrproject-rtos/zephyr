/* Copyright (c) 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DSP_PRINT_FORMAT_H_
#define ZEPHYR_INCLUDE_DSP_PRINT_FORMAT_H_

#include <inttypes.h>
#include <stdlib.h>
#include <zephyr/dsp/types.h>
#include <zephyr/sys/util.h>

/**
 * @ingroup math_dsp
 * @defgroup math_printing Helper macros for printing Q values.
 *
 * Extends the existing inttypes headers for print formatting. Usage:
 * @code{c}
 * printk("Value=%" PRIq(6) "\n", PRIq_arg(value, 6, 2));
 * @endcode
 *
 * For a Q value representing 0.5, the expected output will be:
 *   "Value=2.000000"
 *
 * @{
 */

/**
 * @brief Insert Q value format string
 *
 * @param[in] precision Number of decimal places to print, from 1 to 9
 */
#define PRIq(precision) "s%" PRIu32 ".%0" STRINGIFY(precision) PRIu32

static inline int64_t ___PRIq_arg_shift(int64_t q, int shift)
{
	if (shift < 0) {
		return llabs(q) >> -shift;
	} else {
		return llabs(q) << shift;
	}
}

#define __EXP2(a, b) a ## b
#define __EXP(a, b) __EXP2(a ## e, b)
#define __CONSTPOW(C, x) __EXP(C, x)

#define __PRIq_arg_shift(q, shift)     ___PRIq_arg_shift(q, ((shift) + (8 * (4 - (int)sizeof(q)))))
#define __PRIq_arg_get(q, shift, h, l) FIELD_GET(GENMASK64(h, l), __PRIq_arg_shift(q, shift))
#define __PRIq_arg_get_int(q, shift)   __PRIq_arg_get(q, shift, 63, 31)
#define __PRIq_arg_get_frac(q, precision, shift)                                                   \
	(((__PRIq_arg_get(q, shift, 30, 0) * (uint64_t)__CONSTPOW(1, precision)) >> 31) +          \
	 ZERO_OR_COMPILE_ERROR(IN_RANGE(precision, 1, 9)))

/**
 * @brief Insert Q value arguments to print format
 *
 * The value is truncated toward zero to @p precision decimal places.
 *
 * @note @p q and @p shift are evaluated more than once, so they must not be expressions with
 *       side effects.
 *
 * @param[in] q The q value
 * @param[in] precision Number of decimal places to print, from 1 to 9
 * @param[in] shift The "scale" to shift @p q by, from -63 to 31
 */
#define PRIq_arg(q, precision, shift)                                                              \
	((q) < 0 ? "-" : ""), (uint32_t)__PRIq_arg_get_int(q, shift),                              \
		(uint32_t)__PRIq_arg_get_frac(q, precision, shift)

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DSP_PRINT_FORMAT_H_ */
