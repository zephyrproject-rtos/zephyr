/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include "babblekit/sync.h"
#include "babblekit/testcase.h"
#include "testlib/adv.h"
#include "testlib/conn.h"
#include "testlib/scan.h"

#include "common.h"

#define DUT_NAME "kept conn"

/* Several connection events: a notification or a disconnection has arrived by then */
#define RX_TIMEOUT K_SECONDS(1)

#define KEPT_CONN_SERVICE_UUID                                                                     \
	BT_UUID_DECLARE_128(0x6b, 0x4d, 0x0e, 0x2a, 0x9a, 0x07, 0x4c, 0x1e, 0x93, 0x52, 0x1b,      \
			    0x2f, 0xd7, 0x81, 0x01, 0x00)
#define KEPT_CONN_CHRC_UUID                                                                        \
	BT_UUID_DECLARE_128(0x6b, 0x4d, 0x0e, 0x2a, 0x9a, 0x07, 0x4c, 0x1e, 0x93, 0x52, 0x1b,      \
			    0x2f, 0xd7, 0x81, 0x01, 0x01)

BT_GATT_SERVICE_DEFINE(kept_conn_svc,
	BT_GATT_PRIMARY_SERVICE(KEPT_CONN_SERVICE_UUID),
	BT_GATT_CHARACTERISTIC(KEPT_CONN_CHRC_UUID, BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE,
			       NULL, NULL, NULL),
	BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

#define CHRC_VALUE_ATTR (&kept_conn_svc.attrs[2])

static K_SEM_DEFINE(connected_sem, 0, 1);
static K_SEM_DEFINE(disconnected_sem, 0, 1);
static K_SEM_DEFINE(subscribed_sem, 0, 1);
static K_SEM_DEFINE(notified_sem, 0, 1);

static void connected(struct bt_conn *conn, uint8_t err)
{
	ARG_UNUSED(conn);

	if (err == BT_HCI_ERR_SUCCESS) {
		k_sem_give(&connected_sem);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(reason);

	k_sem_give(&disconnected_sem);
}

static void test_setup(void)
{
	static struct bt_conn_cb conn_cb = {
		.connected = connected,
		.disconnected = disconnected,
	};
	int err;

	err = bk_sync_init();
	TEST_ASSERT(err == 0, "Failed to open the backchannel (err %d)", err);

	err = bt_conn_cb_register(&conn_cb);
	TEST_ASSERT(err == 0, "Failed to register connection callbacks (err %d)", err);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "bt_enable failed (err %d)", err);
}

static void check_lookup(const bt_addr_le_t *peer, const struct bt_conn *expected)
{
	struct bt_conn *found = bt_conn_lookup_addr_le(BT_ID_DEFAULT, peer);

	TEST_ASSERT(found == expected, "Lookup found conn %d, expected conn %d",
		    found != NULL ? bt_conn_index(found) : -1, bt_conn_index(expected));

	bt_conn_unref(found);
}

static void notify_peer(void)
{
	const uint8_t value = 0U;
	int err;

	err = bt_gatt_notify(NULL, CHRC_VALUE_ATTR, &value, sizeof(value));
	TEST_ASSERT(err == 0, "Notifying the connected peer failed (err %d)", err);

	/* The peer has received it */
	bk_sync_wait();
}

static void dut_peer_reconnects(const bt_addr_le_t *peer, const struct bt_conn *kept)
{
	struct bt_conn *other = NULL;
	struct bt_conn *live = NULL;
	int err;

	bt_testlib_conn_wait_free();

	err = bt_testlib_adv_conn(&live, BT_ID_DEFAULT, DUT_NAME);
	TEST_ASSERT(err == 0, "Advertising failed (err %d)", err);
	TEST_ASSERT(live != kept, "The new connection took the kept object");

	/* The peer has subscribed */
	bk_sync_wait();

	notify_peer();
	check_lookup(peer, live);

	k_sem_reset(&disconnected_sem);

	err = bt_unpair(BT_ID_DEFAULT, peer);
	TEST_ASSERT(err == 0, "bt_unpair failed (err %d)", err);

	err = k_sem_take(&disconnected_sem, RX_TIMEOUT);
	TEST_ASSERT(err == 0, "bt_unpair left the connection up");

	check_lookup(peer, kept);

	/* Both objects are still referenced, so there is none for another connection */
	err = bt_conn_le_create(peer, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT, &other);
	TEST_ASSERT(err == -ENOMEM, "Connecting without a free object gave err %d", err);

	bt_conn_drop(&live);
}

static void dut_connects(const bt_addr_le_t *peer, const struct bt_conn *kept)
{
	struct bt_conn *live = NULL;
	int err;

	bt_testlib_conn_wait_free();

	err = bt_testlib_connect(peer, &live);
	TEST_ASSERT(err == 0, "Connecting failed with the old object kept (err %d)", err);
	TEST_ASSERT(live != kept, "The new connection took the kept object");

	err = bt_testlib_wait_disconnected(live);
	TEST_ASSERT(err == 0, "Waiting for the disconnection failed (err %d)", err);

	bt_conn_drop(&live);
}

static void dut_advertises_directed(const bt_addr_le_t *peer, const struct bt_conn *kept)
{
	struct bt_conn *live;
	int err;

	bt_testlib_conn_wait_free();

	k_sem_reset(&connected_sem);

	err = bt_le_adv_start(BT_LE_ADV_CONN_DIR_LOW_DUTY(peer), NULL, 0, NULL, 0);
	TEST_ASSERT(err == 0, "Directed advertising failed with the old object kept (err %d)", err);

	err = k_sem_take(&connected_sem, K_FOREVER);
	TEST_ASSERT(err == 0, "Waiting for the connection failed (err %d)", err);

	live = bt_conn_lookup_addr_le(BT_ID_DEFAULT, peer);
	TEST_ASSERT(live != NULL && live != kept, "The connection is not found");

	err = bt_testlib_wait_disconnected(live);
	TEST_ASSERT(err == 0, "Waiting for the disconnection failed (err %d)", err);

	bt_conn_drop(&live);
}

/* The peer connects while an outgoing attempt to it is in progress */
static void dut_crossing_connection(const bt_addr_le_t *peer)
{
	struct bt_conn *initiating = NULL;
	struct bt_conn *live = NULL;
	int err;

	bt_testlib_conn_wait_free();

	err = bt_conn_le_create(peer, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT,
				&initiating);
	TEST_ASSERT(err == 0, "Starting the initiator failed (err %d)", err);

	bt_testlib_conn_wait_free();

	err = bt_testlib_adv_conn(&live, BT_ID_DEFAULT, DUT_NAME);
	TEST_ASSERT(err == 0, "Advertising failed (err %d)", err);

	/* The peer has subscribed */
	bk_sync_wait();

	notify_peer();
	check_lookup(peer, live);

	err = bt_conn_disconnect(initiating, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	TEST_ASSERT(err == 0, "Canceling the initiator failed (err %d)", err);

	err = bt_testlib_wait_disconnected(initiating);
	TEST_ASSERT(err == 0, "Waiting for the cancellation failed (err %d)", err);

	bt_conn_drop(&initiating);
	bt_conn_drop(&live);
}

void kept_conn_dut_procedure(void)
{
	struct bt_conn *kept = NULL;
	bt_addr_le_t peer;
	int err;

	TEST_START("kept_conn_dut");

	test_setup();

	err = bt_testlib_adv_conn(&kept, BT_ID_DEFAULT, DUT_NAME);
	TEST_ASSERT(err == 0, "Advertising failed (err %d)", err);

	bt_addr_le_copy(&peer, bt_conn_get_dst(kept));

	err = bt_testlib_wait_disconnected(kept);
	TEST_ASSERT(err == 0, "Waiting for the disconnection failed (err %d)", err);

	check_lookup(&peer, kept);

	dut_peer_reconnects(&peer, kept);
	dut_connects(&peer, kept);
	dut_advertises_directed(&peer, kept);

	bt_conn_drop(&kept);

	dut_crossing_connection(&peer);

	/* The peer ends the simulation once it has been told the DUT is done */
	TEST_PASS("kept_conn_dut");

	bk_sync_send();
}

static void subscribed(struct bt_conn *conn, uint8_t err, struct bt_gatt_subscribe_params *params)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(params);

	TEST_ASSERT(err == 0, "Subscribing failed (ATT err 0x%02x)", err);

	k_sem_give(&subscribed_sem);
}

static uint8_t notified(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
			const void *data, uint16_t length)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(params);
	ARG_UNUSED(length);

	if (data != NULL) {
		k_sem_give(&notified_sem);
	}

	return BT_GATT_ITER_CONTINUE;
}

