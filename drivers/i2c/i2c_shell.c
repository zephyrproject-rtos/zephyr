/*
 * Copyright (c) 2018 Prevas A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <shell/shell.h>
#include <stdlib.h>
#include <drivers/i2c.h>
#include <string.h>
#include <sys/util.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(i2c_shell, CONFIG_LOG_DEFAULT_LEVEL);

#define I2C_DEVICE_PREFIX "I2C_"

extern struct device __device_init_start[];
extern struct device __device_init_end[];

static int cmd_i2c_scan(const struct shell *shell,
			size_t argc, char **argv)
{
	struct device *dev;
	u8_t cnt = 0, first = 0x04, last = 0x77;

	dev = device_get_binding(argv[1]);

	if (!dev) {
		shell_error(shell, "I2C: Device driver %s not found.",
			    argv[1]);
		return -ENODEV;
	}

	shell_print(shell,
		    "     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f");
	for (u8_t i = 0; i <= last; i += 16) {
		shell_fprintf(shell, SHELL_NORMAL, "%02x: ", i);
		for (u8_t j = 0; j < 16; j++) {
			if (i + j < first || i + j > last) {
				shell_fprintf(shell, SHELL_NORMAL, "   ");
				continue;
			}

			struct i2c_msg msgs[1];
			u8_t dst;

			/* Send the address to read from */
			msgs[0].buf = &dst;
			msgs[0].len = 0U;
			msgs[0].flags = I2C_MSG_WRITE | I2C_MSG_STOP;
			if (i2c_transfer(dev, &msgs[0], 1, i + j) == 0) {
				shell_fprintf(shell, SHELL_NORMAL,
					      "%02x ", i + j);
				++cnt;
			} else {
				shell_fprintf(shell, SHELL_NORMAL, "-- ");
			}
		}
		shell_print(shell, "");
	}

	shell_print(shell, "%u devices found on %s",
		    cnt, argv[1]);

	return 0;
}

static void device_name_get(size_t idx, struct shell_static_entry *entry);

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	int device_idx = 0;
	struct device *dev;

	entry->syntax = NULL;
	entry->handler = NULL;
	entry->help  = NULL;
	entry->subcmd = &dsub_device_name;

	for (dev = __device_init_start; dev != __device_init_end; dev++) {
		if ((dev->driver_api != NULL) &&
		strstr(dev->config->name, I2C_DEVICE_PREFIX) != NULL &&
		strcmp(dev->config->name, "") && (dev->config->name != NULL)) {
			if (idx == device_idx) {
				entry->syntax = dev->config->name;
				break;
			}
			device_idx++;
		}
	}
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_i2c_cmds,
			       SHELL_CMD(scan, &dsub_device_name,
					 "Scan I2C devices", cmd_i2c_scan),
			       SHELL_SUBCMD_SET_END     /* Array terminated. */
			       );

SHELL_CMD_REGISTER(i2c, &sub_i2c_cmds, "I2C commands", NULL);
