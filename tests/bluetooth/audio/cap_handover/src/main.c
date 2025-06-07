/* main.c - Application main entry point */

/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/fff.h>
#include <zephyr/sys/util.h>

#include "cap_initiator.h"
#include "conn.h"
#include "test_common.h"
#include "ztest_assert.h"
#include "ztest_test.h"

static void mock_init_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	test_mocks_init();
}

static void mock_destroy_rule_after(const struct ztest_unit_test *test, void *fixture)
{
	test_mocks_cleanup();
}

ZTEST_RULE(mock_rule, mock_init_rule_before, mock_destroy_rule_after);

struct cap_handover_test_suite_fixture {
	struct bt_conn conns[CONFIG_BT_MAX_CONN];
};

static void cap_handover_test_suite_fixture_init(struct cap_handover_test_suite_fixture *fixture)
{
	for (size_t i = 0; i < ARRAY_SIZE(fixture->conns); i++) {
		test_conn_init(&fixture->conns[i]);
	}
}

static void *cap_handover_test_suite_setup(void)
{
	struct cap_handover_test_suite_fixture *fixture;

	fixture = malloc(sizeof(*fixture));
	zassert_not_null(fixture);

	return fixture;
}

static void cap_handover_test_suite_before(void *f)
{
	int err;

	memset(f, 0, sizeof(struct cap_handover_test_suite_fixture));
	cap_handover_test_suite_fixture_init(f);

	err = bt_cap_initiator_register_cb(&mock_cap_initiator_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);
}

static void cap_handover_test_suite_after(void *f)
{
	struct cap_handover_test_suite_fixture *fixture = f;

	bt_cap_initiator_unregister_cb(&mock_cap_initiator_cb);

	for (size_t i = 0; i < ARRAY_SIZE(fixture->conns); i++) {
		mock_bt_conn_disconnected(&fixture->conns[i], BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	}
}

static void cap_handover_test_suite_teardown(void *f)
{
	free(f);
}

ZTEST_SUITE(cap_handover_test_suite, NULL, cap_handover_test_suite_setup,
	    cap_handover_test_suite_before, cap_handover_test_suite_after,
	    cap_handover_test_suite_teardown);

ZTEST_F(cap_handover_test_suite, test_handover_register_cb)
{
	int err = 0;

	zassert_equal(0, err, "Unexpected return value %d", err);
}
