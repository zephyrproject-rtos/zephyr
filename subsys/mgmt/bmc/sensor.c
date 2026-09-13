/*
 * SPDX-FileCopyrightText: Copyright (c) 2025-2026 Tenstorrent USA, Inc.
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/bmc.h>
#include <zephyr/mgmt/bmc/sensor.h>

LOG_MODULE_DECLARE(bmc, CONFIG_BMC_LOG_LEVEL);

int bmc_sensor_dt_read(const struct bmc_sensor *sensor, struct sensor_value *val)
{
	const struct bmc_sensor_dt_ctx *ctx = sensor->ctx;
	int ret;

	if (!device_is_ready(ctx->dev)) {
		LOG_WRN("Sensor device %s not ready", ctx->dev->name);
		return -ENODEV;
	}

	ret = sensor_sample_fetch_chan(ctx->dev, ctx->chan);
	if (ret < 0) {
		LOG_WRN("Failed to fetch %s sample (err=%d)", ctx->dev->name, ret);
		return ret;
	}

	return sensor_channel_get(ctx->dev, ctx->chan, val);
}

int bmc_sensor_read(const struct bmc_sensor *sensor, struct sensor_value *val)
{
	if (sensor == NULL || sensor->read == NULL) {
		return -ENODEV;
	}

	return sensor->read(sensor, val);
}

const struct bmc_sensor *bmc_sensor_get(const char *id)
{
	BMC_SENSOR_FOREACH(sensor) {
		if (strcmp(sensor->id, id) == 0) {
			return sensor;
		}
	}

	return NULL;
}

size_t bmc_sensor_count(void)
{
	size_t count;

	STRUCT_SECTION_COUNT(bmc_sensor, &count);

	return count;
}

#if defined(CONFIG_SHELL)
#include <zephyr/shell/shell.h>

static int cmd_bmc_sensor_show(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (bmc_sensor_count() == 0) {
		shell_print(sh, "No sensors registered");
		return 0;
	}

	BMC_SENSOR_FOREACH(sensor) {
		struct sensor_value val;
		int ret;

		ret = bmc_sensor_read(sensor, &val);
		if (ret < 0) {
			shell_print(sh, "%-16s %s: unavailable (err=%d)", sensor->id,
				    sensor->name, ret);
			continue;
		}

		shell_print(sh, "%-16s %s: %d.%06d %s", sensor->id, sensor->name, val.val1,
			    abs(val.val2), sensor->units != NULL ? sensor->units : "");
	}

	return 0;
}

SHELL_SUBCMD_SET_CREATE(bmc_sensor_subcmds, (bmc, sensor));
SHELL_SUBCMD_ADD((bmc, sensor), show, NULL, "Show sensor readings.", cmd_bmc_sensor_show, 1, 0);
SHELL_SUBCMD_ADD((bmc), sensor, &bmc_sensor_subcmds, "Sensor commands.", NULL, 1, 0);
#endif /* CONFIG_SHELL */
