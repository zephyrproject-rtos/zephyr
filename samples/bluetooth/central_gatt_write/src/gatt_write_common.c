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

/* Count down number of metrics intervals before performing a PHY update */
#define PHY_UPDATE_COUNTDOWN 3U
static uint32_t phy_update_countdown;
static uint8_t phy_param_idx;

static void phy_update_iterate(struct bt_conn *conn)
{
	const struct bt_conn_le_phy_param phy_param[] = {
		/* List of 1M Tx with Rx on other PHYs */
		{
			.options = BT_CONN_LE_PHY_OPT_NONE,
			.pref_tx_phy = BT_GAP_LE_PHY_1M,
			.pref_rx_phy = BT_GAP_LE_PHY_1M,
		}, {
			.options = BT_CONN_LE_PHY_OPT_NONE,
			.pref_tx_phy = BT_GAP_LE_PHY_1M,
			.pref_rx_phy = BT_GAP_LE_PHY_2M,
		}, {
			.options = BT_CONN_LE_PHY_OPT_NONE,
			.pref_tx_phy = BT_GAP_LE_PHY_1M,
			.pref_rx_phy = BT_GAP_LE_PHY_CODED,
		},

		/* List of 2M Tx with Rx on other PHYs */
		{
			.options = BT_CONN_LE_PHY_OPT_NONE,
			.pref_tx_phy = BT_GAP_LE_PHY_2M,
			.pref_rx_phy = BT_GAP_LE_PHY_1M,
		}, {
			.options = BT_CONN_LE_PHY_OPT_NONE,
			.pref_tx_phy = BT_GAP_LE_PHY_2M,
			.pref_rx_phy = BT_GAP_LE_PHY_2M,
		}, {
			.options = BT_CONN_LE_PHY_OPT_NONE,
			.pref_tx_phy = BT_GAP_LE_PHY_2M,
			.pref_rx_phy = BT_GAP_LE_PHY_CODED,
		},

		/* List of Coded PHY S8 Tx with Rx on other PHYs */
		{
			.options = BT_CONN_LE_PHY_OPT_CODED_S8,
			.pref_tx_phy = BT_GAP_LE_PHY_CODED,
			.pref_rx_phy = BT_GAP_LE_PHY_1M,
		}, {
			.options = BT_CONN_LE_PHY_OPT_CODED_S8,
			.pref_tx_phy = BT_GAP_LE_PHY_CODED,
			.pref_rx_phy = BT_GAP_LE_PHY_2M,
		}, {
			.options = BT_CONN_LE_PHY_OPT_CODED_S8,
			.pref_tx_phy = BT_GAP_LE_PHY_CODED,
			.pref_rx_phy = BT_GAP_LE_PHY_CODED,
		},

		/* List of Coded PHY S2 Tx with Rx on other PHYs */
		{
			.options = BT_CONN_LE_PHY_OPT_CODED_S2,
			.pref_tx_phy = BT_GAP_LE_PHY_CODED,
			.pref_rx_phy = BT_GAP_LE_PHY_1M,
		}, {
			.options = BT_CONN_LE_PHY_OPT_CODED_S2,
			.pref_tx_phy = BT_GAP_LE_PHY_CODED,
			.pref_rx_phy = BT_GAP_LE_PHY_2M,
		}, {
			.options = BT_CONN_LE_PHY_OPT_CODED_S2,
			.pref_tx_phy = BT_GAP_LE_PHY_CODED,
			.pref_rx_phy = BT_GAP_LE_PHY_CODED,
		},

		/* Finally stop at 2M Tx with Rx on 2M */
		{
			.options = BT_CONN_LE_PHY_OPT_NONE,
			.pref_tx_phy = BT_GAP_LE_PHY_2M,
			.pref_rx_phy = BT_GAP_LE_PHY_2M,
		},
	};
	int err;

	if (phy_update_countdown--) {
		return;
	}

	phy_update_countdown = PHY_UPDATE_COUNTDOWN;

	phy_param_idx++;
	if (phy_param_idx >= ARRAY_SIZE(phy_param)) {
		/* No more PHY updates, stay at the last index */
		phy_param_idx = ARRAY_SIZE(phy_param);
		return;
	}

	struct bt_conn_info conn_info;

	err = bt_conn_get_info(conn, &conn_info);
	if (err) {
		printk("Failed to get connection info (%d).\n", err);
		return;
	}

	struct bt_conn_le_phy_param conn_phy_param;

	if (conn_info.role == BT_CONN_ROLE_CENTRAL) {
		conn_phy_param.options = phy_param[phy_param_idx].options;
		conn_phy_param.pref_tx_phy = phy_param[phy_param_idx].pref_tx_phy;
		conn_phy_param.pref_rx_phy = phy_param[phy_param_idx].pref_rx_phy;
	} else {
		conn_phy_param.options = phy_param[phy_param_idx].options;
		conn_phy_param.pref_tx_phy = phy_param[phy_param_idx].pref_rx_phy;
		conn_phy_param.pref_rx_phy = phy_param[phy_param_idx].pref_tx_phy;
	}

	printk("%s: PHY Update requested %u %u (%u)\n", __func__,
	       conn_phy_param.pref_tx_phy,
	       conn_phy_param.pref_rx_phy,
	       conn_phy_param.options);

	err = bt_conn_le_phy_update(conn, &conn_phy_param);
	if (err) {
		printk("Failed to update PHY (%d).\n", err);
		return;
	}
}

