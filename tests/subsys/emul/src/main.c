/*
 * Copyright 2022 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_assert.h>
#include <zephyr/ztest_test_new.h>

#define TEST_ACCEL DT_NODELABEL(test_bmi)

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

ZTEST_SUITE(emul, NULL, NULL, NULL, NULL, NULL);