static void peer_connect_and_subscribe(struct bt_conn **conn, bt_addr_le_t *dut)
{
	static struct bt_gatt_subscribe_params params;
	int err;

	err = bt_testlib_scan_find_name(dut, DUT_NAME);
	TEST_ASSERT(err == 0, "The DUT was not found (err %d)", err);

	err = bt_testlib_connect(dut, conn);
	TEST_ASSERT(err == 0, "Connecting failed (err %d)", err);

	/* Both devices run this image, so the handles on the DUT are the same as here */
	params = (struct bt_gatt_subscribe_params){
		.notify = notified,
		.subscribe = subscribed,
		.value = BT_GATT_CCC_NOTIFY,
		.value_handle = bt_gatt_attr_get_handle(CHRC_VALUE_ATTR),
		.ccc_handle = bt_gatt_attr_get_handle(CHRC_VALUE_ATTR) + 1U,
	};

	err = bt_gatt_subscribe(*conn, &params);
	TEST_ASSERT(err == 0, "Subscribing failed (err %d)", err);

	err = k_sem_take(&subscribed_sem, K_FOREVER);
	TEST_ASSERT(err == 0, "Waiting for the subscription failed (err %d)", err);

	bk_sync_send();
}

static void peer_receive_notification(void)
{
	int err;

	err = k_sem_take(&notified_sem, RX_TIMEOUT);
	TEST_ASSERT(err == 0, "No notification received");

	bk_sync_send();
}

