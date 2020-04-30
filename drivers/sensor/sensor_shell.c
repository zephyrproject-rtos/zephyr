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
#include <drivers/sensor.h>

#define SENSOR_GET_HELP \
	"Get sensor data. Channel names are optional. All channels are read " \
	"when no channels are provided. Syntax:\n" \
	"<device_name> <channel name 0> .. <channel name N>"

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
	[SENSOR_CHAN_GAUGE_VOLTAGE] =	"gauge_voltage",
	[SENSOR_CHAN_GAUGE_AVG_CURRENT] = "gauge_avg_current",
	[SENSOR_CHAN_GAUGE_STATE_OF_CHARGE] =	"gauge_state_of_charge",
	[SENSOR_CHAN_GAUGE_TEMP] =	"gauge_temp",
	[SENSOR_CHAN_GAUGE_FULL_CHARGE_CAPACITY] =	"gauge_full_cap",
	[SENSOR_CHAN_GAUGE_REMAINING_CHARGE_CAPACITY] = "gauge_remaining_cap",
	[SENSOR_CHAN_GAUGE_NOM_AVAIL_CAPACITY] =	"gauge_nominal_cap",
	[SENSOR_CHAN_GAUGE_TIME_TO_EMPTY] =	"gauge_time_to_empty",
	[SENSOR_CHAN_GAUGE_TIME_TO_FULL] =	"gauge_time_to_full",
	[SENSOR_CHAN_GAUGE_CYCLE_COUNT] =	"gauge_cycle_count",
	[SENSOR_CHAN_GAUGE_DESIGN_VOLTAGE] =	"gauge_design_voltage",
	[SENSOR_CHAN_GAUGE_DESIRED_CHARGING_CURRENT] =
		 "gauge_desired_charging_current",
};

static int handle_channel_by_name(const struct shell *shell,
					const struct device *dev,
					const char *channel_name)
{
	struct sensor_value value[3];
	char *endptr;
	int err;
	int i;

	/* Attempt to parse channel name as a number first */
	i = strtoul(channel_name, &endptr, 0);

	if (*endptr != '\0') {
		/* Channel name is not a number, look it up */
		for (i = 0; i < ARRAY_SIZE(sensor_channel_name); i++) {
			if (strcmp(channel_name, sensor_channel_name[i]) == 0) {
				break;
			}
		}

		if (i == ARRAY_SIZE(sensor_channel_name)) {
			shell_error(shell, "Channel not supported (%s)",
				    channel_name);
			return -ENOTSUP;
		}
	}

	err = sensor_channel_get(dev, i, value);
	if (err < 0) {
		return err;
	}

	if (i >= ARRAY_SIZE(sensor_channel_name)) {
		shell_print(shell, "channel idx=%d value = %10.6f", i,
			    sensor_value_to_double(&value[0]));
	} else if (i != SENSOR_CHAN_ACCEL_XYZ &&
		i != SENSOR_CHAN_GYRO_XYZ &&
		i != SENSOR_CHAN_MAGN_XYZ) {
		shell_print(shell,
			"channel idx=%d %s = %10.6f", i,
			sensor_channel_name[i],
			sensor_value_to_double(&value[0]));
	} else {
		shell_print(shell,
			"channel idx=%d %s x = %10.6f y = %10.6f z = %10.6f",
			i, sensor_channel_name[i],
			sensor_value_to_double(&value[0]),
			sensor_value_to_double(&value[1]),
			sensor_value_to_double(&value[2]));
	}

	return 0;
}
static int cmd_get_sensor(const struct shell *shell, size_t argc, char *argv[])
{
	const struct device *dev;
	int err;

	dev = device_get_binding(argv[1]);
	if (dev == NULL) {
		shell_error(shell, "Device unknown (%s)", argv[1]);
		return -ENODEV;
	}

	err = sensor_sample_fetch(dev);
	if (err < 0) {
		shell_error(shell, "Failed to read sensor: %d", err);
	}

	if (argc == 2) {
		/* read all channels */
		for (int i = 0; i < ARRAY_SIZE(sensor_channel_name); i++) {
			if (sensor_channel_name[i]) {
				handle_channel_by_name(shell, dev,
							sensor_channel_name[i]);
			}
		}
	} else {
		for (int i = 2; i < argc; i++) {
			err = handle_channel_by_name(shell, dev, argv[i]);
			if (err < 0) {
				shell_error(shell,
					"Failed to read channel (%s)", argv[i]);
			}
		}
	}

	return 0;
}

static void channel_name_get(size_t idx, struct shell_static_entry *entry);

SHELL_DYNAMIC_CMD_CREATE(dsub_channel_name, channel_name_get);

static void channel_name_get(size_t idx, struct shell_static_entry *entry)
{
	int cnt = 0;

	entry->syntax = NULL;
	entry->handler = NULL;
	entry->help  = NULL;
	entry->subcmd = &dsub_channel_name;

	for (int i = 0; i < SENSOR_CHAN_ALL; i++) {
		if (sensor_channel_name[i] != NULL) {
			if (cnt == idx) {
				entry->syntax = sensor_channel_name[i];
				break;
			}
			cnt++;
		}
	}
}

static void device_name_get(size_t idx, struct shell_static_entry *entry);

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_lookup(idx, NULL);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help  = NULL;
	entry->subcmd = &dsub_channel_name;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_sensor,
	SHELL_CMD_ARG(get, &dsub_device_name, SENSOR_GET_HELP, cmd_get_sensor,
			2, 255),
	SHELL_SUBCMD_SET_END
	);

SHELL_CMD_REGISTER(sensor, &sub_sensor, "Sensor commands", NULL);
