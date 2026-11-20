/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_pmbus

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/smbus.h>
#include <zephyr/logging/log.h>
#include <zephyr/pmbus/pmbus.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(pmbus_sensor, CONFIG_SENSOR_LOG_LEVEL);

enum pmbus_sensor_idx {
	PMBUS_SENSOR_READ_VOUT,
	PMBUS_SENSOR_READ_IOUT,
	PMBUS_SENSOR_READ_POUT,
	PMBUS_SENSOR_READ_TEMPERATURE_1,
	PMBUS_SENSOR_COUNT,
};

struct pmbus_sensor_chan {
	enum sensor_channel chan;
	uint8_t cmd;
};

static const struct pmbus_sensor_chan pmbus_sensor_chans[PMBUS_SENSOR_COUNT] = {
	[PMBUS_SENSOR_READ_VOUT] = {SENSOR_CHAN_VOLTAGE, PMBUS_READ_VOUT},
	[PMBUS_SENSOR_READ_IOUT] = {SENSOR_CHAN_CURRENT, PMBUS_READ_IOUT},
	[PMBUS_SENSOR_READ_POUT] = {SENSOR_CHAN_POWER, PMBUS_READ_POUT},
	[PMBUS_SENSOR_READ_TEMPERATURE_1] = {SENSOR_CHAN_AMBIENT_TEMP, PMBUS_READ_TEMPERATURE_1},
};

struct pmbus_sensor_config {
	struct smbus_dt_spec bus;
};

struct pmbus_sensor_data {
	struct pmbus_dev pmbus;
	uint16_t supported;
	int32_t values[PMBUS_SENSOR_COUNT];
};

static int pmbus_sensor_index(const struct device *dev, enum sensor_channel chan)
{
	struct pmbus_sensor_data *data = dev->data;

	for (int i = 0; i < PMBUS_SENSOR_COUNT; i++) {
		if (pmbus_sensor_chans[i].chan == chan) {
			return ((data->supported & BIT(i)) != 0U) ? i : -ENOTSUP;
		}
	}

	return -ENOTSUP;
}

static int pmbus_sensor_read(const struct device *dev, int i)
{
	struct pmbus_sensor_data *data = dev->data;

	return pmbus_read_value(&data->pmbus, 0U, pmbus_sensor_chans[i].cmd, &data->values[i]);
}

static int pmbus_sensor_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct pmbus_sensor_data *data = dev->data;
	int ret;

	if (chan != SENSOR_CHAN_ALL) {
		ret = pmbus_sensor_index(dev, chan);
		if (ret < 0) {
			return ret;
		}

		return pmbus_sensor_read(dev, ret);
	}

	for (int i = 0; i < PMBUS_SENSOR_COUNT; i++) {
		if ((data->supported & BIT(i)) == 0U) {
			continue;
		}

		ret = pmbus_sensor_read(dev, i);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int pmbus_sensor_channel_get(const struct device *dev, enum sensor_channel chan,
				    struct sensor_value *val)
{
	struct pmbus_sensor_data *data = dev->data;
	int i = pmbus_sensor_index(dev, chan);

	if (i < 0) {
		return i;
	}

	return sensor_value_from_milli(val, data->values[i]);
}

static DEVICE_API(sensor, pmbus_sensor_api) = {
	.sample_fetch = pmbus_sensor_sample_fetch,
	.channel_get = pmbus_sensor_channel_get,
};

static int pmbus_sensor_init(const struct device *dev)
{
	const struct pmbus_sensor_config *config = dev->config;
	struct pmbus_sensor_data *data = dev->data;
	int16_t mode;
	int ret;

	ret = pmbus_init(&data->pmbus, &config->bus, 1U);
	if (ret < 0) {
		return ret;
	}

	data->supported = 0U;
	for (int i = 0; i < PMBUS_SENSOR_COUNT; i++) {
		if (pmbus_cmd_supported(&data->pmbus, 0U, pmbus_sensor_chans[i].cmd, 2U)) {
			data->supported |= BIT(i);
		}
	}

	mode = data->pmbus.vout_mode[0];
	if ((data->supported & BIT(PMBUS_SENSOR_READ_VOUT)) != 0U &&
	    (mode < 0 || FIELD_GET(PMBUS_VOUT_MODE_MODE_MASK, mode) != PMBUS_VOUT_MODE_ULINEAR16)) {
		LOG_WRN("%s: output voltage mode not supported", dev->name);
		data->supported &= ~BIT(PMBUS_SENSOR_READ_VOUT);
	}

	LOG_DBG("%s: supported 0x%04x", dev->name, data->supported);

	return 0;
}

#define PMBUS_SENSOR_DEFINE(n)                                                                     \
	static const struct pmbus_sensor_config pmbus_sensor_config_##n = {                        \
		.bus = SMBUS_DT_SPEC_INST_GET(n),                                                  \
	};                                                                                         \
	static struct pmbus_sensor_data pmbus_sensor_data_##n;                                     \
	SENSOR_DEVICE_DT_INST_DEFINE(n, pmbus_sensor_init, NULL, &pmbus_sensor_data_##n,           \
				     &pmbus_sensor_config_##n, POST_KERNEL,                        \
				     CONFIG_SENSOR_INIT_PRIORITY, &pmbus_sensor_api);

DT_INST_FOREACH_STATUS_OKAY(PMBUS_SENSOR_DEFINE)
