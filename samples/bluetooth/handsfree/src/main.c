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
#include <zephyr/bluetooth/hfp_hf.h>

static void connected(struct bt_conn *conn)
{
	printk("HFP HF Connected!\n");
}

static void disconnected(struct bt_conn *conn)
{
	printk("HFP HF Disconnected!\n");
}

static void service(struct bt_conn *conn, uint32_t value)
{
	printk("Service indicator value: %u\n", value);
}

static void call(struct bt_conn *conn, uint32_t value)
{
	printk("Call indicator value: %u\n", value);
}

static void call_setup(struct bt_conn *conn, uint32_t value)
{
	printk("Call Setup indicator value: %u\n", value);
}

static void call_held(struct bt_conn *conn, uint32_t value)
{
	printk("Call Held indicator value: %u\n", value);
}

static void signal(struct bt_conn *conn, uint32_t value)
{
	printk("Signal indicator value: %u\n", value);
}

static void roam(struct bt_conn *conn, uint32_t value)
{
	printk("Roaming indicator value: %u\n", value);
}

static void battery(struct bt_conn *conn, uint32_t value)
{
	printk("Battery indicator value: %u\n", value);
}

static void ring_cb(struct bt_conn *conn)
{
	printk("Incoming Call...\n");
}

static struct bt_hfp_hf_cb hf_cb = {
	.connected = connected,
	.disconnected = disconnected,
	.service = service,
	.call = call,
	.call_setup = call_setup,
	.call_held = call_held,
	.signal = signal,
	.roam = roam,
	.battery = battery,
	.ring_indication = ring_cb,
};

static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	err = bt_br_set_connectable(true);
	if (err) {
		printk("BR/EDR set/rest connectable failed (err %d)\n", err);
		return;
	}
	err = bt_br_set_discoverable(true);
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

void main(void)
{
	int err;

	handsfree_enable();

	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
	}
}
