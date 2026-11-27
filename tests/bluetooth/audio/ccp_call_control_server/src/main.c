/* main.c - Application main entry point */

/*
 * Copyright (c) 2024-2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/assigned_numbers.h>
#include <zephyr/bluetooth/audio/ccp.h>
#include <zephyr/bluetooth/audio/tbs.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/ztest_test.h>
#include <zephyr/ztest_assert.h>

#include "conn.h"

DEFINE_FFF_GLOBALS;

#define DEFAULT_BEARER_NAME "test"
#define DEFAULT_BEARER_UCI  "un999"
#define DEFAULT_BEARER_TECH BT_BEARER_TECH_3G
#define DEFAULT_BEARER_URI_SCHEMES "tel,skype"

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
		.uci = DEFAULT_BEARER_UCI,
		.uri_schemes_supported = DEFAULT_BEARER_URI_SCHEMES,
		.gtbs = true,
		.authorization_required = false,
		.technology = DEFAULT_BEARER_TECH,
		.optional_opcodes = 0,
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
			.uci = DEFAULT_BEARER_UCI,
			.uri_schemes_supported = DEFAULT_BEARER_URI_SCHEMES,
			.gtbs = false,
			.authorization_required = false,
			.technology = BT_BEARER_TECH_3G,
			.optional_opcodes = 0,
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
		.uci = DEFAULT_BEARER_UCI,
		.uri_schemes_supported = DEFAULT_BEARER_URI_SCHEMES,
		.gtbs = true,
		.authorization_required = false,
		.technology = BT_BEARER_TECH_3G,
		.optional_opcodes = 0,
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
		.uci = DEFAULT_BEARER_UCI,
		.uri_schemes_supported = DEFAULT_BEARER_URI_SCHEMES,
		.gtbs = false,
		.authorization_required = false,
		.technology = BT_BEARER_TECH_3G,
		.optional_opcodes = 0,
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
		.uci = DEFAULT_BEARER_UCI,
		.uri_schemes_supported = DEFAULT_BEARER_URI_SCHEMES,
		.gtbs = true,
		.authorization_required = false,
		.technology = BT_BEARER_TECH_3G,
		.optional_opcodes = 0,
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
		.uci = DEFAULT_BEARER_UCI,
		.uri_schemes_supported = DEFAULT_BEARER_URI_SCHEMES,
		.gtbs = false,
		.authorization_required = false,
		.technology = BT_BEARER_TECH_3G,
		.optional_opcodes = 0,
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
	char res_bearer_name[CONFIG_BT_CCP_CALL_CONTROL_SERVER_PROVIDER_NAME_MAX_LENGTH + 1];
	const char *new_bearer_name = "New bearer name";
	int err;

	register_default_bearer(fixture);

	err = bt_ccp_call_control_server_set_bearer_provider_name(fixture->bearers[0],
								  new_bearer_name);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_ccp_call_control_server_get_bearer_provider_name(
		fixture->bearers[0], res_bearer_name, sizeof(res_bearer_name));
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
	char inval_bearer_name[CONFIG_BT_CCP_CALL_CONTROL_SERVER_PROVIDER_NAME_MAX_LENGTH + 2];
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(inval_bearer_name); i++) {
		inval_bearer_name[i] = 'a';
	}
	inval_bearer_name[sizeof(inval_bearer_name) - 1U] = '\0';

	register_default_bearer(fixture);

	err = bt_ccp_call_control_server_set_bearer_provider_name(fixture->bearers[0],
								  inval_bearer_name);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_bt_ccp_call_control_server_get_bearer_provider_name)
{
	char res_bearer_name[CONFIG_BT_CCP_CALL_CONTROL_SERVER_PROVIDER_NAME_MAX_LENGTH + 1];
	int err;

	register_default_bearer(fixture);

	err = bt_ccp_call_control_server_get_bearer_provider_name(
		fixture->bearers[0], res_bearer_name, sizeof(res_bearer_name));
	zassert_equal(err, 0, "Unexpected return value %d", err);

	zassert_str_equal(DEFAULT_BEARER_NAME, res_bearer_name, "%s != %s", DEFAULT_BEARER_NAME,
			  res_bearer_name);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_bt_ccp_call_control_server_get_bearer_provider_name_inval_not_registered)
{
	char res_bearer_name[CONFIG_BT_CCP_CALL_CONTROL_SERVER_PROVIDER_NAME_MAX_LENGTH + 1];
	int err;

	/* Register and unregister bearer to get a valid pointer but where it is unregistered*/
	register_default_bearer(fixture);
	err = bt_ccp_call_control_server_unregister_bearer(fixture->bearers[0]);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_ccp_call_control_server_get_bearer_provider_name(
		fixture->bearers[0], res_bearer_name, sizeof(res_bearer_name));
	zassert_equal(err, -EFAULT, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_bt_ccp_call_control_server_get_bearer_provider_name_inval_null_bearer)
{
	char res_bearer_name[CONFIG_BT_CCP_CALL_CONTROL_SERVER_PROVIDER_NAME_MAX_LENGTH + 1];
	int err;

	register_default_bearer(fixture);

	err = bt_ccp_call_control_server_get_bearer_provider_name(NULL, res_bearer_name,
								  sizeof(res_bearer_name));
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_bt_ccp_call_control_server_get_bearer_provider_name_inval_null_name)
{
	int err;

	register_default_bearer(fixture);

	err = bt_ccp_call_control_server_get_bearer_provider_name(fixture->bearers[0], NULL, 0);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_bt_ccp_call_control_server_get_bearer_provider_name_inval_size_0)
{
	char res_bearer_name[CONFIG_BT_CCP_CALL_CONTROL_SERVER_PROVIDER_NAME_MAX_LENGTH + 1];
	int err;

	register_default_bearer(fixture);

	err = bt_ccp_call_control_server_get_bearer_provider_name(fixture->bearers[0],
								  res_bearer_name, 0);
	zassert_equal(err, -ENOMEM, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_bt_ccp_call_control_server_get_bearer_provider_name_inval_small_size)
{
	char res_bearer_name[CONFIG_BT_CCP_CALL_CONTROL_SERVER_PROVIDER_NAME_MAX_LENGTH + 1];
	int err;

	register_default_bearer(fixture);

	err = bt_ccp_call_control_server_get_bearer_provider_name(fixture->bearers[0],
								  res_bearer_name, 1);
	zassert_equal(err, -ENOMEM, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_server_test_suite, test_bt_ccp_call_control_server_get_bearer_uci)
{
	char res_bearer_uci[BT_TBS_MAX_UCI_SIZE];
	int err;

	register_default_bearer(fixture);

	err = bt_ccp_call_control_server_get_bearer_uci(fixture->bearers[0], res_bearer_uci);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	zassert_str_equal(DEFAULT_BEARER_UCI, res_bearer_uci, "%s != %s", DEFAULT_BEARER_UCI,
			  res_bearer_uci);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_bt_ccp_call_control_server_get_bearer_uci_inval_not_registered)
{
	char res_bearer_uci[BT_TBS_MAX_UCI_SIZE];
	int err;

	/* Register and unregister bearer to get a valid pointer but where it is unregistered*/
	register_default_bearer(fixture);
	err = bt_ccp_call_control_server_unregister_bearer(fixture->bearers[0]);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_ccp_call_control_server_get_bearer_uci(fixture->bearers[0], res_bearer_uci);
	zassert_equal(err, -EFAULT, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_bt_ccp_call_control_server_get_bearer_uci_inval_null_bearer)
{
	char res_bearer_uci[BT_TBS_MAX_UCI_SIZE];
	int err;

	register_default_bearer(fixture);

	err = bt_ccp_call_control_server_get_bearer_uci(NULL, res_bearer_uci);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_bt_ccp_call_control_server_get_bearer_uci_inval_null_uci)
{
	int err;

	register_default_bearer(fixture);

	err = bt_ccp_call_control_server_get_bearer_uci(fixture->bearers[0], NULL);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_server_test_suite, test_bt_ccp_call_control_server_set_bearer_tech)
{
	const enum bt_bearer_tech new_bearer_tech = BT_BEARER_TECH_3G;
	enum bt_bearer_tech res_bearer_tech;
	int err;

	register_default_bearer(fixture);

	err = bt_ccp_call_control_server_set_bearer_tech(fixture->bearers[0], new_bearer_tech);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_ccp_call_control_server_get_bearer_tech(fixture->bearers[0], &res_bearer_tech);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	zassert_equal(new_bearer_tech, res_bearer_tech, "%d != %d", new_bearer_tech,
		      res_bearer_tech);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_bt_ccp_call_control_server_set_bearer_tech_inval_not_registered)
{
	const enum bt_bearer_tech new_bearer_tech = BT_BEARER_TECH_3G;
	int err;

	/* Register and unregister bearer to get a valid pointer but where it is unregistered*/
	register_default_bearer(fixture);
	err = bt_ccp_call_control_server_unregister_bearer(fixture->bearers[0]);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_ccp_call_control_server_set_bearer_tech(fixture->bearers[0], new_bearer_tech);
	zassert_equal(err, -EFAULT, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_bt_ccp_call_control_server_set_bearer_tech_inval_null_bearer)
{
	const enum bt_bearer_tech new_bearer_tech = BT_BEARER_TECH_3G;
	int err;

	register_default_bearer(fixture);

	err = bt_ccp_call_control_server_set_bearer_tech(NULL, new_bearer_tech);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_bt_ccp_call_control_server_set_bearer_tech_inval_tech)
{
	int err;

	register_default_bearer(fixture);

	err = bt_ccp_call_control_server_set_bearer_tech(fixture->bearers[0], 0x10);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_server_test_suite, test_bt_ccp_call_control_server_get_bearer_tech)
{
	enum bt_bearer_tech res_bearer_tech;
	int err;

	register_default_bearer(fixture);

	err = bt_ccp_call_control_server_get_bearer_tech(fixture->bearers[0], &res_bearer_tech);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	zassert_equal(DEFAULT_BEARER_TECH, res_bearer_tech, "%d != %d", DEFAULT_BEARER_TECH,
		      res_bearer_tech);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_bt_ccp_call_control_server_get_bearer_tech_inval_not_registered)
{
	enum bt_bearer_tech res_bearer_tech;
	int err;

	/* Register and unregister bearer to get a valid pointer but where it is unregistered*/
	register_default_bearer(fixture);
	err = bt_ccp_call_control_server_unregister_bearer(fixture->bearers[0]);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_ccp_call_control_server_get_bearer_tech(fixture->bearers[0], &res_bearer_tech);
	zassert_equal(err, -EFAULT, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_bt_ccp_call_control_server_get_bearer_tech_inval_null_bearer)
{
	enum bt_bearer_tech res_bearer_tech;
	int err;

	register_default_bearer(fixture);

	err = bt_ccp_call_control_server_get_bearer_tech(NULL, &res_bearer_tech);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_bt_ccp_call_control_server_get_bearer_tech_inval_null_tech)
{
	int err;

	register_default_bearer(fixture);

	err = bt_ccp_call_control_server_get_bearer_tech(fixture->bearers[0], NULL);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_bt_ccp_call_control_server_set_bearer_uri_schemes)
{
	char res_uri_schemes[CONFIG_BT_CCP_CALL_CONTROL_SERVER_URI_SCHEMES_MAX_LENGTH + 1];
	/* Contains a mix of simple URIs, but also ones that use special characters like `-`, `+`,
	 * `.` and capital letters.
	 */
	const char *new_uri_schemes =
		"tel,chrome-extension,coap+tcp,xmlrpc.beep,machineProvisioningProgressReporter";
	int err;

	register_default_bearer(fixture);

	err = bt_ccp_call_control_server_set_bearer_uri_schemes(fixture->bearers[0],
								new_uri_schemes);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_ccp_call_control_server_get_bearer_uri_schemes(
		fixture->bearers[0], res_uri_schemes, sizeof(res_uri_schemes));
	zassert_equal(err, 0, "Unexpected return value %d", err);

	zassert_str_equal(new_uri_schemes, res_uri_schemes, "%s != %s", new_uri_schemes,
			  res_uri_schemes);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_bt_ccp_call_control_server_set_bearer_uri_schemes_no_change)
{
	char res_uri_schemes[CONFIG_BT_CCP_CALL_CONTROL_SERVER_URI_SCHEMES_MAX_LENGTH + 1];
	/* Contains a mix of simple URIs, but also ones that use special characters like `-`, `+`,
	 * `.` and capital letters.
	 */
	const char *same_uri_schemes = DEFAULT_BEARER_URI_SCHEMES;
	int err;

	register_default_bearer(fixture);

	/* Verify that the `same_uri_schemes` is already set */
	err = bt_ccp_call_control_server_get_bearer_uri_schemes(
		fixture->bearers[0], res_uri_schemes, sizeof(res_uri_schemes));
	zassert_equal(err, 0, "Unexpected return value %d", err);

	zassert_str_equal(same_uri_schemes, res_uri_schemes, "%s != %s", same_uri_schemes,
			  res_uri_schemes);

	/* Attempt to set again */
	err = bt_ccp_call_control_server_set_bearer_uri_schemes(fixture->bearers[0],
								same_uri_schemes);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	/* Verify no change */
	err = bt_ccp_call_control_server_get_bearer_uri_schemes(
		fixture->bearers[0], res_uri_schemes, sizeof(res_uri_schemes));
	zassert_equal(err, 0, "Unexpected return value %d", err);

	zassert_str_equal(same_uri_schemes, res_uri_schemes, "%s != %s", same_uri_schemes,
			  res_uri_schemes);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_bt_ccp_call_control_server_set_bearer_uri_schemes_inval_not_registered)
{
	const char *new_uri_schemes = "tel";
	int err;

	/* Register and unregister bearer to get a valid pointer but where it is unregistered*/
	register_default_bearer(fixture);
	err = bt_ccp_call_control_server_unregister_bearer(fixture->bearers[0]);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_ccp_call_control_server_set_bearer_uri_schemes(fixture->bearers[0],
								new_uri_schemes);
	zassert_equal(err, -EFAULT, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_bt_ccp_call_control_server_set_bearer_uri_schemes_inval_null_bearer)
{
	const char *new_uri_schemes = "tel";
	int err;

	register_default_bearer(fixture);

	err = bt_ccp_call_control_server_set_bearer_uri_schemes(NULL, new_uri_schemes);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_bt_ccp_call_control_server_set_bearer_uri_schemes_inval_null_uri_schemes)
{
	int err;

	register_default_bearer(fixture);

	err = bt_ccp_call_control_server_set_bearer_uri_schemes(fixture->bearers[0], NULL);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_bt_ccp_call_control_server_set_bearer_uri_schemes_inval_empty_uri_schemes)
{
	const char *inval_bearer_uri_schemes = "";
	int err;

	register_default_bearer(fixture);

	err = bt_ccp_call_control_server_set_bearer_uri_schemes(fixture->bearers[0],
								inval_bearer_uri_schemes);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_bt_ccp_call_control_server_set_bearer_uri_schemes_inval_long_uri_schemes)
{
	char inval_bearer_uri_schemes[CONFIG_BT_CCP_CALL_CONTROL_SERVER_URI_SCHEMES_MAX_LENGTH + 2];
	int err;

	for (size_t i = 0; i < ARRAY_SIZE(inval_bearer_uri_schemes); i++) {
		inval_bearer_uri_schemes[i] = 'a';
	}
	inval_bearer_uri_schemes[ARRAY_SIZE(inval_bearer_uri_schemes) - 1] = '\0';

	register_default_bearer(fixture);

	err = bt_ccp_call_control_server_set_bearer_uri_schemes(fixture->bearers[0],
								inval_bearer_uri_schemes);
	zassert_equal(err, -ENOMEM, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_bt_ccp_call_control_server_set_bearer_uri_schemes_inval_inval_chars)
{
	char *inval_bearer_uri_schemes_utf8 = "tel,🌍";
	int err;

	register_default_bearer(fixture);

	err = bt_ccp_call_control_server_set_bearer_uri_schemes(fixture->bearers[0],
								inval_bearer_uri_schemes_utf8);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);

	char *inval_bearer_uri_schemes_missing_after_comma = "tel,";

	err = bt_ccp_call_control_server_set_bearer_uri_schemes(
		fixture->bearers[0], inval_bearer_uri_schemes_missing_after_comma);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);

	char *inval_bearer_uri_schemes_special_char = "tel,a@";

	err = bt_ccp_call_control_server_set_bearer_uri_schemes(
		fixture->bearers[0], inval_bearer_uri_schemes_special_char);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);

	char *inval_bearer_uri_schemes_number_first = "123,tel";

	err = bt_ccp_call_control_server_set_bearer_uri_schemes(
		fixture->bearers[0], inval_bearer_uri_schemes_number_first);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);

	char *inval_bearer_uri_schemes_number_first_2 = "tel,123";

	err = bt_ccp_call_control_server_set_bearer_uri_schemes(
		fixture->bearers[0], inval_bearer_uri_schemes_number_first_2);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_bt_ccp_call_control_server_get_bearer_uri_schemes)
{
	char res_uri_schemes[CONFIG_BT_CCP_CALL_CONTROL_SERVER_URI_SCHEMES_MAX_LENGTH + 1];
	int err;

	register_default_bearer(fixture);

	err = bt_ccp_call_control_server_get_bearer_uri_schemes(
		fixture->bearers[0], res_uri_schemes, sizeof(res_uri_schemes));
	zassert_equal(err, 0, "Unexpected return value %d", err);

	zassert_str_equal(DEFAULT_BEARER_URI_SCHEMES, res_uri_schemes, "%s != %s",
			  DEFAULT_BEARER_NAME, res_uri_schemes);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_bt_ccp_call_control_server_get_bearer_uri_schemes_inval_not_registered)
{
	char res_uri_schemes[CONFIG_BT_CCP_CALL_CONTROL_SERVER_URI_SCHEMES_MAX_LENGTH + 1];
	int err;

	/* Register and unregister bearer to get a valid pointer but where it is unregistered*/
	register_default_bearer(fixture);
	err = bt_ccp_call_control_server_unregister_bearer(fixture->bearers[0]);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_ccp_call_control_server_get_bearer_uri_schemes(
		fixture->bearers[0], res_uri_schemes, sizeof(res_uri_schemes));
	zassert_equal(err, -EFAULT, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_bt_ccp_call_control_server_get_bearer_uri_schemes_inval_null_bearer)
{
	char res_uri_schemes[CONFIG_BT_CCP_CALL_CONTROL_SERVER_URI_SCHEMES_MAX_LENGTH + 1];
	int err;

	register_default_bearer(fixture);

	err = bt_ccp_call_control_server_get_bearer_uri_schemes(NULL, res_uri_schemes,
								sizeof(res_uri_schemes));
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_bt_ccp_call_control_server_get_bearer_uri_schemes_inval_null_uri_schemes)
{
	int err;

	register_default_bearer(fixture);

	err = bt_ccp_call_control_server_get_bearer_uri_schemes(
		fixture->bearers[0], NULL,
		CONFIG_BT_CCP_CALL_CONTROL_SERVER_URI_SCHEMES_MAX_LENGTH);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_server_test_suite,
	       test_bt_ccp_call_control_server_get_bearer_uri_schemes_inval_zero_size)
{
	char res_uri_schemes[CONFIG_BT_CCP_CALL_CONTROL_SERVER_URI_SCHEMES_MAX_LENGTH + 1];
	int err;

	register_default_bearer(fixture);

	err = bt_ccp_call_control_server_get_bearer_uri_schemes(fixture->bearers[0],
								res_uri_schemes, 0U);
	zassert_equal(err, -ENOMEM, "Unexpected return value %d", err);
}

/* Read a characteristic value of the TBS or GTBS server, as a connected client would */
static ssize_t read_attr(const struct bt_gatt_attr *attr, void *buf, uint16_t len)
{
	struct bt_conn conn = {
		.index = 0U,
		.info.type = BT_CONN_TYPE_LE,
		.info.role = BT_CONN_ROLE_PERIPHERAL,
		.info.state = BT_CONN_STATE_CONNECTED,
		.info.security.level = BT_SECURITY_L2,
		.info.security.enc_key_size = BT_ENC_KEY_SIZE_MAX,
		.info.security.flags = BT_SECURITY_FLAG_OOB | BT_SECURITY_FLAG_SC,
	};

	return attr->read(&conn, attr, buf, len, 0U);
}

static void register_gtbs_and_tbs(struct ccp_call_control_server_test_suite_fixture *fixture,
				  char *gtbs_uri_schemes, char *tbs_uri_schemes)
{
	struct bt_tbs_register_param register_param = {
		.provider_name = DEFAULT_BEARER_NAME,
		.uci = DEFAULT_BEARER_UCI,
		.uri_schemes_supported = gtbs_uri_schemes,
		.gtbs = true,
		.authorization_required = false,
		.technology = DEFAULT_BEARER_TECH,
		.optional_opcodes = BT_TBS_OPTIONAL_OPCODE_HOLD | BT_TBS_OPTIONAL_OPCODE_JOIN,
	};
	int err;

	err = bt_ccp_call_control_server_register_bearer(&register_param, &fixture->bearers[0]);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	register_param.uri_schemes_supported = tbs_uri_schemes;
	register_param.gtbs = false;

	err = bt_ccp_call_control_server_register_bearer(&register_param, &fixture->bearers[1]);
	zassert_equal(err, 0, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_server_test_suite, test_tbs_read_uri_scheme_lists)
{
	/* The GTBS list holds its own URI schemes, then those of the TBS bearer it does not have,
	 * so both lists read the same
	 */
	char gtbs_uri_schemes[] = "tel";
	char expected[] = "tel,skype,sip";
	const struct bt_gatt_attr *attr = NULL;
	size_t attr_cnt = 0U;

	if (CONFIG_BT_CCP_CALL_CONTROL_SERVER_BEARER_COUNT == 1) {
		ztest_test_skip();
	}

	register_gtbs_and_tbs(fixture, gtbs_uri_schemes, expected);

	/* Read the list of each bearer twice, as reading the GTBS list must not change the list of
	 * the TBS bearer
	 */
	for (int round = 0; round < 2; round++) {
		attr = NULL;

		for (attr_cnt = 0U;; attr_cnt++) {
			char value[sizeof(expected) + 8];
			ssize_t ret;

			attr = bt_gatt_find_by_uuid(attr == NULL ? NULL : attr + 1, 0,
						    BT_UUID_TBS_URI_LIST);
			if (attr == NULL) {
				break;
			}

			(void)memset(value, 0, sizeof(value));
			ret = read_attr(attr, value, sizeof(value) - 1U);
			zassert_equal(ret, strlen(expected), "[%d][%zu]: Unexpected length %zd",
				      round, attr_cnt, ret);
			zassert_str_equal(value, expected, "[%d][%zu]: Unexpected list \"%s\"",
					  round, attr_cnt, value);
		}

		zassert_equal(attr_cnt, 2U, "Unexpected number of URI scheme lists %zu", attr_cnt);
	}
}

static ZTEST_F(ccp_call_control_server_test_suite, test_tbs_read_status_flags)
{
	/* Differs from the Optional Opcodes of the bearer */
	const uint16_t status_flags = BT_TBS_STATUS_FLAG_SILENT_MOD;
	const struct bt_gatt_attr *attr;
	uint16_t value;
	ssize_t ret;
	int err;

	register_default_bearer(fixture);

	err = bt_tbs_set_status_flags(BT_TBS_GTBS_INDEX, status_flags);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	attr = bt_gatt_find_by_uuid(NULL, 0, BT_UUID_TBS_STATUS_FLAGS);
	zassert_not_null(attr, "Could not find Status Flags");

	ret = read_attr(attr, &value, sizeof(value));
	zassert_equal(ret, sizeof(value), "Unexpected length %zd", ret);
	zassert_equal(sys_le16_to_cpu(value), status_flags, "Unexpected Status Flags 0x%04x",
		      sys_le16_to_cpu(value));
}

static K_SEM_DEFINE(conn_foreach_entered, 0, 1);
static K_SEM_DEFINE(conn_foreach_release, 0, 1);

/* TBS walks the connections with tbs_mutex held, so blocking here keeps the mutex locked */
static void blocking_conn_foreach(enum bt_conn_type type, bt_conn_foreach_cb func, void *data)
{
	ARG_UNUSED(type);
	ARG_UNUSED(func);
	ARG_UNUSED(data);

	k_sem_give(&conn_foreach_entered);
	(void)k_sem_take(&conn_foreach_release, K_FOREVER);
}

#define REMOTE_INCOMING_STACK_SIZE 2048
static K_THREAD_STACK_DEFINE(remote_incoming_stack, REMOTE_INCOMING_STACK_SIZE);
static struct k_thread remote_incoming_thread;
static int remote_incoming_ret;

static int remote_incoming(void)
{
	return bt_tbs_remote_incoming(BT_TBS_GTBS_INDEX, "tel:123", "tel:456", "test");
}

static void remote_incoming_thread_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	remote_incoming_ret = remote_incoming();
}

static ZTEST_F(ccp_call_control_server_test_suite, test_tbs_remote_incoming_busy)
{
	int busy_ret = 0;
	int err;

	register_default_bearer(fixture);

	/* Hold tbs_mutex in another thread, in the middle of a remote incoming call */
	bt_conn_foreach_fake.custom_fake = blocking_conn_foreach;
	(void)k_thread_create(&remote_incoming_thread, remote_incoming_stack,
			      K_THREAD_STACK_SIZEOF(remote_incoming_stack),
			      remote_incoming_thread_fn, NULL, NULL, NULL,
			      k_thread_priority_get(k_current_get()), 0, K_NO_WAIT);
	err = k_sem_take(&conn_foreach_entered, K_SECONDS(1));

	/* Block only the first walk, as the other thread walks the connections again */
	bt_conn_foreach_fake.custom_fake = NULL;

	if (err == 0) {
		busy_ret = remote_incoming();
	}

	/* Let the other thread finish before any assertion can end the test. Once released it does
	 * not block again, and it must not be aborted, as it may hold tbs_mutex.
	 */
	k_sem_give(&conn_foreach_release);
	(void)k_thread_join(&remote_incoming_thread, K_FOREVER);

	RESET_FAKE(bt_conn_foreach);

	zassert_equal(err, 0, "The other thread did not lock tbs_mutex");
	zassert_equal(busy_ret, -EBUSY, "Unexpected return value %d", busy_ret);
	zassert_true(remote_incoming_ret > 0, "Unexpected return value %d", remote_incoming_ret);

	/* The call that failed with -EBUSY must not take a call slot */
	for (int i = 1; i < CONFIG_BT_TBS_MAX_CALLS; i++) {
		const int ret = remote_incoming();

		zassert_true(ret > 0, "[%d]: Unexpected return value %d", i, ret);
	}
}
