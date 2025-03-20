/* main.c - Application main entry point */

/*
 * Copyright (c) 2024 Demant A/S
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/pacs.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/ztest_assert.h>
#include <zephyr/ztest_test.h>

#include "assert.h"
#include "bap_unicast_server.h"
#include "bap_stream.h"

#include "test_common.h"

static void ascs_register_test_suite_after(void *f)
{
	int err;

	err = bt_bap_unicast_server_unregister();

	/* In case of a failure due to pending state change, try again */
	while (err != 0 && err != -EALREADY) {
		/* Only -EBUSY indicate pending state change */
		zassert_equal(err, -EBUSY, "unexpected err response %d", err);
		k_sleep(K_MSEC(10));
		err = bt_bap_unicast_server_unregister();
	}
}

ZTEST_SUITE(ascs_register_test_suite, NULL, NULL, NULL, ascs_register_test_suite_after, NULL);

static ZTEST(ascs_register_test_suite, test_ascs_register_with_null_param)
{
	int err;

	err = bt_bap_unicast_server_register(NULL);
	zassert_equal(err, -EINVAL, "Unexpected err response %d", err);
}

static ZTEST(ascs_register_test_suite, test_ascs_register_twice)
{
	int err;
	struct bt_bap_unicast_server_register_param param = {
		CONFIG_BT_ASCS_MAX_ASE_SNK_COUNT,
		CONFIG_BT_ASCS_MAX_ASE_SRC_COUNT,
		&mock_bap_unicast_server_cb
	};

	/* Setup already registered once, so calling once here should be sufficient */
	err = bt_bap_unicast_server_register(&param);
	zassert_equal(err, 0, "Unexpected err response %d", err);

	/* Setup already registered once, so calling once here should be sufficient */
	err = bt_bap_unicast_server_register(&param);
	zassert_equal(err, -EALREADY, "Unexpected err response %d", err);

	err = bt_bap_unicast_server_unregister();
	zassert_equal(err, 0, "Unexpected err response %d", err);
}

static ZTEST(ascs_register_test_suite, test_ascs_register_too_many_sinks)
{
	int err;
	struct bt_bap_unicast_server_register_param param = {
		CONFIG_BT_ASCS_MAX_ASE_SNK_COUNT + 1,
		CONFIG_BT_ASCS_MAX_ASE_SRC_COUNT,
		&mock_bap_unicast_server_cb
	};

	err = bt_bap_unicast_server_register(&param);
	zassert_equal(err, -EINVAL, "Unexpected err response %d", err);
}

static ZTEST(ascs_register_test_suite, test_ascs_register_too_many_sources)
{
	int err;
	struct bt_bap_unicast_server_register_param param = {
		CONFIG_BT_ASCS_MAX_ASE_SNK_COUNT,
		CONFIG_BT_ASCS_MAX_ASE_SRC_COUNT + 1,
		&mock_bap_unicast_server_cb
	};

	err = bt_bap_unicast_server_register(&param);
	zassert_equal(err, -EINVAL, "Unexpected err response %d", err);
}

static ZTEST(ascs_register_test_suite, test_ascs_register_zero_ases)
{
	int err;
	struct bt_bap_unicast_server_register_param param = {0, 0, &mock_bap_unicast_server_cb};

	err = bt_bap_unicast_server_register(&param);
	zassert_equal(err, -EINVAL, "Unexpected err response %d", err);
}

static ZTEST(ascs_register_test_suite, test_ascs_register_without_cb)
{
	int err;
	struct bt_bap_unicast_server_register_param param = {
		CONFIG_BT_ASCS_MAX_ASE_SNK_COUNT,
		CONFIG_BT_ASCS_MAX_ASE_SRC_COUNT + 1,
		NULL
	};

	err = bt_bap_unicast_server_register(&param);
	zassert_equal(err, -EINVAL, "Unexpected err response %d", err);
}

static ZTEST(ascs_register_test_suite, test_ascs_register_fewer_than_max_ases)
{
	int err;
	struct bt_bap_unicast_server_register_param param = {
		CONFIG_BT_ASCS_MAX_ASE_SNK_COUNT > 0 ? CONFIG_BT_ASCS_MAX_ASE_SNK_COUNT - 1 : 0,
		CONFIG_BT_ASCS_MAX_ASE_SRC_COUNT > 0 ? CONFIG_BT_ASCS_MAX_ASE_SRC_COUNT - 1 : 0,
		&mock_bap_unicast_server_cb
	};

	err = bt_bap_unicast_server_register(&param);
	zassert_equal(err, 0, "Unexpected err response %d", err);
}

static ZTEST(ascs_register_test_suite, test_ascs_unregister_without_register)
{
	int err;

	err = bt_bap_unicast_server_unregister();
	zassert_equal(err, -EALREADY, "Unexpected err response %d", err);
}

static ZTEST(ascs_register_test_suite, test_ascs_unregister_with_cbs_registered)
{
	struct bt_bap_unicast_server_register_param param = {
		CONFIG_BT_ASCS_MAX_ASE_SNK_COUNT,
		CONFIG_BT_ASCS_MAX_ASE_SRC_COUNT,
		&mock_bap_unicast_server_cb
	};
	int err;

	err = bt_bap_unicast_server_register(&param);
	zassert_equal(err, 0, "Unexpected err response %d", err);

	err = bt_bap_unicast_server_unregister();
	zassert_equal(err, 0, "Unexpected err response %d", err);
}
