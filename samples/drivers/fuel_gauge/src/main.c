/*
 * Copyright (c) 2023 Alvaro Garcia Gomez <maxpowel@gmail.com>
 * Copyright (c) 2025 Philipp Steiner <philipp.steiner1987@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/sys/util.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/fuel_gauge.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app);

const char *fuel_gauge_prop_to_str(enum fuel_gauge_prop_type prop)
{
	switch (prop) {
	case FUEL_GAUGE_AVG_CURRENT:
		return "FUEL_GAUGE_AVG_CURRENT";
	case FUEL_GAUGE_CURRENT:
		return "FUEL_GAUGE_CURRENT";
	case FUEL_GAUGE_CHARGE_CUTOFF:
		return "FUEL_GAUGE_CHARGE_CUTOFF";
	case FUEL_GAUGE_CYCLE_COUNT:
		return "FUEL_GAUGE_CYCLE_COUNT";
	case FUEL_GAUGE_CONNECT_STATE:
		return "FUEL_GAUGE_CONNECT_STATE";
	case FUEL_GAUGE_FLAGS:
		return "FUEL_GAUGE_FLAGS";
	case FUEL_GAUGE_FULL_CHARGE_CAPACITY:
		return "FUEL_GAUGE_FULL_CHARGE_CAPACITY";
	case FUEL_GAUGE_PRESENT_STATE:
		return "FUEL_GAUGE_PRESENT_STATE";
	case FUEL_GAUGE_REMAINING_CAPACITY:
		return "FUEL_GAUGE_REMAINING_CAPACITY";
	case FUEL_GAUGE_RUNTIME_TO_EMPTY:
		return "FUEL_GAUGE_RUNTIME_TO_EMPTY";
	case FUEL_GAUGE_RUNTIME_TO_FULL:
		return "FUEL_GAUGE_RUNTIME_TO_FULL";
	case FUEL_GAUGE_SBS_MFR_ACCESS:
		return "FUEL_GAUGE_SBS_MFR_ACCESS";
	case FUEL_GAUGE_ABSOLUTE_STATE_OF_CHARGE:
		return "FUEL_GAUGE_ABSOLUTE_STATE_OF_CHARGE";
	case FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE:
		return "FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE";
	case FUEL_GAUGE_TEMPERATURE:
		return "FUEL_GAUGE_TEMPERATURE";
	case FUEL_GAUGE_VOLTAGE:
		return "FUEL_GAUGE_VOLTAGE";
	case FUEL_GAUGE_SBS_MODE:
		return "FUEL_GAUGE_SBS_MODE";
	case FUEL_GAUGE_CHARGE_CURRENT:
		return "FUEL_GAUGE_CHARGE_CURRENT";
	case FUEL_GAUGE_CHARGE_VOLTAGE:
		return "FUEL_GAUGE_CHARGE_VOLTAGE";
	case FUEL_GAUGE_STATUS:
		return "FUEL_GAUGE_STATUS";
	case FUEL_GAUGE_DESIGN_CAPACITY:
		return "FUEL_GAUGE_DESIGN_CAPACITY";
	case FUEL_GAUGE_DESIGN_VOLTAGE:
		return "FUEL_GAUGE_DESIGN_VOLTAGE";
	case FUEL_GAUGE_SBS_ATRATE:
		return "FUEL_GAUGE_SBS_ATRATE";
	case FUEL_GAUGE_SBS_ATRATE_TIME_TO_FULL:
		return "FUEL_GAUGE_SBS_ATRATE_TIME_TO_FULL";
	case FUEL_GAUGE_SBS_ATRATE_TIME_TO_EMPTY:
		return "FUEL_GAUGE_SBS_ATRATE_TIME_TO_EMPTY";
	case FUEL_GAUGE_SBS_ATRATE_OK:
		return "FUEL_GAUGE_SBS_ATRATE_OK";
	case FUEL_GAUGE_SBS_REMAINING_CAPACITY_ALARM:
		return "FUEL_GAUGE_SBS_REMAINING_CAPACITY_ALARM";
	case FUEL_GAUGE_SBS_REMAINING_TIME_ALARM:
		return "FUEL_GAUGE_SBS_REMAINING_TIME_ALARM";
	case FUEL_GAUGE_MANUFACTURER_NAME:
		return "FUEL_GAUGE_MANUFACTURER_NAME";
	case FUEL_GAUGE_DEVICE_NAME:
		return "FUEL_GAUGE_DEVICE_NAME";
	case FUEL_GAUGE_DEVICE_CHEMISTRY:
		return "FUEL_GAUGE_DEVICE_CHEMISTRY";
	case FUEL_GAUGE_CURRENT_DIRECTION:
		return "FUEL_GAUGE_CURRENT_DIRECTION";
	case FUEL_GAUGE_STATE_OF_CHARGE_ALARM:
		return "FUEL_GAUGE_STATE_OF_CHARGE_ALARM";
	case FUEL_GAUGE_LOW_VOLTAGE_ALARM:
		return "FUEL_GAUGE_LOW_VOLTAGE_ALARM";
	default:
		return "Unknown fuel gauge property";
	}
}

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(fuel_gauge0));
	int ret = 0;

	if (dev == NULL) {
		LOG_ERR("no device found.");
		return 0;
	}

	if (!device_is_ready(dev)) {
		LOG_ERR("Error: Device \"%s\" is not ready; check the driver initialization logs "
			"for errors.",
			dev->name);
		return 0;
	}

	LOG_INF("Found device \"%s\"", dev->name);

	{
		LOG_INF("Test-Read generic fuel gauge properties to verify which are supported");
		LOG_INF("Info: not all properties are supported by all fuel gauges!");

		fuel_gauge_prop_t test_props[] = {
			FUEL_GAUGE_AVG_CURRENT,
			FUEL_GAUGE_CURRENT,
			FUEL_GAUGE_CHARGE_CUTOFF,
			FUEL_GAUGE_CYCLE_COUNT,
			FUEL_GAUGE_CONNECT_STATE,
			FUEL_GAUGE_FLAGS,
			FUEL_GAUGE_FULL_CHARGE_CAPACITY,
			FUEL_GAUGE_PRESENT_STATE,
			FUEL_GAUGE_REMAINING_CAPACITY,
			FUEL_GAUGE_RUNTIME_TO_EMPTY,
			FUEL_GAUGE_RUNTIME_TO_FULL,
			FUEL_GAUGE_SBS_MFR_ACCESS,
			FUEL_GAUGE_ABSOLUTE_STATE_OF_CHARGE,
			FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE,
			FUEL_GAUGE_TEMPERATURE,
			FUEL_GAUGE_VOLTAGE,
			FUEL_GAUGE_SBS_MODE,
			FUEL_GAUGE_CHARGE_CURRENT,
			FUEL_GAUGE_CHARGE_VOLTAGE,
			FUEL_GAUGE_STATUS,
			FUEL_GAUGE_DESIGN_CAPACITY,
			FUEL_GAUGE_DESIGN_VOLTAGE,
			FUEL_GAUGE_SBS_ATRATE,
			FUEL_GAUGE_SBS_ATRATE_TIME_TO_FULL,
			FUEL_GAUGE_SBS_ATRATE_TIME_TO_EMPTY,
			FUEL_GAUGE_SBS_ATRATE_OK,
			FUEL_GAUGE_SBS_REMAINING_CAPACITY_ALARM,
			FUEL_GAUGE_SBS_REMAINING_TIME_ALARM,
			FUEL_GAUGE_MANUFACTURER_NAME,
			FUEL_GAUGE_DEVICE_NAME,
			FUEL_GAUGE_DEVICE_CHEMISTRY,
			FUEL_GAUGE_CURRENT_DIRECTION,
			FUEL_GAUGE_STATE_OF_CHARGE_ALARM,
			FUEL_GAUGE_LOW_VOLTAGE_ALARM,
		};

		union fuel_gauge_prop_val test_vals[ARRAY_SIZE(test_props)];

		for (size_t i = 0; i < ARRAY_SIZE(test_props); i++) {

			if (test_props[i] == FUEL_GAUGE_MANUFACTURER_NAME) {
				struct sbs_gauge_manufacturer_name mfg_name;

				ret = fuel_gauge_get_buffer_prop(dev, FUEL_GAUGE_MANUFACTURER_NAME,
								 &mfg_name, sizeof(mfg_name));
				if (ret == -ENOTSUP) {
					LOG_INF("Property \"%s\" is not supported",
						fuel_gauge_prop_to_str(test_props[i]));
				} else if (ret < 0) {
					LOG_ERR("Error: cannot get property \"%s\": %d",
						fuel_gauge_prop_to_str(test_props[i]), ret);
				} else {
					LOG_INF("Property \"%s\" is supported",
						fuel_gauge_prop_to_str(test_props[i]));
					LOG_INF("Manufacturer name: %s",
						mfg_name.manufacturer_name);
				}
			} else if (test_props[i] == FUEL_GAUGE_DEVICE_NAME) {
				struct sbs_gauge_device_name dev_name;

				ret = fuel_gauge_get_buffer_prop(dev, FUEL_GAUGE_DEVICE_NAME,
								 &dev_name, sizeof(dev_name));
				if (ret == -ENOTSUP) {
					LOG_INF("Property \"%s\" is not supported",
						fuel_gauge_prop_to_str(test_props[i]));
				} else if (ret < 0) {
					LOG_ERR("Error: cannot get property \"%s\": %d",
						fuel_gauge_prop_to_str(test_props[i]), ret);
				} else {
					LOG_INF("Property \"%s\" is supported",
						fuel_gauge_prop_to_str(test_props[i]));
					LOG_INF("Device name: %s", dev_name.device_name);
				}
			} else if (test_props[i] == FUEL_GAUGE_DEVICE_CHEMISTRY) {
				struct sbs_gauge_device_chemistry device_chemistry;

				ret = fuel_gauge_get_buffer_prop(dev, FUEL_GAUGE_DEVICE_CHEMISTRY,
								 &device_chemistry,
								 sizeof(device_chemistry));
				if (ret == -ENOTSUP) {
					LOG_INF("Property \"%s\" is not supported",
						fuel_gauge_prop_to_str(test_props[i]));
				} else if (ret < 0) {
					LOG_ERR("Error: cannot get property \"%s\": %d",
						fuel_gauge_prop_to_str(test_props[i]), ret);
				} else {
					LOG_INF("Property \"%s\" is supported",
						fuel_gauge_prop_to_str(test_props[i]));
					LOG_INF("Device chemistry: %s",
						device_chemistry.device_chemistry);
				}
			} else {
				/* For all other properties, use the generic get_props function */

				ret = fuel_gauge_get_props(dev, &test_props[i], &test_vals[i], 1);

				if (ret == -ENOTSUP) {
					LOG_INF("Property \"%s\" is not supported",
						fuel_gauge_prop_to_str(test_props[i]));
				} else if (ret < 0) {
					LOG_ERR("Error: cannot get property \"%s\": %d",
						fuel_gauge_prop_to_str(test_props[i]), ret);
				} else {
					LOG_INF("Property \"%s\" is supported",
						fuel_gauge_prop_to_str(test_props[i]));

					switch (test_props[i]) {
					case FUEL_GAUGE_AVG_CURRENT:
						LOG_INF("  Avg current: %d",
							test_vals[i].avg_current);
						break;
					case FUEL_GAUGE_CURRENT:
						LOG_INF("  Current: %d", test_vals[i].current);
						break;
					case FUEL_GAUGE_CYCLE_COUNT:
						LOG_INF("  Cycle count: %" PRIu32,
							test_vals[i].cycle_count);
						break;
					case FUEL_GAUGE_CONNECT_STATE:
						LOG_INF("  Connect state: 0x%" PRIx32,
							test_vals[i].connect_state);
						break;
					case FUEL_GAUGE_FLAGS:
						LOG_INF("  Flags: 0x%" PRIx32, test_vals[i].flags);
						break;
					case FUEL_GAUGE_FULL_CHARGE_CAPACITY:
						LOG_INF("  Full charge capacity: %" PRIu32,
							test_vals[i].full_charge_capacity);
						break;
					case FUEL_GAUGE_PRESENT_STATE:
						LOG_INF("  Present state: %d",
							test_vals[i].present_state);
						break;
					case FUEL_GAUGE_REMAINING_CAPACITY:
						LOG_INF("  Remaining capacity: %" PRIu32,
							test_vals[i].remaining_capacity);
						break;
					case FUEL_GAUGE_RUNTIME_TO_EMPTY:
						LOG_INF("  Runtime to empty: %" PRIu32,
							test_vals[i].runtime_to_empty);
						break;
					case FUEL_GAUGE_RUNTIME_TO_FULL:
						LOG_INF("  Runtime to full: %" PRIu32,
							test_vals[i].runtime_to_full);
						break;
					case FUEL_GAUGE_SBS_MFR_ACCESS:
						LOG_INF("  SBS MFR access: %" PRIu16,
							test_vals[i].sbs_mfr_access_word);
						break;
					case FUEL_GAUGE_ABSOLUTE_STATE_OF_CHARGE:
						LOG_INF("  Absolute state of charge: %" PRIu8,
							test_vals[i].absolute_state_of_charge);
						break;
					case FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE:
						LOG_INF("  Relative state of charge: %" PRIu8,
							test_vals[i].relative_state_of_charge);
						break;
					case FUEL_GAUGE_TEMPERATURE:
						LOG_INF("  Temperature: %" PRIu16,
							test_vals[i].temperature);
						break;
					case FUEL_GAUGE_VOLTAGE:
						LOG_INF("  Voltage: %d", test_vals[i].voltage);
						break;
					case FUEL_GAUGE_SBS_MODE:
						LOG_INF("  SBS mode: %" PRIu16,
							test_vals[i].sbs_mode);
						break;
					case FUEL_GAUGE_CHARGE_CURRENT:
						LOG_INF("  Charge current: %" PRIu32,
							test_vals[i].chg_current);
						break;
					case FUEL_GAUGE_CHARGE_VOLTAGE:
						LOG_INF("  Charge voltage: %" PRIu32,
							test_vals[i].chg_voltage);
						break;
					case FUEL_GAUGE_STATUS:
						LOG_INF("  Status: 0x%" PRIx16,
							test_vals[i].fg_status);
						break;
					case FUEL_GAUGE_DESIGN_CAPACITY:
						LOG_INF("  Design capacity: %" PRIu16,
							test_vals[i].design_cap);
						break;
					case FUEL_GAUGE_DESIGN_VOLTAGE:
						LOG_INF("  Design voltage: %" PRIx16,
							test_vals[i].design_volt);
						break;
					case FUEL_GAUGE_SBS_ATRATE:
						LOG_INF("  SBS at rate: %" PRIi16,
							test_vals[i].sbs_at_rate);
						break;
					case FUEL_GAUGE_SBS_ATRATE_TIME_TO_FULL:
						LOG_INF("  SBS at rate time to full: %" PRIu16,
							test_vals[i].sbs_at_rate_time_to_full);
						break;
					case FUEL_GAUGE_SBS_ATRATE_TIME_TO_EMPTY:
						LOG_INF("  SBS at rate time to empty: %" PRIu16,
							test_vals[i].sbs_at_rate_time_to_empty);
						break;
					case FUEL_GAUGE_SBS_ATRATE_OK:
						LOG_INF("  SBS at rate ok: %d",
							test_vals[i].sbs_at_rate_ok);
						break;
					case FUEL_GAUGE_SBS_REMAINING_CAPACITY_ALARM:
						LOG_INF("  SBS remaining capacity alarm: %" PRIu16,
							test_vals[i].sbs_remaining_capacity_alarm);
						break;
					case FUEL_GAUGE_SBS_REMAINING_TIME_ALARM:
						LOG_INF("  SBS remaining time alarm: %" PRIu16,
							test_vals[i].sbs_remaining_time_alarm);
						break;
					case FUEL_GAUGE_CURRENT_DIRECTION:
						LOG_INF("  Current direction: %" PRIu16,
							test_vals[i].current_direction);
						break;
					case FUEL_GAUGE_STATE_OF_CHARGE_ALARM:
						LOG_INF("  State of charge alarm: %" PRIu8,
							test_vals[i].state_of_charge_alarm);
						break;
					case FUEL_GAUGE_LOW_VOLTAGE_ALARM:
						LOG_INF("  Low voltage alarm: %" PRIu32,
							test_vals[i].low_voltage_alarm);
						break;
					}
				}
			}
		}
	}

	LOG_INF("Polling fuel gauge data 'FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE' & "
		"'FUEL_GAUGE_VOLTAGE'");

	while (1) {
		fuel_gauge_prop_t poll_props[] = {
			FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE,
			FUEL_GAUGE_VOLTAGE,
		};

		union fuel_gauge_prop_val poll_vals[ARRAY_SIZE(poll_props)];

		ret = fuel_gauge_get_props(dev, poll_props, poll_vals, ARRAY_SIZE(poll_props));

		if (ret < 0) {
			LOG_ERR("Error: cannot get properties");
		} else {
			LOG_INF("Fuel gauge data: Charge: %d%%, Voltage: %dmV",
				poll_vals[0].relative_state_of_charge, poll_vals[1].voltage / 1000);
		}

		k_sleep(K_MSEC(5000));
	}
	return 0;
}
