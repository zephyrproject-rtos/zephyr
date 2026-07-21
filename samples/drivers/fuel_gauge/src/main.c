/*
 * Copyright (c) 2023 Alvaro Garcia Gomez <maxpowel@gmail.com>
 * Copyright (c) 2025 Philipp Steiner <philipp.steiner1987@gmail.com>
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/fuel_gauge.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app);

/*
 * Keep in sync with enum fuel_gauge_prop_type in include/zephyr/drivers/fuel_gauge.h
 * from FUEL_GAUGE_AVG_CURRENT_UA through FUEL_GAUGE_THERM_VOLTAGE_UV (order matters).
 */
#define FUEL_GAUGE_PROP_X_LIST                                                                     \
	X(FUEL_GAUGE_AVG_CURRENT_UA)                                                               \
	X(FUEL_GAUGE_CURRENT_UA)                                                                   \
	X(FUEL_GAUGE_CHARGE_CUTOFF)                                                                \
	X(FUEL_GAUGE_CYCLE_COUNT)                                                                  \
	X(FUEL_GAUGE_CONNECT_STATE)                                                                \
	X(FUEL_GAUGE_FLAGS)                                                                        \
	X(FUEL_GAUGE_FULL_CHARGE_CAPACITY_UAH)                                                     \
	X(FUEL_GAUGE_PRESENT_STATE)                                                                \
	X(FUEL_GAUGE_REMAINING_CAPACITY_UAH)                                                       \
	X(FUEL_GAUGE_RUNTIME_TO_EMPTY_MINS)                                                        \
	X(FUEL_GAUGE_RUNTIME_TO_FULL_MINS)                                                         \
	X(FUEL_GAUGE_SBS_MFR_ACCESS)                                                               \
	X(FUEL_GAUGE_ABSOLUTE_STATE_OF_CHARGE_PCT)                                                 \
	X(FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE_PCT)                                                 \
	X(FUEL_GAUGE_TEMPERATURE_DK)                                                               \
	X(FUEL_GAUGE_VOLTAGE_UV)                                                                   \
	X(FUEL_GAUGE_SBS_MODE)                                                                     \
	X(FUEL_GAUGE_CHARGE_CURRENT_UA)                                                            \
	X(FUEL_GAUGE_CHARGE_VOLTAGE_UV)                                                            \
	X(FUEL_GAUGE_STATUS)                                                                       \
	X(FUEL_GAUGE_DESIGN_CAPACITY)                                                              \
	X(FUEL_GAUGE_DESIGN_VOLTAGE_MV)                                                            \
	X(FUEL_GAUGE_SBS_ATRATE)                                                                   \
	X(FUEL_GAUGE_SBS_ATRATE_TIME_TO_FULL_MINS)                                                 \
	X(FUEL_GAUGE_SBS_ATRATE_TIME_TO_EMPTY_MINS)                                                \
	X(FUEL_GAUGE_SBS_ATRATE_OK)                                                                \
	X(FUEL_GAUGE_SBS_REMAINING_CAPACITY_ALARM)                                                 \
	X(FUEL_GAUGE_SBS_REMAINING_TIME_ALARM_MINS)                                                \
	X(FUEL_GAUGE_MANUFACTURER_NAME)                                                            \
	X(FUEL_GAUGE_DEVICE_NAME)                                                                  \
	X(FUEL_GAUGE_DEVICE_CHEMISTRY)                                                             \
	X(FUEL_GAUGE_CURRENT_DIRECTION)                                                            \
	X(FUEL_GAUGE_STATE_OF_CHARGE_ALARM_PCT)                                                    \
	X(FUEL_GAUGE_LOW_VOLTAGE_ALARM_UV)                                                         \
	X(FUEL_GAUGE_HIGH_VOLTAGE_ALARM_UV)                                                        \
	X(FUEL_GAUGE_LOW_CURRENT_ALARM_UA)                                                         \
	X(FUEL_GAUGE_HIGH_CURRENT_ALARM_UA)                                                        \
	X(FUEL_GAUGE_LOW_TEMPERATURE_ALARM_DK)                                                     \
	X(FUEL_GAUGE_HIGH_TEMPERATURE_ALARM_DK)                                                    \
	X(FUEL_GAUGE_LOW_GPIO_ALARM_UV)                                                            \
	X(FUEL_GAUGE_HIGH_GPIO_ALARM_UV)                                                           \
	X(FUEL_GAUGE_GPIO_VOLTAGE_UV)                                                              \
	X(FUEL_GAUGE_ADC_MODE)                                                                     \
	X(FUEL_GAUGE_CC_CONFIG)                                                                    \
	X(FUEL_GAUGE_STATE_OF_HEALTH)                                                              \
	X(FUEL_GAUGE_THERM_VOLTAGE_UV)