/* Interval between storing the measured write rate */
#define METRICS_INTERVAL 1U /* seconds */

static struct bt_gatt_exchange_params mtu_exchange_params;
static uint32_t write_count;
static uint32_t write_len;
static uint32_t write_rate;

/* Globals, reused by central_gatt_write and peripheral_gatt_write samples */
struct bt_conn *conn_connected;
uint32_t last_write_rate;
void (*start_scan_func)(void);

static void write_cmd_cb(struct bt_conn *conn, void *user_data)
{
	static uint32_t cycle_stamp;
	uint64_t delta;

	delta = k_cycle_get_32() - cycle_stamp;
	delta = k_cyc_to_ns_floor64(delta);

	if (delta == 0) {
		/* Skip division by zero */
		return;
	}

	/* if last data rx-ed was greater than 1 second in the past,
	 * reset the metrics.
	 */
	if (delta > (METRICS_INTERVAL * NSEC_PER_SEC)) {
		printk("%s: count= %u, len= %u, rate= %u bps.\n", __func__,
		       write_count, write_len, write_rate);

		last_write_rate = write_rate;

		write_count = 0U;
		write_len = 0U;
		write_rate = 0U;
		cycle_stamp = k_cycle_get_32();

		if (IS_ENABLED(CONFIG_BT_USER_PHY_UPDATE)) {
			phy_update_iterate(conn);
		}

	} else {
		uint16_t len;

		write_count++;

		/* Extract the 16-bit data length stored in user_data */
		len = (uint32_t)user_data & 0xFFFF;

		write_len += len;
		write_rate = ((uint64_t)write_len << 3) * (METRICS_INTERVAL * NSEC_PER_SEC) /
			     delta;
	}
}

static void mtu_exchange_cb(struct bt_conn *conn, uint8_t err,
			    struct bt_gatt_exchange_params *params)
{
	printk("%s: MTU exchange %s (%u)\n", __func__,
	       err == 0U ? "successful" : "failed",
	       bt_gatt_get_mtu(conn));
}

static int mtu_exchange(struct bt_conn *conn)
{
	int err;

	printk("%s: Current MTU = %u\n", __func__, bt_gatt_get_mtu(conn));

	mtu_exchange_params.func = mtu_exchange_cb;

	printk("%s: Exchange MTU...\n", __func__);
	err = bt_gatt_exchange_mtu(conn, &mtu_exchange_params);
	if (err) {
		printk("%s: MTU exchange failed (err %d)", __func__, err);
	}

	return err;
}

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	struct bt_conn_info conn_info;
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		printk("%s: Failed to connect to %s (%u)\n", __func__, addr,
		       conn_err);
		return;
	}

	err = bt_conn_get_info(conn, &conn_info);
	if (err) {
		printk("Failed to get connection info (%d).\n", err);
		return;
	}

	printk("%s: %s role %u\n", __func__, addr, conn_info.role);

	conn_connected = bt_conn_ref(conn);

	(void)mtu_exchange(conn);

#if defined(CONFIG_BT_SMP)
	if (conn_info.role == BT_CONN_ROLE_CENTRAL) {
		err = bt_conn_set_security(conn, BT_SECURITY_L2);
		if (err) {
			printk("Failed to set security (%d).\n", err);
		}
	}
