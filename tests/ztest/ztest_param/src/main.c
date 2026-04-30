/* SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

/*
 * Comprehensive tests for value-parameterized test support (ZTEST_P).
 *
 * Use-cases covered:
 *
 *  1. ZTEST_F() in the same suite: fixture is reachable and intact.
 *  2. ZTEST() in the same suite: no param context exposed.
 *  3. Scalar (int) parameters via ZTEST_DEFINE_PARAM_VALUES().
 *  4. ztest_get_current_param_index() reports the correct iteration index.
 *  5. Struct parameters via ZTEST_DEFINE_PARAM_VALUES_ARRAY().
 *  6. Multiple ZTEST_INSTANTIATE_TEST_SUITE_P() calls for the same body.
 *  7. Fixture sentinel is NEVER overwritten by any parameter value.
 */

/* ---- Fixture ------------------------------------------------------------ */

#define SENTINEL 0xDEADBEEFU

struct ztest_params_fixture {
	uint32_t sentinel;
};

static void *suite_setup(void)
{
	static struct ztest_params_fixture f;

	f.sentinel = SENTINEL;
	return &f;
}

static void suite_teardown(void *data)
{
	struct ztest_params_fixture *f = (struct ztest_params_fixture *)data;

	/*
	 * If the fixture pointer had ever been overwritten by a param value
	 * this dereference would crash.  The zassert is belt-and-suspenders.
	 */
	zassert_equal(f->sentinel, SENTINEL,
		      "teardown: fixture sentinel was corrupted");
}

ZTEST_SUITE(ztest_params, NULL, suite_setup, NULL, NULL, suite_teardown);

/* ---- Use-case 1: ZTEST_F – fixture is reachable and intact -------------- */

ZTEST_F(ztest_params, test_fixture_intact)
{
	zassert_equal(fixture->sentinel, SENTINEL,
		      "fixture sentinel corrupted before test ran");
}

/* ---- Use-case 2: ZTEST – no param context for ordinary tests ------------ */

ZTEST(ztest_params, test_no_param_context)
{
	zassert_false(ztest_has_current_param(),
		      "plain ZTEST must not have an active param context");
	zassert_is_null(ztest_get_current_param(),
			"plain ZTEST current param pointer must be NULL");
	zassert_equal(ztest_get_current_param_index(), 0U,
		      "plain ZTEST param index must be 0");
}

/* ---- Use-case 3: scalar (int) parameters, fixture preserved ------------- */

/*
 * Instantiated with three integer values {1, 2, 3}.  For every invocation:
 *   - param context is active,
 *   - ZTEST_GET_PARAM(int) returns the expected value,
 *   - the fixture sentinel is still intact (not overwritten by the param).
 */
ZTEST_P(ztest_params, test_int_param)
{
	struct ztest_params_fixture *f = (struct ztest_params_fixture *)data;
	int val = ZTEST_GET_PARAM(int);

	zassert_true(ztest_has_current_param(),
		     "param context must be active inside ZTEST_P");
	zassert_not_null(ztest_get_current_param(),
			 "current param pointer must not be NULL");
	zassert_equal(f->sentinel, SENTINEL,
		      "fixture overwritten by int param %d", val);
	zassert_true(val == 1 || val == 2 || val == 3,
		     "unexpected int param: %d", val);
}

ZTEST_DEFINE_PARAM_VALUES(int_vals, int, 1, 2, 3);
ZTEST_INSTANTIATE_TEST_SUITE_P(ints, ztest_params, test_int_param, int_vals);

/* ---- Use-case 4: param index is reported correctly ---------------------- */

/*
 * Values are {10, 20, 30}; the test verifies that
 * ztest_get_current_param_index() equals (value / 10 - 1).
 */
ZTEST_P(ztest_params, test_param_index)
{
	size_t idx = ztest_get_current_param_index();
	int val    = ZTEST_GET_PARAM(int);

	zassert_equal(idx, (size_t)(val / 10 - 1),
		      "param index %zu does not match value %d", idx, val);
}

ZTEST_DEFINE_PARAM_VALUES(index_vals, int, 10, 20, 30);
ZTEST_INSTANTIATE_TEST_SUITE_P(idx_check, ztest_params,
			       test_param_index, index_vals);

/* ---- Use-case 5: struct parameters -------------------------------------- */

struct point {
	int x;
	int y;
};

/*
 * Tests that ZTEST_GET_PARAM_PTR() works for aggregate types and that the
 * fixture is not corrupted when elem_size > sizeof(void *).
 */
ZTEST_P(ztest_params, test_struct_param)
{
	struct ztest_params_fixture *f = (struct ztest_params_fixture *)data;
	const struct point *p          = ZTEST_GET_PARAM_PTR(struct point);

	zassert_equal(f->sentinel, SENTINEL,
		      "fixture overwritten by struct param at (%d,%d)",
		      p->x, p->y);
	zassert_true(p->x > 0 && p->y > 0,
		     "expected positive coordinates, got (%d, %d)",
		     p->x, p->y);
}

static const struct point point_arr[] = {
	{1, 2},
	{3, 4},
	{5, 6},
};

ZTEST_DEFINE_PARAM_VALUES_ARRAY(point_vals, point_arr);
ZTEST_INSTANTIATE_TEST_SUITE_P(points, ztest_params,
			       test_struct_param, point_vals);

/* ---- Use-case 6: multiple instantiations of the same test body ---------- */

/*
 * The same ZTEST_P body is instantiated twice with disjoint value sets.
 * The framework must run it once per set; values from different sets must
 * not bleed into each other.
 */
ZTEST_P(ztest_params, test_multi_instantiate)
{
	int val = ZTEST_GET_PARAM(int);

	zassert_true(val == 100 || val == 200 || val == -1 || val == -2,
		     "unexpected value in multi-instantiated test: %d", val);
}

ZTEST_DEFINE_PARAM_VALUES(pos_vals, int, 100, 200);
ZTEST_INSTANTIATE_TEST_SUITE_P(positive, ztest_params,
			       test_multi_instantiate, pos_vals);

ZTEST_DEFINE_PARAM_VALUES(neg_vals, int, -1, -2);
ZTEST_INSTANTIATE_TEST_SUITE_P(negative, ztest_params,
			       test_multi_instantiate, neg_vals);
