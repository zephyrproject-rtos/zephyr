/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * LE Connection Subrating - peripheral demo for nRF54L15 DK.
 *
 * Advertises as "subrate_demo" and accepts a connection from the subrate_central
 * sample, which drives the subrate factor (Core Spec, Vol 6, Part B, 5.1.19).
 * Logs each applied factor and blinks the LED; the LED is otherwise off so it
 * does not perturb a current measurement (the peripheral skips events too).
 */
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>

#define HEARTBEAT_MS 2000

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static volatile int pending_blink;
static volatile bool is_connected;
static volatile uint16_t applied_factor = 1U;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static void blink(int times)
{
	for (int i = 0; i < times; i++) {
		(void)gpio_pin_set_dt(&led, 1);
		k_sleep(K_MSEC(120));
		(void)gpio_pin_set_dt(&led, 0);
		k_sleep(K_MSEC(120));
	}
}

static void start_adv(void)
{
	int err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), NULL, 0);

	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
	}
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	ARG_UNUSED(conn);

	if (err) {
		printk("Failed to connect (err 0x%02x)\n", err);
		return;
	}
	printk("[%lld] Connected to central\n", k_uptime_get());
	applied_factor = 1U;
	is_connected = true;
	pending_blink = 1;
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(conn);

	printk("[%lld] Disconnected (reason 0x%02x); advertising again\n", k_uptime_get(), reason);
	is_connected = false;
	start_adv();
}

static void subrate_changed(struct bt_conn *conn,
			    const struct bt_conn_le_subrate_changed *params)
{
	ARG_UNUSED(conn);

	if (params->status != BT_HCI_ERR_SUCCESS) {
		printk("Subrate change failed (status 0x%02x)\n", params->status);
		return;
	}
	applied_factor = params->factor;
	printk("[%lld] Subrate factor now %u (continuation %u, peripheral latency %u)\n",
	       k_uptime_get(), params->factor, params->continuation_number,
	       params->peripheral_latency);
	pending_blink = 1;
}

BT_CONN_CB_DEFINE(conn_cb) = {
	.connected = connected,
	.disconnected = disconnected,
	.subrate_changed = subrate_changed,
};

int main(void)
{
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
	printk("Subrating peripheral started; advertising as \"%s\"\n", CONFIG_BT_DEVICE_NAME);

	start_adv();

	int64_t next_beat_ms = 0;

	while (1) {
		int64_t now = k_uptime_get();

		if (pending_blink) {
			int n = pending_blink;

			pending_blink = 0;
			blink(n);
			now = k_uptime_get();
		}

		if (is_connected && now >= next_beat_ms) {
			next_beat_ms = now + HEARTBEAT_MS;
			printk("[%lld] alive, factor %u\n", now, applied_factor);
		}

		k_sleep(K_MSEC(50));
	}

	return 0;
}
