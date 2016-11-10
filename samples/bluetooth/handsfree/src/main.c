/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>
#include <misc/byteorder.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/hfp_hf.h>

static void connected(struct bt_conn *conn)
{
	printk("HFP HF Connected!\n");
}

static void disconnected(struct bt_conn *conn)
{
	printk("HFP HF Disconnected!\n");
}

static struct bt_hfp_hf_cb hf_cb = {
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
