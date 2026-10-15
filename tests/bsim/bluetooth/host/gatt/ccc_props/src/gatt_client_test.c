/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>

#include "babblekit/testcase.h"
#include "babblekit/flags.h"
#include "common.h"

DEFINE_FLAG_STATIC(flag_connected);
DEFINE_FLAG_STATIC(flag_discovered);
DEFINE_FLAG_STATIC(flag_write_complete);
DEFINE_FLAG_STATIC(flag_subscribed);
DEFINE_FLAG_STATIC(flag_notified);
DEFINE_FLAG_STATIC(flag_indicated);

static struct bt_conn *g_conn;

/* Characteristic value and CCC descriptor handles, filled in by discovery */
static uint16_t notify_chrc_value_handle;
static uint16_t indicate_chrc_value_handle;
static uint16_t both_chrc_value_handle;
static uint16_t notify_ccc_handle;
static uint16_t indicate_ccc_handle;
static uint16_t both_ccc_handle;
static uint16_t bare_ccc_handle;

/* Handle range of the most recently discovered service */
static uint16_t svc_start_handle;
static uint16_t svc_end_handle;

static uint8_t expected_write_err;

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err != 0) {
		TEST_FAIL("Failed to connect to %s (%u)", bt_conn_dst_str(conn), err);
		return;
	}

	__ASSERT_NO_MSG(g_conn == conn);

	SET_FLAG(flag_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	if (conn != g_conn) {
		return;
	}

	bt_conn_drop(&g_conn);
	UNSET_FLAG(flag_connected);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	int err;

	if (g_conn != NULL) {
		return;
	}

	if (type != BT_HCI_ADV_IND && type != BT_HCI_ADV_DIRECT_IND) {
		return;
	}

	err = bt_le_scan_stop();
	if (err != 0) {
		TEST_FAIL("Could not stop scan: %d", err);
		return;
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
				BT_LE_CONN_PARAM_DEFAULT, &g_conn);
	if (err != 0) {
		TEST_FAIL("Could not connect to peer: %d", err);
	}
}

static uint8_t service_discover_func(struct bt_conn *conn,
				     const struct bt_gatt_attr *attr,
				     struct bt_gatt_discover_params *params)
{
	const struct bt_gatt_service_val *svc;

	if (attr == NULL) {
		(void)memset(params, 0, sizeof(*params));
		SET_FLAG(flag_discovered);

		return BT_GATT_ITER_STOP;
	}

	svc = attr->user_data;
	svc_start_handle = attr->handle;
	svc_end_handle = svc->end_handle;

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t chrc_discover_func(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr,
				  struct bt_gatt_discover_params *params)
{
	const struct bt_gatt_chrc *chrc;

	if (attr == NULL) {
		(void)memset(params, 0, sizeof(*params));
		SET_FLAG(flag_discovered);

		return BT_GATT_ITER_STOP;
	}

	chrc = attr->user_data;

	if (bt_uuid_cmp(chrc->uuid, TEST_NOTIFY_CHRC_UUID) == 0) {
		notify_chrc_value_handle = chrc->value_handle;
	} else if (bt_uuid_cmp(chrc->uuid, TEST_INDICATE_CHRC_UUID) == 0) {
		indicate_chrc_value_handle = chrc->value_handle;
	} else if (bt_uuid_cmp(chrc->uuid, TEST_BOTH_CHRC_UUID) == 0) {
		both_chrc_value_handle = chrc->value_handle;
	}

	return BT_GATT_ITER_CONTINUE;
}

/* Associate each discovered CCC with the characteristic whose value handle is
 * the closest one preceding the descriptor.
 */
static uint8_t ccc_discover_func(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr,
				 struct bt_gatt_discover_params *params)
{
	const struct {
		uint16_t value_handle;
		uint16_t *ccc_handle;
	} chrcs[] = {
		{ notify_chrc_value_handle, &notify_ccc_handle },
		{ indicate_chrc_value_handle, &indicate_ccc_handle },
		{ both_chrc_value_handle, &both_ccc_handle },
	};
	uint16_t best_value_handle = 0;
	uint16_t *ccc_handle = NULL;

	if (attr == NULL) {
		(void)memset(params, 0, sizeof(*params));
		SET_FLAG(flag_discovered);

		return BT_GATT_ITER_STOP;
	}

