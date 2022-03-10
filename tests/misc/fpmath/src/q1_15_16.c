/* Copyright (c) 2022 Google Inc
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/math/fp.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>

#include "fpmath/common.h"

ZTEST_SUITE(q1_15_16, NULL, NULL, NULL, NULL, NULL);

ZTEST(q1_15_16, test_identity_out_of_bounds)
{
	q1_15_16_t q;

	ztest_set_assert_valid(true);
	q_assign(q, 32768);

	ztest_set_assert_valid(true);
	q_assign(q, 32768.75);

	ztest_set_assert_valid(true);
	q_assign(q, q_static(q1_23_8_t, 62768.7));

	ztest_set_assert_valid(true);
	q_assign(q, -32769);

	ztest_set_assert_valid(true);
	q_assign(q, -32769.5f);

	ztest_set_assert_valid(true);
	q_assign(q, q_static(q1_23_8_t, -62768.7));
}

ZTEST(q1_15_16, test_additive_out_of_bounds)
{
	q1_15_16_t q = q_static(q1_15_16_t, 30000);

	ztest_set_assert_valid(true);
	q_add(q, 5000.75);

	q_assign(q, -31000);
	ztest_set_assert_valid(true);
	q_sub(q, 7823.95f);
}

ZTEST(q1_15_16, test_multiplicitive_out_of_bounds)
{
	q1_15_16_t q = q_static(q1_15_16_t, 17123.75);

	ztest_set_assert_valid(true);
	q_mul(q, 2);

	ztest_set_assert_valid(true);
	q_div(q, -3.5);
}

ZTEST(q1_15_16, test_identity_uint)
{
	q1_15_16_t q = {0};

	q_assign(q, UINT8_C(5));
	qassert(5 << 16, q);

	q_assign(q, UINT16_C(87));
	qassert(87 << 16, q);

	q_assign(q, UINT32_C(122));
	qassert(122 << 16, q);
}

ZTEST(q1_15_16, test_identity_int)
{
	q1_15_16_t q = {0};

	q_assign(q, INT8_C(-5));
	qassert(0xfffb0000, q);

	q_assign(q, INT16_C(-87));
	qassert(0xffa90000, q);

	q_assign(q, INT32_C(-122));
	qassert(0xff860000, q);
}

ZTEST(q1_15_16, test_identity_float)
{
	q1_15_16_t q = {0};

	q_assign(q, 5.5f);
	qassert(0x58000, q);

	q_assign(q, 14.75);
	qassert(0xec000, q);
}

ZTEST(q1_15_16, test_identity_q)
{
	q1_15_16_t q = {0};

	q_assign(q, ((q1_23_8_t){
			    .value = 0x51234,
		    }));
	qassert(0x5123400, q);

	q_assign(q, ((q1_0_31_t){
			    .value = 0x71234567,
		    }));
	qassert(0xe246, q);
}

ZTEST(q1_15_16, test_add_int)
{
	q1_15_16_t q = {
		.value = 0,
	};

	/* 0 + 3, expect 3 */
	q_add(q, 3);
	qassert(0x30000, q);

	/* 3 + -1, expect 2 */
	q_add(q, -1);
	qassert(0x20000, q);

	/* 2 + -5, expect -3 */
	q_add(q, -5);
	qassert(0xfffd0000, q);

	/* -3 + 10, expect 7 */
	q_add(q, 10);
	qassert(0x70000, q);
}

ZTEST(q1_15_16, test_add_float)
{
	q1_15_16_t q = {
		.value = 0,
	};

	/* 0 + 3.5, expect 3.5 */
	q_add(q, 3.5f);
	qassert(0x38000, q);

	/* 3.5 + -1.3, expect 2.2 */
	q_add(q, -1.3);
	qassert(0x23333, q);

	/* 2.2 + -17.33, expect -15.13 */
	q_add(q, -17.33f);
	qassert(0xfff0deb8, q);

	/* -15.13 + 22.7, expect 7.57 */
	q_add(q, 22.7);
	qassert(0x791eb, q);
}

ZTEST(q1_15_16, test_add_q)
{
	q1_15_16_t q = {
		.value = 0,
	};

	q_add(q, ((q1_23_8_t){
			 .value = 0x123,
		 }));
	qassert(0x12300, q);

	q.value = 0;
	q_add(q, ((q1_15_16_t){
			 .value = 0x12345,
		 }));
	qassert(0x12345, q);

	q.value = 0;
	q_add(q, ((q1_0_31_t){
			 .value = 0x12345678,
		 }));
	qassert(0x2468, q);
}

ZTEST(q1_15_16, test_sub_int)
{
	q1_15_16_t q = {
		.value = 0,
	};

	/* 0 - -3, expect 3 */
	q_sub(q, -3);
	qassert(0x30000, q);

	/* 3 - 1, expect 2 */
	q_sub(q, 1);
	qassert(0x20000, q);

	/* 2 - 5, expect -3 */
	q_sub(q, 5);
	qassert(0xfffd0000, q);

	/* -3 - -10, expect 7 */
	q_sub(q, -10);
	qassert(0x70000, q);
}

