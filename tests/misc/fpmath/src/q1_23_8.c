/* Copyright (c) 2022 Google Inc
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/math/fp.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>

#include "fpmath/common.h"

ZTEST_SUITE(q1_23_8, NULL, NULL, NULL, NULL, NULL);

ZTEST(q1_23_8, test_identity_out_of_bounds)
{
	q1_23_8_t q;

	ztest_set_assert_valid(true);
	q_assign(q, 8388608);

	ztest_set_assert_valid(true);
	q_assign(q, 8388608.75);

	ztest_set_assert_valid(true);
	q_assign(q, -8388609);

	ztest_set_assert_valid(true);
	q_assign(q, -8388609.5f);
}

ZTEST(q1_23_8, test_additive_out_of_bounds)
{
	q1_23_8_t q = q_static(q1_23_8_t, 8123456);

	ztest_set_assert_valid(true);
	q_add(q, 345678.75);

	q_assign(q, -8123456);
	ztest_set_assert_valid(true);
	q_sub(q, 765432.95f);
}

ZTEST(q1_23_8, test_multiplicitive_out_of_bounds)
{
	q1_23_8_t q = q_static(q1_23_8_t, 8123456.75);

	ztest_set_assert_valid(true);
	q_mul(q, 2);

	ztest_set_assert_valid(true);
	q_div(q, -3.5);
}

ZTEST(q1_23_8, test_identity_uint)
{
	q1_23_8_t q = {0};

	q_assign(q, UINT8_C(5));
	qassert(5 << 8, q);

	q_assign(q, UINT16_C(87));
	qassert(87 << 8, q);

	q_assign(q, UINT32_C(122));
	qassert(122 << 8, q);
}

ZTEST(q1_23_8, test_identity_int)
{
	q1_23_8_t q = {0};

	q_assign(q, INT8_C(-5));
	qassert(0xfffffb00, q);

	q_assign(q, INT16_C(-87));
	qassert(0xffffa900, q);

	q_assign(q, INT32_C(-122));
	qassert(0xffff8600, q);
}

ZTEST(q1_23_8, test_identity_float)
{
	q1_23_8_t q = {0};

	q_assign(q, 5.5f);
	qassert(0x580, q);

	q_assign(q, 14.75);
	qassert(0xec0, q);
}

ZTEST(q1_23_8, test_identity_q)
{
	q1_23_8_t q = {0};

	q_assign(q, ((q1_15_16_t){
			    .value = 0x51234,
		    }));
	qassert(0x512, q);

	q_assign(q, ((q1_0_31_t){
			    .value = 0x71234567,
		    }));
	qassert(0xe2, q);
}

ZTEST(q1_23_8, test_add_int)
{
	q1_23_8_t q = {
		.value = 0,
	};

	/* 0 + 3, expect 3 */
	q_add(q, 3);
	qassert(0x300, q);

	/* 3 + -1, expect 2 */
	q_add(q, -1);
	qassert(0x200, q);

	/* 2 + -5, expect -3 */
	q_add(q, -5);
	qassert(0xfffffd00, q);

	/* -3 + 10, expect 7 */
	q_add(q, 10);
	qassert(0x700, q);
}

ZTEST(q1_23_8, test_add_float)
{
	q1_23_8_t q = {
		.value = 0,
	};

	/* 0 + 3.5, expect 3.5 */
	q_add(q, 3.5f);
	qassert(0x380, q);

	/* 3.5 + -1.3, expect 2.2 */
	q_add(q, -1.3);
	qassert(0x233, q);

	/* 2.2 + -17.33, expect -15.13 */
	q_add(q, -17.33f);
	qassert(0xfffff0de, q);

	/* -15.13 + 22.7, expect 7.57 */
	q_add(q, 22.7);
	qassert(0x791, q);
}

ZTEST(q1_23_8, test_add_q)
{
	q1_23_8_t q = {
		.value = 0,
	};

	q_add(q, ((q1_23_8_t){
			 .value = 0x123,
		 }));
	qassert(0x123, q);

	q.value = 0;
	q_add(q, ((q1_15_16_t){
			 .value = 0x12345,
		 }));
	qassert(0x123, q);

	q.value = 0;
	q_add(q, ((q1_0_31_t){
			 .value = 0x12345678,
		 }));
	qassert(0x24, q);
}

ZTEST(q1_23_8, test_sub_int)
{
	q1_23_8_t q = {
		.value = 0,
	};

	/* 0 - -3, expect 3 */
	q_sub(q, -3);
	qassert(0x300, q);

	/* 3 - 1, expect 2 */
	q_sub(q, 1);
	qassert(0x200, q);

	/* 2 - 5, expect -3 */
	q_sub(q, 5);
	qassert(0xfffffd00, q);

	/* -3 - -10, expect 7 */
	q_sub(q, -10);
	qassert(0x700, q);
}

