/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>

extern int mtu_exchange(struct bt_conn *conn);
extern int write_cmd(struct bt_conn *conn);
extern struct bt_conn *conn_connected;
extern uint32_t last_write_rate;
extern uint32_t *write_countdown;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static void mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	printk("Updated MTU: TX: %d RX: %d bytes\n", tx, rx);
}

#if defined(CONFIG_BT_SMP)
static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
}

static struct bt_conn_auth_cb auth_callbacks = {
	.cancel = auth_cancel,
};
#endif /* CONFIG_BT_SMP */

static struct bt_gatt_cb gatt_callbacks = {
	.att_mtu_updated = mtu_updated
};

#define BT_LE_SCAN_PASSIVE_ALLOW_DUPILCATES \
		BT_LE_SCAN_PARAM(BT_LE_SCAN_TYPE_PASSIVE, \
				 BT_LE_SCAN_OPT_NONE, \
				 BT_GAP_SCAN_FAST_INTERVAL_MIN, \
				 BT_GAP_SCAN_FAST_INTERVAL_MIN)

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	printk("Device found: %s (RSSI %d)\n", addr_str, rssi);
}

uint32_t peripheral_gatt_write(uint32_t count)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0U;
	}

	printk("Bluetooth initialized\n");

	bt_gatt_cb_register(&gatt_callbacks);

	/* On target, users can enable simultaneous (background) scanning but by default do not have
	 * the scanning enabled.
	 * If both Central plus Peripheral role is built together then
	 * Peripheral is scanning (on 1M and Coded PHY windows) while there
	 * is simultaneous Write Commands.
	 */
	if (IS_ENABLED(CONFIG_BT_OBSERVER) && !IS_ENABLED(CONFIG_USE_PHY_UPDATE_ITERATION_ONCE)) {
		printk("Start continuous passive scanning...");
		err = bt_le_scan_start(BT_LE_SCAN_PASSIVE_ALLOW_DUPILCATES,
				       device_found);
		if (err != 0) {
			printk("Scan start failed (%d).\n", err);
			return err;
		}
		printk("success.\n");
	}

#if defined(CONFIG_BT_SMP)
	(void)bt_conn_auth_cb_register(&auth_callbacks);
#endif /* CONFIG_BT_SMP */

#if defined(CONFIG_BT_USER_PHY_UPDATE)
	err = bt_conn_le_set_default_phy(BT_GAP_LE_PHY_1M, BT_GAP_LE_PHY_1M);
	if (err) {
		printk("Failed to set default PHY (err %d)\n", err);
		return 0U;
	}
#endif /* CONFIG_BT_USER_PHY_UPDATE */

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return 0U;
	}

	printk("Advertising successfully started\n");

	conn_connected = NULL;
	last_write_rate = 0U;
	write_countdown = &count;

	if (count != 0U) {
		printk("GATT Write countdown %u on connection.\n", count);
	} else {
		printk("GATT Write forever on connection.\n");
	}

	while (true) {
		struct bt_conn *conn = NULL;

		if (conn_connected) {
			/* Get a connection reference to ensure that a
			 * reference is maintained in case disconnected
			 * callback is called while we perform GATT Write
			 * command.
			 */
			conn = bt_conn_ref(conn_connected);
		}

		if (conn) {
			write_cmd(conn);
			bt_conn_unref(conn);

			if (count) {
				if ((count % 1000U) == 0U) {
					printk("GATT Write countdown %u\n", count);
				}

				count--;
				if (!count) {
					break;
				}
			}

			k_yield();
		} else {
			k_sleep(K_SECONDS(1));
		}
	}

	return last_write_rate;
}