	for (size_t i = 0; i < ARRAY_SIZE(chrcs); i++) {
		if (chrcs[i].value_handle != 0 &&
		    chrcs[i].value_handle < attr->handle &&
		    chrcs[i].value_handle > best_value_handle) {
			best_value_handle = chrcs[i].value_handle;
			ccc_handle = chrcs[i].ccc_handle;
		}
	}

	if (ccc_handle != NULL) {
		*ccc_handle = attr->handle;
	}

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t bare_ccc_discover_func(struct bt_conn *conn,
				      const struct bt_gatt_attr *attr,
				      struct bt_gatt_discover_params *params)
{
	if (attr == NULL) {
		(void)memset(params, 0, sizeof(*params));
		SET_FLAG(flag_discovered);

		return BT_GATT_ITER_STOP;
	}

	bare_ccc_handle = attr->handle;

	return BT_GATT_ITER_CONTINUE;
}

static void discover(uint8_t type, const struct bt_uuid *uuid, uint16_t start_handle,
		     uint16_t end_handle, bt_gatt_discover_func_t func)
{
	static struct bt_gatt_discover_params discover_params;
	int err;

	discover_params.type = type;
	discover_params.uuid = uuid;
	discover_params.start_handle = start_handle;
	discover_params.end_handle = end_handle;
	discover_params.func = func;

	UNSET_FLAG(flag_discovered);

	err = bt_gatt_discover(g_conn, &discover_params);
	if (err != 0) {
		TEST_FAIL("Discover failed (err %d)", err);
	}

	WAIT_FOR_FLAG(flag_discovered);
}

static void gatt_discover(void)
{
	/* The test service: characteristics, then their CCC descriptors */
	discover(BT_GATT_DISCOVER_PRIMARY, TEST_SERVICE_UUID,
		 BT_ATT_FIRST_ATTRIBUTE_HANDLE, BT_ATT_LAST_ATTRIBUTE_HANDLE,
		 service_discover_func);
	discover(BT_GATT_DISCOVER_CHARACTERISTIC, NULL,
		 svc_start_handle, svc_end_handle, chrc_discover_func);
	discover(BT_GATT_DISCOVER_STD_CHAR_DESC, BT_UUID_GATT_CCC,
		 svc_start_handle, svc_end_handle, ccc_discover_func);

	/* The bare service: only a CCC, with no characteristic declaration */
	discover(BT_GATT_DISCOVER_PRIMARY, TEST_BARE_SERVICE_UUID,
		 BT_ATT_FIRST_ATTRIBUTE_HANDLE, BT_ATT_LAST_ATTRIBUTE_HANDLE,
		 service_discover_func);
	discover(BT_GATT_DISCOVER_STD_CHAR_DESC, BT_UUID_GATT_CCC,
		 svc_start_handle, svc_end_handle, bare_ccc_discover_func);

	if (notify_chrc_value_handle == 0 || indicate_chrc_value_handle == 0 ||
	    both_chrc_value_handle == 0 || notify_ccc_handle == 0 ||
	    indicate_ccc_handle == 0 || both_ccc_handle == 0 || bare_ccc_handle == 0) {
		TEST_FAIL("Did not discover all required attributes");
	}
}

static void ccc_write_cb(struct bt_conn *conn, uint8_t err,
			 struct bt_gatt_write_params *params)
{
	if (err != expected_write_err) {
		TEST_FAIL("CCC write to handle 0x%04x returned 0x%02x, expected 0x%02x",
			  params->handle, err, expected_write_err);
	}

	(void)memset(params, 0, sizeof(*params));
	SET_FLAG(flag_write_complete);
}

/* Write the CCC directly instead of through bt_gatt_subscribe(), to control the
 * exact value written and to verify the resulting ATT error code. The bare CCC
 * also has no characteristic value that a subscription could be created for.
 */
static void ccc_write(uint16_t ccc_handle, uint16_t value, uint8_t expect_err)
{
	static struct bt_gatt_write_params write_params;
	static uint16_t le_value;
	int err;

	le_value = sys_cpu_to_le16(value);

	write_params.data = &le_value;
	write_params.length = sizeof(le_value);
	write_params.func = ccc_write_cb;
	write_params.handle = ccc_handle;

	expected_write_err = expect_err;
	UNSET_FLAG(flag_write_complete);

	err = bt_gatt_write(g_conn, &write_params);
	if (err != 0) {
		TEST_FAIL("bt_gatt_write failed: %d", err);
	}

	WAIT_FOR_FLAG(flag_write_complete);
}

static void subscribed_cb(struct bt_conn *conn, uint8_t err,
			  struct bt_gatt_subscribe_params *params)
{
	if (err != 0) {
		TEST_FAIL("Subscribe failed (err 0x%02x)", err);
	}

