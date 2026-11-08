/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include <fake_hci.h>

/* The test Controller reports the timeout of high duty cycle directed advertising with a
 * connection complete event. Without this quirk an LE Advertising Set Terminated event follows.
 */
#define CONN_COMPLETE_ONLY                                                                         \
	((BT_DT_HCI_QUIRKS_GET(DT_CHOSEN(zephyr_bt_hci)) &                                         \
	  BT_HCI_QUIRK_NO_ADV_TERM_ON_DIR_TIMEOUT) != 0)

/* Advertising handle of the most recent LE Set Extended Advertising Parameters command. */
static uint8_t last_adv_handle;

/* Error of the most recent connected callback. */
static uint8_t conn_err;

static K_SEM_DEFINE(connected_sem, 0, 1);
static K_SEM_DEFINE(sent_sem, 0, 1);

/* Store the advertising handle, which the LE Advertising Set Terminated event of the test
 * Controller has to carry. The Controller answers the command.
 */
static bool cmd_cb(uint16_t opcode, struct net_buf *buf)
{
	const struct bt_hci_cp_le_set_ext_adv_param *cp = (const void *)buf->data;

	if (opcode == BT_HCI_OP_LE_SET_EXT_ADV_PARAM) {
		zassert_true(buf->len >= sizeof(*cp), "Unexpected command length %u", buf->len);
		last_adv_handle = cp->handle;
	}

	return false;
}

/* Let the test Controller report that no connection was created before the duration expired. */
static void conn_complete_timeout(void)
{
	struct bt_hci_evt_le_enh_conn_complete *cc;
	struct net_buf *buf;

	buf = fake_hci_le_meta_evt(BT_HCI_EVT_LE_ENH_CONN_COMPLETE, sizeof(*cc));
	cc = net_buf_add(buf, sizeof(*cc));
	(void)memset(cc, 0, sizeof(*cc));
	cc->status = BT_HCI_ERR_ADV_TIMEOUT;
	cc->role = BT_HCI_ROLE_PERIPHERAL;
	fake_hci_recv(buf);
}

/* Let the test Controller report the timeout with the events that it sends, and check that the
 * Host ends the advertising when the event that it has to act on arrives.
 */
static void adv_timeout(void)
{
	int err;

	conn_complete_timeout();

	if (!CONN_COMPLETE_ONLY) {
		struct bt_hci_evt_le_adv_set_terminated *term;
		struct net_buf *buf;

		err = k_sem_take(&connected_sem, K_MSEC(100));
		zassert_equal(err, -EAGAIN, "Advertising ended by the connection complete event");

		buf = fake_hci_le_meta_evt(BT_HCI_EVT_LE_ADV_SET_TERMINATED, sizeof(*term));
		term = net_buf_add(buf, sizeof(*term));
		(void)memset(term, 0, sizeof(*term));
		term->status = BT_HCI_ERR_ADV_TIMEOUT;
		term->adv_handle = last_adv_handle;
		fake_hci_recv(buf);
	}

	err = k_sem_take(&connected_sem, K_SECONDS(1));
	zassert_ok(err, "No connected callback");
	zassert_equal(conn_err, BT_HCI_ERR_ADV_TIMEOUT, "Unexpected error 0x%02x", conn_err);
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	ARG_UNUSED(conn);

	conn_err = err;
	k_sem_give(&connected_sem);
}

static void adv_sent(struct bt_le_ext_adv *adv, struct bt_le_ext_adv_sent_info *info)
{
	ARG_UNUSED(adv);
	ARG_UNUSED(info);

	k_sem_give(&sent_sem);
}

static const struct bt_le_ext_adv_cb adv_cb = {
	.sent = adv_sent,
};

static void *adv_dir_timeout_setup(void)
{
	static struct bt_conn_cb conn_cb = {
		.connected = connected,
	};
	static const struct fake_hci_cb cb = {
		.cmd = cmd_cb,
	};
	int err;

	(void)memset(fake_hci.bd_addr.val, 0x11, sizeof(fake_hci.bd_addr.val));
	/* Only report support for LE Extended Advertising */
	fake_hci.le_features[BT_LE_FEAT_BIT_EXT_ADV / 8] = BIT(BT_LE_FEAT_BIT_EXT_ADV % 8);
	fake_hci_cb_set(&cb);

	err = bt_enable(NULL);
	zassert_ok(err, "Bluetooth init failed: %d", err);

	err = bt_conn_cb_register(&conn_cb);
	zassert_ok(err, "Failed to register connection callbacks: %d", err);

	return NULL;
}

