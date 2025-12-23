/* main.c - Application main entry point */

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/ccp.h>
#include <zephyr/bluetooth/audio/tbs.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/fff.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/ztest_test.h>
#include <zephyr/ztest_assert.h>

DEFINE_FFF_GLOBALS;

#define DEFAULT_BEARER_NAME "test"

struct ccp_call_control_server_test_suite_fixture {
	/** Need 1 additional bearer than the max to trigger some corner cases */
	struct bt_ccp_call_control_server_bearer
		*bearers[CONFIG_BT_CCP_CALL_CONTROL_SERVER_BEARER_COUNT + 1];
};

static void *ccp_call_control_server_test_suite_setup(void)
{
	struct ccp_call_control_server_test_suite_fixture *fixture;

	fixture = malloc(sizeof(*fixture));
	zassert_not_null(fixture);

	return fixture;
}

static void ccp_call_control_server_test_suite_before(void *f)
{
	memset(f, 0, sizeof(struct ccp_call_control_server_test_suite_fixture));
}

static void ccp_call_control_server_test_suite_after(void *f)
{
	struct ccp_call_control_server_test_suite_fixture *fixture = f;

	/* We unregister from largest to lowest index, as GTBS shall be unregistered last and is
	 * always at index 0
	 */
	for (size_t i = ARRAY_SIZE(fixture->bearers); i > 0; i--) {
		/* Since size_t cannot be negative, we cannot use the regular i >= 0 when counting
		 * downwards as that will always be true, so we use an additional index variable
		 */
		const size_t index_to_unreg = i - 1;

		if (fixture->bearers[index_to_unreg] != NULL) {
			bt_ccp_call_control_server_unregister_bearer(
				fixture->bearers[index_to_unreg]);
		}

		fixture->bearers[index_to_unreg] = NULL;
	}
}

static void ccp_call_control_server_test_suite_teardown(void *f)
{
	free(f);
}

ZTEST_SUITE(ccp_call_control_server_test_suite, NULL, ccp_call_control_server_test_suite_setup,
	    ccp_call_control_server_test_suite_before, ccp_call_control_server_test_suite_after,
	    ccp_call_control_server_test_suite_teardown);

