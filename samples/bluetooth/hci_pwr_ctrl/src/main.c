/* main.c - Application main entry point */

/*
 * Copyright (c) 2019 Andrei Stoica
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_vs.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/hrs.h>

static struct bt_conn *default_conn;
static uint16_t default_conn_handle;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_HRS_VAL)),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)
#define DEVICE_BEACON_TXPOWER_NUM  8

static struct k_thread pwr_thread_data;
static K_THREAD_STACK_DEFINE(pwr_thread_stack, 512);

static const int8_t txpower[DEVICE_BEACON_TXPOWER_NUM] = {4, 0, -3, -8,
							  -15, -18, -23, -30};
static const struct bt_le_adv_param *param =
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE,
			0x0020, 0x0020, NULL);

static void read_conn_rssi(uint16_t handle, int8_t *rssi)
{
	struct net_buf *buf, *rsp = NULL;
	struct bt_hci_cp_read_rssi *cp;
	struct bt_hci_rp_read_rssi *rp;

	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_READ_RSSI, sizeof(*cp));
	if (!buf) {
		printk("Unable to allocate command buffer\n");
		return;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(handle);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_RSSI, buf, &rsp);
	if (err) {
		printk("Read RSSI err: %d\n", err);
		return;
	}

	rp = (void *)rsp->data;
	*rssi = rp->rssi;

	net_buf_unref(rsp);
}


static void set_tx_power(uint8_t handle_type, uint16_t handle, int8_t tx_pwr_lvl)
{
	struct bt_hci_cp_vs_write_tx_power_level *cp;
	struct bt_hci_rp_vs_write_tx_power_level *rp;
	struct net_buf *buf, *rsp = NULL;
	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_VS_WRITE_TX_POWER_LEVEL,
				sizeof(*cp));
	if (!buf) {
		printk("Unable to allocate command buffer\n");
		return;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(handle);
	cp->handle_type = handle_type;
	cp->tx_power_level = tx_pwr_lvl;

	err = bt_hci_cmd_send_sync(BT_HCI_OP_VS_WRITE_TX_POWER_LEVEL,
				   buf, &rsp);
	if (err) {
		printk("Set Tx power err: %d\n", err);
		return;
	}

	rp = (void *)rsp->data;
	printk("Actual Tx Power: %d\n", rp->selected_tx_power);

	net_buf_unref(rsp);
}

static void get_tx_power(uint8_t handle_type, uint16_t handle, int8_t *tx_pwr_lvl)
{
	struct bt_hci_cp_vs_read_tx_power_level *cp;
	struct bt_hci_rp_vs_read_tx_power_level *rp;
	struct net_buf *buf, *rsp = NULL;
	int err;

	*tx_pwr_lvl = 0xFF;
	buf = bt_hci_cmd_create(BT_HCI_OP_VS_READ_TX_POWER_LEVEL,
				sizeof(*cp));
	if (!buf) {
		printk("Unable to allocate command buffer\n");
		return;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(handle);
	cp->handle_type = handle_type;

	err = bt_hci_cmd_send_sync(BT_HCI_OP_VS_READ_TX_POWER_LEVEL,
				   buf, &rsp);
	if (err) {
		printk("Read Tx power err: %d\n", err);
		return;
	}

	rp = (void *)rsp->data;
	*tx_pwr_lvl = rp->tx_power_level;

	net_buf_unref(rsp);
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int8_t txp;
	int ret;

	if (err) {
		printk("Connection failed (err 0x%02x)\n", err);
	} else {
		default_conn = bt_conn_ref(conn);
		ret = bt_hci_get_conn_handle(default_conn,
					     &default_conn_handle);
		if (ret) {
			printk("No connection handle (err %d)\n", ret);
		} else {
			/* Send first at the default selected power */
			bt_addr_le_to_str(bt_conn_get_dst(conn),
							  addr, sizeof(addr));
			printk("Connected via connection (%d) at %s\n",
			       default_conn_handle, addr);
			get_tx_power(BT_HCI_VS_LL_HANDLE_TYPE_CONN,
				     default_conn_handle, &txp);
			printk("Connection (%d) - Initial Tx Power = %d\n",
			       default_conn_handle, txp);

			set_tx_power(BT_HCI_VS_LL_HANDLE_TYPE_CONN,
				     default_conn_handle,
				     BT_HCI_VS_LL_TX_POWER_LEVEL_NO_PREF);
			get_tx_power(BT_HCI_VS_LL_HANDLE_TYPE_CONN,
				     default_conn_handle, &txp);
			printk("Connection (%d) - Tx Power = %d\n",
			       default_conn_handle, txp);
		}
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason 0x%02x)\n", reason);

	if (default_conn) {
		bt_conn_unref(default_conn);
		default_conn = NULL;
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	/* Start advertising */
	err = bt_le_adv_start(param, ad, ARRAY_SIZE(ad),
			      sd, ARRAY_SIZE(sd));
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Dynamic Tx power Beacon started\n");
}