BUILD_ASSERT(FUEL_GAUGE_THERM_VOLTAGE_UV + 1 == FUEL_GAUGE_COMMON_COUNT);

static const char *const fuel_gauge_prop_strs[FUEL_GAUGE_COMMON_COUNT] = {
#define X(name) [name] = #name,
	FUEL_GAUGE_PROP_X_LIST
#undef X
};

const char *fuel_gauge_prop_to_str(enum fuel_gauge_prop_type prop)
{
	if (prop < FUEL_GAUGE_COMMON_COUNT && fuel_gauge_prop_strs[prop] != NULL) {
		return fuel_gauge_prop_strs[prop];
	}

	return "Unknown fuel gauge property";
}

static void decode_and_log_sbs_mode(uint16_t sbs_mode)
{
	if (sbs_mode & BIT(0)) {
		LOG_INF("    Internal charge controller supported");
	}
	if (sbs_mode & BIT(1)) {
		LOG_INF("    Primary or secondary battery support");
	}
	LOG_INF("    Conditioning: %s", (sbs_mode & BIT(7)) ? "cycle requested" : "Battery OK");
	if (sbs_mode & BIT(8)) {
		LOG_INF("    Internal charge control enabled");
	}
	if (sbs_mode & BIT(1)) {
		LOG_INF("    Battery operating in its %s role",
			(sbs_mode & BIT(9)) ? "primary" : "secondary");
	}
	if (sbs_mode & BIT(13)) {
		LOG_INF("    Alarm warning broadcast disabled");
	}
	if (sbs_mode & BIT(14)) {
		LOG_INF("    Charging voltage and current broadcast disabled");
	}
	LOG_INF("    Capacity reporting in %s",
		(sbs_mode & BIT(15)) ? "10 mW and 10 mWh" : "mA and mAh");
}

static void decode_and_log_fg_status(uint16_t fg_status)
{
	if (fg_status & BIT(4)) {
		LOG_INF("    Fully discharged");
	}
	if (fg_status & BIT(5)) {
		LOG_INF("    Fully charged");
	}
	if (fg_status & BIT(6)) {
		LOG_INF("    Discharging");
	}
	if (fg_status & BIT(7)) {
		LOG_INF("    Initialized");
	}
	if (fg_status & BIT(8)) {
		LOG_INF("    Remaining time alarm");
	}
	if (fg_status & BIT(9)) {
		LOG_INF("    Remaining capacity alarm");
	}
	if (fg_status & BIT(11)) {
		LOG_INF("    Terminate discharge alarm");
	}
	if (fg_status & BIT(12)) {
		LOG_INF("    Over temp alarm");
	}
	if (fg_status & BIT(14)) {
		LOG_INF("    Terminate charge alarm");
	}
	if (fg_status & BIT(15)) {
		LOG_INF("    Over charged alarm");
	}
}

