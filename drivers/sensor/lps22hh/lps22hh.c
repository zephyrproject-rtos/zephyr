/* ST Microelectronics LPS22HH pressure and temperature sensor
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lps22hh.pdf
 */

#define DT_DRV_COMPAT st_lps22hh

#include <drivers/sensor.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <sys/byteorder.h>
#include <sys/__assert.h>
#include <logging/log.h>

#include "lps22hh.h"

LOG_MODULE_REGISTER(LPS22HH, CONFIG_SENSOR_LOG_LEVEL);

static inline int lps22hh_set_odr_raw(const struct device *dev, uint8_t odr)
{
	struct lps22hh_data *data = dev->data;

	return lps22hh_data_rate_set(data->ctx, odr);
}

static int lps22hh_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	struct lps22hh_data *data = dev->data;
	union axis1bit32_t raw_press;
	union axis1bit16_t raw_temp;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	if (lps22hh_pressure_raw_get(data->ctx, raw_press.u8bit) < 0) {
		LOG_DBG("Failed to read sample");
		return -EIO;
	}
	if (lps22hh_temperature_raw_get(data->ctx, raw_temp.u8bit) < 0) {
		LOG_DBG("Failed to read sample");
		return -EIO;
	}

	data->sample_press = raw_press.i32bit;
	data->sample_temp = raw_temp.i16bit;

	return 0;
}

static inline void lps22hh_press_convert(struct sensor_value *val,
					 int32_t raw_val)
{
	/* Pressure sensitivity is 4096 LSB/hPa */
	/* Convert raw_val to val in kPa */
	val->val1 = (raw_val >> 12) / 10;
	val->val2 = (raw_val >> 12) % 10 * 100000 +
		(((int32_t)((raw_val) & 0x0FFF) * 100000L) >> 12);
}

static inline void lps22hh_temp_convert(struct sensor_value *val,
					int16_t raw_val)
{
	/* Temperature sensitivity is 100 LSB/deg C */
	val->val1 = raw_val / 100;
	val->val2 = ((int32_t)raw_val % 100) * 10000;
}

static int lps22hh_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct lps22hh_data *data = dev->data;

	if (chan == SENSOR_CHAN_PRESS) {
		lps22hh_press_convert(val, data->sample_press);
	} else if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		lps22hh_temp_convert(val, data->sample_temp);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static const uint16_t lps22hh_map[] = {0, 1, 10, 25, 50, 75, 100, 200};

static int lps22hh_odr_set(const struct device *dev, uint16_t freq)
{
	int odr;

	for (odr = 0; odr < ARRAY_SIZE(lps22hh_map); odr++) {
		if (freq == lps22hh_map[odr]) {
			break;
		}
	}

	if (odr == ARRAY_SIZE(lps22hh_map)) {
		LOG_DBG("bad frequency");
		return -EINVAL;
	}

	if (lps22hh_set_odr_raw(dev, odr) < 0) {
		LOG_DBG("failed to set sampling rate");
		return -EIO;
	}

	return 0;
}

static int lps22hh_attr_set(const struct device *dev,
			    enum sensor_channel chan,
			    enum sensor_attribute attr,
			    const struct sensor_value *val)
{
	if (chan != SENSOR_CHAN_ALL) {
		LOG_WRN("attr_set() not supported on this channel.");
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return lps22hh_odr_set(dev, val->val1);
	default:
		LOG_DBG("operation not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api lps22hh_api_funcs = {
	.attr_set = lps22hh_attr_set,
	.sample_fetch = lps22hh_sample_fetch,
	.channel_get = lps22hh_channel_get,
#if CONFIG_LPS22HH_TRIGGER
	.trigger_set = lps22hh_trigger_set,
#endif
};

static int lps22hh_init_chip(const struct device *dev)
{
	struct lps22hh_data *data = dev->data;
	uint8_t chip_id;

	if (lps22hh_device_id_get(data->ctx, &chip_id) < 0) {
		LOG_DBG("Failed reading chip id");
		return -EIO;
	}

	if (chip_id != LPS22HH_ID) {
		LOG_DBG("Invalid chip id 0x%x", chip_id);
		return -EIO;
	}

	if (lps22hh_set_odr_raw(dev, CONFIG_LPS22HH_SAMPLING_RATE) < 0) {
		LOG_DBG("Failed to set sampling rate");
		return -EIO;
	}

	if (lps22hh_block_data_update_set(data->ctx, PROPERTY_ENABLE) < 0) {
		LOG_DBG("Failed to set BDU");
		return -EIO;
	}

	return 0;
}

static int lps22hh_init(const struct device *dev)
{
	const struct lps22hh_config * const config = dev->config;
	struct lps22hh_data *data = dev->data;

	data->dev = dev;

	data->bus = device_get_binding(config->master_dev_name);
	if (!data->bus) {
		LOG_DBG("bus master not found: %s", config->master_dev_name);
		return -EINVAL;
	}

	config->bus_init(dev);

	if (lps22hh_init_chip(dev) < 0) {
		LOG_DBG("Failed to initialize chip");
		return -EIO;
	}

#ifdef CONFIG_LPS22HH_TRIGGER
	if (lps22hh_init_interrupt(dev) < 0) {
		LOG_ERR("Failed to initialize interrupt.");
		return -EIO;
	}
#endif

	return 0;
}

static struct lps22hh_data lps22hh_data;

static const struct lps22hh_config lps22hh_config = {
	.master_dev_name = DT_INST_BUS_LABEL(0),
#ifdef CONFIG_LPS22HH_TRIGGER
	.drdy_port	= DT_INST_GPIO_LABEL(0, drdy_gpios),
	.drdy_pin	= DT_INST_GPIO_PIN(0, drdy_gpios),
	.drdy_flags	= DT_INST_GPIO_FLAGS(0, drdy_gpios),
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	.bus_init = lps22hh_spi_init,
	.spi_conf.frequency = DT_INST_PROP(0, spi_max_frequency),
	.spi_conf.operation = (SPI_OP_MODE_MASTER | SPI_MODE_CPOL |
			       SPI_MODE_CPHA | SPI_WORD_SET(8) |
			       SPI_LINES_SINGLE),
	.spi_conf.slave     = DT_INST_REG_ADDR(0),
#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	.gpio_cs_port	    = DT_INST_SPI_DEV_CS_GPIOS_LABEL(0),
	.cs_gpio	    = DT_INST_SPI_DEV_CS_GPIOS_PIN(0),
	.cs_gpio_flags	    = DT_INST_SPI_DEV_CS_GPIOS_FLAGS(0),

	.spi_conf.cs        =  &lps22hh_data.cs_ctrl,
#else
	.spi_conf.cs        = NULL,
#endif
#elif DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	.bus_init = lps22hh_i2c_init,
	.i2c_slv_addr = DT_INST_REG_ADDR(0),
#else
#error "BUS MACRO NOT DEFINED IN DTS"
#endif
};

DEVICE_AND_API_INIT(lps22hh, DT_INST_LABEL(0), lps22hh_init,
		    &lps22hh_data, &lps22hh_config, POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY, &lps22hh_api_funcs);
