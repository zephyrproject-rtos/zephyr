/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/devicetree.h>

ZTEST_SUITE(snippet_tests, NULL, NULL, NULL, NULL, NULL);

ZTEST(snippet_tests, test_foo)
{
	zassert_equal(CONFIG_FOO, 2, "foo snippet should set CONFIG_FOO=2");
}

ZTEST(snippet_tests, test_bar)
{
	zassert_equal(DT_NODE_EXISTS(DT_PATH(deleted_by_bar)), 0,
		      "bar snippet should delete /deleted-by-bar node");
}
