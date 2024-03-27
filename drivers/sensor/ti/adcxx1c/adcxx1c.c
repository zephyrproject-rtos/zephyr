/*
 * Copyright (c) 2024 Bert Outtier
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_adcxx1c

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/init.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>

#include "adcxx1c.h"

LOG_MODULE_REGISTER(ADCXX1C, CONFIG_SENSOR_LOG_LEVEL);

int adcxx1c_read_regs(const struct device *dev, uint8_t reg, int16_t *out)
{
	const struct adcxx1c_config *cfg = dev->config;
	uint8_t rx_buf[2];

	if (i2c_write_read_dt(&cfg->bus, &reg, 1, rx_buf, sizeof(rx_buf)) != 0) {
		return -EIO;
	}

	*out = sys_get_be16(rx_buf);
	return 0;
}

int adcxx1c_write_reg(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct adcxx1c_config *cfg = dev->config;

	return i2c_reg_write_byte_dt(&cfg->bus, reg, val);
}

static int adcxx1c_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct adcxx1c_data *data = dev->data;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	if (adcxx1c_read_regs(dev, ADCXX1C_CONV_RES_ADDR, &data->v_sample) < 0) {
		LOG_ERR("Failed to read result!");
		return -EIO;
	}

	return 0;
}

static int adcxx1c_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	const struct adcxx1c_data *data = dev->data;

	if (chan == SENSOR_CHAN_VOLTAGE) {
		val->val1 = (int32_t)(data->v_sample >> (ADCXX1C_RES_12BITS - data->bits)) &
			    GENMASK(data->bits - 1, 0);
		val->val2 = 0;
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static inline int validate_bits(int res)
{
	if (res != ADCXX1C_RES_8BITS && res != ADCXX1C_RES_10BITS && res != ADCXX1C_RES_12BITS) {
		LOG_ERR("invalid resolution value: %d", res);
		return -EINVAL;
	}
	return 0;
}

static inline int set_bits(const struct device *dev)
{
	const struct adcxx1c_config *cfg = dev->config;
	struct adcxx1c_data *data = dev->data;

	if (cfg->resolution < 0) {
		if (cfg->variant < 0) {
			LOG_ERR("please specify at least resolution or variant!");
			return -EIO;
		}
		switch (cfg->variant) {
		case ADCXX1C_TYPE_ADC081C:
			data->bits = ADCXX1C_RES_8BITS;
			break;
		case ADCXX1C_TYPE_ADC101C:
			data->bits = ADCXX1C_RES_10BITS;
			break;
		case ADCXX1C_TYPE_ADC121C:
			data->bits = ADCXX1C_RES_12BITS;
			break;
		default:
			LOG_ERR("invalid variant: %d", cfg->variant);
			return -EIO;
		}
	} else {
		data->bits = cfg->resolution;
	}
	return validate_bits(data->bits);
}

int adcxx1c_init(const struct device *dev)
{
	const struct adcxx1c_config *cfg = dev->config;
	struct adcxx1c_data *data = dev->data;

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("I2C bus %s is not ready!", cfg->bus.bus->name);
		return -EINVAL;
	}

	/* set resolution based on variant */
	if (set_bits(dev) < 0) {
		LOG_ERR("failed to set resolution");
		return -EIO;
	}

	/* write (cfg->cycle << 5) to ADCXX1C_CONF_ADDR */
	data->v_sample = 0;
	data->conf = cfg->cycle << 5;

	if (adcxx1c_write_reg(dev, ADCXX1C_CONF_ADDR, data->conf) < 0) {
		LOG_ERR("Failed to write cycle to config");
		return -EIO;
	}

#ifdef CONFIG_ADCXX1C_TRIGGER
	data->dev = dev;
	if (adcxx1c_init_interrupt(dev) < 0) {
		LOG_ERR("Failed to initialize interrupt");
		return -EIO;
	}
#endif

	return 0;
}

static const struct sensor_driver_api adcxx1c_api_funcs = {
#ifdef CONFIG_ADCXX1C_TRIGGER
	.attr_set = adcxx1c_attr_set,
	.attr_get = adcxx1c_attr_get,
	.trigger_set = adcxx1c_trigger_set,
#endif
	.sample_fetch = adcxx1c_sample_fetch,
	.channel_get = adcxx1c_channel_get,
};

#ifdef CONFIG_ADCXX1C_TRIGGER
#define ADCXX1C_TRIGGER_INIT(inst) .alert_gpio = GPIO_DT_SPEC_INST_GET(inst, alert_gpios),
#else
#define ADCXX1C_TRIGGER_INIT(inst)
#endif

#define ADCXX1C_DEFINE(inst)                                                                       \
	static struct adcxx1c_data adcxx1c_data_##inst;                                            \
                                                                                                   \
	static const struct adcxx1c_config adcxx1c_config_##inst = {                               \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.resolution = DT_INST_PROP_OR(inst, resolution, -1),                               \
		.variant = DT_INST_ENUM_IDX_OR(inst, variant, -1),                                 \
		.cycle = DT_INST_ENUM_IDX_OR(inst, cycle, ADCXX1C_CYCLE_DISABLED),                 \
		ADCXX1C_TRIGGER_INIT(inst)};                                                       \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, adcxx1c_init, NULL, &adcxx1c_data_##inst,               \
				     &adcxx1c_config_##inst, POST_KERNEL,                          \
				     CONFIG_SENSOR_INIT_PRIORITY, &adcxx1c_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(ADCXX1C_DEFINE)
