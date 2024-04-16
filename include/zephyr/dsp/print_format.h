/* Copyright (c) 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_DSP_PRINT_FORMAT_H
#define ZEPHYR_INCLUDE_ZEPHYR_DSP_PRINT_FORMAT_H

#include <inttypes.h>
#include <stdlib.h>
#include <zephyr/dsp/types.h>

/**
 * @ingroup math_dsp
 * @defgroup math_printing Helper macros for printing Q values.
 *
 * Extends the existing inttypes headers for print formatting. Useage:
 * @code{c}
 * printk("Value=%" PRIq "\n", PRIq_arg(value, 6, 2));
 * @endcode
 *
 * For a Q value representing 0.5, the expected output will be:
 *   "Value=2.000000"
 *
 * @{
 */

/**
 * @brief Insert Q value format string
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
	((__PRIq_arg_get(q, shift, 30, 0) * __CONSTPOW(1, precision)) / INT32_MAX)

/**
 * @brief Insert Q value arguments to print format
 *
 * @param[in] q The q value
 * @param[in] precision Number of decimal points to print
 * @param[in] shift The "scale" to shift @p q by
 */
#define PRIq_arg(q, precision, shift)                                                              \
	((q) < 0 ? "-" : ""), (uint32_t)__PRIq_arg_get_int(q, shift),                              \
		(uint32_t)__PRIq_arg_get_frac(q, precision, shift)

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_ZEPHYR_DSP_PRINT_FORMAT_H */
