/* main.c - Application main entry point */

/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/pacs.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/fff.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/ztest_assert.h>
#include <zephyr/ztest_test.h>

DEFINE_FFF_GLOBALS;

static void pacs_test_suite_after(void *f)
{
	/* attempt to clean up after any failures */
	(void)bt_pacs_unregister();
}

ZTEST_SUITE(pacs_test_suite, NULL, NULL, NULL, pacs_test_suite_after, NULL);

/* Helper macro to define parameters ignoring unsupported features */
#define PACS_REGISTER_PARAM(_snk_pac, _snk_loc, _src_pac, _src_loc)                                \
	(struct bt_pacs_register_param)                                                            \
	{                                                                                          \
		IF_ENABLED(CONFIG_BT_PAC_SNK, (.snk_pac = (_snk_pac),))                            \
		IF_ENABLED(CONFIG_BT_PAC_SNK_LOC, (.snk_loc = (_snk_loc),))                        \
		IF_ENABLED(CONFIG_BT_PAC_SRC, (.src_pac = (_src_pac),))                            \
		IF_ENABLED(CONFIG_BT_PAC_SRC_LOC, (.src_loc = (_src_loc),))                        \
	}

static ZTEST(pacs_test_suite, test_pacs_register)
{
	const struct bt_pacs_register_param pacs_params[] = {
		/* valid snk_pac combinations */
		PACS_REGISTER_PARAM(true, true, true, true),
		PACS_REGISTER_PARAM(true, true, true, false),
		PACS_REGISTER_PARAM(true, true, false, false),
		PACS_REGISTER_PARAM(true, false, true, true),
		PACS_REGISTER_PARAM(true, false, true, false),
		PACS_REGISTER_PARAM(true, false, false, false),

		/* valid src_pac combinations */
		PACS_REGISTER_PARAM(true, true, true, true),
		PACS_REGISTER_PARAM(true, false, true, true),
		PACS_REGISTER_PARAM(false, false, true, true),
		PACS_REGISTER_PARAM(true, true, true, false),
		PACS_REGISTER_PARAM(true, false, true, false),
		PACS_REGISTER_PARAM(false, false, true, false),
	};

	for (size_t i = 0U; i < ARRAY_SIZE(pacs_params); i++) {
		int err;

		err = bt_pacs_register(&pacs_params[i]);
		zassert_equal(err, 0, "[%zu]: Unexpected return value %d", i, err);

		err = bt_pacs_unregister();
		zassert_equal(err, 0, "[%zu]: Unexpected return value %d", i, err);
	}
}

static ZTEST(pacs_test_suite, test_pacs_register_inval_null_param)
{
	int err;

	err = bt_pacs_register(NULL);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST(pacs_test_suite, test_pacs_register_inval_double_register)
{
	const struct bt_pacs_register_param pacs_param =
		PACS_REGISTER_PARAM(true, true, true, true);
	int err;

	err = bt_pacs_register(&pacs_param);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_pacs_register(&pacs_param);
	zassert_equal(err, -EALREADY, "Unexpected return value %d", err);
}

static ZTEST(pacs_test_suite, test_pacs_register_inval_snk_loc_without_snk_pac)
{
	const struct bt_pacs_register_param pacs_param =
		PACS_REGISTER_PARAM(false, true, true, true);
	int err;

	if (!(IS_ENABLED(CONFIG_BT_PAC_SNK) && IS_ENABLED(CONFIG_BT_PAC_SNK_LOC))) {
		ztest_test_skip();
	}

	err = bt_pacs_register(&pacs_param);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST(pacs_test_suite, test_pacs_register_inval_src_loc_without_src_pac)
{
	const struct bt_pacs_register_param pacs_param =
		PACS_REGISTER_PARAM(true, true, false, true);
	int err;

	if (!(IS_ENABLED(CONFIG_BT_PAC_SRC) && IS_ENABLED(CONFIG_BT_PAC_SRC_LOC))) {
		ztest_test_skip();
	}

	err = bt_pacs_register(&pacs_param);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST(pacs_test_suite, test_pacs_register_inval_no_pac)
{
	const struct bt_pacs_register_param pacs_param =
		PACS_REGISTER_PARAM(false, false, false, false);
	int err;

	if (!(IS_ENABLED(CONFIG_BT_PAC_SNK) && IS_ENABLED(CONFIG_BT_PAC_SNK_LOC))) {
		ztest_test_skip();
	}

	err = bt_pacs_register(&pacs_param);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}
