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
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/tmap.h>

#include "tmap_bms.h"

static int init(void)
{
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		printk("Bluetooth enable failed (err %d)\n", err);
		return err;
	}

	printk("Bluetooth initialized\n");

	return 0;
}

int main(void)
{
	int err;

	err = init();
	if (err != 0) {
		return err;
	}

	printk("Initializing TMAP and setting role\n");
	/* Initialize TMAP */
	err = bt_tmap_register(BT_TMAP_ROLE_BMS);
	if (err != 0) {
		return err;
	}

	/* Initialize CAP Initiator */
	err = cap_initiator_init();
	if (err != 0) {
		return err;
	}
	printk("CAP initialized\n");

	/* Configure and start broadcast stream */
	err = cap_initiator_setup();
	if (err != 0) {
		return err;
	}

	return 0;
}
