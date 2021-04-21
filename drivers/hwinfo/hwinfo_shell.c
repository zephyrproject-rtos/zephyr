/*
 * Copyright (c) 2019 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/printk.h>
#include <shell/shell.h>
#include <drivers/hwinfo.h>
#include <zephyr/types.h>
#include <logging/log.h>

static int cmd_get_device_id(const struct shell *shell, size_t argc, char **argv)
{
	uint8_t dev_id[16];
	ssize_t length;
	int i;

	length = hwinfo_get_device_id(dev_id, sizeof(dev_id));

	if (length == -ENOTSUP) {
		shell_error(shell, "Not supported by hardware");
		return -ENOTSUP;
	} else if (length < 0) {
		shell_error(shell, "Error: %d", length);
		return length;
	}

	shell_fprintf(shell, SHELL_NORMAL, "Length: %d\n", length);
	shell_fprintf(shell, SHELL_NORMAL, "ID: 0x");

	for (i = 0 ; i < length ; i++) {
		shell_fprintf(shell, SHELL_NORMAL, "%02x", dev_id[i]);
	}

	shell_fprintf(shell, SHELL_NORMAL, "\n");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_hwinfo,
	SHELL_CMD_ARG(devid, NULL, "Show device id", cmd_get_device_id, 1, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_ARG_REGISTER(hwinfo, &sub_hwinfo, "HWINFO commands", NULL, 2, 0);
