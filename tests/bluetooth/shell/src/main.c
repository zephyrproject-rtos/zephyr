/** @file
 *  @brief Interactive Bluetooth LE shell application
 *
 *  Application allows implement Bluetooth LE functional commands performing
 *  simple diagnostic interaction between LE host stack and LE controller
 */

/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <misc/printk.h>
#include <misc/byteorder.h>
#include <zephyr.h>

#include <shell/shell.h>

#include <gatt/hrs.h>

#define DEVICE_NAME CONFIG_BLUETOOTH_DEVICE_NAME

#define MY_SHELL_MODULE "btshell"

static bool hrs_simulate;

static int cmd_hrs_simulate(int argc, char *argv[])
{
	if (argc < 2) {
		return -EINVAL;
	}

	if (!strcmp(argv[1], "on")) {
		static bool hrs_registered;

		if (!hrs_registered) {
			printk("Registering HRS Service\n");
			hrs_init(0x01);
			hrs_registered = true;
		}

		printk("Start HRS simulation\n");
		hrs_simulate = true;
	} else if (!strcmp(argv[1], "off")) {
		printk("Stop HRS simulation\n");
		hrs_simulate = false;
	} else {
		printk("Incorrect value: %s\n", argv[1]);
		return -EINVAL;
	}

	return 0;
}

#define HELP_NONE "[none]"
#define HELP_ADDR_LE "<address: XX:XX:XX:XX:XX:XX> <type: (public|random)>"

static const struct shell_cmd commands[] = {
	{ "hrs-simulate", cmd_hrs_simulate,
	  "register and simulate Heart Rate Service <value: on, off>" },
	{ NULL, NULL }
};

void main(void)
{
	printk("Type \"help\" for supported commands.\n");
	printk("Before any Bluetooth commands you must \"select bt\" and then "
	       "run \"init\".\n");

	SHELL_REGISTER(MY_SHELL_MODULE, commands);
	shell_register_default_module(MY_SHELL_MODULE);

	while (1) {
		k_sleep(MSEC_PER_SEC);

		/* Heartrate measurements simulation */
		if (hrs_simulate) {
			hrs_notify();
		}
	}
}
