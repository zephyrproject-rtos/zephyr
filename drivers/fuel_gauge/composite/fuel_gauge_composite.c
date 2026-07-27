/*
 * Copyright (c) 2024, Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_fuel_gauge_composite

#include <zephyr/device.h>
#include <zephyr/drivers/charger.h>
#include <zephyr/drivers/fuel_gauge.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/battery.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

struct composite_config {
	const struct device *source_primary;
	const struct device *source_secondary;
	const struct device *charger;
	int32_t ocv_lookup_table[BATTERY_OCV_TABLE_LEN];
	uint32_t charge_capacity_microamp_hours;
	int32_t factory_internal_resistance_micro_ohms;
	enum battery_chemistry chemistry;
	bool fg_channels;
};

struct composite_data {
	k_ticks_t next_reading;
};

LOG_MODULE_REGISTER(composite_fg);

static int composite_fetch(const struct device *dev)
{
	int rc, rc2;

	rc = pm_device_runtime_get(dev);
	if (rc < 0) {
		return rc;
	}
	rc = sensor_sample_fetch(dev);
	/* Unconditionally release the PM constraint */
	rc2 = pm_device_runtime_put(dev);
	if (rc == 0) {
		rc = rc2;
	}
	return rc;
}

static int composite_channel_get(const struct device *dev, enum sensor_channel chan,
				 struct sensor_value *val)
{
	const struct composite_config *config = dev->config;
	int rc;

	rc = sensor_channel_get(config->source_primary, chan, val);
	if ((rc == -ENOTSUP) && config->source_secondary) {
		rc = sensor_channel_get(config->source_secondary, chan, val);
	}
	return rc;
}

static int composite_get_prop_state_of_charge(const struct device *dev,
					      union fuel_gauge_prop_val *val)
{
	const struct composite_config *config = dev->config;
	enum sensor_channel sensor_chan;
	struct sensor_value sensor_val;
	union charger_propval propval;
	int32_t voltage;
	uint8_t soc;
	int rc;

	/* Attempt to query the state of charge directly */
	rc = composite_channel_get(dev, SENSOR_CHAN_GAUGE_STATE_OF_CHARGE, &sensor_val);
	if (rc == 0) {
		val->absolute_state_of_charge_pct = sensor_val.val1;
		return 0;

	} else if (rc != -ENOTSUP) {
		/* Error other than an unsupported channel */
		return rc;
	}

	if (config->ocv_lookup_table[0] == -1) {
		/* No OCV table to support estimation */
		return -ENOTSUP;
	}

	/* Unsupported channel, compute from OCV table */
	sensor_chan = config->fg_channels ? SENSOR_CHAN_GAUGE_VOLTAGE : SENSOR_CHAN_VOLTAGE;
	rc = composite_channel_get(dev, sensor_chan, &sensor_val);
	if (rc != 0) {
		/* Failed to retrieve voltage, no way to compute SoC */
		return rc;
	}
	voltage = sensor_value_to_micro(&sensor_val);

	/* If an internal resistance is set, attempt to correct voltage based on current */
	if (config->factory_internal_resistance_micro_ohms > 0) {
		sensor_chan =
			config->fg_channels ? SENSOR_CHAN_GAUGE_AVG_CURRENT : SENSOR_CHAN_CURRENT;
		rc = composite_channel_get(dev, sensor_chan, &sensor_val);
		if (rc == 0) {
			/* Only apply correction is current can be measured, failure to measure
			 * current is not a failure to measure SoC.
			 */
			int32_t voltage_drop;
			int64_t current;

			current = sensor_value_to_micro(&sensor_val);
			/* Drop across the internal resistance.
			 * V,I,R in micro units, V=IR must be divided by 10^6.
			 */
			voltage_drop = (current * config->factory_internal_resistance_micro_ohms) /
				       1000000;
			LOG_DBG("Raw Voltage: %7d uV Current: %8lld uA Drop: %5d uV", voltage,
				current, voltage_drop);
			/* Adjust voltage according to drop */
			voltage -= voltage_drop;
		}
	}

	/* Convert voltage to state of charge */
	soc = battery_soc_lookup(config->ocv_lookup_table, voltage) / 1000;

	/* Limit according to charging state */
	if ((config->charger != NULL) && device_is_ready(config->charger) &&
	    (charger_get_prop(config->charger, CHARGER_PROP_STATUS, &propval) == 0)) {
		if ((soc >= 100) && (propval.status == CHARGER_STATUS_CHARGING)) {
			/* OCV says at least 100%, but charger reports still charging */
			LOG_DBG("Clamping SoC to 99%%");
			soc = 99;
		}
		if ((soc < 100) && (propval.status == CHARGER_STATUS_FULL)) {
			/* OCV reports less than 100%, but charger reports full */
			LOG_DBG("Forcing SoC to 100%%");
			soc = 100;
		}
	}

	val->relative_state_of_charge_pct = soc;
	return 0;
}

static int composite_get_prop(const struct device *dev, fuel_gauge_prop_t prop,
			      union fuel_gauge_prop_val *val)
{
	const struct composite_config *config = dev->config;
	struct composite_data *data = dev->data;
	const k_ticks_t validity_ticks =
		k_ms_to_ticks_near64(CONFIG_FUEL_GAUGE_COMPOSITE_DATA_VALIDITY_MS);
	enum sensor_channel sensor_chan;
	struct sensor_value sensor_val;
	int rc = 0;