static void peer_disconnect(struct bt_conn **conn)
{
	int err;

	err = bt_testlib_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	TEST_ASSERT(err == 0, "Disconnecting failed (err %d)", err);
}

void kept_conn_peer_procedure(void)
{
	struct bt_conn *conn = NULL;
	bt_addr_le_t dut;
	int err;

	TEST_START("kept_conn_peer");

	test_setup();

	/* The DUT keeps its reference to this connection */
	err = bt_testlib_scan_find_name(&dut, DUT_NAME);
	TEST_ASSERT(err == 0, "The DUT was not found (err %d)", err);

	err = bt_testlib_connect(&dut, &conn);
	TEST_ASSERT(err == 0, "Connecting failed (err %d)", err);

	peer_disconnect(&conn);

	k_sem_reset(&disconnected_sem);

	/* dut_peer_reconnects() */
	peer_connect_and_subscribe(&conn, &dut);
	peer_receive_notification();

	err = k_sem_take(&disconnected_sem, RX_TIMEOUT);
	TEST_ASSERT(err == 0, "The DUT's bt_unpair left the connection up");

	bt_conn_drop(&conn);

	/* dut_connects() */
	err = bt_testlib_adv_conn(&conn, BT_ID_DEFAULT, NULL);
	TEST_ASSERT(err == 0, "Advertising failed (err %d)", err);

	peer_disconnect(&conn);

	/* dut_advertises_directed() */
	err = bt_testlib_connect(&dut, &conn);
	TEST_ASSERT(err == 0, "Connecting failed (err %d)", err);

	peer_disconnect(&conn);

	/* dut_crossing_connection() */
	peer_connect_and_subscribe(&conn, &dut);
	peer_receive_notification();

	bk_sync_wait();

	TEST_PASS_AND_EXIT("kept_conn_peer");
}