ZTEST_SUITE(bt_adv_dir_timeout, NULL, adv_dir_timeout_setup, NULL, NULL, NULL);

static const bt_addr_le_t peer = {
	.type = BT_ADDR_LE_RANDOM,
	.a = { { 0x01, 0x02, 0x03, 0x04, 0x05, 0xc6 } },
};

static const bt_addr_le_t other_peer = {
	.type = BT_ADDR_LE_RANDOM,
	.a = { { 0x06, 0x05, 0x04, 0x03, 0x02, 0xc1 } },
};

static ZTEST(bt_adv_dir_timeout, test_ext_adv_start)
{
	struct bt_le_ext_adv *adv;
	int err;

	err = bt_le_ext_adv_create(BT_LE_ADV_CONN_DIR(&peer), &adv_cb, &adv);
	zassert_ok(err, "Failed to create advertising set: %d", err);

	/* The second round fails to start if the first timeout left the set enabled */
	for (size_t i = 0U; i < 2U; i++) {
		struct bt_le_ext_adv_info info;

		err = bt_le_ext_adv_start(
			adv, BT_LE_EXT_ADV_START_PARAM(BT_GAP_ADV_HIGH_DUTY_CYCLE_MAX_TIMEOUT, 0));
		zassert_ok(err, "Failed to start advertising: %d", err);

		adv_timeout();

		err = k_sem_take(&sent_sem, K_SECONDS(1));
		zassert_ok(err, "No sent callback");

		err = bt_le_ext_adv_get_info(adv, &info);
		zassert_ok(err, "Failed to get advertising set info: %d", err);
		zassert_equal(info.ext_adv_state, BT_LE_EXT_ADV_STATE_DISABLED,
			      "Advertising set still enabled");
	}

	err = bt_le_ext_adv_delete(adv);
	zassert_ok(err, "Failed to delete advertising set: %d", err);
}

static ZTEST(bt_adv_dir_timeout, test_adv_start)
{
	/* The second round fails to start if the first timeout left the advertiser enabled */
	for (size_t i = 0U; i < 2U; i++) {
		int err;

		err = bt_le_adv_start(BT_LE_ADV_CONN_DIR(&peer), NULL, 0, NULL, 0);
		zassert_ok(err, "Failed to start advertising: %d", err);

		adv_timeout();
	}
}

/* A connection complete event for the timeout that arrives after the application stopped the
 * set has no effect, on that set or on its next start.
 */
static ZTEST(bt_adv_dir_timeout, test_ext_adv_stopped)
{
	struct bt_le_ext_adv_info info;
	struct bt_le_ext_adv *adv;
	int err;

	err = bt_le_ext_adv_create(BT_LE_ADV_CONN_DIR(&peer), &adv_cb, &adv);
	zassert_ok(err, "Failed to create advertising set: %d", err);

	err = bt_le_ext_adv_start(
		adv, BT_LE_EXT_ADV_START_PARAM(BT_GAP_ADV_HIGH_DUTY_CYCLE_MAX_TIMEOUT, 0));
	zassert_ok(err, "Failed to start advertising: %d", err);

	err = bt_le_ext_adv_stop(adv);
	zassert_ok(err, "Failed to stop advertising: %d", err);

	conn_complete_timeout();

	err = k_sem_take(&connected_sem, K_MSEC(100));
	zassert_equal(err, -EAGAIN, "Connected callback for a stopped set");
	err = k_sem_take(&sent_sem, K_NO_WAIT);
	zassert_equal(err, -EBUSY, "Sent callback for a stopped set");

	err = bt_le_ext_adv_start(
		adv, BT_LE_EXT_ADV_START_PARAM(BT_GAP_ADV_HIGH_DUTY_CYCLE_MAX_TIMEOUT, 0));
	zassert_ok(err, "Failed to restart advertising: %d", err);

	err = bt_le_ext_adv_get_info(adv, &info);
	zassert_ok(err, "Failed to get advertising set info: %d", err);
	zassert_equal(info.ext_adv_state, BT_LE_EXT_ADV_STATE_ENABLED, "Advertising set disabled");

	err = bt_le_ext_adv_stop(adv);
	zassert_ok(err, "Failed to stop advertising: %d", err);

	err = bt_le_ext_adv_delete(adv);
	zassert_ok(err, "Failed to delete advertising set: %d", err);
}

