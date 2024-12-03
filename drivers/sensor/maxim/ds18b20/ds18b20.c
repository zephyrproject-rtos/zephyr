/*
 * Copyright (c) 2022 Thomas Stranger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Driver for DS18B20 and DS18S20 1-Wire temperature sensors
 * Datasheets for the compatible sensors are available at:
 * - https://www.analog.com/media/en/technical-documentation/data-sheets/ds18b20.pdf
 * - https://www.analog.com/media/en/technical-documentation/data-sheets/ds18s20.pdf
 *
 * Parasite power configuration is not supported by the driver.
 */

#include <stdbool.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/w1_sensor.h>
#include "zephyr/drivers/w1.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util_macro.h>

LOG_MODULE_REGISTER(DS18B20, CONFIG_SENSOR_LOG_LEVEL);

#define DS18B20_CMD_CONVERT_T         0x44
#define DS18B20_CMD_WRITE_SCRATCHPAD  0x4E
#define DS18B20_CMD_READ_SCRATCHPAD   0xBE
#define DS18B20_CMD_COPY_SCRATCHPAD   0x48
#define DS18B20_CMD_RECALL_EEPROM     0xB8
#define DS18B20_CMD_READ_POWER_SUPPLY 0xB4

/* resolution is set using bit 5 and 6 of configuration register
 * macro only valid for values 9 to 12
 */
#define DS18B20_RESOLUTION_POS		5
#define DS18B20_RESOLUTION_MASK		(BIT_MASK(2) << DS18B20_RESOLUTION_POS)
/* convert resolution in bits to scratchpad config format */
#define DS18B20_RESOLUTION(res)		((res - 9) << DS18B20_RESOLUTION_POS)
/* convert resolution in bits to array index (for resolution specific elements) */
#define DS18B20_RESOLUTION_INDEX(res)	(res - 9)

#define DS18B20_FAMILYCODE 0x28
#define DS18S20_FAMILYCODE 0x10

enum chip_type {type_ds18b20, type_ds18s20};

struct ds18b20_scratchpad {
	int16_t temp;
	uint8_t alarm_temp_high;
	uint8_t alarm_temp_low;
	uint8_t config;
	uint8_t res[3];
	uint8_t crc;
} __packed;

struct ds18b20_config {
	const struct device *bus;
	uint8_t family;
	uint8_t resolution;
	enum chip_type chip;
};

struct ds18b20_data {
	struct w1_slave_config config;
	struct ds18b20_scratchpad scratchpad;
	bool lazy_loaded;
};

static int ds18b20_configure(const struct device *dev);
static int ds18b20_read_scratchpad(const struct device *dev, struct ds18b20_scratchpad *scratchpad);

/* measure wait time for 9-bit, 10-bit, 11-bit, 12-bit resolution respectively */
static const uint16_t measure_wait_ds18b20_ms[4] = { 94, 188, 376, 750 };

/* ds18s20 always needs 750ms */
static const uint16_t measure_wait_ds18s20_ms = { 750 };

static inline void ds18b20_temperature_from_raw(const struct device *dev,
						uint8_t *temp_raw,
						struct sensor_value *val)
{
	const struct ds18b20_config *cfg = dev->config;
	int16_t temp = sys_get_le16 (temp_raw);

	if (cfg->chip == type_ds18s20) {
		val->val1 = temp / 2;
		val->val2 = (temp % 2) * 5000000;
	} else {
		val->val1 = temp / 16;
		val->val2 = (temp % 16) * 1000000 / 16;
	}
}

static inline bool slave_responded(uint8_t *rx_buf, size_t len)
{
	uint8_t cmp_byte = 0xff;

	for (int i = 0; i < len; i++) {
		cmp_byte &= rx_buf[i];
	}

	return (cmp_byte == 0xff) ? false : true;
}

/*
 * Write scratch pad, read back, then copy to eeprom
 */
static int ds18b20_write_scratchpad(const struct device *dev,
				    struct ds18b20_scratchpad scratchpad)
{
	struct ds18b20_data *data = dev->data;
	const struct ds18b20_config *cfg = dev->config;
	const struct device *bus = cfg->bus;
	int ret;
	uint8_t sp_data[4] = {
		DS18B20_CMD_WRITE_SCRATCHPAD,
		scratchpad.alarm_temp_high,
		scratchpad.alarm_temp_low,
		scratchpad.config
	};

