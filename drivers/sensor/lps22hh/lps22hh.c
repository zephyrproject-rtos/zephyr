/* ST Microelectronics LPS22HH pressure and temperature sensor
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lps22hh.pdf
 */

#include <sensor.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <misc/byteorder.h>
#include <misc/__assert.h>
#include <logging/log.h>

#include "lps22hh.h"

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
LOG_MODULE_REGISTER(LPS22HH);

static inline int lps22hh_set_odr_raw(struct device *dev, u8_t odr)
{
	struct lps22hh_data *data = dev->driver_data;

	return data->hw_tf->update_reg(data, LPS22HH_REG_CTRL_REG1,
				       LPS22HH_MASK_CTRL_REG1_ODR, odr);
}

static int lps22hh_sample_fetch(struct device *dev,
				enum sensor_channel chan)
{
	struct lps22hh_data *data = dev->driver_data;
	u8_t out[5];

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	if (data->hw_tf->read_data(data, LPS22HH_REG_PRESS_OUT_XL,
				   out, sizeof(out)) < 0) {
		LOG_DBG("Failed to read sample");
		return -EIO;
	}

	data->sample_press = (s32_t)((u32_t)(out[0]) |
				     ((u32_t)(out[1]) << 8) |
				     ((u32_t)(out[2]) << 16));
	data->sample_temp = (s16_t)((u16_t)(out[3]) |
				    ((u16_t)(out[4]) << 8));

	return 0;
}

static inline void lps22hh_press_convert(struct sensor_value *val,
					 s32_t raw_val)
{
	/* Pressure sensitivity is 4096 LSB/hPa */
	/* Convert raw_val to val in kPa */
	val->val1 = (raw_val >> 12) / 10;
	val->val2 = (raw_val >> 12) % 10 * 100000 +
		(((s32_t)((raw_val) & 0x0FFF) * 100000L) >> 12);
}

static inline void lps22hh_temp_convert(struct sensor_value *val,
					s16_t raw_val)
{
	/* Temperature sensitivity is 100 LSB/deg C */
	val->val1 = raw_val / 100;
	val->val2 = ((s32_t)raw_val % 100) * 10000;
}

static int lps22hh_channel_get(struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct lps22hh_data *data = dev->driver_data;

	if (chan == SENSOR_CHAN_PRESS) {
		lps22hh_press_convert(val, data->sample_press);
	} else if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		lps22hh_temp_convert(val, data->sample_temp);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static const u16_t lps22hh_map[] = {0, 1, 10, 25, 50, 75, 100, 200};

static int lps22hh_odr_set(struct device *dev, u16_t freq)
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

static int lps22hh_attr_set(struct device *dev, enum sensor_channel chan,
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

static int lps22hh_init_chip(struct device *dev)
{
	struct lps22hh_data *data = dev->driver_data;
	u8_t chip_id;

	if (data->hw_tf->read_reg(data, LPS22HH_REG_WHO_AM_I, &chip_id) < 0) {
		LOG_DBG("Failed reading chip id");
		return -EIO;
	}

	if (chip_id != LPS22HH_VAL_WHO_AM_I) {
		LOG_DBG("Invalid chip id 0x%x", chip_id);
		return -EIO;
	}

	if (lps22hh_set_odr_raw(dev, CONFIG_LPS22HH_SAMPLING_RATE) < 0) {
		LOG_DBG("Failed to set sampling rate");
		return -EIO;
	}

	if (data->hw_tf->update_reg(data,
				    LPS22HH_REG_CTRL_REG1,
				    LPS22HH_MASK_CTRL_REG1_BDU, 1) < 0) {
		LOG_DBG("Failed to set BDU");
		return -EIO;
	}

	return 0;
}

static int lps22hh_init(struct device *dev)
{
	const struct lps22hh_config * const config = dev->config->config_info;
	struct lps22hh_data *data = dev->driver_data;

	data->bus = device_get_binding(config->master_dev_name);
	if (!data->bus) {
		LOG_DBG("I2c master not found: %s", config->master_dev_name);
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

static const struct lps22hh_config lps22hh_config = {
	.master_dev_name = DT_ST_LPS22HH_0_BUS_NAME,
#if defined(DT_ST_LPS22HH_BUS_SPI)
	.bus_init = lps22hh_spi_init,
#elif defined(DT_ST_LPS22HH_BUS_I2C)
	.bus_init = lps22hh_i2c_init,
#else
#error "BUS MACRO NOT DEFINED IN DTS"
#endif
};

static struct lps22hh_data lps22hh_data;

DEVICE_AND_API_INIT(lps22hh, DT_ST_LPS22HH_0_LABEL, lps22hh_init,
		    &lps22hh_data, &lps22hh_config, POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY, &lps22hh_api_funcs);
