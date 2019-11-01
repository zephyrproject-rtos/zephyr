/* ST Microelectronics STTS751 temperature sensor
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/stts751.pdf
 */

#include <sensor.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <sys/byteorder.h>
#include <sys/__assert.h>
#include <logging/log.h>

#include "stts751.h"

LOG_MODULE_REGISTER(STTS751, CONFIG_SENSOR_LOG_LEVEL);

static inline int stts751_set_odr_raw(struct device *dev, u8_t odr)
{
	struct stts751_data *data = dev->driver_data;

	return stts751_temp_data_rate_set(data->ctx, odr);
}

static int stts751_sample_fetch(struct device *dev,
				enum sensor_channel chan)
{
	struct stts751_data *data = dev->driver_data;
	union axis1bit16_t raw_temp;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	if (stts751_temperature_raw_get(data->ctx, &raw_temp.i16bit) < 0) {
		LOG_DBG("Failed to read sample");
		return -EIO;
	}

	data->sample_temp = raw_temp.i16bit;

	return 0;
}

static inline void stts751_temp_convert(struct sensor_value *val,
					s16_t raw_val)
{
	val->val1 = raw_val / 256;
	val->val2 = ((s32_t)raw_val % 256) * 10000;
}

static int stts751_channel_get(struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct stts751_data *data = dev->driver_data;

	if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		stts751_temp_convert(val, data->sample_temp);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static const struct {
	s32_t rate;
	s32_t rate_dec;
} stts751_map[] = {
			{0, 62500},
			{0, 125000},
			{0, 250000},
			{0, 500000},
			{1, 0},
			{2, 0},
			{4, 0},
			{8, 0},
			{16, 0},
			{32, 0},
		};

static int stts751_odr_set(struct device *dev,
			   const struct sensor_value *val)
{
	int odr;

	for (odr = 0; odr < ARRAY_SIZE(stts751_map); odr++) {
		if (val->val1 == stts751_map[odr].rate &&
		    val->val2 == stts751_map[odr].rate_dec) {
			break;
		}
	}

	if (odr == ARRAY_SIZE(stts751_map)) {
		LOG_DBG("bad frequency");
		return -EINVAL;
	}

	if (stts751_set_odr_raw(dev, odr) < 0) {
		LOG_DBG("failed to set sampling rate");
		return -EIO;
	}

	return 0;
}

static int stts751_attr_set(struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr,
			    const struct sensor_value *val)
{
	if (chan != SENSOR_CHAN_ALL) {
		LOG_WRN("attr_set() not supported on this channel.");
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return stts751_odr_set(dev, val);
	default:
		LOG_DBG("operation not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api stts751_api_funcs = {
	.attr_set = stts751_attr_set,
	.sample_fetch = stts751_sample_fetch,
	.channel_get = stts751_channel_get,
#if CONFIG_STTS751_TRIGGER
	.trigger_set = stts751_trigger_set,
#endif
};

static int stts751_init_chip(struct device *dev)
{
	struct stts751_data *data = dev->driver_data;
	stts751_id_t chip_id;

	if (stts751_device_id_get(data->ctx, &chip_id) < 0) {
		LOG_DBG("Failed reading chip id");
		return -EIO;
	}

	if (chip_id.manufacturer_id != STTS751_ID_MAN) {
		LOG_DBG("Invalid chip id 0x%x", chip_id.manufacturer_id);
		return -EIO;
	}

	if (stts751_set_odr_raw(dev, CONFIG_STTS751_SAMPLING_RATE) < 0) {
		LOG_DBG("Failed to set sampling rate");
		return -EIO;
	}

	if (stts751_resolution_set(data->ctx, STTS751_11bit) < 0) {
		LOG_DBG("Failed to set resolution");
		return -EIO;
	}

	return 0;
}

static int stts751_init(struct device *dev)
{
	const struct stts751_config * const config = dev->config->config_info;
	struct stts751_data *data = dev->driver_data;

	data->bus = device_get_binding(config->master_dev_name);
	if (!data->bus) {
		LOG_DBG("bus master not found: %s", config->master_dev_name);
		return -EINVAL;
	}

	config->bus_init(dev);

	if (stts751_init_chip(dev) < 0) {
		LOG_DBG("Failed to initialize chip");
		return -EIO;
	}

#ifdef CONFIG_STTS751_TRIGGER
	if (stts751_init_interrupt(dev) < 0) {
		LOG_ERR("Failed to initialize interrupt.");
		return -EIO;
	}
#endif

	return 0;
}

static struct stts751_data stts751_data;

static const struct stts751_config stts751_config = {
	.master_dev_name = DT_INST_0_ST_STTS751_BUS_NAME,
#ifdef CONFIG_STTS751_TRIGGER
	.event_port	= DT_INST_0_ST_STTS751_DRDY_GPIOS_CONTROLLER,
	.event_pin	= DT_INST_0_ST_STTS751_DRDY_GPIOS_PIN,
#endif
#if defined(DT_ST_STTS751_BUS_I2C)
	.bus_init = stts751_i2c_init,
	.i2c_slv_addr = DT_INST_0_ST_STTS751_BASE_ADDRESS,
#else
#error "BUS MACRO NOT DEFINED IN DTS"
#endif
};

DEVICE_AND_API_INIT(stts751, DT_INST_0_ST_STTS751_LABEL, stts751_init,
		    &stts751_data, &stts751_config, POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY, &stts751_api_funcs);
