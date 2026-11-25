/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Xiaomi Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/device.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>
#include <zephyr/ztest.h>

#include "host/conn_internal.h"
#include "host/gatt_internal.h"
#include "host/hci_core.h"
#include "host/settings.h"

#define DT_DRV_COMPAT zephyr_bt_hci_test

/* The host is not enabled, the driver only has to exist */
static int driver_open(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int driver_send(const struct device *dev, struct net_buf *buf)
{
	ARG_UNUSED(dev);

	net_buf_unref(buf);

	return 0;
}

static DEVICE_API(bt_hci, driver_api) = {
	.open = driver_open,
	.send = driver_send,
};

#define TEST_DEVICE_INIT(inst)                                                                     \
	static struct bt_hci_driver_data driver_data_##inst = {0};                                 \
	static const struct bt_hci_driver_config driver_config_##inst =                            \
		BT_DT_HCI_DRIVER_CONFIG_INST_GET(inst);                                            \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &driver_data_##inst, &driver_config_##inst,        \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &driver_api)

DT_INST_FOREACH_STATUS_OKAY(TEST_DEVICE_INIT)

/* GATT keys that were stored or deleted */
static size_t gatt_keys;

static bool is_gatt_key(const char *key)
{
	static const char *const prefixes[] = { "bt/ccc/", "bt/cf/", "bt/sc/" };

	ARRAY_FOR_EACH(prefixes, i) {
		if (strncmp(key, prefixes[i], strlen(prefixes[i])) == 0) {
			return true;
		}
	}

	return false;
}

void bt_testing_settings_store_hook(const char *key, const void *value, size_t val_len)
{
	ARGS_UNUSED(value, val_len);

	if (is_gatt_key(key)) {
		gatt_keys++;
	}
}

void bt_testing_settings_delete_hook(const char *key)
{
	if (is_gatt_key(key)) {
		gatt_keys++;
	}
}

/* As the Security Manager or SSP does once a pairing has completed */
static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	struct bt_conn_auth_info_cb *listener;

	SYS_SLIST_FOR_EACH_CONTAINER(&bt_auth_info_cbs, listener, node) {
		if (listener->pairing_complete != NULL) {
			listener->pairing_complete(conn, bonded);
		}
	}
}

static void *setup(void)
{
	bt_gatt_init();

	return NULL;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	gatt_keys = 0U;
}

ZTEST_SUITE(host_gatt_br_pairing, NULL, setup, before, NULL, NULL);

/* GATT stores the state of a bonded LE peer when the pairing completes */
static ZTEST(host_gatt_br_pairing, test_le_bond_stored)
{
	static const bt_addr_le_t peer = {
		.type = BT_ADDR_LE_PUBLIC,
		.a = { .val = { 0x11, 0x12, 0x13, 0x14, 0x15, 0x16 } },
	};
	struct bt_conn *conn;

	conn = bt_conn_add_le(BT_ID_DEFAULT, &peer);
	zassert_not_null(conn);

	pairing_complete(conn, true);

	zassert_true(gatt_keys > 0U, "LE bond not stored");

	bt_conn_unref(conn);
}

/* A BR/EDR bond has no GATT state: GATT stores nothing, instead of the state of
 * an address read from the LE part of the connection object.
 */
static ZTEST(host_gatt_br_pairing, test_br_bond_not_stored)
{
	static const bt_addr_t peer = { .val = { 0x21, 0x22, 0x23, 0x24, 0x25, 0x26 } };
	struct bt_conn *conn;

	conn = bt_conn_add_br(&peer);
	zassert_not_null(conn);

	pairing_complete(conn, true);

	zassert_equal(gatt_keys, 0U, "%zu GATT keys stored for a BR/EDR bond", gatt_keys);

	bt_conn_unref(conn);
}
