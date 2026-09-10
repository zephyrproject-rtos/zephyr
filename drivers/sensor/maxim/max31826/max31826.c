/*
 * Copyright (c) 2022 Thomas Stranger
 * Copyright (c) 2026 Foss Analytical A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Driver for the MAX31826 1-Wire temperature sensor.
 * Datasheet:
 * - https://www.analog.com/media/en/technical-documentation/data-sheets/MAX31826.pdf
 *
 * The MAX31826 provides a fixed 12-bit temperature measurement
 * (0.0625 degC/LSB) and additionally contains 1Kb of lockable EEPROM.
 * Only the temperature sensor functionality is exposed by this driver.
 *
 * Parasite power configuration is not supported by this driver.
 *
 * Based on the DS18B20 driver.
 */

#include <stdbool.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/w1_sensor.h>
#include <zephyr/drivers/w1.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>

#define DT_DRV_COMPAT maxim_max31826

LOG_MODULE_REGISTER(MAX31826, CONFIG_SENSOR_LOG_LEVEL);

#define MAX31826_CMD_CONVERT_T       0x44
#define MAX31826_CMD_READ_SCRATCHPAD 0xBE

#define MAX31826_FAMILYCODE 0x3B

/* The MAX31826 has a fixed 12-bit resolution; the max conversion time
 * (tCONV) is 150 ms.
 */
#define MAX31826_CONV_TIME_MS 150

/*
 * Scratchpad 1 layout, read with the Read Scratchpad 1 (0xBE) command.
 * Bytes 0-1 hold the 16-bit signed temperature register (LSB first),
 * byte 4 is the read-only configuration register (AD[3:0] location pins),
 * bytes 2, 3, 5, 6 and 7 are reserved and read back as 0xFF, and byte 8
 * is the CRC-8 of bytes 0-7.
 */
struct max31826_scratchpad {
	int16_t temp;
	uint8_t reserved2;
	uint8_t reserved3;
	uint8_t config;
	uint8_t res[3];
	uint8_t crc;
} __packed;

struct max31826_config {
	const struct device *bus;
	uint8_t family;
};

struct max31826_data {
	struct w1_slave_config config;
	struct max31826_scratchpad scratchpad;
	bool lazy_loaded;
};

static int max31826_configure(const struct device *dev);

static inline void max31826_temperature_from_raw(uint8_t *temp_raw,
						  struct sensor_value *val)
{
	int16_t temp = sys_get_le16(temp_raw);

	val->val1 = temp / 16;
	val->val2 = (temp % 16) * 1000000 / 16;
}

static inline bool slave_responded(uint8_t *rx_buf, size_t len)
{
	uint8_t cmp_byte = 0xff;

	for (int i = 0; i < len; i++) {
		cmp_byte &= rx_buf[i];
	}

	return (cmp_byte == 0xff) ? false : true;
}

static int max31826_read_scratchpad(const struct device *dev,
				    struct max31826_scratchpad *scratchpad)
{
	struct max31826_data *data = dev->data;
	const struct max31826_config *cfg = dev->config;
	const struct device *bus = cfg->bus;
	int ret;
	uint8_t cmd = MAX31826_CMD_READ_SCRATCHPAD;
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
static int max31826_temperature_convert(const struct device *dev)
{
	int ret;
	struct max31826_data *data = dev->data;
	const struct max31826_config *cfg = dev->config;
	const struct device *bus = cfg->bus;

	(void)w1_lock_bus(bus);
	ret = w1_reset_select(bus, &data->config);
	if (ret != 0) {
		goto out;
	}
	ret = w1_write_byte(bus, MAX31826_CMD_CONVERT_T);
out:
	(void)w1_unlock_bus(bus);
	return ret;
}

static int max31826_sample_fetch(const struct device *dev,
				 enum sensor_channel chan)
{
	struct max31826_data *data = dev->data;
	int status;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL ||
			chan == SENSOR_CHAN_AMBIENT_TEMP);

	if (!data->lazy_loaded) {
		status = max31826_configure(dev);
		if (status < 0) {
			return status;
		}
		data->lazy_loaded = true;
	}

	status = max31826_temperature_convert(dev);
	if (status < 0) {
		LOG_DBG("W1 fetch error");
		return status;
	}
	k_msleep(MAX31826_CONV_TIME_MS);
	return max31826_read_scratchpad(dev, &data->scratchpad);
}

static int max31826_channel_get(const struct device *dev,
				enum sensor_channel chan,
				struct sensor_value *val)
{
	struct max31826_data *data = dev->data;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	max31826_temperature_from_raw((uint8_t *)&data->scratchpad.temp, val);
	return 0;
}

static int max31826_configure(const struct device *dev)
{
	const struct max31826_config *cfg = dev->config;
	struct max31826_data *data = dev->data;

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

	LOG_DBG("Init %s: ROM=%016llx\n", dev->name,
		w1_rom_to_uint64(&data->config.rom));

	return 0;
}

static int max31826_attr_set(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr,
			     const struct sensor_value *thr)
{
	struct max31826_data *data = dev->data;

	if ((enum sensor_attribute_w1)attr != SENSOR_ATTR_W1_ROM) {
		return -ENOTSUP;
	}

	data->lazy_loaded = false;
	w1_sensor_value_to_rom(thr, &data->config.rom);
	return 0;
}

static DEVICE_API(sensor, max31826_driver_api) = {
	.attr_set = max31826_attr_set,
	.sample_fetch = max31826_sample_fetch,
	.channel_get = max31826_channel_get,
};

static int max31826_init(const struct device *dev)
{
	const struct max31826_config *cfg = dev->config;
	struct max31826_data *data = dev->data;

	if (device_is_ready(cfg->bus) == 0) {
		LOG_ERR_DEVICE_NOT_READY(cfg->bus);
		return -ENODEV;
	}

	w1_uint64_to_rom(0ULL, &data->config.rom);
	data->lazy_loaded = false;
	/* in multidrop configurations the rom is needed, but is not set during
	 * driver initialization, therefore do lazy initialization in all cases.
	 */

	return 0;
}

#define MAX31826_DEFINE(inst)						\
	static struct max31826_data max31826_data_##inst;		\
	static const struct max31826_config max31826_config_##inst = {	\
		.bus = DEVICE_DT_GET(DT_INST_BUS(inst)),		\
		.family = (uint8_t)DT_INST_PROP_OR(inst, family_code,	\
						   MAX31826_FAMILYCODE),\
	};								\
	SENSOR_DEVICE_DT_INST_DEFINE(inst,				\
				     max31826_init,			\
				     NULL,				\
				     &max31826_data_##inst,		\
				     &max31826_config_##inst,		\
				     POST_KERNEL,			\
				     CONFIG_SENSOR_INIT_PRIORITY,	\
				     &max31826_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MAX31826_DEFINE)
