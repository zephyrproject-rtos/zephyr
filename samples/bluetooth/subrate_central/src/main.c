/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * LE Connection Subrating - central demo for two nRF54L15 DKs.
 *
 * Scans for the subrate_peripheral sample, connects at a fixed connection
 * interval, then automatically cycles the subrate factor through active / idle /
 * dormant tiers (1 / 8 / 33) on a fixed dwell timer using central-initiated
 * Connection Subrate Updates (Core Spec, Vol 6, Part B, 5.1.19).
 *
 * It runs autonomously - no button - so you can flash, reset, and observe the
 * result over RTT (logging is routed to SEGGER RTT) and measure the board
 * current at each tier. A heartbeat line confirms the link survives each tier
 * (factor 33 sits just under the supervision-timeout bound). The LED blinks the
 * tier index when a factor is applied, then stays off so it does not perturb a
 * current measurement.
 */
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>

/* Must match the subrate_peripheral sample's CONFIG_BT_DEVICE_NAME. */
#define TARGET_NAME "subrate_demo"

#define CONN_INTERVAL 24U  /* 30 ms (1.25 ms units) */
#define CONN_TIMEOUT  200U /* 2 s (10 ms units) */

/* Hold each factor this long (long enough for a current reading) before
 * advancing to the next tier.
 */
#define TIER_DWELL_MS 10000
#define HEARTBEAT_MS  2000

/* active / idle / dormant subrate factors (1 = subrating off). */
static const uint16_t tiers[] = {1U, 8U, 33U};
static size_t tier_idx;

static struct bt_conn *default_conn;
static volatile uint16_t applied_factor;
static volatile int64_t last_change_ms;

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static volatile int pending_blink;

static void blink(int times)
{
	for (int i = 0; i < times; i++) {
		(void)gpio_pin_set_dt(&led, 1);
		k_sleep(K_MSEC(120));
		(void)gpio_pin_set_dt(&led, 0);
		k_sleep(K_MSEC(120));
	}
}

static bool name_match(struct bt_data *data, void *user_data)
{
	bool *found = user_data;

	if (data->type == BT_DATA_NAME_COMPLETE &&
	    data->data_len == sizeof(TARGET_NAME) - 1 &&
	    memcmp(data->data, TARGET_NAME, data->data_len) == 0) {
		*found = true;
		return false; /* stop parsing */
	}
	return true;
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	bool found = false;
	int err;

	ARG_UNUSED(rssi);

	if (default_conn || (type != BT_GAP_ADV_TYPE_ADV_IND &&
			     type != BT_GAP_ADV_TYPE_ADV_DIRECT_IND)) {
		return;
	}

	bt_data_parse(ad, name_match, &found);
	if (!found) {
		return;
	}

	(void)bt_le_scan_stop();

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
				BT_LE_CONN_PARAM(CONN_INTERVAL, CONN_INTERVAL, 0, CONN_TIMEOUT),
				&default_conn);
	if (err) {
		printk("Create connection failed (err %d)\n", err);
		(void)bt_le_scan_start(BT_LE_SCAN_ACTIVE, device_found);
	}
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	ARG_UNUSED(conn);

	if (err) {
		printk("Failed to connect (err 0x%02x)\n", err);
		bt_conn_unref(default_conn);
		default_conn = NULL;
		(void)bt_le_scan_start(BT_LE_SCAN_ACTIVE, device_found);
		return;
	}

	tier_idx = 0U;
	applied_factor = 1U;
	last_change_ms = k_uptime_get();
	printk("[%lld] Connected; auto-cycling factor (1/8/33) every %d ms\n",
	       last_change_ms, TIER_DWELL_MS);
	pending_blink = 1;
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(conn);

	printk("[%lld] Disconnected (reason 0x%02x); scanning again\n", k_uptime_get(), reason);
	if (default_conn) {
		bt_conn_unref(default_conn);
		default_conn = NULL;
	}
	(void)bt_le_scan_start(BT_LE_SCAN_ACTIVE, device_found);
}

static void subrate_changed(struct bt_conn *conn,
			    const struct bt_conn_le_subrate_changed *params)
{
	ARG_UNUSED(conn);

	if (params->status != BT_HCI_ERR_SUCCESS) {
		printk("[%lld] Subrate change failed (status 0x%02x)\n",
		       k_uptime_get(), params->status);
		return;
	}
	applied_factor = params->factor;
	printk("[%lld] Subrate factor now %u (continuation %u, peripheral latency %u)\n",
	       k_uptime_get(), params->factor, params->continuation_number,
	       params->peripheral_latency);
	pending_blink = (int)tier_idx + 1;
}

BT_CONN_CB_DEFINE(conn_cb) = {
	.connected = connected,
	.disconnected = disconnected,
	.subrate_changed = subrate_changed,
};

int main(void)
{
	int64_t next_beat_ms = 0;
	int err;

	if (!gpio_is_ready_dt(&led)) {
		printk("LED not ready\n");
		return 0;
	}
	(void)gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}
	printk("Subrating central started; scanning for \"%s\"\n", TARGET_NAME);

	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, device_found);
	if (err) {
		printk("Scan start failed (err %d)\n", err);
		return 0;
	}

	while (1) {
		int64_t now = k_uptime_get();

		if (pending_blink) {
			int n = pending_blink;

			pending_blink = 0;
			blink(n);
			now = k_uptime_get();
		}

		if (default_conn) {
			if (now - last_change_ms >= TIER_DWELL_MS) {
				last_change_ms = now;
				tier_idx = (tier_idx + 1U) % ARRAY_SIZE(tiers);

				struct bt_conn_le_subrate_param param = {
					.subrate_min = tiers[tier_idx],
					.subrate_max = tiers[tier_idx],
					.max_latency = 0U,
					.continuation_number = 0U,
					.supervision_timeout = CONN_TIMEOUT,
				};

				printk("[%lld] Requesting subrate factor %u\n", now, tiers[tier_idx]);
				err = bt_conn_le_subrate_request(default_conn, &param);
				if (err) {
					printk("[%lld] Subrate request (factor %u) failed (err %d)\n",
					       now, tiers[tier_idx], err);
				}
			}

			if (now >= next_beat_ms) {
				next_beat_ms = now + HEARTBEAT_MS;
				printk("[%lld] alive, factor %u\n", now, applied_factor);
			}
		}

		k_sleep(K_MSEC(100));
	}

	return 0;
}
