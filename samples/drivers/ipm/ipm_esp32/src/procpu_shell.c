/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/dt-bindings/gpio/nordic-npm6001-gpio.h>
#include <zephyr/dt-bindings/regulator/npm6001.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/printk.h>

/* Command usage info. */
#define START_HELP ("<cmd>\n\nStart the APPCPU")
#define STOP_HELP  ("<cmd>\n\nStop the APPCPU")

void esp_appcpu_image_start(unsigned int hdr_offset);
void esp_appcpu_image_stop(void);

static int cmd_appcpu_start(const struct shell *sh, size_t argc, char *argv[])
{
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	printk("start appcpu\n");

	esp_appcpu_image_start(0x20);

	return 0;
}

static int cmd_appcpu_stop(const struct shell *sh, size_t argc, char *argv[])
{
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	printk("stop appcpu\n");

	esp_appcpu_image_stop();

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_amp,
			       /* Alphabetically sorted to ensure correct Tab autocompletion. */
			       SHELL_CMD_ARG(appstart, NULL, START_HELP, cmd_appcpu_start, 1, 0),
			       SHELL_CMD_ARG(appstop, NULL, STOP_HELP, cmd_appcpu_stop, 1, 0),
			       SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(amp, &sub_amp, "AMP debug commands.", NULL);
