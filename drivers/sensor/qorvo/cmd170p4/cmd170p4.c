/*
 * Copyright (c) 2026 HawkEye 360
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT qorvo_cmd170p4

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(CMD170P4, CONFIG_SENSOR_LOG_LEVEL);

/**
 * Power detector compensation table entry
 * Maps detector voltage (mV) to RF output power (milli-dBm)
 */
struct cmd170p4_compensation {
	const int32_t millivolts;
	const int32_t power_mdbm;
};

/**
 * Compensation table for CMD170P4 detector
 * Generated from manufacturer's equation: dBm = 18.94512456 × V^0.2549963344
 * Note: Physical detector maxes at 5V (~29.6 dBm), extended range for extrapolation
 */
static const struct cmd170p4_compensation comp_table[] = {
	{    0,      0 },
	{   45,   8590 },
	{  164,  11950 },
	{  356,  14560 },
	{  662,  17050 },
	{ 1105,  19430 },
	{ 1698,  21680 },
	{ 2452,  23810 },
	{ 3374,  25830 },
	{ 4469,  27750 },
	{ 5741,  29580 },
};

struct cmd170p4_config {
	const struct adc_dt_spec adc_channel;
};

struct cmd170p4_data {
	int32_t raw;
};

/**
 * Find the two compensation table entries that bracket the given voltage
 */
static void cmd170p4_lookup_comp(int32_t millivolts, int *i_low, int *i_high)
{
	int high = ARRAY_SIZE(comp_table) - 1;
	int low = 0;

	/* Handle out of range conditions */
	if (millivolts <= comp_table[low].millivolts) {
		*i_low = *i_high = low;
		return;
	}

	if (millivolts >= comp_table[high].millivolts) {
		*i_low = *i_high = high;
		return;
	}

	/* Binary search for the bracketing interval */
	while (high - low > 1) {
		int mid = (low + high) / 2;

		if (millivolts < comp_table[mid].millivolts) {
			high = mid;
		} else {
			low = mid;
		}
	}

	*i_low = low;
	*i_high = high;
}

/**
 * Convert detector voltage to RF power using linear interpolation
 * @param millivolts detector voltage in mV
 * @return RF power in milli-dBm (mdBm)
 */
static int32_t cmd170p4_get_mdbm(int32_t millivolts)
{
	int32_t linx0, linx1, liny0, liny1;
	int low, high;

	cmd170p4_lookup_comp(millivolts, &low, &high);

	/* If at a table boundary, return exact value */
	if (low == high) {
		return comp_table[low].power_mdbm;
	}

	/* Linear interpolation: y = liny0 + (liny1 - liny0) * (x - linx0) / (linx1 - linx0) */
	linx0 = comp_table[low].millivolts;
	liny0 = comp_table[low].power_mdbm;
	linx1 = comp_table[high].millivolts;
	liny1 = comp_table[high].power_mdbm;

	return liny0 + ((liny1 - liny0) * (millivolts - linx0)) / (linx1 - linx0);
}

static int cmd170p4_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct cmd170p4_config *cfg = dev->config;
	struct cmd170p4_data *data = dev->data;
	int res;
	struct adc_sequence sequence = {
		.options = NULL,
		.buffer = &data->raw,
		.buffer_size = sizeof(data->raw),
		.calibrate = false,
	};

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_POWER_DBM) {
		return -ENOTSUP;
	}

	adc_sequence_init_dt(&cfg->adc_channel, &sequence);
	res = adc_read(cfg->adc_channel.dev, &sequence);
	if (res < 0) {
		LOG_ERR("ADC read failed: %d", res);
	}

	return res;
}

static int cmd170p4_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	const struct cmd170p4_config *cfg = dev->config;
	struct cmd170p4_data *data = dev->data;
	int32_t millivolts;
	int32_t mdbm;

	switch (chan) {
	case SENSOR_CHAN_POWER_DBM:
		millivolts = data->raw;
		adc_raw_to_millivolts_dt(&cfg->adc_channel, &millivolts);

		mdbm = cmd170p4_get_mdbm(millivolts);
		sensor_value_from_milli(val, mdbm);

		LOG_DBG("ADC=%d mV=%d dBm=%d.%03d",
			data->raw, millivolts, val->val1, val->val2 / 1000);
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

static DEVICE_API(sensor, cmd170p4_driver_api) = {
	.sample_fetch = cmd170p4_sample_fetch,
	.channel_get = cmd170p4_channel_get,
};

static int cmd170p4_init(const struct device *dev)
{
	const struct cmd170p4_config *cfg = dev->config;
	int err;

	if (!adc_is_ready_dt(&cfg->adc_channel)) {
		LOG_ERR("ADC controller device is not ready");
		return -ENODEV;
	}

	err = adc_channel_setup_dt(&cfg->adc_channel);
	if (err < 0) {
		LOG_ERR("Could not setup ADC channel: %d", err);
		return err;
	}

	LOG_DBG("CMD170P4 initialized");
	return 0;
}

#define CMD170P4_DEFINE(inst)                                              \
	static struct cmd170p4_data cmd170p4_data_##inst;                  \
                                                                           \
	static const struct cmd170p4_config cmd170p4_config_##inst = {     \
		.adc_channel = ADC_DT_SPEC_INST_GET(inst),                 \
	};                                                                 \
                                                                           \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, cmd170p4_init, NULL,            \
				     &cmd170p4_data_##inst,                \
				     &cmd170p4_config_##inst, POST_KERNEL, \
				     CONFIG_SENSOR_INIT_PRIORITY,          \
				     &cmd170p4_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CMD170P4_DEFINE)
