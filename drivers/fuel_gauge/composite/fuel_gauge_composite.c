/*
 * Copyright (c) 2024, Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_fuel_gauge_composite

#include <zephyr/device.h>
#include <zephyr/drivers/fuel_gauge.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/battery.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/kernel.h>

struct composite_config {
	const struct device *source_primary;
	const struct device *source_secondary;
	int32_t ocv_lookup_table[BATTERY_OCV_TABLE_LEN];
	uint32_t charge_capacity_microamp_hours;
	enum battery_chemistry chemistry;
	bool fg_channels;
};

struct composite_data {
	k_ticks_t next_reading;
};

static int composite_fetch(const struct device *dev)
{
	int rc;

	rc = pm_device_runtime_get(dev);
	if (rc < 0) {
		return rc;
	}
	rc = sensor_sample_fetch(dev);
	if (rc < 0) {
		return rc;
	}
	return pm_device_runtime_put(dev);
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

static int composite_get_prop(const struct device *dev, fuel_gauge_prop_t prop,
			      union fuel_gauge_prop_val *val)
{
	const struct composite_config *config = dev->config;
	struct composite_data *data = dev->data;
	k_ticks_t now = k_uptime_ticks();
	struct sensor_value sensor_val;
	enum sensor_channel sensor_chan;
	int64_t voltage;
	int rc = 0;

	/* Validate at build time that equivalent channel output fields still match */
	BUILD_ASSERT(sizeof(val->absolute_state_of_charge) ==
		     sizeof(val->relative_state_of_charge));
	BUILD_ASSERT(offsetof(union fuel_gauge_prop_val, absolute_state_of_charge) ==
		     offsetof(union fuel_gauge_prop_val, relative_state_of_charge));
	BUILD_ASSERT(sizeof(val->current) == sizeof(val->avg_current));
	BUILD_ASSERT(offsetof(union fuel_gauge_prop_val, current) ==
		     offsetof(union fuel_gauge_prop_val, avg_current));

	if (now >= data->next_reading) {
		/* Trigger a sample on the input devices */
		rc = composite_fetch(config->source_primary);
		if ((rc == 0) && config->source_secondary) {
			rc = composite_fetch(config->source_secondary);
		}
		if (rc != 0) {
			return rc;
		}
		/* Update timestamp for next reading */
		data->next_reading =
			now + k_ms_to_ticks_near64(CONFIG_FUEL_GAUGE_COMPOSITE_DATA_VALIDITY_MS);
	}

	switch (prop) {
	case FUEL_GAUGE_FULL_CHARGE_CAPACITY:
		rc = composite_channel_get(dev, SENSOR_CHAN_GAUGE_FULL_AVAIL_CAPACITY, &sensor_val);
		if (rc == -ENOTSUP) {
			if (config->charge_capacity_microamp_hours == 0) {
				return -ENOTSUP;
			}
			val->full_charge_capacity = config->charge_capacity_microamp_hours;
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
	case FUEL_GAUGE_VOLTAGE:
		sensor_chan = config->fg_channels ? SENSOR_CHAN_GAUGE_VOLTAGE : SENSOR_CHAN_VOLTAGE;
		rc = composite_channel_get(dev, sensor_chan, &sensor_val);
		val->voltage = sensor_value_to_micro(&sensor_val);
		break;
	case FUEL_GAUGE_ABSOLUTE_STATE_OF_CHARGE:
	case FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE:
		rc = composite_channel_get(dev, SENSOR_CHAN_GAUGE_STATE_OF_CHARGE, &sensor_val);
		if (rc == 0) {
			val->absolute_state_of_charge = sensor_val.val1;
		} else if (rc == -ENOTSUP) {
			if (config->ocv_lookup_table[0] == -1) {
				return -ENOTSUP;
			}
			/* Fetch the voltage from the sensor */
			sensor_chan = config->fg_channels ? SENSOR_CHAN_GAUGE_VOLTAGE
							  : SENSOR_CHAN_VOLTAGE;
			rc = composite_channel_get(dev, sensor_chan, &sensor_val);
			voltage = sensor_value_to_micro(&sensor_val);
			if (rc == 0) {
				/* Convert voltage to state of charge */
				val->relative_state_of_charge =
					battery_soc_lookup(config->ocv_lookup_table, voltage) /
					1000;
			}
		}
		break;
	case FUEL_GAUGE_CURRENT:
	case FUEL_GAUGE_AVG_CURRENT:
		sensor_chan =
			config->fg_channels ? SENSOR_CHAN_GAUGE_AVG_CURRENT : SENSOR_CHAN_CURRENT;
		rc = composite_channel_get(dev, sensor_chan, &sensor_val);
		val->current = sensor_value_to_micro(&sensor_val);
		break;
	case FUEL_GAUGE_TEMPERATURE:
		sensor_chan = config->fg_channels ? SENSOR_CHAN_GAUGE_TEMP : SENSOR_CHAN_DIE_TEMP;
		rc = composite_channel_get(dev, sensor_chan, &sensor_val);
		/* Output unit = 0.1K (10x increase + 273.0) */
		val->temperature = sensor_value_to_deci(&sensor_val) + 2730;
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

#define COMPOSITE_INIT(inst)                                                                       \
	static const struct composite_config composite_##inst##_config = {                         \
		.source_primary = DEVICE_DT_GET(DT_INST_PROP(inst, source_primary)),               \
		.source_secondary = DEVICE_DT_GET_OR_NULL(DT_INST_PROP(inst, source_secondary)),   \
		.ocv_lookup_table =                                                                \
			BATTERY_OCV_TABLE_DT_GET(DT_DRV_INST(inst), ocv_capacity_table_0),         \
		.charge_capacity_microamp_hours =                                                  \
			DT_INST_PROP_OR(inst, charge_full_design_microamp_hours, 0),               \
		.chemistry = BATTERY_CHEMISTRY_DT_GET(inst),                                       \
		.fg_channels = DT_INST_PROP(inst, fuel_gauge_channels),                            \
	};                                                                                         \
	static struct composite_data composite_##inst##_data;                                      \
	DEVICE_DT_INST_DEFINE(inst, fuel_gauge_composite_init, NULL, &composite_##inst##_data,     \
			      &composite_##inst##_config, POST_KERNEL,                             \
			      CONFIG_SENSOR_INIT_PRIORITY, &composite_api);

DT_INST_FOREACH_STATUS_OKAY(COMPOSITE_INIT)