/* The timeout of the high duty cycle set must leave a low duty cycle directed set enabled. */
static ZTEST(bt_adv_dir_timeout, test_ext_adv_low_duty_unaffected)
{
	struct bt_le_ext_adv *low_duty;
	struct bt_le_ext_adv_info info;
	struct bt_le_ext_adv *adv;
	int err;

	if (!CONN_COMPLETE_ONLY || CONFIG_BT_EXT_ADV_MAX_ADV_SET < 2) {
		ztest_test_skip();
	}

	err = bt_le_ext_adv_create(BT_LE_ADV_CONN_DIR_LOW_DUTY(&other_peer), NULL, &low_duty);
	zassert_ok(err, "Failed to create low duty cycle set: %d", err);

	err = bt_le_ext_adv_create(BT_LE_ADV_CONN_DIR(&peer), &adv_cb, &adv);
	zassert_ok(err, "Failed to create high duty cycle set: %d", err);

	err = bt_le_ext_adv_start(low_duty, BT_LE_EXT_ADV_START_DEFAULT);
	zassert_ok(err, "Failed to start low duty cycle advertising: %d", err);

	err = bt_le_ext_adv_start(
		adv, BT_LE_EXT_ADV_START_PARAM(BT_GAP_ADV_HIGH_DUTY_CYCLE_MAX_TIMEOUT, 0));
	zassert_ok(err, "Failed to start high duty cycle advertising: %d", err);

	adv_timeout();

	err = k_sem_take(&sent_sem, K_SECONDS(1));
	zassert_ok(err, "No sent callback");

	err = bt_le_ext_adv_get_info(adv, &info);
	zassert_ok(err, "Failed to get advertising set info: %d", err);
	zassert_equal(info.ext_adv_state, BT_LE_EXT_ADV_STATE_DISABLED,
		      "High duty cycle set still enabled");

	err = bt_le_ext_adv_get_info(low_duty, &info);
	zassert_ok(err, "Failed to get advertising set info: %d", err);
	zassert_equal(info.ext_adv_state, BT_LE_EXT_ADV_STATE_ENABLED,
		      "Low duty cycle set disabled");

	err = bt_le_ext_adv_stop(low_duty);
	zassert_ok(err, "Failed to stop advertising: %d", err);

	err = bt_le_ext_adv_delete(low_duty);
	zassert_ok(err, "Failed to delete advertising set: %d", err);

	err = bt_le_ext_adv_delete(adv);
	zassert_ok(err, "Failed to delete advertising set: %d", err);
}

/* The connection complete event names no advertising set, so with the quirk only one high duty
 * cycle directed set can be enabled.
 */
static ZTEST(bt_adv_dir_timeout, test_ext_adv_second_high_duty)
{
	struct bt_le_ext_adv *second;
	struct bt_le_ext_adv *adv;
	int err;

	if (!CONN_COMPLETE_ONLY || CONFIG_BT_EXT_ADV_MAX_ADV_SET < 2) {
		ztest_test_skip();
	}

	err = bt_le_ext_adv_create(BT_LE_ADV_CONN_DIR(&peer), &adv_cb, &adv);
	zassert_ok(err, "Failed to create advertising set: %d", err);

	err = bt_le_ext_adv_start(
		adv, BT_LE_EXT_ADV_START_PARAM(BT_GAP_ADV_HIGH_DUTY_CYCLE_MAX_TIMEOUT, 0));
	zassert_ok(err, "Failed to start advertising: %d", err);

	err = bt_le_ext_adv_create(BT_LE_ADV_CONN_DIR(&other_peer), NULL, &second);
	zassert_ok(err, "Failed to create second set: %d", err);

	err = bt_le_ext_adv_start(
		second, BT_LE_EXT_ADV_START_PARAM(BT_GAP_ADV_HIGH_DUTY_CYCLE_MAX_TIMEOUT, 0));
	zassert_equal(err, -EBUSY, "Second high duty cycle set started: %d", err);

	err = bt_le_ext_adv_delete(second);
	zassert_ok(err, "Failed to delete second set: %d", err);

	err = bt_le_adv_start(BT_LE_ADV_CONN_DIR(&other_peer), NULL, 0, NULL, 0);
	zassert_equal(err, -EBUSY, "Legacy high duty cycle advertising started: %d", err);

	adv_timeout();

	err = k_sem_take(&sent_sem, K_SECONDS(1));
	zassert_ok(err, "No sent callback");

	err = bt_le_ext_adv_delete(adv);
	zassert_ok(err, "Failed to delete advertising set: %d", err);
}
