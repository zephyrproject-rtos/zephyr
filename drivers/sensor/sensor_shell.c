/*
 * Copyright (c) 2018 Diego Sueiro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <shell/shell.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <device.h>
#include <sensor.h>

extern struct device __device_init_start[];
extern struct device __device_init_end[];

const char *sensor_channel_name[SENSOR_CHAN_ALL] = {
	[SENSOR_CHAN_ACCEL_X] =		"accel_x",
	[SENSOR_CHAN_ACCEL_Y] =		"accel_y",
	[SENSOR_CHAN_ACCEL_Z] =		"accel_z",
	[SENSOR_CHAN_ACCEL_XYZ] =	"accel_xyz",
	[SENSOR_CHAN_GYRO_X] =		"gyro_x",
	[SENSOR_CHAN_GYRO_Y] =		"gyro_y",
	[SENSOR_CHAN_GYRO_Z] =		"gyro_z",
	[SENSOR_CHAN_GYRO_XYZ] =	"gyro_xyz",
	[SENSOR_CHAN_MAGN_X] =		"magn_x",
	[SENSOR_CHAN_MAGN_Y] =		"magn_y",
	[SENSOR_CHAN_MAGN_Z] =		"magn_z",
	[SENSOR_CHAN_MAGN_XYZ] =	"magn_xyz",
	[SENSOR_CHAN_DIE_TEMP] =	"die_temp",
	[SENSOR_CHAN_AMBIENT_TEMP] =	"ambient_temp",
	[SENSOR_CHAN_PRESS] =		"press",
	[SENSOR_CHAN_PROX] =		"prox",
	[SENSOR_CHAN_HUMIDITY] =	"humidity",
	[SENSOR_CHAN_LIGHT] =		"light",
	[SENSOR_CHAN_IR] =		"ir",
	[SENSOR_CHAN_RED] =		"red",
	[SENSOR_CHAN_GREEN] =		"green",
	[SENSOR_CHAN_BLUE] =		"blue",
	[SENSOR_CHAN_ALTITUDE] =	"altitude",
	[SENSOR_CHAN_PM_1_0] =		"pm_1_0",
	[SENSOR_CHAN_PM_2_5] =		"pm_2_5",
	[SENSOR_CHAN_PM_10] =		"pm_10",
	[SENSOR_CHAN_DISTANCE] =	"distance",
	[SENSOR_CHAN_CO2] =		"co2",
	[SENSOR_CHAN_VOC] =		"voc",
	[SENSOR_CHAN_VOLTAGE] =		"voltage",
	[SENSOR_CHAN_CURRENT] =		"current",
	[SENSOR_CHAN_ROTATION] =	"rotation",
};

static int cmd_list_sensor_channels(const struct shell *shell,
				   size_t argc, char *argv[])
{
	int err;
	struct device *dev;
	struct sensor_value value[3];

	dev = device_get_binding(argv[1]);

	if (dev != NULL) {

		err = sensor_sample_fetch(dev);
		if (err) {
			shell_error(shell, "sensor_sample_fetch failed err=%d",
				    err);
			return err;
		}

		for (unsigned int i = 0; i < SENSOR_CHAN_ALL; i++) {
			if (!sensor_channel_get(dev, i, value)) {
				shell_print(shell, "idx=%d name=%s",
					    i, sensor_channel_name[i]);
			}
		}

	} else {
		shell_error(shell, "Could not bind the %s sensor", argv[1]);
		return -ENXIO;
	}

	return 0;
}

static int cmd_get_sensor(const struct shell *shell, size_t argc, char *argv[])
{
	int err;
	struct device *dev;
	struct sensor_value value[3];
	int idx;

	dev = device_get_binding(argv[1]);

	if (dev != NULL) {
		err = sensor_sample_fetch(dev);
		if (err) {
			shell_error(shell, "sensor_sample_fetch failed err=%d",
				    err);
			return err;
		}

		if (isdigit((unsigned char)argv[2][0])) {
			idx = (u8_t)atoi(argv[2]);
			if (!sensor_channel_get(dev, idx, value)) {
				if (idx != SENSOR_CHAN_ACCEL_XYZ &&
				    idx != SENSOR_CHAN_GYRO_XYZ &&
				    idx != SENSOR_CHAN_MAGN_XYZ) {
					shell_print(shell,
					    "channel idx=%d %s = %10.6f", idx,
					    sensor_channel_name[idx],
					    sensor_value_to_double(&value[0]));
				} else {
					shell_print(shell,
					    "channel idx=%d %s = %10.6f", idx,
					    sensor_channel_name[idx],
					    sensor_value_to_double(&value[0]));
					shell_print(shell,
					    "channel idx=%d %s = %10.6f", idx,
					    sensor_channel_name[idx],
					    sensor_value_to_double(&value[1]));
					shell_print(shell,
					    "channel idx=%d %s = %10.6f", idx,
					    sensor_channel_name[idx],
					    sensor_value_to_double(&value[2]));
				}
			} else {
				shell_error(shell,
					"Could not read channel idx %d=%s",
					idx, sensor_channel_name[idx]);
				return -EIO;
			}
			return 0;
		}

		for (unsigned int i = 0; i < SENSOR_CHAN_ALL; i++) {
			if (!sensor_channel_get(dev, i, value)) {
				if (i != SENSOR_CHAN_ACCEL_XYZ &&
				    i != SENSOR_CHAN_GYRO_XYZ &&
				    i != SENSOR_CHAN_MAGN_XYZ) {
					shell_print(shell,
					    "channel idx=%d %s = %10.6f", i,
					    sensor_channel_name[i],
					    sensor_value_to_double(&value[0]));
				} else {
					shell_print(shell,
					    "channel idx=%d %s = %10.6f", i,
					    sensor_channel_name[i],
					    sensor_value_to_double(&value[0]));
					shell_print(shell,
					    "channel idx=%d %s = %10.6f", i,
					    sensor_channel_name[i],
					    sensor_value_to_double(&value[1]));
					shell_print(shell,
					    "channel idx=%d %s = %10.6f", i,
					    sensor_channel_name[i],
					    sensor_value_to_double(&value[2]));
				}
			}
		}

	} else {
		shell_error(shell, "Could not bind the %s sensor", argv[1]);
		return -ENXIO;
	}

	return 0;
}

static int cmd_list_sensors(const struct shell *shell,
			      size_t argc, char **argv)
{
	struct device *dev;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(shell, "devices:");
	for (dev = __device_init_start; dev != __device_init_end; dev++) {
		if (dev->driver_api != NULL) {
			shell_print(shell, "- %s", dev->config->name);
		}
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_sensor,
	SHELL_CMD_ARG(get, NULL, "<device_name> [chanel_idx]", cmd_get_sensor,
		      2, 1),
	SHELL_CMD(list, NULL, "List configured sensors", cmd_list_sensors),
	SHELL_CMD_ARG(list_channels, NULL, "<device_name>",
		  cmd_list_sensor_channels, 2, 0),
	SHELL_SUBCMD_SET_END
	);

SHELL_CMD_REGISTER(sensor, &sub_sensor, "Sensor commands", NULL);
