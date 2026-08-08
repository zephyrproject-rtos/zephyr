/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TESTS_SUBSYS_MP_CORE_UNIT_SRC_MP_TEST_HELPERS_H_
#define TESTS_SUBSYS_MP_CORE_UNIT_SRC_MP_TEST_HELPERS_H_

#include <zephyr/mp/mp_value.h>
#include <zephyr/ztest.h>

#define validate_boolean_value(value, expected)                                                    \
	({                                                                                         \
		zassert_not_null(value);                                                           \
		zassert_equal((value)->type, MP_TYPE_BOOLEAN);                                     \
		zassert_equal(mp_value_get_boolean(value), expected);                              \
	})

#define validate_int_value(value, expected)                                                        \
	({                                                                                         \
		zassert_not_null(value);                                                           \
		zassert_equal((value)->type, MP_TYPE_INT);                                         \
		zassert_equal(mp_value_get_int(value), expected);                                  \
	})

#define validate_uint_value(value, expected)                                                       \
	({                                                                                         \
		zassert_not_null(value);                                                           \
		zassert_equal((value)->type, MP_TYPE_UINT);                                        \
		zassert_equal(mp_value_get_uint(value), expected);                                 \
	})

#define validate_string_value(value, expected)                                                     \
	({                                                                                         \
		zassert_not_null(value);                                                           \
		zassert_equal((value)->type, MP_TYPE_STRING);                                      \
		zassert_str_equal(mp_value_get_string(value), expected);                           \
	})

#define validate_int_fraction_value(value, num, denom)                                             \
	({                                                                                         \
		zassert_not_null(value);                                                           \
		zassert_equal((value)->type, MP_TYPE_INT_FRACTION);                                \
		zassert_equal(mp_value_get_fraction_numerator(value), num);                        \
		zassert_equal(mp_value_get_fraction_denominator(value), denom);                    \
	})

#define validate_uint_fraction_value(value, num, denom)                                            \
	({                                                                                         \
		zassert_not_null(value);                                                           \
		zassert_equal((value)->type, MP_TYPE_UINT_FRACTION);                               \
		zassert_equal(mp_value_get_fraction_numerator(value), num);                        \
		zassert_equal(mp_value_get_fraction_denominator(value), denom);                    \
	})

#define validate_int_range_value(value, min, max, step)                                            \
	({                                                                                         \
		zassert_not_null(value);                                                           \
		zassert_equal((value)->type, MP_TYPE_INT_RANGE);                                   \
		zassert_equal(mp_value_get_int_range_min(value), min);                             \
		zassert_equal(mp_value_get_int_range_max(value), max);                             \
		zassert_equal(mp_value_get_int_range_step(value), step);                           \
	})

#define validate_uint_range_value(value, min, max, step)                                           \
	({                                                                                         \
		zassert_not_null(value);                                                           \
		zassert_equal((value)->type, MP_TYPE_UINT_RANGE);                                  \
		zassert_equal(mp_value_get_uint_range_min(value), min);                            \
		zassert_equal(mp_value_get_uint_range_max(value), max);                            \
		zassert_equal(mp_value_get_uint_range_step(value), step);                          \
	})

#define validate_fraction_int_range(value, min_num, min_denom, max_num, max_denom, step_num,       \
				    step_denom)                                                    \
	({                                                                                         \
		zassert_not_null(value);                                                           \
		zassert_equal((value)->type, MP_TYPE_INT_FRACTION_RANGE);                          \
		const struct mp_value *frac = mp_value_get_fraction_range_min(value);              \
		zassert_equal(mp_value_get_fraction_numerator(frac), min_num);                     \
		zassert_equal(mp_value_get_fraction_denominator(frac), min_denom);                 \
		frac = mp_value_get_fraction_range_max(value);                                     \
		zassert_equal(mp_value_get_fraction_numerator(frac), max_num);                     \
		zassert_equal(mp_value_get_fraction_denominator(frac), max_denom);                 \
		frac = mp_value_get_fraction_range_step(value);                                    \
		zassert_equal(mp_value_get_fraction_numerator(frac), step_num);                    \
		zassert_equal(mp_value_get_fraction_denominator(frac), step_denom);                \
	})

#define validate_uint_fraction_range(value, min_num, min_denom, max_num, max_denom, step_num,      \
				     step_denom)                                                   \
	({                                                                                         \
		zassert_not_null(value);                                                           \
		zassert_equal((value)->type, MP_TYPE_UINT_FRACTION_RANGE);                         \
		const struct mp_value *frac = mp_value_get_fraction_range_min(value);              \
		zassert_equal(mp_value_get_fraction_numerator(frac), min_num);                     \
		zassert_equal(mp_value_get_fraction_denominator(frac), min_denom);                 \
		frac = mp_value_get_fraction_range_max(value);                                     \
		zassert_equal(mp_value_get_fraction_numerator(frac), max_num);                     \
		zassert_equal(mp_value_get_fraction_denominator(frac), max_denom);                 \
		frac = mp_value_get_fraction_range_step(value);                                    \
		zassert_equal(mp_value_get_fraction_numerator(frac), step_num);                    \
		zassert_equal(mp_value_get_fraction_denominator(frac), step_denom);                \
	})

#define validate_list_value_type_and_size(value, expected_size)                                    \
	({                                                                                         \
		zassert_not_null(value);                                                           \
		zassert_equal((value)->type, MP_TYPE_LIST);                                        \
		zassert_equal(mp_value_list_get_size(value), expected_size);                       \
	})

#endif /* TESTS_SUBSYS_MP_CORE_UNIT_SRC_MP_TEST_HELPERS_H_ */
