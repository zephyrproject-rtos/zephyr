/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <zephyr/mpipe/mpipe_value.h>

ZTEST_SUITE(mpipe_value_api, NULL, NULL, NULL, NULL, NULL);

ZTEST(mpipe_value_api, test_new_values)
{
	struct mpipe_value bt;

	mpipe_value_set(&bt, MPIPE_TYPE_BOOLEAN, true);

	zassert_equal(bt.type, MPIPE_TYPE_BOOLEAN, "type != BOOLEAN");
	zassert_true(mpipe_value_get_boolean(&bt), "value != true");

	struct mpipe_value bf;

	mpipe_value_set(&bf, MPIPE_TYPE_BOOLEAN, false);

	zassert_false(mpipe_value_get_boolean(&bf), "value != false");

	struct mpipe_value iv;

	mpipe_value_set(&iv, MPIPE_TYPE_INT, -42);

	zassert_equal(iv.type, MPIPE_TYPE_INT, "type != INT");
	zassert_equal(mpipe_value_get_int(&iv), -42, "value != -42");

	struct mpipe_value uv;

	mpipe_value_set(&uv, MPIPE_TYPE_UINT, 123U);

	zassert_equal(uv.type, MPIPE_TYPE_UINT, "type != UINT");
	zassert_equal(mpipe_value_get_uint(&uv), 123U, "value != 123");

	struct mpipe_value rv;

	mpipe_value_set(&rv, MPIPE_TYPE_INT_RANGE, 8000, 48000, 8000);

	zassert_equal(rv.type, MPIPE_TYPE_INT_RANGE, "type != INT_RANGE");
	zassert_equal(mpipe_value_get_int_range_min(&rv), 8000, "min != 8000");
	zassert_equal(mpipe_value_get_int_range_max(&rv), 48000, "max != 48000");
	zassert_equal(mpipe_value_get_int_range_step(&rv), 8000, "step != 8000");

	struct mpipe_value ez;

	mpipe_value_set(&ez, MPIPE_TYPE_INT, 0);

	zassert_equal(mpipe_value_get_int(&ez), 0, "value != 0");
}

/* Two primitive values intersect when they are the same value, and not otherwise */
ZTEST(mpipe_value_api, test_primitive_equality)
{
	struct mpipe_value a;
	struct mpipe_value b;
	struct mpipe_value result;

	mpipe_value_set(&a, MPIPE_TYPE_INT, -42);
	mpipe_value_set(&b, MPIPE_TYPE_INT, -42);
	zassert_ok(mpipe_value_intersect(&a, &b, &result), "equal INT values did not intersect");
	zassert_equal(mpipe_value_get_int(&result), -42, "INT result != -42");

	mpipe_value_set(&b, MPIPE_TYPE_INT, 42);
	zassert_equal(mpipe_value_intersect(&a, &b, &result), -ENOENT,
		      "different INT values intersected");

	mpipe_value_set(&a, MPIPE_TYPE_UINT, 100U);
	mpipe_value_set(&b, MPIPE_TYPE_UINT, 100U);
	zassert_ok(mpipe_value_intersect(&a, &b, &result), "equal UINT values did not intersect");
	zassert_equal(mpipe_value_get_uint(&result), 100U, "UINT result != 100");

	mpipe_value_set(&b, MPIPE_TYPE_UINT, 200U);
	zassert_equal(mpipe_value_intersect(&a, &b, &result), -ENOENT,
		      "different UINT values intersected");

	mpipe_value_set(&a, MPIPE_TYPE_BOOLEAN, true);
	mpipe_value_set(&b, MPIPE_TYPE_BOOLEAN, true);
	zassert_ok(mpipe_value_intersect(&a, &b, &result),
		   "equal BOOLEAN values did not intersect");
	zassert_true(mpipe_value_get_boolean(&result), "BOOLEAN result != true");

	mpipe_value_set(&b, MPIPE_TYPE_BOOLEAN, false);
	zassert_equal(mpipe_value_intersect(&a, &b, &result), -ENOENT,
		      "different BOOLEAN values intersected");
}

