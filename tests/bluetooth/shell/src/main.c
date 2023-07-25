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
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/uart.h>

#include <zephyr/shell/shell.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/services/hrs.h>

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME

#if defined(CONFIG_BT_HRS)
static bool hrs_simulate;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
		      BT_UUID_16_ENCODE(BT_UUID_HRS_VAL),
		      BT_UUID_16_ENCODE(BT_UUID_BAS_VAL),
		      BT_UUID_16_ENCODE(BT_UUID_DIS_VAL)),
};

static int cmd_hrs_simulate(const struct shell *sh,
			    size_t argc, char *argv[])
{
	static bool hrs_registered;
	int err;

	if (!strcmp(argv[1], "on")) {
		if (!hrs_registered && IS_ENABLED(CONFIG_BT_BROADCASTER)) {
			shell_print(sh, "Registering HRS Service");
			hrs_registered = true;
			err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad,
					      ARRAY_SIZE(ad), NULL, 0);
			if (err) {
				shell_error(sh, "Advertising failed to start"
					    " (err %d)\n", err);
				return -ENOEXEC;
			}

			printk("Advertising successfully started\n");
		}

		shell_print(sh, "Start HRS simulation");
		hrs_simulate = true;
	} else if (!strcmp(argv[1], "off")) {
		shell_print(sh, "Stop HRS simulation");

		if (hrs_registered && IS_ENABLED(CONFIG_BT_BROADCASTER)) {
			bt_le_adv_stop();
		}

		hrs_simulate = false;
	} else {
		shell_print(sh, "Incorrect value: %s", argv[1]);
		shell_help(sh);
		return -ENOEXEC;
	}

	return 0;
}
#endif /* CONFIG_BT_HRS */

#define HELP_NONE "[none]"
#define HELP_ADDR_LE "<address: XX:XX:XX:XX:XX:XX> <type: (public|random)>"

SHELL_STATIC_SUBCMD_SET_CREATE(hrs_cmds,
#if defined(CONFIG_BT_HRS)
	SHELL_CMD_ARG(simulate, NULL,
		"register and simulate Heart Rate Service <value: on, off>",
		cmd_hrs_simulate, 2, 0),
#endif /* CONFIG_BT_HRS*/
	SHELL_SUBCMD_SET_END
);

static int cmd_hrs(const struct shell *sh, size_t argc, char **argv)
{
	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}

SHELL_CMD_ARG_REGISTER(hrs, &hrs_cmds, "Heart Rate Service shell commands",
		       cmd_hrs, 2, 0);

#if defined(CONFIG_BT_HRS)
static void hrs_notify(void)
{
	static uint8_t heartrate = 90U;

	/* Heartrate measurements simulation */
	heartrate++;
	if (heartrate == 160U) {
		heartrate = 90U;
	}

	bt_hrs_notify(heartrate);
}
#endif /* CONFIG_BT_HRS */

int main(void)
{
#if DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_shell_uart), zephyr_cdc_acm_uart)
	const struct device *dev;
	uint32_t dtr = 0;

	dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_shell_uart));
	if (!device_is_ready(dev) || usb_enable(NULL)) {
		return 0;
	}

	while (!dtr) {
		uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
		k_sleep(K_MSEC(100));
	}
#endif

	printk("Type \"help\" for supported commands.");
	printk("Before any Bluetooth commands you must `bt init` to initialize"
	       " the stack.\n");

	while (1) {
		k_sleep(K_SECONDS(1));

#if defined(CONFIG_BT_HRS)
		/* Heartrate measurements simulation */
		if (hrs_simulate) {
			hrs_notify();
		}
#endif /* CONFIG_BT_HRS */
	}
}
