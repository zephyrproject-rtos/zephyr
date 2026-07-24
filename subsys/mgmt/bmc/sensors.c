/*
 * SPDX-FileCopyrightText: © 2025-2026 Tenstorrent USA, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/shell/shell.h>

#include "bmc_init.h"
#include "sensors.h"

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bmc_sensors);

#define DIE_TEMP_NODE DT_NODELABEL(die_temp)

#if DT_NODE_EXISTS(DIE_TEMP_NODE)

static const struct device *die_temp = DEVICE_DT_GET(DIE_TEMP_NODE);

int read_die_temperature(struct sensor_value *val)
{
	int rc;

	if (!device_is_ready(die_temp)) {
		LOG_WRN("Device %s not ready.\n", die_temp->name);
		return -EIO;
	}

	rc = sensor_sample_fetch(die_temp);
	if (rc) {
		LOG_WRN("Failed to fetch sample (%d)\n", rc);
		return rc;
	}

	rc = sensor_channel_get(die_temp, SENSOR_CHAN_DIE_TEMP, val);
	if (rc) {
		LOG_WRN("Failed to get data (%d)\n", rc);
		return rc;
	}

	return 0;
}

static int cmd_show_sensors(const struct shell *sh, size_t argc, char **argv)
{
	struct sensor_value val;
	int rc;
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!is_boot_finished()) {
		shell_error(sh, "must wait for boot to finish");
		return -EAGAIN;
	}

	rc = read_die_temperature(&val);
	if (rc) {
		shell_error(sh, "Could not read die temp sensor (err=%d)", rc);
		return rc;
	}

	shell_print(sh, "Sensor %s: %d.%d °C", die_temp->name, val.val1, val.val2 / 100000);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_sensors_cmds,
	SHELL_CMD(show,		NULL, "Show sensors.", cmd_show_sensors),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(sensors, &sub_sensors_cmds, "Sensors commands", NULL);

#else
int read_die_temperature(struct sensor_value *val)
{
	return -ENODEV;
}
#endif
