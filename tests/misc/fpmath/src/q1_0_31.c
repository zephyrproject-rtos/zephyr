/* Copyright (c) 2022 Google Inc
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/math/fp.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>

#include "fpmath/common.h"

ZTEST_SUITE(q1_0_31, NULL, NULL, NULL, NULL, NULL);

ZTEST(q1_0_31, test_identity_out_of_bounds)
{
	q1_0_31_t q;

	ztest_set_assert_valid(true);
	q_assign(q, 1);

	ztest_set_assert_valid(true);
	q_assign(q, 1.5);

	ztest_set_assert_valid(true);
	q_assign(q, q_static(q1_15_16_t, 3.7));

	ztest_set_assert_valid(true);
	q_assign(q, -1);

	ztest_set_assert_valid(true);
	q_assign(q, -1.5f);

	ztest_set_assert_valid(true);
	q_assign(q, q_static(q1_23_8_t, -3.7));
}

ZTEST(q1_0_31, test_additive_out_of_bounds)
{
	q1_0_31_t q = q_static(q1_0_31_t, 0.75);

	ztest_set_assert_valid(true);
	q_add(q, 0.75);

	q_assign(q, -0.5);
	ztest_set_assert_valid(true);
	q_sub(q, 0.95f);
}

ZTEST(q1_0_31, test_multiplicitive_out_of_bounds)
{
	q1_0_31_t q = q_static(q1_0_31_t, 0.75);

	ztest_set_assert_valid(true);
	q_mul(q, 2);

	ztest_set_assert_valid(true);
	q_div(q, -0.5);
}

ZTEST(q1_0_31, test_identity_float)
{
	q1_0_31_t q = {0};

	q_assign(q, 0.55f);
	qassert(0x46666680, q);

	q_assign(q, -0.75);
	qassert(0xa0000000, q);
}

ZTEST(q1_0_31, test_identity_q)
{
	q1_0_31_t q = {0};

	q_assign(q, ((q1_23_8_t){.value = 0x78}));
	qassert(0x3c000000, q);

	q_assign(q, ((q1_15_16_t){.value = (int32_t)(0xffffffd5)}));
	qassert(0xffea8000, q);
}

ZTEST(q1_0_31, test_add_float)
{
	q1_0_31_t q = q_static(q1_0_31_t, 0.5);

	q_add(q, 0.25);
	qassert(0x60000000, q);

	q_add(q, -1);
	qassert(0xe0000000, q);
}

ZTEST(q1_0_31, test_add_q)
{
	q1_0_31_t q = {0};

	q_add(q, q_static(q1_23_8_t, 0.25));
	qassert(0x20000000, q);

	q_add(q, q_static(q1_15_16_t, -0.33));
	qassert(0xf5c28000, q);
}

ZTEST(q1_0_31, test_sub_float)
{
	q1_0_31_t q = q_static(q1_0_31_t, 0.5);

	q_sub(q, 0.25);
	qassert(0x20000000, q);

	q_sub(q, -0.5f);
	qassert(0x60000000, q);
}

ZTEST(q1_0_31, test_sub_q)
{
	q1_0_31_t q = {0};

	q_sub(q, q_static(q1_23_8_t, 0.25));
	qassert(0xe0000000, q);

	q_sub(q, q_static(q1_15_16_t, -0.33));
	qassert(0xa3d8000, q);
}

ZTEST(q1_0_31, test_mul_int)
{
	q1_0_31_t q = q_static(q1_0_31_t, 0.25);

	q_mul(q, 2);
	qassert(0x40000000, q);

	q_mul(q, -1);
	qassert(0xc0000000, q);
}

ZTEST(q1_0_31, test_mul_float)
{
	q1_0_31_t q = q_static(q1_0_31_t, 0.25);

	q_mul(q, 1.5);
	qassert(0x30000000, q);

	q_mul(q, -2.33);
	qassert(0x9028f5c2, q);
}

ZTEST(q1_0_31, test_mul_q)
{
	q1_0_31_t q = q_static(q1_0_31_t, 0.25);

	q_mul(q, q_static(q1_23_8_t, 1.5));
	qassert(0x30000000, q);

	q_mul(q, q_static(q1_15_16_t, -2.33));
	qassert(0x9028f000, q);
}

ZTEST(q1_0_31, test_div_int)
{
	q1_0_31_t q = q_static(q1_0_31_t, 0.25);

	q_div(q, 2);
	qassert(0x10000000, q);

	q_div(q, -1);
	qassert(0xf0000000, q);
}

ZTEST(q1_0_31, test_div_float)
{
	q1_0_31_t q = q_static(q1_0_31_t, 0.25);

	q_div(q, 0.5);
	qassert(0x40000000, q);

	q_div(q, -7.33);
	qassert(0xf744cd5c, q);
}

ZTEST(q1_0_31, test_div_q)
{
	q1_0_31_t q = q_static(q1_0_31_t, 0.25);

	q_div(q, q_static(q1_23_8_t, 1.0 / 3.0));
	qassert(0x60606060, q);

	q_div(q, q_static(q1_15_16_t, -15.33));
	qassert(0xf9b69511, q);
}
