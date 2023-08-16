/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>

#include <zephyr/shell/shell.h>
#include <zephyr/sys/util.h>

#include <stdlib.h>
#include <string.h>
#include <zephyr/drivers/ilc.h>

#define MAX_CHANNEL_PER_COUNTER 32

static int cmd_list_counter(const struct shell *shctx, size_t argc, char *argv[])
{
	const struct device *ilc_dev;
	struct ilc_params param;
	unsigned int counter_value[MAX_CHANNEL_PER_COUNTER];
	int ret;
	float cal_time;

	ilc_dev = device_get_binding(argv[1]);

	if (ilc_dev == NULL) {
		shell_error(shctx, "Given ILC device was not found");
		return -ENODEV;
	}

	if (!device_is_ready(ilc_dev)) {
		shell_error(shctx, "ILC driver not ready");
		return -ENODEV;
	}

	ret = ilc_read_params(ilc_dev, &param);

	if (ret != 0) {
		shell_error(shctx, "ILC read params failed");
		return ret;
	}

	for (int index = 0; index < param.port_count; index++) {
		ret = ilc_read_counter(ilc_dev, &counter_value[index], index);

		if (ret != 0) {
			shell_error(shctx, "ILC read counter channel failed");
			return ret;
		}
	}

	shell_print(shctx, "Total Port Configured in Interrupt latency Counter %d",
		    param.port_count);
	for (int index = 0; index < param.port_count; index++) {
		cal_time = ((1 / param.frequency) * counter_value[index]);
		shell_print(shctx, "Channel No: %d  - Time: %f sec", index, cal_time);
	}

	return 0;
}

static int cmd_enable_ilc(const struct shell *shctx, size_t argc, char *argv[])
{
	const struct device *ilc_dev;
	int ret;

	ilc_dev = device_get_binding(argv[1]);

	if (ilc_dev == NULL) {
		shell_error(shctx, "Given ILC device was not found");
		return -ENODEV;
	}

	ret = ilc_enable(ilc_dev);

	if (ret != 0) {
		shell_error(shctx, "ILC Enable failed");
		return ret;
	}

	shell_error(shctx, "ILC Enable Successfully");

	return ret;
}

static int cmd_disable_ilc(const struct shell *shctx, size_t argc, char *argv[])
{
	const struct device *ilc_dev;
	int ret;

	ilc_dev = device_get_binding(argv[1]);

	if (ilc_dev == NULL) {
		shell_error(shctx, "Given ILC device was not found");
		return -ENODEV;
	}

	ret = ilc_disable(ilc_dev);

	if (ret != 0) {
		shell_error(shctx, "ILC disable failed");
		return ret;
	}

	shell_error(shctx, "ILC Disable Successfully");

	return ret;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_ilc,
	SHELL_CMD_ARG(enable, NULL, "Enable ILC. Usage: ilc enable <device>", cmd_enable_ilc, 2, 0),
	SHELL_CMD_ARG(disable, NULL, "Disable ILC. Usage: ilc disable <device>", cmd_disable_ilc, 2,
		      0),
	SHELL_CMD_ARG(list, NULL, "Show Counter Value . Usage: ilc list <device>", cmd_list_counter,
		      2, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(ilc, &sub_ilc, "Value display for interrupt latency counter", NULL);