	SET_FLAG(flag_subscribed);
}

static uint8_t notify_func(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
			   const void *data, uint16_t length)
{
	if (data == NULL) {
		return BT_GATT_ITER_STOP;
	}

	if (params->value == BT_GATT_CCC_NOTIFY) {
		SET_FLAG(flag_notified);
	} else {
		SET_FLAG(flag_indicated);
	}

	return BT_GATT_ITER_CONTINUE;
}

static void subscribe(struct bt_gatt_subscribe_params *params, uint16_t value_handle,
		      uint16_t ccc_handle, uint16_t value)
{
	int err;

	params->ccc_handle = ccc_handle;
	params->value_handle = value_handle;
	params->value = value;
	params->subscribe = subscribed_cb;
	params->notify = notify_func;

	UNSET_FLAG(flag_subscribed);

	err = bt_gatt_subscribe(g_conn, params);
	if (err != 0) {
		TEST_FAIL("bt_gatt_subscribe failed: %d", err);
	}

	WAIT_FOR_FLAG(flag_subscribed);
}

static void test_main(void)
{
	static struct bt_gatt_subscribe_params notify_params;
	static struct bt_gatt_subscribe_params indicate_params;
	int err;

	bt_conn_cb_register(&conn_callbacks);

	err = bt_enable(NULL);
	if (err != 0) {
		TEST_FAIL("Bluetooth init failed (err %d)", err);
	}

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err != 0) {
		TEST_FAIL("Scanning failed to start (err %d)", err);
	}

	WAIT_FOR_FLAG(flag_connected);

	gatt_discover();

	/* Configurations the characteristics do not support are rejected */
	ccc_write(notify_ccc_handle, BT_GATT_CCC_INDICATE,
		  BT_ATT_ERR_CCC_IMPROPER_CONF);
	ccc_write(notify_ccc_handle, BT_GATT_CCC_NOTIFY | BT_GATT_CCC_INDICATE,
		  BT_ATT_ERR_CCC_IMPROPER_CONF);
	ccc_write(indicate_ccc_handle, BT_GATT_CCC_NOTIFY,
		  BT_ATT_ERR_CCC_IMPROPER_CONF);

	/* Supported configurations are accepted and delivered */
	subscribe(&notify_params, notify_chrc_value_handle, notify_ccc_handle,
		  BT_GATT_CCC_NOTIFY);
	WAIT_FOR_FLAG(flag_notified);

	subscribe(&indicate_params, indicate_chrc_value_handle, indicate_ccc_handle,
		  BT_GATT_CCC_INDICATE);
	WAIT_FOR_FLAG(flag_indicated);

	/* A combined value is accepted when both bits are supported */
	ccc_write(both_ccc_handle, BT_GATT_CCC_NOTIFY | BT_GATT_CCC_INDICATE,
		  BT_ATT_ERR_SUCCESS);

	/* Clearing is always accepted, even on a fresh configuration */
	ccc_write(both_ccc_handle, 0, BT_ATT_ERR_SUCCESS);
	ccc_write(notify_ccc_handle, 0, BT_ATT_ERR_SUCCESS);

	/* A CCC with no characteristic declaration cannot be validated, so
	 * any configuration is let through.
	 */
	ccc_write(bare_ccc_handle, BT_GATT_CCC_NOTIFY | BT_GATT_CCC_INDICATE,
		  BT_ATT_ERR_SUCCESS);

	err = bt_conn_disconnect(g_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err != 0) {
		TEST_FAIL("Disconnect failed (err %d)", err);
	}

	WAIT_FOR_FLAG_UNSET(flag_connected);

	TEST_PASS("GATT client passed");
}

static const struct bst_test_instance test_gatt_client[] = {
	{
		.test_id = "gatt_client",
		.test_main_f = test_main,
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_gatt_client_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_gatt_client);
}