static void hrs_notify(void)
{
	static uint8_t heartrate = 90U;

	/* Heartrate measurements simulation */
	heartrate++;
	if (heartrate == 160U) {
		heartrate = 90U;
	}

	bt_hrs_notify(heartrate);
}

void modulate_tx_power(void *p1, void *p2, void *p3)
{
	int8_t txp_get = 0;
	uint8_t idx = 0;

	while (1) {
		if (!default_conn) {
			printk("Set Tx power level to %d\n", txpower[idx]);
			set_tx_power(BT_HCI_VS_LL_HANDLE_TYPE_ADV,
				     0, txpower[idx]);

			k_sleep(K_SECONDS(5));

			printk("Get Tx power level -> ");
			get_tx_power(BT_HCI_VS_LL_HANDLE_TYPE_ADV,
				     0, &txp_get);
			printk("TXP = %d\n", txp_get);

			idx = (idx+1) % DEVICE_BEACON_TXPOWER_NUM;
		} else {
			int8_t rssi = 0xFF;
			int8_t txp_adaptive;

			idx = 0;

			read_conn_rssi(default_conn_handle, &rssi);
			printk("Connected (%d) - RSSI = %d\n",
			       default_conn_handle, rssi);
			if (rssi > -70) {
				txp_adaptive = -20;
			} else if (rssi > -90) {
				txp_adaptive = -12;
			} else {
				txp_adaptive = -4;
			}
			printk("Adaptive Tx power selected = %d\n",
			       txp_adaptive);
			set_tx_power(BT_HCI_VS_LL_HANDLE_TYPE_CONN,
				     default_conn_handle, txp_adaptive);
			get_tx_power(BT_HCI_VS_LL_HANDLE_TYPE_CONN,
				     default_conn_handle, &txp_get);
			printk("Connection (%d) TXP = %d\n",
			       default_conn_handle, txp_get);

			k_sleep(K_SECONDS(1));
		}
	}
}

int main(void)
{
	int8_t txp_get = 0xFF;
	int err;

	default_conn = NULL;
	printk("Starting Dynamic Tx Power Beacon Demo\n");

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
	}

	printk("Get Tx power level ->");
	get_tx_power(BT_HCI_VS_LL_HANDLE_TYPE_ADV, 0, &txp_get);
	printk("-> default TXP = %d\n", txp_get);

	/* Wait for 5 seconds to give a chance users/testers
	 * to check that default Tx power is indeed the one
	 * selected in Kconfig.
	 */
	k_sleep(K_SECONDS(5));

	k_thread_create(&pwr_thread_data, pwr_thread_stack,
			K_THREAD_STACK_SIZEOF(pwr_thread_stack),
			modulate_tx_power, NULL, NULL, NULL,
			K_PRIO_COOP(10),
			0, K_NO_WAIT);
	k_thread_name_set(&pwr_thread_data, "DYN TX");

	while (1) {
		hrs_notify();
		k_sleep(K_SECONDS(2));
	}
	return 0;
}
