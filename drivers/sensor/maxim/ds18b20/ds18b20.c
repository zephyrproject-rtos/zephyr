/*
 * Copyright (c) 2022 Thomas Stranger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Driver for DS18B20 1-Wire temperature sensors
 * A datasheet is available at:
 * https://datasheets.maximintegrated.com/en/ds/DS18B20.pdf
 *
 * Parasite power configuration is not supported by the driver.
 */
#define DT_DRV_COMPAT maxim_ds18b20

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/w1_sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>

#include "ds18b20.h"

LOG_MODULE_REGISTER(DS18B20, CONFIG_SENSOR_LOG_LEVEL);

static int ds18b20_configure(const struct device *dev);

/* measure wait time for 9-bit, 10-bit, 11-bit, 12-bit resolution respectively */
static const uint16_t measure_wait_ms[4] = { 94, 188, 376, 750 };

static inline void ds18b20_temperature_from_raw(uint8_t *temp_raw,
						struct sensor_value *val)
{
	int16_t temp = sys_get_le16(temp_raw);

	val->val1 = temp / 16;
	val->val2 = (temp % 16) * 1000000 / 16;
}

/*
 * Write scratch pad, read back, then copy to eeprom
 */
static int ds18b20_write_scratchpad(const struct device *dev,
				    struct ds18b20_scratchpad scratchpad)
{
	struct ds18b20_data *data = dev->data;
	const struct device *bus = ds18b20_bus(dev);
	uint8_t sp_data[4] = {
		DS18B20_CMD_WRITE_SCRATCHPAD,
		scratchpad.alarm_temp_high,
		scratchpad.alarm_temp_low,
		scratchpad.config
	};

	return w1_write_read(bus, &data->config, sp_data, sizeof(sp_data), NULL, 0);
}

static int ds18b20_read_scratchpad(const struct device *dev,
				   struct ds18b20_scratchpad *scratchpad)
{
	struct ds18b20_data *data = dev->data;
	const struct device *bus = ds18b20_bus(dev);
	uint8_t cmd = DS18B20_CMD_READ_SCRATCHPAD;

	return w1_write_read(bus, &data->config, &cmd, 1,
			     (uint8_t *)&scratchpad[0], 9);
}

/* Starts sensor temperature conversion without waiting for completion. */
static int ds18b20_temperature_convert(const struct device *dev)
{
	int ret;
	struct ds18b20_data *data = dev->data;
	const struct device *bus = ds18b20_bus(dev);

	(void)w1_lock_bus(bus);
	ret = w1_reset_select(bus, &data->config);
	if (ret != 0) {
		goto out;
	}
	ret = w1_write_byte(bus, DS18B20_CMD_CONVERT_T);
out:
	(void)w1_unlock_bus(bus);
	return ret;
}

/*
 * Write resolution into configuration struct,
 * but don't write it to the sensor yet.
 */
static void ds18b20_set_resolution(const struct device *dev, uint8_t resolution)
{
	struct ds18b20_data *data = dev->data;

	data->scratchpad.config &= ~DS18B20_RESOLUTION_MASK;
	data->scratchpad.config |= DS18B20_RESOLUTION(resolution);
}

static int ds18b20_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	const struct ds18b20_config *cfg = dev->config;
	struct ds18b20_data *data = dev->data;
	int status;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL ||
			chan == SENSOR_CHAN_AMBIENT_TEMP);

	if (!data->lazy_loaded) {
		status = ds18b20_configure(dev);
		if (status < 0) {
			return status;
		}
		data->lazy_loaded = true;
	}

	status = ds18b20_temperature_convert(dev);
	if (status < 0) {
		LOG_DBG("W1 fetch error");
		return status;
	}
	k_msleep(measure_wait_ms[DS18B20_RESOLUTION_INDEX(cfg->resolution)]);
	return ds18b20_read_scratchpad(dev, &data->scratchpad);
}

static int ds18b20_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct ds18b20_data *data = dev->data;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	ds18b20_temperature_from_raw((uint8_t *)&data->scratchpad.temp, val);
	return 0;
}

static int ds18b20_configure(const struct device *dev)
{
	const struct ds18b20_config *cfg = dev->config;
	struct ds18b20_data *data = dev->data;
	int ret;

	if (w1_reset_bus(cfg->bus) <= 0) {
		LOG_ERR("No 1-Wire slaves connected");
		return -ENODEV;
	}

	/* In single drop configurations the rom can be read from device */
	if (w1_get_slave_count(cfg->bus) == 1) {
		if (w1_rom_to_uint64(&data->config.rom) == 0ULL) {
			(void)w1_read_rom(cfg->bus, &data->config.rom);
		}
	} else if (w1_rom_to_uint64(&data->config.rom) == 0ULL) {
		LOG_DBG("nr: %d", w1_get_slave_count(cfg->bus));
		LOG_ERR("ROM required, because multiple slaves are on the bus");
		return -EINVAL;
	}

	if ((cfg->family != 0) && (cfg->family != data->config.rom.family)) {
		LOG_ERR("Found 1-Wire slave is not a DS18B20");
		return -EINVAL;
	}

	/* write default configuration */
	ds18b20_set_resolution(dev, cfg->resolution);
	ret = ds18b20_write_scratchpad(dev, data->scratchpad);
	if (ret < 0) {
		return ret;
	}
	LOG_DBG("Init DS18B20: ROM=%016llx\n",
		w1_rom_to_uint64(&data->config.rom));

	return 0;
}

int ds18b20_attr_set(const struct device *dev, enum sensor_channel chan,
		     enum sensor_attribute attr, const struct sensor_value *thr)
{
	struct ds18b20_data *data = dev->data;

	if ((enum sensor_attribute_w1)attr != SENSOR_ATTR_W1_ROM) {
		return -ENOTSUP;
	}

	data->lazy_loaded = false;
	w1_sensor_value_to_rom(thr, &data->config.rom);
	return 0;
}

static const struct sensor_driver_api ds18b20_driver_api = {
	.attr_set = ds18b20_attr_set,
	.sample_fetch = ds18b20_sample_fetch,
	.channel_get = ds18b20_channel_get,
};

static int ds18b20_init(const struct device *dev)
{
	const struct ds18b20_config *cfg = dev->config;
	struct ds18b20_data *data = dev->data;

	if (device_is_ready(cfg->bus) == 0) {
		LOG_DBG("w1 bus is not ready");
		return -ENODEV;
	}

	w1_uint64_to_rom(0ULL, &data->config.rom);
	data->lazy_loaded = false;
	/* in multidrop configurations the rom is need, but is not set during
	 * driver initialization, therefore do lazy initialization in all cases.
	 */

	return 0;
}

#define DS18B20_CONFIG_INIT(inst)					       \
	{								       \
		.bus = DEVICE_DT_GET(DT_INST_BUS(inst)),		       \
		.family = (uint8_t)DT_INST_PROP_OR(inst, family_code, 0x28),   \
		.resolution = DT_INST_PROP(inst, resolution),		       \
	}

#define DS18B20_DEFINE(inst)						\
	static struct ds18b20_data ds18b20_data_##inst;			\
	static const struct ds18b20_config ds18b20_config_##inst =	\
		DS18B20_CONFIG_INIT(inst);				\
	SENSOR_DEVICE_DT_INST_DEFINE(inst,				\
			      ds18b20_init,				\
			      NULL,					\
			      &ds18b20_data_##inst,			\
			      &ds18b20_config_##inst,			\
			      POST_KERNEL,				\
			      CONFIG_SENSOR_INIT_PRIORITY,		\
			      &ds18b20_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DS18B20_DEFINE)
