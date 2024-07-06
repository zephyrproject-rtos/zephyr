/*
 * Copyright (c) 2021 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

/* Currently only nRF5 hardware is supported by this sample. */
#include "bt_clock_hal_nrf5.h"

#define LATENCY_MS      20U /* 20.0 ms, FT=2 with CIS reliability policy, otherwise w/o effect */
#define SDU_INTERVAL_US (10U * USEC_PER_MSEC) /* 10.0 ms */
#define NUM_RETRIES     1U                    /* SDU tx retries, influences ISO interval length */

/* We define an application, HCI transport and controller specific guard time
 * which needs to protect against a race between SDU preparation and transmission
 * under worst-case conditions including:
 * - time between scheduling SDU preparation work and its actual execution from
 *   the work queue
 * - SDU preparation time
 * - HCI transport latency
 * - internal controller proccessing latency
 */
#define SDU_PREPARATION_GUARD_TIME_US 3000U

static void start_scan(void);

static struct bt_conn *default_conn;
static struct bt_iso_chan iso_chan;

static struct bt_iso_tx_info last_iso_tx_sync_info;
static inline bool is_synchronized(void)
{
	return last_iso_tx_sync_info.seq_num != 0;
}

static uint32_t seq_num;
static int32_t sdu_interval_us;

