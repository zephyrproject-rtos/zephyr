/*
 * Copyright 2020 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

/**
 * @brief Collect the values for several channels
 *
 * @param dev Sensor device to read from
 * @param ... any number of pairs of arguments:
 *	first is the sensor channel to read (-1 to terminate the list)
 *	second is a pointer to the struct sensor_value to put it in
 * @return 0 on success
 * @return negative error code from sensor API on failure
 */
static int get_channels(const struct device *dev, ...)
{
	va_list ptr;
	int i;

	va_start(ptr, dev);
	for (i = 0;; i++) {
		int chan;
		struct sensor_value *val;
		int err;

		chan = va_arg(ptr, int);
		if (chan == -1) {
			break;
		}
		val = va_arg(ptr, struct sensor_value *);
		err = sensor_channel_get(dev, chan, val);
		if (err < 0) {
			va_end(ptr);
			return err;
		}
	}

	va_end(ptr);
	return 0;
}

/* battery */
static int cmd_battery(const struct shell *sh, size_t argc, char **argv)
{
	struct sensor_value temp, volt, current, i_desired, charge_remain;
	struct sensor_value charge, v_desired, v_design, cap, nom_cap;
	struct sensor_value full, empty;
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(battery));
	bool allowed;
	int err;

	if (!device_is_ready(dev)) {
		shell_error(sh, "Device not ready (%s)", argv[1]);
		return -ENODEV;
	}

	err = sensor_sample_fetch(dev);
	if (err < 0) {
		shell_error(sh, "Failed to read sensor: %d", err);
	}

	err = get_channels(dev,
			   SENSOR_CHAN_GAUGE_TEMP, &temp,
			   SENSOR_CHAN_GAUGE_VOLTAGE, &volt,
			   SENSOR_CHAN_GAUGE_AVG_CURRENT, &current,
			   SENSOR_CHAN_GAUGE_DESIRED_VOLTAGE, &v_desired,
			   SENSOR_CHAN_GAUGE_DESIRED_CHARGING_CURRENT,
			   &i_desired,
			   SENSOR_CHAN_GAUGE_STATE_OF_CHARGE, &charge,
			   SENSOR_CHAN_GAUGE_DESIGN_VOLTAGE, &v_design,
			   SENSOR_CHAN_GAUGE_REMAINING_CHARGE_CAPACITY,
			   &charge_remain,
			   SENSOR_CHAN_GAUGE_FULL_CHARGE_CAPACITY, &cap,
			   SENSOR_CHAN_GAUGE_NOM_AVAIL_CAPACITY, &nom_cap,
			   SENSOR_CHAN_GAUGE_TIME_TO_FULL, &full,
			   SENSOR_CHAN_GAUGE_TIME_TO_EMPTY, &empty,
			   -1);
	if (err < 0) {
		return err;
	}

	shell_print(sh, "Temp:  %.1d.%02d C",
		    temp.val1, temp.val2 / 10000);
	shell_print(sh, "V: %5d.%02d V",
		    volt.val1, volt.val2 / 10000);
	shell_print(sh, "V-desired: %d.%02d V",
		    v_desired.val1, v_desired.val2 / 10000);
	shell_fprintf_normal(sh, "I:    %lld mA",
		    sensor_value_to_milli(&current));
	if (current.val1 > 0) {
		shell_fprintf_normal(sh, " (CHG)");
	} else if (current.val1 < 0) {
		shell_fprintf_normal(sh, " (DISCHG)");
	}
	shell_fprintf_normal(sh, "\n");
	shell_print(sh, "I-desired: %5d mA",
		    i_desired.val1);
	allowed = i_desired.val1 && v_desired.val2 && charge.val1 < 100;
	shell_print(sh, "Charging: %sAllowed",
		    allowed ? "" : "Not ");
	shell_print(sh, "Charge: %d %%", charge.val1);
	shell_print(sh, "V-design: %d.%02d V",
		    v_design.val1, v_design.val2 / 10000);
	shell_print(sh, "Remaining: %d mAh",
		    charge_remain.val1);
	shell_print(sh, "Cap-full: %d mAh", cap.val1);
	shell_print(sh, "Design: %d mAh", nom_cap.val1);
	shell_print(sh, "Time full: %dh:%02d",
		    full.val1 / 60, full.val1 % 60);
	shell_print(sh, "Time empty: %dh:%02d",
		    empty.val1 / 60, empty.val1 % 60);

	return 0;
}

SHELL_CMD_REGISTER(battery, NULL, "Battery status", cmd_battery);
