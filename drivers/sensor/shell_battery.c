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

struct ch_val_result {
		struct sensor_value val;
		bool supported;
};

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
		struct ch_val_result *val;
		int err;

		chan = va_arg(ptr, int);
		if (chan == -1) {
			break;
		}
		val = va_arg(ptr, struct ch_val_result *);
		err = sensor_channel_get(dev, chan, &val->val);
		if (err == -ENOTSUP) {
			val->supported = false;
		} else if (err < 0) {
			va_end(ptr);
			return err;
		} else {
			val->supported = true;
		}
	}

	va_end(ptr);
	return 0;
}

/* battery */
static int cmd_battery(const struct shell *sh, size_t argc, char **argv)
{
	struct ch_val_result temp, volt, current, i_desired, charge_remain;
	struct ch_val_result charge, v_desired, v_design, cap, nom_cap;
	struct ch_val_result full, empty;
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

	if (temp.supported) {
		shell_print(sh, "Temp:  %.1d.%02d C",
				temp.val.val1, temp.val.val2 / 10000);
	}

	if (volt.supported) {
		shell_print(sh, "V: %5d.%02d V",
				volt.val.val1, volt.val.val2 / 10000);
	}

	if (v_desired.supported) {
		shell_print(sh, "V-desired: %d.%02d V",
				v_desired.val.val1, v_desired.val.val2 / 10000);
	}

	if (current.supported) {
		shell_fprintf_normal(sh, "I:    %lld mA",
				sensor_value_to_milli(&current.val));
		if (current.val.val1 > 0) {
			shell_fprintf_normal(sh, " (CHG)");
		} else if (current.val.val1 < 0) {
			shell_fprintf_normal(sh, " (DISCHG)");
		} else {
			shell_fprintf_normal(sh, " (UNKWN)");
		}
		shell_fprintf_normal(sh, "\n");
	}

	if (i_desired.supported) {
		shell_print(sh, "I-desired: %5d mA",
				i_desired.val.val1);
		allowed = i_desired.val.val1 && v_desired.val.val2 && charge.val.val1 < 100;
		shell_print(sh, "Charging: %sAllowed",
				allowed ? "" : "Not ");
	}

	if (charge.supported) {
		shell_print(sh, "Charge: %d %%", charge.val.val1);
	}

	if (v_design.supported) {
		shell_print(sh, "V-design: %d.%02d V",
				v_design.val.val1, v_design.val.val2 / 10000);
	}

	if (charge_remain.supported) {
		shell_print(sh, "Remaining: %d mAh",
				charge_remain.val.val1);
	}

	if (cap.supported) {
		shell_print(sh, "Cap-full: %d mAh", cap.val.val1);
	}

	if (nom_cap.supported) {
		shell_print(sh, "Design: %d mAh", nom_cap.val.val1);
	}

	if (full.supported) {
		shell_print(sh, "Time full: %dh:%02d",
				full.val.val1 / 60, full.val.val1 % 60);
	}

	if (empty.supported) {
		shell_print(sh, "Time empty: %dh:%02d",
				empty.val.val1 / 60, empty.val.val1 % 60);
	}

	return 0;
}

SHELL_CMD_REGISTER(battery, NULL, "Battery status", cmd_battery);