static void register_default_bearer(struct ccp_call_control_server_test_suite_fixture *fixture)
{
	const struct bt_tbs_register_param register_param = {
		.provider_name = DEFAULT_BEARER_NAME,
		.uci = "un999",
		.uri_schemes_supported = "tel",
		.gtbs = true,
		.authorization_required = false,
		.technology = BT_TBS_TECHNOLOGY_3G,
		.supported_features = 0,
	};
	int err;

	err = bt_ccp_call_control_server_register_bearer(&register_param, &fixture->bearers[0]);
	zassert_equal(err, 0, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_server_test_suite, test_ccp_call_control_server_register_bearer)
{
	register_default_bearer(fixture);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_ccp_call_control_server_register_multiple_bearers)
{
	if (CONFIG_BT_CCP_CALL_CONTROL_SERVER_BEARER_COUNT == 1) {
		ztest_test_skip();
	}

	register_default_bearer(fixture);

	for (int i = 1; i < CONFIG_BT_CCP_CALL_CONTROL_SERVER_BEARER_COUNT; i++) {
		struct bt_tbs_register_param register_param = {
			.provider_name = "test",
			.uci = "un999",
			.uri_schemes_supported = "tel",
			.gtbs = false,
			.authorization_required = false,
			.technology = BT_TBS_TECHNOLOGY_3G,
			.supported_features = 0,
		};
		int err;

		err = bt_ccp_call_control_server_register_bearer(&register_param,
								 &fixture->bearers[i]);
		zassert_equal(err, 0, "Unexpected return value %d", err);
	}
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_ccp_call_control_server_register_bearer_inval_null_param)
{
	int err;

	err = bt_ccp_call_control_server_register_bearer(NULL, &fixture->bearers[0]);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_ccp_call_control_server_register_bearer_inval_null_bearer)
{
	const struct bt_tbs_register_param register_param = {
		.provider_name = "test",
		.uci = "un999",
		.uri_schemes_supported = "tel",
		.gtbs = true,
		.authorization_required = false,
		.technology = BT_TBS_TECHNOLOGY_3G,
		.supported_features = 0,
	};
	int err;

	err = bt_ccp_call_control_server_register_bearer(&register_param, NULL);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_ccp_call_control_server_register_bearer_inval_no_gtbs)
{
	const struct bt_tbs_register_param register_param = {
		.provider_name = "test",
		.uci = "un999",
		.uri_schemes_supported = "tel",
		.gtbs = false,
		.authorization_required = false,
		.technology = BT_TBS_TECHNOLOGY_3G,
		.supported_features = 0,
	};
	int err;

	err = bt_ccp_call_control_server_register_bearer(&register_param, &fixture->bearers[0]);
	zassert_equal(err, -EAGAIN, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_ccp_call_control_server_register_bearer_inval_double_gtbs)
{
	const struct bt_tbs_register_param register_param = {
		.provider_name = "test",
		.uci = "un999",
		.uri_schemes_supported = "tel",
		.gtbs = true,
		.authorization_required = false,
		.technology = BT_TBS_TECHNOLOGY_3G,
		.supported_features = 0,
	};
	int err;

	if (CONFIG_BT_CCP_CALL_CONTROL_SERVER_BEARER_COUNT == 1) {
		ztest_test_skip();
	}

	register_default_bearer(fixture);

	err = bt_ccp_call_control_server_register_bearer(&register_param, &fixture->bearers[1]);
	zassert_equal(err, -EALREADY, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_ccp_call_control_server_register_bearer_inval_cnt)
{
	const struct bt_tbs_register_param register_param = {
		.provider_name = "test",
		.uci = "un999",
		.uri_schemes_supported = "tel",
		.gtbs = false,
		.authorization_required = false,
		.technology = BT_TBS_TECHNOLOGY_3G,
		.supported_features = 0,
	};
	int err;

	if (CONFIG_BT_CCP_CALL_CONTROL_SERVER_BEARER_COUNT == 1) {
		ztest_test_skip();
	}

	register_default_bearer(fixture);

	for (int i = 1; i < CONFIG_BT_CCP_CALL_CONTROL_SERVER_BEARER_COUNT; i++) {

		err = bt_ccp_call_control_server_register_bearer(&register_param,
								 &fixture->bearers[i]);
		zassert_equal(err, 0, "Unexpected return value %d", err);
	}

	err = bt_ccp_call_control_server_register_bearer(
		&register_param, &fixture->bearers[CONFIG_BT_CCP_CALL_CONTROL_SERVER_BEARER_COUNT]);
	zassert_equal(err, -ENOMEM, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_server_test_suite, test_ccp_call_control_server_unregister_bearer)
{
	int err;

	register_default_bearer(fixture);

	err = bt_ccp_call_control_server_unregister_bearer(fixture->bearers[0]);
	zassert_equal(err, 0, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_ccp_call_control_server_unregister_bearer_inval_double_unregister)
{
	int err;

	register_default_bearer(fixture);

	err = bt_ccp_call_control_server_unregister_bearer(fixture->bearers[0]);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_ccp_call_control_server_unregister_bearer(fixture->bearers[0]);
	zassert_equal(err, -EALREADY, "Unexpected return value %d", err);

	fixture->bearers[0] = NULL;
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_ccp_call_control_server_unregister_bearer_inval_null_bearer)
{
	int err;

	err = bt_ccp_call_control_server_unregister_bearer(NULL);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_bt_ccp_call_control_server_set_bearer_provider_name)
{
	const char *new_bearer_name = "New bearer name";
	const char *res_bearer_name;
	int err;

	register_default_bearer(fixture);

	err = bt_ccp_call_control_server_set_bearer_provider_name(fixture->bearers[0],
								  new_bearer_name);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_ccp_call_control_server_get_bearer_provider_name(fixture->bearers[0],
								  &res_bearer_name);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	zassert_str_equal(new_bearer_name, res_bearer_name, "%s != %s", new_bearer_name,
			  res_bearer_name);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_bt_ccp_call_control_server_set_bearer_provider_name_inval_not_registered)
{
	const char *new_bearer_name = "New bearer name";
	int err;

	/* Register and unregister bearer to get a valid pointer but where it is unregistered*/
	register_default_bearer(fixture);
	err = bt_ccp_call_control_server_unregister_bearer(fixture->bearers[0]);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_ccp_call_control_server_set_bearer_provider_name(fixture->bearers[0],
								  new_bearer_name);
	zassert_equal(err, -EFAULT, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_bt_ccp_call_control_server_set_bearer_provider_name_inval_null_bearer)
{
	const char *new_bearer_name = "New bearer name";
	int err;

	register_default_bearer(fixture);

	err = bt_ccp_call_control_server_set_bearer_provider_name(NULL, new_bearer_name);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_bt_ccp_call_control_server_set_bearer_provider_name_inval_null_name)
{
	int err;

	register_default_bearer(fixture);

	err = bt_ccp_call_control_server_set_bearer_provider_name(fixture->bearers[0], NULL);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_bt_ccp_call_control_server_set_bearer_provider_name_inval_empty_name)
{
	const char *inval_bearer_name = "";
	int err;

	register_default_bearer(fixture);

	err = bt_ccp_call_control_server_set_bearer_provider_name(fixture->bearers[0],
								  inval_bearer_name);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_bt_ccp_call_control_server_set_bearer_provider_name_inval_long_name)
{
	char inval_bearer_name[CONFIG_BT_CCP_CALL_CONTROL_SERVER_PROVIDER_NAME_MAX_LENGTH + 1];
	int err;

	for (size_t i = 0; i < ARRAY_SIZE(inval_bearer_name); i++) {
		inval_bearer_name[i] = 'a';
	}

	register_default_bearer(fixture);

	err = bt_ccp_call_control_server_set_bearer_provider_name(fixture->bearers[0],
								  inval_bearer_name);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_bt_ccp_call_control_server_get_bearer_provider_name)
{
	const char *res_bearer_name;
	int err;

	register_default_bearer(fixture);

	err = bt_ccp_call_control_server_get_bearer_provider_name(fixture->bearers[0],
								  &res_bearer_name);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	zassert_str_equal(DEFAULT_BEARER_NAME, res_bearer_name, "%s != %s", DEFAULT_BEARER_NAME,
			  res_bearer_name);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_bt_ccp_call_control_server_get_bearer_provider_name_inval_not_registered)
{
	const char *res_bearer_name;
	int err;

	/* Register and unregister bearer to get a valid pointer but where it is unregistered*/
	register_default_bearer(fixture);
	err = bt_ccp_call_control_server_unregister_bearer(fixture->bearers[0]);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_ccp_call_control_server_get_bearer_provider_name(fixture->bearers[0],
								  &res_bearer_name);
	zassert_equal(err, -EFAULT, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_bt_ccp_call_control_server_get_bearer_provider_name_inval_null_bearer)
{
	const char *res_bearer_name;
	int err;

	register_default_bearer(fixture);

	err = bt_ccp_call_control_server_get_bearer_provider_name(NULL, &res_bearer_name);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_bt_ccp_call_control_server_get_bearer_provider_name_inval_null_name)
{
	int err;

	register_default_bearer(fixture);

	err = bt_ccp_call_control_server_get_bearer_provider_name(fixture->bearers[0], NULL);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}