ZTEST(q1_23_8, test_sub_float)
{
	q1_23_8_t q = {
		.value = 0,
	};

	/* 0 - -3.5, expect 3.5 */
	q_sub(q, -3.5f);
	qassert(0x380, q);

	/* 3.5 - 1.3, expect 2.2 */
	q_sub(q, 1.3);
	qassert(0x234, q);

	/* 2.2 - 17.33, expect -15.13 */
	q_sub(q, 17.33f);
	qassert(0xfffff0e0, q);

	/* -15.13 - -22.7, expect 7.57 */
	q_sub(q, -22.7);
	qassert(0x794, q);
}

ZTEST(q1_23_8, test_sub_q)
{
	q1_23_8_t q = {
		.value = 0,
	};

	q_sub(q, ((q1_23_8_t){
			 .value = 0x123,
		 }));
	qassert(0xfffffedd, q);

	q.value = 0;
	q_sub(q, ((q1_15_16_t){
			 .value = 0x12345,
		 }));
	qassert(0xfffffedd, q);

	q.value = 0;
	q_sub(q, ((q1_0_31_t){
			 .value = 0x12345678,
		 }));
	qassert(0xffffffdc, q);
}

ZTEST(q1_23_8, test_mul_int)
{
	q1_23_8_t q = q_static(q1_23_8_t, 1);

	/* 1 * 3, expect 3 */
	q_mul(q, 3);
	qassert(0x300, q);

	/* 3 * 2, expect 6 */
	q_mul(q, 2);
	qassert(0x600, q);

	/* 6 * -5, expect -30 */
	q_mul(q, -5);
	qassert(0xffffe200, q);

	/* -30 * -2, expect 60 */
	q_mul(q, -2);
	qassert(0x3c00, q);
}

ZTEST(q1_23_8, test_mul_float)
{
	q1_23_8_t q = q_static(q1_23_8_t, 1);

	/* 1 * 3.5, expect 3.5 */
	q_mul(q, 3.5f);
	qassert(0x380, q);

	/* 3.5 * 1.3, expect 4.55 */
	q_mul(q, 1.3);
	qassert(0x48a, q);

	/* 4.55 * -3.2, expect -14.56 */
	q_mul(q, -3.2f);
	qassert(0xfffff175, q);

	/* -14.56 * -2.27, expect 33.0512 */
	q_mul(q, -2.27);
	qassert(0x2110, q);
}

ZTEST(q1_23_8, test_mul_q)
{
	q1_23_8_t q = q_static(q1_23_8_t, 2);

	/* 2 * 3.2 , expected 6.4*/
	q_mul(q, q_static(q1_23_8_t, 3.2));
	qassert(0x666, q);

	/* 6.4 * -2.23456, expected -14 76/255 (−14.301184)*/
	q_mul(q, q_static(q1_15_16_t, -2.23456));
	qassert(0xfffff1ad, q);

	/* -14 76/255 * -0.1234, expected 1 202/255 */
	q_mul(q, q_static(q1_0_31_t, -0.1234));
	qassert(0x1ca, q);
}

ZTEST(q1_23_8, test_div_int)
{
	q1_23_8_t q = q_static(q1_23_8_t, 1);

	/* 1 / 2, expect 0.5 */
	q_div(q, 2);
	qassert(0x80, q);

	/* 0.5 / -4, expect -0.125 */
	q_div(q, -4);
	qassert(0xffffffe0, q);

	/* -0.125 / -1, expect 0.125 */
	q_div(q, -1);
	qassert(0x20, q);
}

ZTEST(q1_23_8, test_div_float)
{
	q1_23_8_t q = q_static(q1_23_8_t, 5.0);

	/* 5 / 2.5, expect 2 */
	q_div(q, 2.5);
	qassert(0x200, q);

	/* 2 / 1.5, expect 1.33... */
	q_div(q, 1.5f);
	qassert(0x155, q);

	/* 4/3 / 1/3, expect 4 (with 3/255 error) */
	q_div(q, (1.0 / 3.0));
	qassert(0x403, q);
}

ZTEST(q1_23_8, test_div_q)
{
	q1_23_8_t q = q_static(q1_23_8_t, 5.0);

	/* 5 / 2.5, expect 2 */
	q_div(q, q_static(q1_23_8_t, 2.5));
	qassert(0x200, q);

	/* 2 / 1.5, expect 1.33... */
	q_div(q, q_static(q1_15_16_t , 1.5f));
	qassert(0x155, q);

	/* 4/3 / 1/3, expect 4 (with 3/255 error) */
	q_div(q, q_static(q1_0_31_t, (1.0 / 3.0)));
	qassert(0x403, q);
}
