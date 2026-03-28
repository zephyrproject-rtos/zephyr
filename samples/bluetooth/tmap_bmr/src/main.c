/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/tmap.h>

#include "tmap_bmr.h"

int main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		printk("Bluetooth init failed (err %d)\n", err);
		return err;
	}

	printk("Bluetooth initialized\n");

	printk("Initializing TMAP and setting role\n");
	err = bt_tmap_register(BT_TMAP_ROLE_BMR);
	if (err != 0) {
		return err;
	}

	err = vcp_vol_renderer_init();
	if (err != 0) {
		return err;
	}
	printk("VCP initialized\n");

	printk("Initializing BAP Broadcast Sink\n");
	err = bap_broadcast_sink_init();
	if (err != 0) {
		return err;
	}

	printk("Starting BAP Broadcast Sink\n");
	err = bap_broadcast_sink_run();
	if (err != 0) {
		return err;
	}

	return 0;
}
