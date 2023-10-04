/*
 * Copyright 2022 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "emul_tester.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_assert.h>
#include <zephyr/ztest_test_new.h>

#define TEST_ACCEL DT_NODELABEL(test_bmi)

#define TEST_EMUL_A DT_NODELABEL(emul_tester_a)
#define TEST_EMUL_B DT_NODELABEL(emul_tester_b)

ZTEST(emul, test_emul_dt_get)
{
	/* This variable is static to verify that the result of EMUL_DT_GET is a
	 * compile-time constant.
	 */
	static const struct emul *emul_static = EMUL_DT_GET(TEST_ACCEL);

	/* Verify that EMUL_DT_GET returned the expected struct emul. */
	zassert_not_null(emul_static, "EMUL_DT_GET returned NULL");
	zassert_ok(strcmp(emul_static->dev->name, DT_NODE_FULL_NAME(TEST_ACCEL)),
		   "Unexpected device name %s", emul_static->dev->name);
}

ZTEST(emul, test_emul_backend_api)
{
	static const struct emul *emul_a = EMUL_DT_GET(TEST_EMUL_A);
	static const struct emul *emul_b = EMUL_DT_GET(TEST_EMUL_B);
	int set_action_value;
	int scale;
	int get_action_value;

	set_action_value = 5;

	scale = DT_PROP(TEST_EMUL_A, scale);
	zassert_not_null(emul_a, "emul_tester_a not found");
	zassert_ok(emul_tester_backend_set_action(emul_a, set_action_value));
	zassert_ok(emul_tester_backend_get_action(emul_a, &get_action_value));
	zassert_equal(get_action_value, set_action_value * scale);

	scale = DT_PROP(TEST_EMUL_B, scale);
	zassert_not_null(emul_b, "emul_tester_b not found");
	zassert_ok(emul_tester_backend_set_action(emul_b, set_action_value));
	zassert_ok(emul_tester_backend_get_action(emul_b, &get_action_value));
	zassert_equal(get_action_value, set_action_value * scale);
}

ZTEST_SUITE(emul, NULL, NULL, NULL, NULL, NULL);
