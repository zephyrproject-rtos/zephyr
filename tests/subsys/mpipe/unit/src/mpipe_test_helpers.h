/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TESTS_SUBSYS_MPIPE_UNIT_SRC_MPIPE_TEST_HELPERS_H_
#define TESTS_SUBSYS_MPIPE_UNIT_SRC_MPIPE_TEST_HELPERS_H_

#include <zephyr/mpipe/mpipe_value.h>
#include <zephyr/ztest.h>

#define validate_boolean_value(value, expected)                                                    \
	({                                                                                         \
		zassert_not_null(value);                                                           \
		zassert_equal((value)->type, MPIPE_TYPE_BOOLEAN);                                  \
		zassert_equal(mpipe_value_get_boolean(value), expected);                           \
	})

#define validate_int_value(value, expected)                                                        \
	({                                                                                         \
		zassert_not_null(value);                                                           \
		zassert_equal((value)->type, MPIPE_TYPE_INT);                                      \
		zassert_equal(mpipe_value_get_int(value), expected);                               \
	})

#define validate_uint_value(value, expected)                                                       \
	({                                                                                         \
		zassert_not_null(value);                                                           \
		zassert_equal((value)->type, MPIPE_TYPE_UINT);                                     \
		zassert_equal(mpipe_value_get_uint(value), expected);                              \
	})

#define validate_int_range_value(value, min, max, step)                                            \
	({                                                                                         \
		zassert_not_null(value);                                                           \
		zassert_equal((value)->type, MPIPE_TYPE_INT_RANGE);                                \
		zassert_equal(mpipe_value_get_int_range_min(value), min);                          \
		zassert_equal(mpipe_value_get_int_range_max(value), max);                          \
		zassert_equal(mpipe_value_get_int_range_step(value), step);                        \
	})

#define validate_uint_range_value(value, min, max, step)                                           \
	({                                                                                         \
		zassert_not_null(value);                                                           \
		zassert_equal((value)->type, MPIPE_TYPE_UINT_RANGE);                               \
		zassert_equal(mpipe_value_get_uint_range_min(value), min);                         \
		zassert_equal(mpipe_value_get_uint_range_max(value), max);                         \
		zassert_equal(mpipe_value_get_uint_range_step(value), step);                       \
	})

#endif /* TESTS_SUBSYS_MPIPE_UNIT_SRC_MPIPE_TEST_HELPERS_H_ */
