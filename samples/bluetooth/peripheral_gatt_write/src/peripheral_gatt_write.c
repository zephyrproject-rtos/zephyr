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

#include "gatt_write.h"

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static int start_adv(void)
{
	return bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
}

static void mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	printk("Updated MTU: TX: %d RX: %d bytes\n", tx, rx);
}

#if defined(CONFIG_BT_SMP)
static void auth_cancel(struct bt_conn *conn)
{
	printk("Pairing cancelled: %s\n", bt_conn_dst_str(conn));
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
	printk("Device found: %s (RSSI %d)\n", bt_addr_le_str(addr), rssi);
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

	err = start_adv();
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return 0U;
	}

	printk("Advertising successfully started\n");

	/* Re-advertise on disconnect so a reset/reconnecting peer resumes. */
	restart_adv_func = start_adv;

	conn_connected = NULL;
	last_write_rate = 0U;
	write_countdown = &count;

	if (count != 0U) {
		printk("GATT Write countdown %u on connection.\n", count);
	} else {
		printk("GATT Write forever on connection.\n");
	}

	while (true) {
		if (conn_connected == NULL) {
			(void)k_sleep(K_SECONDS(1));
			continue;
		}

		/* Hold a reference across the transmit so the connection is not
		 * freed by the disconnected callback mid-transmit. bt_conn_ref()
		 * returns NULL if the connection was torn down concurrently.
		 */
		struct bt_conn *conn = conn_connected;

		if (conn != NULL) {
			conn = bt_conn_ref(conn);
		}

		if (conn == NULL) {
			continue;
		}

		if (IS_ENABLED(CONFIG_USE_NOTIFY)) {
			err = notify_poll(conn) ? notify_data(conn) : -EAGAIN;
			if (err != 0) {
				/* Not ready (-EAGAIN), out of buffers (-ENOMEM) or
				 * the link is going down: back off one connection
				 * interval. Sleeping (not yielding) lets time
				 * advance so a pending disconnect is delivered.
				 */
				k_timeout_t delay = conn_interval_get(conn);

				bt_conn_unref(conn);
				(void)k_sleep(delay);
				continue;
			}
		} else {
			err = write_cmd(conn);
			if (err != 0 && err != -ENOMEM) {
				/* -ENOMEM just means no TX buffer was free this
				 * iteration and is retried on the next one; any
				 * other error is unexpected.
				 */
				printk("%s: write_cmd failed (%d)\n", __func__, err);
			}
		}

		bt_conn_unref(conn);

		/* Passing `0` will not use the countdown; the block below is
		 * optimized out by the linker in that case.
		 */
		if (count != 0U) {
			if ((count % 1000U) == 0U) {
				printk("GATT %s countdown %u\n",
				       IS_ENABLED(CONFIG_USE_NOTIFY) ? "Notify" : "Write",
				       count);
			}

			count--;
			if (count == 0U) {
				break;
			}
		}

		k_yield();
	}

	return last_write_rate;
}