ZTEST(mpipe_value_api, test_intersect)
{
	struct mpipe_value a;

	mpipe_value_set(&a, MPIPE_TYPE_INT, 48000);
	struct mpipe_value b;

	mpipe_value_set(&b, MPIPE_TYPE_INT, 48000);
	struct mpipe_value result;

	zassert_ok(mpipe_value_intersect(&a, &b, &result), "intersect(equal values) failed");
	zassert_equal(mpipe_value_get_int(&result), 48000, "result != 48000");

	struct mpipe_value range;

	mpipe_value_set(&range, MPIPE_TYPE_INT_RANGE, 8000, 48000, 8000);
	struct mpipe_value val;

	mpipe_value_set(&val, MPIPE_TYPE_INT, 16000);

	zassert_ok(mpipe_value_intersect(&range, &val, &result),
		   "intersect(value in &range) failed");
	zassert_equal(mpipe_value_get_int(&result), 16000, "result != 16000");
}

ZTEST(mpipe_value_api, test_copy_and_is_primitive)
{
	struct mpipe_value original;

	struct mpipe_value copy;

	/* A value owns nothing, so copying one is a plain assignment */
	mpipe_value_set(&original, MPIPE_TYPE_INT, 999);
	copy = original;
	zassert_equal(mpipe_value_get_int(&copy), 999, "copied value != 999");

	struct mpipe_value rorig;
	struct mpipe_value rcopy;

	mpipe_value_set(&rorig, MPIPE_TYPE_INT_RANGE, 1, 100, 1);
	rcopy = rorig;
	zassert_equal(mpipe_value_get_int_range_min(&rcopy), 1, "min != 1");
	zassert_equal(mpipe_value_get_int_range_max(&rcopy), 100, "max != 100");
	zassert_equal(mpipe_value_get_int_range_step(&rcopy), 1, "step != 1");

	struct mpipe_value iv;

	mpipe_value_set(&iv, MPIPE_TYPE_INT, 1);

	zassert_true(mpipe_value_is_primitive(&iv), "INT not primitive");

	struct mpipe_value bv;

	mpipe_value_set(&bv, MPIPE_TYPE_BOOLEAN, true);

	zassert_true(mpipe_value_is_primitive(&bv), "BOOLEAN not primitive");

	struct mpipe_value rv;

	mpipe_value_set(&rv, MPIPE_TYPE_INT_RANGE, 0, 10, 1);

	zassert_false(mpipe_value_is_primitive(&rv), "INT_RANGE is primitive");

	struct mpipe_value ci_a;

	mpipe_value_set(&ci_a, MPIPE_TYPE_INT, 10);
	struct mpipe_value ci_b;

	mpipe_value_set(&ci_b, MPIPE_TYPE_INT, 10);

	struct mpipe_value ci_out;

	zassert_ok(mpipe_value_intersect(&ci_a, &ci_b, &ci_out),
		   "same-type values cannot intersect");

	struct mpipe_value ci_range;

	mpipe_value_set(&ci_range, MPIPE_TYPE_INT_RANGE, 0, 100, 1);
	struct mpipe_value ci_val;

	mpipe_value_set(&ci_val, MPIPE_TYPE_INT, 50);

	zassert_ok(mpipe_value_intersect(&ci_range, &ci_val, &ci_out),
		   "&range and value cannot intersect");
}

ZTEST(mpipe_value_api, test_set_updates_value)
{
	struct mpipe_value val;

	mpipe_value_set(&val, MPIPE_TYPE_INT, 10);

	mpipe_value_set(&val, MPIPE_TYPE_INT, 99);
	zassert_equal(mpipe_value_get_int(&val), 99, "value != 99 after set");
}

ZTEST(mpipe_value_api, test_sanity)
{
	struct mpipe_value int_val;

	mpipe_value_set(&int_val, MPIPE_TYPE_INT, 42);
	struct mpipe_value uint_val;

	mpipe_value_set(&uint_val, MPIPE_TYPE_UINT, 42U);

	struct mpipe_value result;

	/* The same number under two types is not a common value */
	zassert_equal(mpipe_value_intersect(&int_val, &uint_val, &result), -ENOENT,
		      "values of different types intersect != -ENOENT");

	struct mpipe_value a;

	mpipe_value_set(&a, MPIPE_TYPE_INT, 100);
	struct mpipe_value b;

	mpipe_value_set(&b, MPIPE_TYPE_INT, 200);

	zassert_equal(mpipe_value_intersect(&a, &b, &result), -ENOENT,
		      "disjoint values intersect != -ENOENT");
}