	/* Validate at build time that equivalent channel output fields still match */
	BUILD_ASSERT(sizeof(val->absolute_state_of_charge_pct) ==
		     sizeof(val->relative_state_of_charge_pct));
	BUILD_ASSERT(offsetof(union fuel_gauge_prop_val, absolute_state_of_charge_pct) ==
		     offsetof(union fuel_gauge_prop_val, relative_state_of_charge_pct));
	BUILD_ASSERT(sizeof(val->current_ua) == sizeof(val->avg_current_ua));
	BUILD_ASSERT(offsetof(union fuel_gauge_prop_val, current_ua) ==
		     offsetof(union fuel_gauge_prop_val, avg_current_ua));

	if (k_uptime_ticks() >= data->next_reading) {
		/* Trigger a sample on the input devices */
		rc = composite_fetch(config->source_primary);
		if ((rc == 0) && config->source_secondary) {
			rc = composite_fetch(config->source_secondary);
		}
		if (rc != 0) {
			return rc;
		}
		/* Update timestamp for next reading */
		data->next_reading = k_uptime_ticks() + validity_ticks;
	}

	switch (prop) {
	case FUEL_GAUGE_FULL_CHARGE_CAPACITY_UAH:
		rc = composite_channel_get(dev, SENSOR_CHAN_GAUGE_FULL_AVAIL_CAPACITY, &sensor_val);
		if (rc == -ENOTSUP) {
			if (config->charge_capacity_microamp_hours == 0) {
				return -ENOTSUP;
			}
			val->full_charge_capacity_uah = config->charge_capacity_microamp_hours;
			rc = 0;
		}
		break;
	case FUEL_GAUGE_DESIGN_CAPACITY:
		rc = composite_channel_get(dev, SENSOR_CHAN_GAUGE_FULL_CHARGE_CAPACITY,
					   &sensor_val);
		if (rc == -ENOTSUP) {
			if (config->charge_capacity_microamp_hours == 0) {
				return -ENOTSUP;
			}
			val->design_cap = config->charge_capacity_microamp_hours / 1000;
			rc = 0;
		}
		break;
	case FUEL_GAUGE_VOLTAGE_UV:
		sensor_chan = config->fg_channels ? SENSOR_CHAN_GAUGE_VOLTAGE : SENSOR_CHAN_VOLTAGE;
		rc = composite_channel_get(dev, sensor_chan, &sensor_val);
		val->voltage_uv = sensor_value_to_micro(&sensor_val);
		break;
	case FUEL_GAUGE_ABSOLUTE_STATE_OF_CHARGE_PCT:
	case FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE_PCT:
		rc = composite_get_prop_state_of_charge(dev, val);
		break;
	case FUEL_GAUGE_CURRENT_UA:
	case FUEL_GAUGE_AVG_CURRENT_UA:
		sensor_chan =
			config->fg_channels ? SENSOR_CHAN_GAUGE_AVG_CURRENT : SENSOR_CHAN_CURRENT;
		rc = composite_channel_get(dev, sensor_chan, &sensor_val);
		val->current_ua = sensor_value_to_micro(&sensor_val);
		break;
	case FUEL_GAUGE_TEMPERATURE_DK:
		sensor_chan = config->fg_channels ? SENSOR_CHAN_GAUGE_TEMP : SENSOR_CHAN_DIE_TEMP;
		rc = composite_channel_get(dev, sensor_chan, &sensor_val);
		/* Output unit = 0.1K (10x increase + 273.0) */
		val->temperature_dk = sensor_value_to_deci(&sensor_val) + 2730;
		break;
	default:
		return -ENOTSUP;
	}

	return rc;
}

static int fuel_gauge_composite_init(const struct device *dev)
{
	const struct composite_config *config = dev->config;

	/* Validate sources are ready */
	if (!device_is_ready(config->source_primary)) {
		return -ENODEV;
	}
	if (config->source_secondary && !device_is_ready(config->source_secondary)) {
		return -ENODEV;
	}
	return 0;
}

static DEVICE_API(fuel_gauge, composite_api) = {
	.get_property = composite_get_prop,
};

#define BATTERY_RESISTANCE(inst) DT_INST_PROP_OR(inst, factory_internal_resistance_micro_ohms, 0)

#define COMPOSITE_INIT(inst)                                                                       \
	BUILD_ASSERT(BATTERY_RESISTANCE(inst) >= 0);                                               \
	static const struct composite_config composite_##inst##_config = {                         \
		.source_primary = DEVICE_DT_GET(DT_INST_PROP(inst, source_primary)),               \
		.source_secondary = DEVICE_DT_GET_OR_NULL(DT_INST_PROP(inst, source_secondary)),   \
		.charger = DEVICE_DT_GET_OR_NULL(DT_INST_PROP(inst, charger_status)),              \
		.ocv_lookup_table =                                                                \
			BATTERY_OCV_TABLE_DT_GET(DT_DRV_INST(inst), ocv_capacity_table_0),         \
		.charge_capacity_microamp_hours =                                                  \
			DT_INST_PROP_OR(inst, charge_full_design_microamp_hours, 0),               \
		.factory_internal_resistance_micro_ohms = BATTERY_RESISTANCE(inst),                \
		.chemistry = BATTERY_CHEMISTRY_DT_GET(inst),                                       \
		.fg_channels = DT_INST_PROP(inst, fuel_gauge_channels),                            \
	};                                                                                         \
	static struct composite_data composite_##inst##_data;                                      \
	DEVICE_DT_INST_DEFINE(inst, fuel_gauge_composite_init, NULL, &composite_##inst##_data,     \
			      &composite_##inst##_config, POST_KERNEL,                             \
			      CONFIG_SENSOR_INIT_PRIORITY, &composite_api);

DT_INST_FOREACH_STATUS_OKAY(COMPOSITE_INIT)