#endif

	if (IS_ENABLED(CONFIG_BT_USER_PHY_UPDATE)) {
		phy_update_countdown = PHY_UPDATE_COUNTDOWN;
		phy_param_idx = 0U;
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct bt_conn_info conn_info;
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	err = bt_conn_get_info(conn, &conn_info);
	if (err) {
		printk("Failed to get connection info (%d).\n", err);
		return;
	}

	printk("%s: %s role %u, reason %u %s\n", __func__, addr, conn_info.role,
	       reason, bt_hci_err_to_str(reason));

	conn_connected = NULL;

	bt_conn_unref(conn);

	if (conn_info.role == BT_CONN_ROLE_CENTRAL) {
		start_scan_func();
	}
}

static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	printk("%s: int (0x%04x, 0x%04x) lat %u to %u\n", __func__,
	       param->interval_min, param->interval_max, param->latency,
	       param->timeout);

	return true;
}

static void le_param_updated(struct bt_conn *conn, uint16_t interval,
			     uint16_t latency, uint16_t timeout)
{
	printk("%s: int 0x%04x lat %u to %u\n", __func__, interval,
	       latency, timeout);
}

#if defined(CONFIG_BT_SMP)
static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	printk("%s: to level %u, err %s(%u)\n", __func__, level, bt_security_err_to_str(err), err);
}
#endif

#if defined(CONFIG_BT_USER_PHY_UPDATE)
static void le_phy_updated(struct bt_conn *conn,
			   struct bt_conn_le_phy_info *param)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("LE PHY Updated: %s Tx 0x%x, Rx 0x%x\n", addr, param->tx_phy,
	       param->rx_phy);
}
#endif /* CONFIG_BT_USER_PHY_UPDATE */

#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
static void le_data_len_updated(struct bt_conn *conn,
				struct bt_conn_le_data_len_info *info)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Data length updated: %s max tx %u (%u us) max rx %u (%u us)\n",
	       addr, info->tx_max_len, info->tx_max_time, info->rx_max_len,
	       info->rx_max_time);
}
#endif /* CONFIG_BT_USER_DATA_LEN_UPDATE */

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.le_param_req = le_param_req,
	.le_param_updated = le_param_updated,

#if defined(CONFIG_BT_SMP)
	.security_changed = security_changed,
#endif

#if defined(CONFIG_BT_USER_PHY_UPDATE)
	.le_phy_updated = le_phy_updated,
#endif /* CONFIG_BT_USER_PHY_UPDATE */

#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
	.le_data_len_updated = le_data_len_updated,
#endif /* CONFIG_BT_USER_DATA_LEN_UPDATE */
};

int write_cmd(struct bt_conn *conn)
{
	static uint8_t data[BT_ATT_MAX_ATTRIBUTE_LEN] = {0, };
	static uint16_t data_len;
	uint16_t data_len_max;
	int err;

	data_len_max = bt_gatt_get_mtu(conn) - 3;
	if (data_len_max > BT_ATT_MAX_ATTRIBUTE_LEN) {
		data_len_max = BT_ATT_MAX_ATTRIBUTE_LEN;
	}

#if TEST_FRAGMENTATION_WITH_VARIABLE_LENGTH_DATA
	/* Use incremental length data for every write command */
	/* TODO: Include test case in BabbleSim tests */
	static bool decrement;

	if (decrement) {
		data_len--;
		if (data_len <= 1) {
			data_len = 1;
			decrement = false;
		}
	} else {
		data_len++;
		if (data_len >= data_len_max) {
			data_len = data_len_max;
			decrement = true;
		}
	}
#else
	/* Use fixed length data for every write command */
	data_len = data_len_max;
#endif

	/* Pass the 16-bit data length value (instead of reference) in
	 * user_data so that unique value is pass for each write callback.
	 * Using handle 0x0001, we do not care if it is writable, we just want
	 * to transmit the data across.
	 */
	err = bt_gatt_write_without_response_cb(conn, 0x0001, data, data_len,
						false, write_cmd_cb,
						(void *)((uint32_t)data_len));
	if (err) {
		printk("%s: Write cmd failed (%d).\n", __func__, err);
	}

	return err;
}