	ret = w1_write_read(bus, &data->config, sp_data, sizeof(sp_data), NULL, 0);
	if (ret != 0) {
		return ret;
	}

	ret = ds18b20_read_scratchpad(dev, &scratchpad);
	if (ret != 0) {
		return ret;
	}

	if ((sp_data[3] & DS18B20_RESOLUTION_MASK) !=
	    (scratchpad.config & DS18B20_RESOLUTION_MASK)) {
		return -EIO;
	}

	return 0;
}

static int ds18b20_read_scratchpad(const struct device *dev,
				   struct ds18b20_scratchpad *scratchpad)
{
	struct ds18b20_data *data = dev->data;
	const struct ds18b20_config *cfg = dev->config;
	const struct device *bus = cfg->bus;
	int ret;
	uint8_t cmd = DS18B20_CMD_READ_SCRATCHPAD;
	uint8_t crc;

	memset(scratchpad, 0, sizeof(*scratchpad));
	ret = w1_write_read(bus, &data->config, &cmd, 1,
			     (uint8_t *)scratchpad, sizeof(*scratchpad));
	if (ret != 0) {
		return ret;
	}

	if (!slave_responded((uint8_t *)scratchpad, sizeof(*scratchpad))) {
		LOG_WRN("Slave not reachable");
		return -ENODEV;
	}

	crc = w1_crc8((uint8_t *)scratchpad, sizeof(*scratchpad) - 1);
	if (crc != scratchpad->crc) {
		LOG_WRN("CRC does not match");
		return -EIO;
	}

	return 0;
}

/* Starts sensor temperature conversion without waiting for completion. */
static int ds18b20_temperature_convert(const struct device *dev)
{
	int ret;
	struct ds18b20_data *data = dev->data;
	const struct ds18b20_config *cfg = dev->config;
	const struct device *bus = cfg->bus;

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

static uint16_t measure_wait_ms(const struct device *dev)
{
	const struct ds18b20_config *cfg = dev->config;

	if (cfg->chip == type_ds18s20) {
		return measure_wait_ds18s20_ms;
	}

	return measure_wait_ds18b20_ms[DS18B20_RESOLUTION_INDEX(cfg->resolution)];
}

static int ds18b20_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
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
	k_msleep(measure_wait_ms(dev));
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

	ds18b20_temperature_from_raw(dev, (uint8_t *)&data->scratchpad.temp, val);
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
		LOG_ERR("Found 1-Wire slave is not a %s", dev->name);
		return -EINVAL;
	}

	/* write default configuration */
	if (cfg->chip == type_ds18b20) {
		ds18b20_set_resolution(dev, cfg->resolution);
		ret = ds18b20_write_scratchpad(dev, data->scratchpad);
		if (ret < 0) {
			return ret;
		}
	}
	LOG_DBG("Init %s: ROM=%016llx\n", dev->name,
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

static DEVICE_API(sensor, ds18b20_driver_api) = {
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

#define DS18B20_CONFIG_INIT(inst, default_family_code, chip_type)    \
	{								\
		.bus = DEVICE_DT_GET(DT_INST_BUS(inst)),		       \
		.family = (uint8_t)DT_INST_PROP_OR(inst, family_code, default_family_code),   \
		.resolution = DT_INST_PROP_OR(inst, resolution, 12),		\
		.chip = chip_type,	\
	}

#define DS18B20_DEFINE(inst, name, family_code, chip_type)		\
	static struct ds18b20_data data_##name##_##inst;			\
	static const struct ds18b20_config config_##name##_##inst =	\
		DS18B20_CONFIG_INIT(inst, family_code, chip_type);	\
	SENSOR_DEVICE_DT_INST_DEFINE(inst,				\
			      ds18b20_init,				\
			      NULL,					\
			      &data_##name##_##inst,			\
			      &config_##name##_##inst,			\
			      POST_KERNEL,				\
			      CONFIG_SENSOR_INIT_PRIORITY,		\
			      &ds18b20_driver_api);

#define DT_DRV_COMPAT maxim_ds18b20
DT_INST_FOREACH_STATUS_OKAY_VARGS(DS18B20_DEFINE, DT_DRV_COMPAT,
				DS18B20_FAMILYCODE,
				type_ds18b20)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT maxim_ds18s20
DT_INST_FOREACH_STATUS_OKAY_VARGS(DS18B20_DEFINE, DT_DRV_COMPAT,
				DS18S20_FAMILYCODE,
				type_ds18s20)
#undef DT_DRV_COMPAT
