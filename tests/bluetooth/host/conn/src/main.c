/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>

#include "mocks/addr_internal.h"
#include "mocks/att_internal.h"
#include "mocks/bt_str.h"
#include "mocks/buf_view.h"
#include "mocks/hci_core.h"
#include "mocks/id.h"
#include "mocks/kernel.h"
#include "mocks/l2cap_internal.h"
#include "mocks/scan.h"
#include "mocks/smp.h"
#include "mocks/spinlock.h"
#include "mocks/sys_clock.h"

DEFINE_FFF_GLOBALS;

static void fff_reset_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	ADDR_INTERNAL_MOCKS_FFF_FAKES_LIST(RESET_FAKE);
	ATT_INTERNAL_MOCKS_FFF_FAKES_LIST(RESET_FAKE);
	BUF_VIEW_MOCKS_FFF_FAKES_LIST(RESET_FAKE);
	HCI_CORE_MOCKS_FFF_FAKES_LIST(RESET_FAKE);
	ID_MOCKS_FFF_FAKES_LIST(RESET_FAKE);
	KERNEL_MOCKS_FFF_FAKES_LIST(RESET_FAKE);
	L2CAP_INTERNAL_MOCKS_FFF_FAKES_LIST(RESET_FAKE);
	SCAN_MOCKS_FFF_FAKES_LIST(RESET_FAKE);
	SMP_MOCKS_FFF_FAKES_LIST(RESET_FAKE);
	SPINLOCK_MOCKS_FFF_FAKES_LIST(RESET_FAKE);
	SYS_CLOCK_MOCKS_FFF_FAKES_LIST(RESET_FAKE);
}

ZTEST_RULE(fff_reset_rule, fff_reset_rule_before, NULL);

ZTEST_SUITE(conn, NULL, NULL, NULL, NULL, NULL);

/*
 * Test that bt_conn_le_create() returns -EINVAL if conn is not NULL and
 * CONFIG_BT_CONN_CHECK_NULL_BEFORE_CREATE is enabled.
 *
 * The test must be compiled with and without CONFIG_BT_CONN_CHECK_NULL_BEFORE_CREATE
 * to ensure that the -EINVAL error is returned only when this Kconfig option is enabled.
 */
ZTEST(conn, test_bt_conn_le_create_check_null_conn)
{
	bt_addr_le_t peer = {.a.val = {0x01}};
	const struct bt_conn_le_create_param create_param = {
		.options = BT_CONN_LE_OPT_NONE,
		.interval = BT_GAP_SCAN_FAST_INTERVAL,
		.window = BT_GAP_SCAN_FAST_WINDOW,
		.interval_coded = 0,
		.window_coded = 0,
		.timeout = 100UL * MSEC_PER_SEC / 10,
	};
	struct bt_conn *conn;
	int err;

	/* Set conn to any non-NULL value to trigger the check. */
	conn = (struct bt_conn *)1;

	err = bt_conn_le_create(&peer, &create_param, BT_LE_CONN_PARAM_DEFAULT, &conn);
	/* err must not be -EINVAL if CONFIG_BT_CONN_CHECK_NULL_BEFORE_CREATE is not enabled,
	 * otherwise -EAGAIN is returned.
	 *
	 * The printk is used to see what actually was compiled.
	 */
#if defined(CONFIG_BT_CONN_CHECK_NULL_BEFORE_CREATE)
	printk("Expected error -EINVAL\n");
	zassert_equal(err, -EINVAL, "Failed starting initiator (err %d)", err);
#else
	printk("Expected error -EAGAIN\n");
	zassert_equal(err, -EAGAIN, "Failed starting initiator (err %d)", err);
#endif

	/* If the conn is NULL, next error must not be -EINVAL. */
	conn = NULL;
	err = bt_conn_le_create(&peer, &create_param, BT_LE_CONN_PARAM_DEFAULT, &conn);
	zassert_not_equal(err, -EINVAL, "Failed starting initiator (err %d)", err);
}

/*
 * Test that bt_conn_le_create_synced() returns -EINVAL if conn is not NULL and
 * CONFIG_BT_CONN_CHECK_NULL_BEFORE_CREATE is enabled.
 *
 * The test must be compiled with and without CONFIG_BT_CONN_CHECK_NULL_BEFORE_CREATE
 * to ensure that the -EINVAL error is returned only when this Kconfig option is enabled.
 */
ZTEST(conn, test_bt_conn_le_create_synced_check_null_conn)
{
	bt_addr_le_t peer = {.a.val = {0x01}};
	struct bt_le_conn_param conn_param = {
		.interval_min = 0x30,
		.interval_max = 0x30,
		.latency = 0,
		.timeout = 400,
	};
	struct bt_conn_le_create_synced_param synced_param = {
		.peer = &peer,
	};
	struct bt_le_ext_adv *adv = NULL;
	struct bt_conn *conn;
	int err;

	/* Set conn to any non-NULL value to trigger the check. */
	conn = (struct bt_conn *)1;
	err = bt_conn_le_create_synced(adv, &synced_param, &conn_param, &conn);

	/* err must not be -EINVAL if CONFIG_BT_CONN_CHECK_NULL_BEFORE_CREATE is not enabled,
	 * otherwise -EAGAIN is returned.
	 *
	 * The printk is used to see what actually was compiled.
	 */
#if defined(CONFIG_BT_CONN_CHECK_NULL_BEFORE_CREATE)
	printk("Expected error -EINVAL\n");
	zassert_equal(err, -EINVAL, "Failed starting initiator (err %d)", err);
#else
	printk("Expected error -EAGAIN\n");
	zassert_equal(err, -EAGAIN, "Failed starting initiator (err %d)", err);
#endif

	/* If the conn is NULL, next error must not be -EINVAL. */
	conn = NULL;
	err = bt_conn_le_create_synced(adv, &synced_param, &conn_param, &conn);
	zassert_not_equal(err, -EINVAL, "Failed starting initiator (err %d)", err);
}
