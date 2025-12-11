/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/devicetree.h>

/* Initial test config values set by `prj.conf` */
#define TEST_FOO_VAL_INIT		(774392)
#define TEST_BAR_VAL_INIT		(182834)
#define TEST_COMMON_VAL_INIT		(588411)

/* Test config values set by the `foo` snippet */
#define TEST_FOO_VAL_FOO		(464372)
#define TEST_COMMON_VAL_FOO		(271983)

/* Test config values set by the `bar` snippet */
#define TEST_BAR_VAL_BAR		(964183)
#define TEST_COMMON_VAL_BAR		(109234)

ZTEST_SUITE(snippet_tests, NULL, NULL, NULL, NULL, NULL);

ZTEST(snippet_tests, test_overlay_config)
{
	if (IS_ENABLED(CONFIG_TEST_TYPE_NONE)) {
		/*
		 * When no snippet is applied, test that the config values
		 * correspond to the initial values set by `prj.conf`.
		 */
		zassert_equal(CONFIG_TEST_FOO_VAL, TEST_FOO_VAL_INIT);
		zassert_equal(CONFIG_TEST_BAR_VAL, TEST_BAR_VAL_INIT);
		zassert_equal(CONFIG_TEST_COMMON_VAL, TEST_COMMON_VAL_INIT);
	} else if (IS_ENABLED(CONFIG_TEST_TYPE_FOO)) {
		/*
		 * When `foo` snippet is applied, test that the foo and common
		 * config values correspond to the values set by the `foo`
		 * snippet.
		 */
		zassert_equal(CONFIG_TEST_FOO_VAL, TEST_FOO_VAL_FOO);
		zassert_equal(CONFIG_TEST_BAR_VAL, TEST_BAR_VAL_INIT);
		zassert_equal(CONFIG_TEST_COMMON_VAL, TEST_COMMON_VAL_FOO);
	} else if (IS_ENABLED(CONFIG_TEST_TYPE_BAR)) {
		/*
		 * When `bar` snippet is applied, test that the bar and common
		 * config values correspond to the values set by the `bar`
		 * snippet.
		 */
		zassert_equal(CONFIG_TEST_FOO_VAL, TEST_FOO_VAL_INIT);
		zassert_equal(CONFIG_TEST_BAR_VAL, TEST_BAR_VAL_BAR);
		zassert_equal(CONFIG_TEST_COMMON_VAL, TEST_COMMON_VAL_BAR);
	} else if (IS_ENABLED(CONFIG_TEST_TYPE_FOO_BAR)) {
		/*
		 * When `foo` and `bar` snippets are applied, with `bar`
		 * specified after `foo`, test that the common config value
		 * corresponds to the value set by the `bar` snippet.
		 */
		zassert_equal(CONFIG_TEST_FOO_VAL, TEST_FOO_VAL_FOO);
		zassert_equal(CONFIG_TEST_BAR_VAL, TEST_BAR_VAL_BAR);
		zassert_equal(CONFIG_TEST_COMMON_VAL, TEST_COMMON_VAL_BAR);
	} else if (IS_ENABLED(CONFIG_TEST_TYPE_BAR_FOO)) {
		/*
		 * When `foo` and `bar` snippets are applied, with `foo`
		 * specified after `bar`, test that the common config value
		 * corresponds to the value set by the `foo` snippet.
		 */
		zassert_equal(CONFIG_TEST_FOO_VAL, TEST_FOO_VAL_FOO);
		zassert_equal(CONFIG_TEST_BAR_VAL, TEST_BAR_VAL_BAR);
		zassert_equal(CONFIG_TEST_COMMON_VAL, TEST_COMMON_VAL_FOO);
	} else {
		zassert(false, "Invalid test type");
	}
}