NET_BUF_POOL_FIXED_DEFINE(tx_pool, 2, BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static void iso_send_sdu(struct k_work *work);

static K_WORK_DEFINE(iso_send_sdu_work, iso_send_sdu);

static void iso_timer_timeout(struct k_timer *timer);

static K_TIMER_DEFINE(iso_timer, iso_timer_timeout, NULL);

/**
 * @brief Send ISO data SDU
 *
 * This will send an increasing amount of ISO data, starting from 1 octet.
 *
 * First iteration : 0x00
 * Second iteration: 0x00 0x01
 * Third iteration : 0x00 0x01 0x02
 *
 * And so on, until it wraps around the configured ISO TX MTU (CONFIG_BT_ISO_TX_MTU)
 *
 * @param work Pointer to the work structure
 */
static void iso_send_sdu(struct k_work *work)
{
	static uint8_t buf_data[CONFIG_BT_ISO_TX_MTU];
	static bool is_first_sdu = true;
	static size_t len_to_send = 1;

	struct net_buf *buf;
	int ret;

	ARG_UNUSED(work);

	if (unlikely(is_first_sdu)) {
		/* initialize data */
		for (int i = 0; i < ARRAY_SIZE(buf_data); i++) {
			buf_data[i] = (uint8_t)i;
		}
	}

	/* Send a single SDU, wait for synchronization (which may take longer
	 * than one SDU interval) and then start sending regularly with drift
	 * compensation.
	 */
	if (unlikely(is_first_sdu) || is_synchronized()) {
		is_first_sdu = false;

		buf = net_buf_alloc(&tx_pool, K_FOREVER);
		if (!buf) {
			printk("Failed to allocate buffer\n");
			return;
		}

		net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
		net_buf_add_mem(buf, buf_data, len_to_send);

		ret = bt_iso_chan_send(&iso_chan, buf, seq_num);
		if (ret < 0) {
			printk("Failed to send ISO data (%d)\n", ret);
			net_buf_unref(buf);
		}

		len_to_send++;
		if (len_to_send > ARRAY_SIZE(buf_data)) {
			len_to_send = 1;
		}
	}

	ret = bt_iso_chan_get_tx_sync(&iso_chan, &last_iso_tx_sync_info);
	if (ret && ret != -EACCES) {
		printk("Failed obtaining timestamp\n");
	}
	/* Offset should be zero for unframed PDUs */
	__ASSERT_NO_MSG(last_iso_tx_sync_info.offset == 0);
}

/**
 * @brief Get the time difference between two Bluetooth clock values in
 * microseconds. Wraps the returned value if it exceeds the Bluetooth clock
 * cycle.
 *
 * @param minuend    minuend value
 * @param subtrahend subtrahend value (will be deduced from the minuend)
 *
 * @returns time difference in microseconds (potentially wrapped).
 */
static uint32_t bt_clock_time_diff_us(uint32_t minuend, uint32_t subtrahend)
{
	__ASSERT_NO_MSG(minuend <= BT_CLOCK_HAL_WRAPPING_POINT_US);

	uint32_t result = ((uint64_t)minuend + BT_CLOCK_HAL_CYCLE_US - subtrahend) %
			  ((uint64_t)BT_CLOCK_HAL_CYCLE_US);

	return result;
}

/* The time unit in which some ISO protocol time spans are measured, e.g. FT. */
#define ISO_INT_UNIT_US 1250U
/**
 * @brief Calculate the time offset in microseconds until the next SDU must be
 * prepared and sent.
 *
 * @returns The time offset in microseconds.
 */
static int32_t calculate_send_sdu_time_offset_us(void)
{
	uint32_t now_us, next_sdu_ts_us, elapsed_us;
	int skipped_sdu_intervals;
	int32_t timer_offset;

	now_us = hal_ticker_now_usec();
	if (now_us + SDU_PREPARATION_GUARD_TIME_US <= last_iso_tx_sync_info.ts) {
		/* We synchonization to a future SDU whose scheduling point is
		 * beyond the guard time, i.e. the scheduling point can still be
		 * reached.
		 */
		next_sdu_ts_us = last_iso_tx_sync_info.ts;
	} else {
		/* We synchronize on a past SDU or an SDU whose scheduling point
		 * is closer than the guard time and can no longer be reached.
		 */
		elapsed_us = bt_clock_time_diff_us(now_us + SDU_PREPARATION_GUARD_TIME_US,
						   last_iso_tx_sync_info.ts);
		skipped_sdu_intervals =
			DIV_ROUND_UP(elapsed_us + SDU_PREPARATION_GUARD_TIME_US, sdu_interval_us);
		next_sdu_ts_us =
			(last_iso_tx_sync_info.ts + skipped_sdu_intervals * sdu_interval_us) %
			BT_CLOCK_HAL_CYCLE_US;
	}

	timer_offset = bt_clock_time_diff_us(
		next_sdu_ts_us,
		(hal_ticker_now_usec() + SDU_PREPARATION_GUARD_TIME_US) % BT_CLOCK_HAL_CYCLE_US);

	return timer_offset;
}

static void iso_timer_timeout(struct k_timer *timer)
{
	int32_t timer_offset;

	k_work_submit(&iso_send_sdu_work);

	timer_offset = is_synchronized() ? calculate_send_sdu_time_offset_us() : sdu_interval_us;
	k_timer_start(timer, K_USEC(timer_offset), K_FOREVER);
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	int err;

	if (default_conn) {
		/* Already connected */
		return;
	}

	/* We're only interested in connectable events */
	if (type != BT_GAP_ADV_TYPE_ADV_IND && type != BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
		return;
	}

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	printk("Device found: %s (RSSI %d)\n", addr_str, rssi);

	/* connect only to devices in close proximity */
	if (rssi < -50) {
		return;
	}

	if (bt_le_scan_stop()) {
		return;
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT,
				&default_conn);
	if (err) {
		printk("Create conn to %s failed (%u)\n", addr_str, err);
		start_scan();
	}
}

static void start_scan(void)
{
	int err;

	/* This demo doesn't require active scan */
	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err) {
		printk("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");
}

/**
 * @brief Calculate the SDU interval
 *
 * @details Calculates the SDU interval based on current channel information.
 *
 * This demonstrates how to derive the SDU interval from ISO channel information
 * available via HCI even if the original channel configuration is not known
 * (e.g. on a peripheral device).
 *
 * This function may yield, so do not use it in ISR context.
 *
 * @param chan pointer to the ISO channel
 *
 * @returns SDU interval in microseconds, -EINVAL if failed to obtain ISO info.
 */
static int32_t calculate_sdu_interval_us(struct bt_iso_chan *chan)
{
	uint32_t transport_latency_us, cig_sync_delay_us, flush_timeout_us;
	struct bt_iso_unicast_tx_info *iso_unicast_central_info;
	struct bt_iso_unicast_info *iso_unicast_info;
	int32_t derived_sdu_interval_us;
	struct bt_iso_info iso_info;
	int err;

	err = bt_iso_chan_get_info(chan, &iso_info);
	if (err) {
		printk("Failed obtaining ISO info\n");
		return -EINVAL;
	}

	iso_unicast_info = &iso_info.unicast;
	iso_unicast_central_info = &iso_unicast_info->central;

	transport_latency_us = iso_unicast_central_info->latency;
	cig_sync_delay_us = iso_unicast_info->cig_sync_delay;
	flush_timeout_us = iso_unicast_central_info->flush_timeout * ISO_INT_UNIT_US;

	/* Transport_Latency = CIG_Sync_Delay + FT x ISO_Interval - SDU_Interval,
	 * see Bluetooth Core, Version 5.4, Vol 6, Part G, Section 3.2.2.
	 */
	derived_sdu_interval_us = cig_sync_delay_us + flush_timeout_us - transport_latency_us;
	__ASSERT_NO_MSG(derived_sdu_interval_us > 0);

	return derived_sdu_interval_us;
}

static void iso_connected(struct bt_iso_chan *chan)
{
	printk("ISO Channel %p connected\n", chan);

	seq_num = 0U;

	sdu_interval_us = calculate_sdu_interval_us(chan);
	if (sdu_interval_us < 0) {
		printk("Failed calculating the SDU interval\n");
		return;
	}

	/* start send timer */
	k_timer_start(&iso_timer, K_NO_WAIT, K_FOREVER);
}

static void iso_disconnected(struct bt_iso_chan *chan, uint8_t reason)
{
	printk("ISO Channel %p disconnected (reason 0x%02x)\n", chan, reason);
	k_timer_stop(&iso_timer);
	sdu_interval_us = 0;
}

static struct bt_iso_chan_ops iso_ops = {
	.connected	= iso_connected,
	.disconnected	= iso_disconnected,
};

static struct bt_iso_chan_io_qos iso_tx = {
	.sdu = CONFIG_BT_ISO_TX_MTU,
	.phy = BT_GAP_LE_PHY_2M,
	.rtn = NUM_RETRIES,
	.path = NULL,
};

static struct bt_iso_chan_qos iso_qos = {
	.tx = &iso_tx,
	.rx = NULL,
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	struct bt_iso_connect_param connect_param;
	char addr[BT_ADDR_LE_STR_LEN];
	int iso_err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		printk("Failed to connect to %s (%u)\n", addr, err);

		bt_conn_unref(default_conn);
		default_conn = NULL;

		start_scan();
		return;
	}

	if (conn != default_conn) {
		return;
	}

	printk("Connected: %s\n", addr);

	connect_param.acl = conn;
	connect_param.iso_chan = &iso_chan;

	iso_err = bt_iso_chan_connect(&connect_param, 1);

	if (iso_err) {
		printk("Failed to connect iso (%d)\n", iso_err);
		return;
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (conn != default_conn) {
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s (reason 0x%02x)\n", addr, reason);

	bt_conn_unref(default_conn);
	default_conn = NULL;

	start_scan();
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

int main(void)
{
	struct bt_iso_chan *channels[1];
	struct bt_iso_cig_param param;
	struct bt_iso_cig *cig;
	int err;

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}


	printk("Bluetooth initialized\n");

	iso_chan.ops = &iso_ops;
	iso_chan.qos = &iso_qos;
#if defined(CONFIG_BT_SMP)
	iso_chan.required_sec_level = BT_SECURITY_L2,
#endif /* CONFIG_BT_SMP */

	channels[0] = &iso_chan;
	param.cis_channels = channels;
	param.num_cis = ARRAY_SIZE(channels);
	param.sca = BT_GAP_SCA_UNKNOWN;
	param.packing = BT_ISO_PACKING_SEQUENTIAL;
	param.framing = BT_ISO_FRAMING_UNFRAMED;
	param.c_to_p_latency = LATENCY_MS;
	param.p_to_c_latency = LATENCY_MS;
	param.c_to_p_interval = SDU_INTERVAL_US;
	param.p_to_c_interval = SDU_INTERVAL_US;

	err = bt_iso_cig_create(&param, &cig);

	if (err != 0) {
		printk("Failed to create CIG (%d)\n", err);
		return 0;
	}

	start_scan();

	return 0;
}