static void decode_and_log_property(fuel_gauge_prop_t prop, union fuel_gauge_prop_val value)
{
	switch (prop) {
	case FUEL_GAUGE_AVG_CURRENT_UA:
		LOG_INF("  1 min. avg. current: %" PRId32 " μA; neg. = discharging",
			value.avg_current_ua);
		break;
	case FUEL_GAUGE_CURRENT_UA:
		LOG_INF("  Current: %" PRId32 " μA; neg. = discharging", value.current_ua);
		break;
	case FUEL_GAUGE_CHARGE_CUTOFF:
		LOG_INF("  Battery cut off: %s", value.cutoff ? "yes" : "no");
		break;
	case FUEL_GAUGE_CYCLE_COUNT:
		LOG_INF("  Cycle count: %" PRIu32 " charge/discharge cycles", value.cycle_count);
		break;
	case FUEL_GAUGE_CONNECT_STATE:
		LOG_INF("  Connect state: 0x%" PRIX32, value.connect_state);
		break;
	case FUEL_GAUGE_FLAGS:
		LOG_INF("  Flags: 0x%" PRIX32, value.flags);
		break;
	case FUEL_GAUGE_FULL_CHARGE_CAPACITY_UAH:
		LOG_INF("  Full charge capacity: %" PRIu32 " μAh", value.full_charge_capacity_uah);
		break;
	case FUEL_GAUGE_PRESENT_STATE:
		LOG_INF("  Present state: %s", value.present_state ? "yes" : "no");
		break;
	case FUEL_GAUGE_REMAINING_CAPACITY_UAH:
		LOG_INF("  Remaining capacity: %" PRIu32 " μAh", value.remaining_capacity_uah);
		break;
	case FUEL_GAUGE_RUNTIME_TO_EMPTY_MINS:
		LOG_INF("  Runtime to empty: %" PRIu32 " minutes", value.runtime_to_empty_mins);
		break;
	case FUEL_GAUGE_RUNTIME_TO_FULL_MINS:
		LOG_INF("  Runtime to full: %" PRIu32 " minutes", value.runtime_to_full_mins);
		break;
	case FUEL_GAUGE_SBS_MFR_ACCESS:
		LOG_INF("  SBS MFR access: 0x%" PRIX16, value.sbs_mfr_access_word);
		break;
	case FUEL_GAUGE_ABSOLUTE_STATE_OF_CHARGE_PCT:
		LOG_INF("  Absolute state of charge: %" PRIu8 " %%",
			value.absolute_state_of_charge_pct);
		break;
	case FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE_PCT:
		LOG_INF("  Relative state of charge: %" PRIu8 " %%",
			value.relative_state_of_charge_pct);
		break;
	case FUEL_GAUGE_TEMPERATURE_DK:
		LOG_INF("  Temperature: %" PRIu16 " * 0.1 K", value.temperature_dk);
		break;
	case FUEL_GAUGE_VOLTAGE_UV:
		LOG_INF("  Voltage: %d μV", value.voltage_uv);
		break;
	case FUEL_GAUGE_SBS_MODE:
		LOG_INF("  SBS mode: 0x%" PRIX16, value.sbs_mode);
		decode_and_log_sbs_mode(value.sbs_mode);
		break;
	case FUEL_GAUGE_CHARGE_CURRENT_UA:
		LOG_INF("  Desired max. charging current: %" PRIu32 " μA", value.chg_current_ua);
		break;
	case FUEL_GAUGE_CHARGE_VOLTAGE_UV:
		LOG_INF("  Desired max. charging voltage: %" PRIu32 " μV", value.chg_voltage_uv);
		break;
	case FUEL_GAUGE_STATUS:
		LOG_INF("  Status: 0x%" PRIX16, value.fg_status);
		decode_and_log_fg_status(value.fg_status);
		break;
	case FUEL_GAUGE_DESIGN_CAPACITY:
		LOG_INF("  Design capacity: %" PRIu16 " mAh or cWh", value.design_cap);
		break;
	case FUEL_GAUGE_DESIGN_VOLTAGE_MV:
		LOG_INF("  Design voltage: %" PRIu16 " mV", value.design_volt_mv);
		break;
	case FUEL_GAUGE_SBS_ATRATE:
		LOG_INF("  SBS at rate: %" PRIi16 " mA or cW", value.sbs_at_rate);
		break;
	case FUEL_GAUGE_SBS_ATRATE_TIME_TO_FULL_MINS:
		LOG_INF("  SBS at rate time to full: %" PRIu16 " minutes",
			value.sbs_at_rate_time_to_full_mins);
		break;
	case FUEL_GAUGE_SBS_ATRATE_TIME_TO_EMPTY_MINS:
		LOG_INF("  SBS at rate time to empty: %" PRIu16 " minutes",
			value.sbs_at_rate_time_to_empty_mins);
		break;
	case FUEL_GAUGE_SBS_ATRATE_OK:
		LOG_INF("  SBS at rate ok: %s", value.sbs_at_rate_ok ? "yes" : "no");
		break;
	case FUEL_GAUGE_SBS_REMAINING_CAPACITY_ALARM:
		LOG_INF("  SBS remaining capacity alarm: %" PRIu16 " mAh or cWh",
			value.sbs_remaining_capacity_alarm);
		break;
	case FUEL_GAUGE_SBS_REMAINING_TIME_ALARM_MINS:
		LOG_INF("  SBS Remaining time alarm: %" PRIu16 " minutes",
			value.sbs_remaining_time_alarm_mins);
		break;
	case FUEL_GAUGE_CURRENT_DIRECTION:
		LOG_INF("  Current direction: 0x%" PRIX16, value.current_direction);
		break;
	case FUEL_GAUGE_STATE_OF_CHARGE_ALARM_PCT:
		LOG_INF("  State of charge alarm: %" PRIu8 " %%", value.state_of_charge_alarm_pct);
		break;
	case FUEL_GAUGE_LOW_VOLTAGE_ALARM_UV:
		LOG_INF("  Low voltage alarm: %" PRIu32 " μV", value.low_voltage_alarm_uv);
		break;
	case FUEL_GAUGE_HIGH_VOLTAGE_ALARM_UV:
		LOG_INF("  High voltage alarm: %" PRIu32 " μV", value.high_voltage_alarm_uv);
		break;
	case FUEL_GAUGE_LOW_CURRENT_ALARM_UA:
		LOG_INF("  Low current alarm: %" PRIi32 " μA", value.low_current_alarm_ua);
		break;
	case FUEL_GAUGE_HIGH_CURRENT_ALARM_UA:
		LOG_INF("  High current alarm: %" PRIi32 " μA", value.high_current_alarm_ua);
		break;
	case FUEL_GAUGE_LOW_TEMPERATURE_ALARM_DK:
		LOG_INF("  Low temperature alarm: %" PRIu16 " * 0.1 K",
			value.low_temperature_alarm_dk);
		break;
	case FUEL_GAUGE_HIGH_TEMPERATURE_ALARM_DK:
		LOG_INF("  High temperature alarm: %" PRIu16 " * 0.1 K",
			value.high_temperature_alarm_dk);
		break;
	case FUEL_GAUGE_LOW_GPIO_ALARM_UV:
		LOG_INF("  Low GPIO Voltage alarm: %" PRIi32 " μV", value.low_gpio_alarm_uv);
		break;
	case FUEL_GAUGE_HIGH_GPIO_ALARM_UV:
		LOG_INF("  High GPIO Voltage alarm: %" PRIi32 " μV", value.high_gpio_alarm_uv);
		break;
	case FUEL_GAUGE_GPIO_VOLTAGE_UV:
		LOG_INF("  GPIO Voltage: %" PRIi32 " μV", value.gpio_voltage_uv);
		break;
	case FUEL_GAUGE_ADC_MODE:
		LOG_INF("  ADC Mode: 0x%" PRIX8, value.adc_mode);
		break;
	case FUEL_GAUGE_CC_CONFIG:
		LOG_INF("  Coulomb Counter Config: 0x%" PRIX8, value.cc_config);
		break;
	case FUEL_GAUGE_STATE_OF_HEALTH:
		LOG_INF("  State of Health (SoH): %" PRIu8 " %%", value.state_of_health);
		break;
	case FUEL_GAUGE_THERM_VOLTAGE_UV:
		LOG_INF("  Thermistor voltage: %" PRIu32 " μV", value.therm_voltage_uv);
		break;
	default:
		LOG_ERR("Unknown fuel gauge property");
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
		LOG_ERR_DEVICE_NOT_READY(dev);
		return 0;
	}

	LOG_INF("Found device \"%s\"", dev->name);

	{
		LOG_INF("Test-Read generic fuel gauge properties to verify which are supported");
		LOG_INF("Info: not all properties are supported by all fuel gauges!");

		fuel_gauge_prop_t test_props[] = {
			FUEL_GAUGE_AVG_CURRENT_UA,
			FUEL_GAUGE_CURRENT_UA,
			FUEL_GAUGE_CHARGE_CUTOFF,
			FUEL_GAUGE_CYCLE_COUNT,
			FUEL_GAUGE_CONNECT_STATE,
			FUEL_GAUGE_FLAGS,
			FUEL_GAUGE_FULL_CHARGE_CAPACITY_UAH,
			FUEL_GAUGE_PRESENT_STATE,
			FUEL_GAUGE_REMAINING_CAPACITY_UAH,
			FUEL_GAUGE_RUNTIME_TO_EMPTY_MINS,
			FUEL_GAUGE_RUNTIME_TO_FULL_MINS,
			FUEL_GAUGE_SBS_MFR_ACCESS,
			FUEL_GAUGE_ABSOLUTE_STATE_OF_CHARGE_PCT,
			FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE_PCT,
			FUEL_GAUGE_TEMPERATURE_DK,
			FUEL_GAUGE_VOLTAGE_UV,
			FUEL_GAUGE_SBS_MODE,
			FUEL_GAUGE_CHARGE_CURRENT_UA,
			FUEL_GAUGE_CHARGE_VOLTAGE_UV,
			FUEL_GAUGE_STATUS,
			FUEL_GAUGE_DESIGN_CAPACITY,
			FUEL_GAUGE_DESIGN_VOLTAGE_MV,
			FUEL_GAUGE_SBS_ATRATE,
			FUEL_GAUGE_SBS_ATRATE_TIME_TO_FULL_MINS,
			FUEL_GAUGE_SBS_ATRATE_TIME_TO_EMPTY_MINS,
			FUEL_GAUGE_SBS_ATRATE_OK,
			FUEL_GAUGE_SBS_REMAINING_CAPACITY_ALARM,
			FUEL_GAUGE_SBS_REMAINING_TIME_ALARM_MINS,
			FUEL_GAUGE_MANUFACTURER_NAME,
			FUEL_GAUGE_DEVICE_NAME,
			FUEL_GAUGE_DEVICE_CHEMISTRY,
			FUEL_GAUGE_CURRENT_DIRECTION,
			FUEL_GAUGE_STATE_OF_CHARGE_ALARM_PCT,
			FUEL_GAUGE_LOW_VOLTAGE_ALARM_UV,
			FUEL_GAUGE_HIGH_VOLTAGE_ALARM_UV,
			FUEL_GAUGE_LOW_CURRENT_ALARM_UA,
			FUEL_GAUGE_HIGH_CURRENT_ALARM_UA,
			FUEL_GAUGE_LOW_TEMPERATURE_ALARM_DK,
			FUEL_GAUGE_HIGH_TEMPERATURE_ALARM_DK,
			FUEL_GAUGE_LOW_GPIO_ALARM_UV,
			FUEL_GAUGE_HIGH_GPIO_ALARM_UV,
			FUEL_GAUGE_GPIO_VOLTAGE_UV,
			FUEL_GAUGE_ADC_MODE,
			FUEL_GAUGE_CC_CONFIG,
			FUEL_GAUGE_STATE_OF_HEALTH,
			FUEL_GAUGE_THERM_VOLTAGE_UV,
		};

		BUILD_ASSERT(ARRAY_SIZE(test_props) == FUEL_GAUGE_COMMON_COUNT);

		union fuel_gauge_prop_val test_vals[ARRAY_SIZE(test_props)];

		for (size_t i = 0; i < ARRAY_SIZE(test_props); i++) {

			if (test_props[i] == FUEL_GAUGE_MANUFACTURER_NAME) {
				struct sbs_gauge_manufacturer_name mfg_name;

				ret = fuel_gauge_get_buffer_prop(dev, FUEL_GAUGE_MANUFACTURER_NAME,
								 &mfg_name, sizeof(mfg_name));
				if (ret == -ENOTSUP) {
					LOG_DBG("Property \"%s\" is not supported",
						fuel_gauge_prop_to_str(test_props[i]));
				} else if (ret < 0) {
					LOG_ERR("Error: cannot get property \"%s\": %d",
						fuel_gauge_prop_to_str(test_props[i]), ret);
				} else {
					LOG_DBG("Property \"%s\" is supported",
						fuel_gauge_prop_to_str(test_props[i]));
					LOG_INF("Manufacturer name: %.*s",
						mfg_name.manufacturer_name_length,
						mfg_name.manufacturer_name);
				}
			} else if (test_props[i] == FUEL_GAUGE_DEVICE_NAME) {
				struct sbs_gauge_device_name dev_name;

				ret = fuel_gauge_get_buffer_prop(dev, FUEL_GAUGE_DEVICE_NAME,
								 &dev_name, sizeof(dev_name));
				if (ret == -ENOTSUP) {
					LOG_DBG("Property \"%s\" is not supported",
						fuel_gauge_prop_to_str(test_props[i]));
				} else if (ret < 0) {
					LOG_ERR("Error: cannot get property \"%s\": %d",
						fuel_gauge_prop_to_str(test_props[i]), ret);
				} else {
					LOG_DBG("Property \"%s\" is supported",
						fuel_gauge_prop_to_str(test_props[i]));
					LOG_INF("Device name: %.*s", dev_name.device_name_length,
						dev_name.device_name);
				}
			} else if (test_props[i] == FUEL_GAUGE_DEVICE_CHEMISTRY) {
				struct sbs_gauge_device_chemistry device_chemistry;

				ret = fuel_gauge_get_buffer_prop(dev, FUEL_GAUGE_DEVICE_CHEMISTRY,
								 &device_chemistry,
								 sizeof(device_chemistry));
				if (ret == -ENOTSUP) {
					LOG_DBG("Property \"%s\" is not supported",
						fuel_gauge_prop_to_str(test_props[i]));
				} else if (ret < 0) {
					LOG_ERR("Error: cannot get property \"%s\": %d",
						fuel_gauge_prop_to_str(test_props[i]), ret);
				} else {
					LOG_DBG("Property \"%s\" is supported",
						fuel_gauge_prop_to_str(test_props[i]));
					LOG_INF("Device chemistry: %.*s",
						device_chemistry.device_chemistry_length,
						device_chemistry.device_chemistry);
				}
			} else {
				/* For all other properties, use the generic get_props function */

				ret = fuel_gauge_get_props(dev, &test_props[i], &test_vals[i], 1);

				if (ret == -ENOTSUP) {
					LOG_DBG("Property \"%s\" is not supported",
						fuel_gauge_prop_to_str(test_props[i]));
				} else if (ret < 0) {
					LOG_ERR("Error: cannot get property \"%s\": %d",
						fuel_gauge_prop_to_str(test_props[i]), ret);
				} else {
					LOG_DBG("Property \"%s\" is supported",
						fuel_gauge_prop_to_str(test_props[i]));

					decode_and_log_property(test_props[i], test_vals[i]);
				}
			}
		}
	}

	LOG_INF("Polling fuel gauge data 'FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE_PCT' & "
		"'FUEL_GAUGE_VOLTAGE_UV'");

	while (1) {
		fuel_gauge_prop_t poll_props[] = {
			FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE_PCT,
			FUEL_GAUGE_VOLTAGE_UV,
		};

		union fuel_gauge_prop_val poll_vals[ARRAY_SIZE(poll_props)];

		ret = fuel_gauge_get_props(dev, poll_props, poll_vals, ARRAY_SIZE(poll_props));

		if (ret < 0) {
			LOG_ERR("Error: cannot get properties");
		} else {
			LOG_INF("Fuel gauge data: Charge: %" PRIu8 " %%, Voltage: %d mV",
				poll_vals[0].relative_state_of_charge_pct,
				poll_vals[1].voltage_uv / 1000);
		}

		k_sleep(K_MSEC(5000));
	}
	return 0;
}