ZTEST(q1_15_16, test_sub_float)
{
	q1_15_16_t q = {
		.value = 0,
	};

	/* 0 - -3.5, expect 3.5 */
	q_sub(q, -3.5f);
	qassert(0x38000, q);

	/* 3.5 - 1.3, expect 2.2 */
	q_sub(q, 1.3);
	qassert(0x23334, q);

	/* 2.2 - 17.33, expect -15.13 */
	q_sub(q, 17.33f);
	qassert(0xfff0deba, q);

	/* -15.13 - -22.7, expect 7.57 */
	q_sub(q, -22.7);
	qassert(0x791ee, q);
}

ZTEST(q1_15_16, test_sub_q)
{
	q1_15_16_t q = {
		.value = 0,
	};

	q_sub(q, ((q1_23_8_t){
			 .value = 0x123,
		 }));
	qassert(0xfffedd00, q);

	q.value = 0;
	q_sub(q, ((q1_15_16_t){
			 .value = 0x12345,
		 }));
	qassert(0xfffedcbb, q);

	q.value = 0;
	q_sub(q, ((q1_0_31_t){
			 .value = 0x12345678,
		 }));
	qassert(0xffffdb98, q);
}

ZTEST(q1_15_16, test_mul_int)
{
	q1_15_16_t q = q_static(q1_15_16_t, 1);

	/* 1 * 3, expect 3 */
	q_mul(q, 3);
	qassert(0x30000, q);

	/* 3 * 2, expect 6 */
	q_mul(q, 2);
	qassert(0x60000, q);

	/* 6 * -5, expect -30 */
	q_mul(q, -5);
	qassert(0xffe20000, q);

	/* -30 * -2, expect 60 */
	q_mul(q, -2);
	qassert(0x3c0000, q);
}

ZTEST(q1_15_16, test_mul_float)
{
	q1_15_16_t q = q_static(q1_15_16_t, 1);

	/* 1 * 3.5, expect 3.5 */
	q_mul(q, 3.5f);
	qassert(0x38000, q);

	/* 3.5 * 1.3, expect 4.55 */
	q_mul(q, 1.3);
	qassert(0x48cca, q);

	/* 4.55 * -3.2, expect -14.56 */
	q_mul(q, -3.2f);
	qassert(0xfff170a9, q);

	/* -14.56 * -2.27, expect 33.0512 */
	q_mul(q, -2.27);
	qassert(0x210d13, q);
}

ZTEST(q1_15_16, test_mul_q)
{
	q1_15_16_t q = q_static(q1_15_16_t, 2);

	/* 2 * 3.2 , expected 6.4*/
	q_mul(q, q_static(q1_23_8_t, 3.2));
	qassert(0x66600, q);

	/* 6.4 * -2.23456, expected -14 76/255 (−14.301184)*/
	q_mul(q, q_static(q1_15_16_t, -2.23456));
	qassert(0xfff1b3c4, q);

	/* -14 76/255 * -0.1234, expected 1 202/255 */
	q_mul(q, q_static(q1_0_31_t, -0.1234));
	qassert(0x1c3b8, q);
}

ZTEST(q1_15_16, test_div_int)
{
	q1_15_16_t q = q_static(q1_15_16_t, 1);

	/* 1 / 2, expect 0.5 */
	q_div(q, 2);
	qassert(0x8000, q);

	/* 0.5 / -4, expect -0.125 */
	q_div(q, -4);
	qassert(0xffffe000, q);

	/* -0.125 / -1, expect 0.125 */
	q_div(q, -1);
	qassert(0x2000, q);
}

ZTEST(q1_15_16, test_div_float)
{
	q1_15_16_t q = q_static(q1_15_16_t, 5.0);

	/* 5 / 2.5, expect 2 */
	q_div(q, 2.5);
	qassert(0x20000, q);

	/* 2 / 1.5, expect 1.33... */
	q_div(q, 1.5f);
	qassert(0x15555, q);

	/* 4/3 / 1/3, expect 4 (with 3/255 error) */
	q_div(q, (1.0 / 3.0));
	qassert(0x40003, q);
}

ZTEST(q1_15_16, test_div_q)
{
	q1_15_16_t q = q_static(q1_15_16_t, 5.0);

	/* 5 / 2.5, expect 2 */
	q_div(q, q_static(q1_23_8_t, 2.5));
	qassert(0x20000, q);

	/* 2 / 1.5, expect 1.33... */
	q_div(q, q_static(q1_15_16_t , 1.5f));
	qassert(0x15555, q);

	/* 4/3 / 1/3, expect 4 (with 3/255 error) */
	q_div(q, q_static(q1_0_31_t, (1.0 / 3.0)));
	qassert(0x40003, q);
}