ZTEST(snippet_tests, test_dtc_overlay)
{
	if (IS_ENABLED(CONFIG_TEST_TYPE_NONE)) {
		/*
		 * When no snippet is applied, test that both `deleted-by-foo`
		 * and `deleted-by-bar` nodes exist, and that none of the nodes
		 * added by the `foo` and `bar` snippets exist.
		 */
		zassert_true(DT_NODE_EXISTS(DT_PATH(deleted_by_foo)));
		zassert_true(DT_NODE_EXISTS(DT_PATH(deleted_by_bar)));
		zassert_false(DT_NODE_EXISTS(DT_PATH(added_by_foo)));
		zassert_false(DT_NODE_EXISTS(DT_PATH(added_by_bar)));
	} else if (IS_ENABLED(CONFIG_TEST_TYPE_FOO)) {
		/*
		 * When `foo` snippet is applied, test that `deleted-by-foo` is
		 * deleted and `added-by-foo` is added.
		 */
		zassert_false(DT_NODE_EXISTS(DT_PATH(deleted_by_foo)));
		zassert_true(DT_NODE_EXISTS(DT_PATH(deleted_by_bar)));
		zassert_true(DT_NODE_EXISTS(DT_PATH(added_by_foo)));
		zassert_false(DT_NODE_EXISTS(DT_PATH(added_by_bar)));
	} else if (IS_ENABLED(CONFIG_TEST_TYPE_BAR)) {
		/*
		 * When `bar` snippet is applied, test that `deleted-by-bar` is
		 * deleted and `added-by-bar` is added.
		 */
		zassert_true(DT_NODE_EXISTS(DT_PATH(deleted_by_foo)));
		zassert_false(DT_NODE_EXISTS(DT_PATH(deleted_by_bar)));
		zassert_false(DT_NODE_EXISTS(DT_PATH(added_by_foo)));
		zassert_true(DT_NODE_EXISTS(DT_PATH(added_by_bar)));
	} else if (IS_ENABLED(CONFIG_TEST_TYPE_FOO_BAR)) {
		/*
		 * When `foo` and `bar` snippets are applied, with `bar`
		 * specified after `foo`, test that both `deleted-by-foo` and
		 * `deleted-by-bar` are deleted, and that the `added-by-foo` is
		 * deleted by the `bar` snippet.
		 */
		zassert_false(DT_NODE_EXISTS(DT_PATH(deleted_by_foo)));
		zassert_false(DT_NODE_EXISTS(DT_PATH(deleted_by_bar)));
		zassert_false(DT_NODE_EXISTS(DT_PATH(added_by_foo)));
		zassert_true(DT_NODE_EXISTS(DT_PATH(added_by_bar)));
	} else if (IS_ENABLED(CONFIG_TEST_TYPE_BAR_FOO)) {
		/*
		 * When `foo` and `bar` snippets are applied, with `foo`
		 * specified after `bar`, test that both `deleted-by-foo` and
		 * `deleted-by-bar` are deleted, and that the `added-by-bar` is
		 * deleted by the `foo` snippet.
		 */
		zassert_false(DT_NODE_EXISTS(DT_PATH(deleted_by_foo)));
		zassert_false(DT_NODE_EXISTS(DT_PATH(deleted_by_bar)));
		zassert_true(DT_NODE_EXISTS(DT_PATH(added_by_foo)));
		zassert_false(DT_NODE_EXISTS(DT_PATH(added_by_bar)));
	} else {
		zassert(false, "Invalid test type");
	}
}

ZTEST(snippet_tests, test_cmake_include)
{
	if (IS_ENABLED(CONFIG_TEST_TYPE_FOO) ||
	    IS_ENABLED(CONFIG_TEST_TYPE_FOO_BAR) ||
	    IS_ENABLED(CONFIG_TEST_TYPE_BAR_FOO)) {
		zassert_true(DT_NODE_EXISTS(DT_PATH(cmake_dts_configure)));
	} else {
		zassert_false(DT_NODE_EXISTS(DT_PATH(cmake_dts_configure)));
	}
}
