/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/classic/hfp_hf.h>
#include <zephyr/settings/settings.h>

static void hf_connected(struct bt_conn *conn, struct bt_hfp_hf *hf)
{
	printk("HFP HF Connected!\n");
}

static void hf_disconnected(struct bt_hfp_hf *hf)
{
	printk("HFP HF Disconnected!\n");
}

static void hf_sco_connected(struct bt_hfp_hf *hf, struct bt_conn *sco_conn)
{
	printk("HF SCO connected\n");
}

static void hf_sco_disconnected(struct bt_conn *sco_conn, uint8_t reason)
{
	printk("HF SCO disconnected\n");
}

static void hf_service(struct bt_hfp_hf *hf, uint32_t value)
{
	printk("Service indicator value: %u\n", value);
}

static void hf_outgoing(struct bt_hfp_hf *hf, struct bt_hfp_hf_call *call)
{
	printk("HF call %p outgoing\n", call);
}

static void hf_remote_ringing(struct bt_hfp_hf_call *call)
{
	printk("HF remote call %p start ringing\n", call);
}

static void hf_incoming(struct bt_hfp_hf *hf, struct bt_hfp_hf_call *call)
{
	printk("HF call %p incoming\n", call);
}

static void hf_incoming_held(struct bt_hfp_hf_call *call)
{
	printk("HF call %p is held\n", call);
}

static void hf_accept(struct bt_hfp_hf_call *call)
{
	printk("HF call %p accepted\n", call);
}

static void hf_reject(struct bt_hfp_hf_call *call)
{
	printk("HF call %p rejected\n", call);
}

static void hf_terminate(struct bt_hfp_hf_call *call)
{
	printk("HF call %p terminated\n", call);
}

static void hf_signal(struct bt_hfp_hf *hf, uint32_t value)
{
	printk("Signal indicator value: %u\n", value);
}

static void hf_roam(struct bt_hfp_hf *hf, uint32_t value)
{
	printk("Roaming indicator value: %u\n", value);
}

static void hf_battery(struct bt_hfp_hf *hf, uint32_t value)
{
	printk("Battery indicator value: %u\n", value);
}

static void hf_ring_indication(struct bt_hfp_hf_call *call)
{
	printk("HF call %p ring\n", call);
}

static struct bt_hfp_hf_cb hf_cb = {
	.connected = hf_connected,
	.disconnected = hf_disconnected,
	.sco_connected = hf_sco_connected,
	.sco_disconnected = hf_sco_disconnected,
	.service = hf_service,
	.outgoing = hf_outgoing,
	.remote_ringing = hf_remote_ringing,
	.incoming = hf_incoming,
	.incoming_held = hf_incoming_held,
	.accept = hf_accept,
	.reject = hf_reject,
	.terminate = hf_terminate,
	.signal = hf_signal,
	.roam = hf_roam,
	.battery = hf_battery,
	.ring_indication = hf_ring_indication,
};

static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	printk("Bluetooth initialized\n");

	err = bt_br_set_connectable(true);
	if (err) {
		printk("BR/EDR set/rest connectable failed (err %d)\n", err);
		return;
	}
	err = bt_br_set_discoverable(true, false);
	if (err) {
		printk("BR/EDR set discoverable failed (err %d)\n", err);
		return;
	}

	printk("BR/EDR set connectable and discoverable done\n");
}

static void handsfree_enable(void)
{
	int err;

	err = bt_hfp_hf_register(&hf_cb);
	if (err < 0) {
		printk("HFP HF Registration failed (err %d)\n", err);
	}
}

int main(void)
{
	int err;

	handsfree_enable();

	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
	}
	return 0;
}
