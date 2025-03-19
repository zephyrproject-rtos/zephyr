/*
 * Copyright (c) 2024 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT phosense_xbr818

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(XBR818, CONFIG_SENSOR_LOG_LEVEL);

#include "xbr818.h"
#include <zephyr/drivers/sensor/xbr818.h>

static int xbr818_enable_i2c(const struct device *dev)
{
	const struct xbr818_config *config = dev->config;
	int ret;

	if (config->i2c_en.port) {
		ret = gpio_pin_set_dt(&config->i2c_en, 1);
		if (ret != 0) {
			LOG_ERR("%s: could not set i2c_en pin", dev->name);
		}
		k_usleep(10);
		return ret;
	}
	return 0;
}

static int xbr818_disable_i2c(const struct device *dev)
{
	const struct xbr818_config *config = dev->config;
	int ret;

	if (config->i2c_en.port) {
		ret = gpio_pin_set_dt(&config->i2c_en, 0);
		if (ret != 0) {
			LOG_ERR("%s: could not unset i2c_en pin", dev->name);
		}
		return ret;
	}
	return 0;
}

static int xbr818_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct xbr818_config *config = dev->config;
	struct xbr818_data *data = dev->data;
	int ret;

	if (chan != SENSOR_CHAN_PROX && chan != SENSOR_CHAN_ALL) {
		LOG_ERR("%s: requesting unsupported channel %i", dev->name, chan);
		return -ENOTSUP;
	}

	ret = gpio_pin_get_dt(&config->io_val);
	if (ret < 0) {
		return ret;
	}
	data->value = (ret == 1 ? true : false);

	return 0;
}

static int xbr818_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct xbr818_data *data = dev->data;

	if (chan != SENSOR_CHAN_PROX) {
		LOG_ERR("%s: requesting unsupported channel %i", dev->name, chan);
		return -ENOTSUP;
	}

	val->val1 = (data->value ? 1 : 0);
	val->val2 = 0;

	return 0;
}

static int xbr818_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct xbr818_config *config = dev->config;
	int ret;
	uint8_t tmp[3];
	uint64_t tmpf;

	if (chan != SENSOR_CHAN_PROX) {
		LOG_ERR("%s: requesting unsupported channel %i", dev->name, chan);
		return -ENOTSUP;
	}

	if (val->val1 < 0) {
		return -EINVAL;
	}

	ret = xbr818_enable_i2c(dev);
	if (ret != 0) {
		return ret;
	}

	if (attr == SENSOR_ATTR_LOWER_THRESH) {
		if (val->val1 > 0xFFFF || val->val1 < 0) {
			return -EINVAL;
		}
		tmp[0] = val->val1 & 0xFF;
		tmp[1] = (val->val1 & 0xFF00) >> 8;
		ret = i2c_burst_write_dt(&config->i2c, XBR818_THRESHOLD_1, tmp, 2);
	} else if ((enum sensor_attribute_xbr818)attr == SENSOR_ATTR_XBR818_NOISE_FLOOR) {
		if (val->val1 > 0xFFFF || val->val1 < 0) {
			return -EINVAL;
		}
		tmp[0] = val->val1 & 0xFF;
		tmp[1] = (val->val1 & 0xFF00) >> 8;
		ret = i2c_burst_write_dt(&config->i2c, XBR818_THRESHOLD_NOISE_1, tmp, 2);
	} else if ((enum sensor_attribute_xbr818)attr == SENSOR_ATTR_XBR818_DELAY_TIME) {
		if (val->val1 < 0 || val->val2 < 0) {
			return -EINVAL;
		}
		tmpf = (uint64_t)val->val1 * 1000000 + (uint64_t)val->val2;
		tmpf = (tmpf * SENSOR_XBR818_CLOCKRATE) / 1000000;
		if (tmpf > 0xFFFFFF) {
			return -EINVAL;
		}
		tmp[0] = tmpf & 0xFF;
		tmp[1] = (tmpf & 0xFF00) >> 8;
		tmp[2] = (tmpf & 0xFF0000) >> 16;
		ret = i2c_burst_write_dt(&config->i2c, XBR818_DELAY_TIME_1, tmp, 3);
	} else if ((enum sensor_attribute_xbr818)attr == SENSOR_ATTR_XBR818_LOCK_TIME) {
		if (val->val1 < 0 || val->val2 < 0) {
			return -EINVAL;
		}
		tmpf = (uint64_t)val->val1 * 1000000 + (uint64_t)val->val2;
		tmpf = (tmpf * SENSOR_XBR818_CLOCKRATE) / 1000000;
		if (tmpf > 0xFFFFFF) {
			return -EINVAL;
		}
		tmp[0] = tmpf & 0xFF;
		tmp[1] = (tmpf & 0xFF00) >> 8;
		tmp[2] = (tmpf & 0xFF0000) >> 16;
		ret = i2c_burst_write_dt(&config->i2c, XBR818_LOCK_TIME_1, tmp, 3);
	} else if ((enum sensor_attribute_xbr818)attr == SENSOR_ATTR_XBR818_RF_POWER) {
		if (val->val1 > 0x7 || val->val1 < 0) {
			return -EINVAL;
		}
		tmp[0] = val->val1 & 0x7;
		ret = i2c_reg_write_byte_dt(&config->i2c, XBR818_RF_POWER, tmp[0]);
	} else if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
		if (val->val1 > SENSOR_XBR818_CLOCKRATE || val->val1 <= 0) {
			return -EINVAL;
		}
		tmp[0] = SENSOR_XBR818_CLOCKRATE / val->val1;
		if (tmp[0] > 0xFF) {
			return -EINVAL;
		}
		ret = i2c_reg_write_byte_dt(&config->i2c, XBR818_SAMPLE_RATE_DIVIDER, tmp[0]);
	} else {
		ret = xbr818_disable_i2c(dev);
		if (ret != 0) {
			return ret;
		}
		return -ENODEV;
	}

	if (ret != 0) {
		return ret;
	}

	ret = xbr818_disable_i2c(dev);

	return ret;
}

static int xbr818_attr_get(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, struct sensor_value *val)
{
	const struct xbr818_config *config = dev->config;
	int ret;
	uint8_t tmp[3];
	uint64_t tmpf;

	if (chan != SENSOR_CHAN_PROX) {
		LOG_ERR("%s: requesting unsupported channel %i", dev->name, chan);
		return -ENOTSUP;
	}

	ret = xbr818_enable_i2c(dev);
	if (ret != 0) {
		return ret;
	}

	if (attr == SENSOR_ATTR_LOWER_THRESH) {
		ret = i2c_burst_read_dt(&config->i2c, XBR818_THRESHOLD_1, tmp, 2);
		if (ret != 0) {
			return ret;
		}
		val->val1 = tmp[0] & 0xFF;
		val->val1 |= (uint32_t)tmp[1] << 8;
	} else if ((enum sensor_attribute_xbr818)attr == SENSOR_ATTR_XBR818_NOISE_FLOOR) {
		ret = i2c_burst_read_dt(&config->i2c, XBR818_THRESHOLD_NOISE_1, tmp, 2);
		if (ret != 0) {
			return ret;
		}
		val->val1 = tmp[0] & 0xFF;
		val->val1 |= (uint32_t)tmp[1] << 8;
	} else if ((enum sensor_attribute_xbr818)attr == SENSOR_ATTR_XBR818_DELAY_TIME) {
		ret = i2c_burst_read_dt(&config->i2c, XBR818_DELAY_TIME_1, tmp, 3);
		if (ret != 0) {
			return ret;
		}
		val->val1 = tmp[0] & 0xFF;
		val->val1 |= (uint32_t)tmp[1] << 8;
		val->val1 |= (uint32_t)tmp[2] << 16;
		tmpf = (uint64_t)val->val1 * 1000000;
		tmpf /= SENSOR_XBR818_CLOCKRATE;
		val->val1 = tmpf / 1000000;
		val->val2 = tmpf - val->val1 * 1000000;
	} else if ((enum sensor_attribute_xbr818)attr == SENSOR_ATTR_XBR818_LOCK_TIME) {
		ret = i2c_burst_read_dt(&config->i2c, XBR818_LOCK_TIME_1, tmp, 3);
		if (ret != 0) {
			return ret;
		}
		val->val1 = tmp[0] & 0xFF;
		val->val1 |= (uint32_t)tmp[1] << 8;
		val->val1 |= (uint32_t)tmp[2] << 16;
		tmpf = (uint64_t)val->val1 * 1000000;
		tmpf /= SENSOR_XBR818_CLOCKRATE;
		val->val1 = tmpf / 1000000;
		val->val2 = tmpf - val->val1 * 1000000;
	} else if ((enum sensor_attribute_xbr818)attr == SENSOR_ATTR_XBR818_RF_POWER) {
		ret = i2c_reg_read_byte_dt(&config->i2c, XBR818_RF_POWER, tmp);
		if (ret != 0) {
			return ret;
		}
		val->val1 = *tmp & 0x7;
	} else if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
		ret = i2c_reg_read_byte_dt(&config->i2c, XBR818_SAMPLE_RATE_DIVIDER, tmp);
		if (ret != 0) {
			return ret;
		}
		val->val1 = SENSOR_XBR818_CLOCKRATE / *tmp;
	} else {
		ret = xbr818_disable_i2c(dev);
		if (ret != 0) {
			return ret;
		}
		return -ENODEV;
	}

	ret = xbr818_disable_i2c(dev);

	return ret;
}

static void xbr818_work(struct k_work *work)
{
	struct xbr818_data *data = CONTAINER_OF(work, struct xbr818_data, work);

	if (likely(data->handler != NULL)) {
		data->handler(data->dev, data->trigger);
	}
}

static void xbr818_gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct xbr818_data *data = CONTAINER_OF(cb, struct xbr818_data, gpio_cb);

	k_work_submit(&data->work);
}

static int xbr818_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			      sensor_trigger_handler_t handler)
{
	const struct xbr818_config *config = dev->config;
	struct xbr818_data *data = dev->data;
	int ret;

	if (trig->chan != SENSOR_CHAN_PROX) {
		LOG_ERR("%s: requesting unsupported channel %i", dev->name, trig->chan);
		return -ENOTSUP;
	}

	if (trig->type != SENSOR_TRIG_MOTION) {
		LOG_ERR("%s: requesting unsupported trigger %i", dev->name, trig->type);
		return -ENOTSUP;
	}

	data->handler = handler;
	data->trigger = trig;
	ret = gpio_pin_interrupt_configure_dt(&config->io_val, GPIO_INT_EDGE_RISING);
	if (ret < 0) {
		return ret;
	}

	if (handler) {
		ret = gpio_add_callback(config->io_val.port, &data->gpio_cb);
	} else {
		ret = gpio_remove_callback(config->io_val.port, &data->gpio_cb);
	}

	return ret;
}

static int xbr818_init_defaults(const struct device *dev)
{
	const struct xbr818_config *config = dev->config;
	int ret = 0;
	uint8_t data[3];

	ret |= i2c_reg_write_byte_dt(&config->i2c, XBR818_IO_ACTIVE_VALUE_REG, 0x03);
	__ASSERT(ret == 0, "Error sending XBR818 defaults for XBR818_IO_ACTIVE_VALUE_REG");
	ret |= i2c_reg_write_byte_dt(&config->i2c, XBR818_RF_EN_SEL, 0x20);
	__ASSERT(ret == 0, "Error sending XBR818 defaults for XBR818_RF_EN_SEL");
	ret |= i2c_reg_write_byte_dt(&config->i2c, XBR818_SAMPLE_RATE_DIVIDER, 0x20);
	__ASSERT(ret == 0, "Error sending XBR818 defaults for XBR818_SAMPLE_RATE_DIVIDER");
	ret |= i2c_reg_write_byte_dt(&config->i2c, XBR818_RF_POWER, 0x45);
	__ASSERT(ret == 0, "Error sending XBR818 defaults for XBR818_RF_POWER");
	ret |= i2c_reg_write_byte_dt(&config->i2c, XBR818_TIMER_CTRL, 0x21);
	__ASSERT(ret == 0, "Error sending XBR818 defaults for XBR818_TIMER_CTRL");

	data[0] = 0x5a;
	data[1] = 0x01;
	ret |= i2c_burst_write_dt(&config->i2c, XBR818_THRESHOLD_1, data, 2);
	__ASSERT(ret == 0, "Error sending XBR818 defaults for XBR818_THRESHOLD");

	data[0] = 0x55;
	data[1] = 0x01;
	ret |= i2c_burst_write_dt(&config->i2c, XBR818_THRESHOLD_NOISE_1, data, 2);
	__ASSERT(ret == 0, "Error sending XBR818 defaults for XBR818_THRESHOLD_NOISE");

	/* 0.1 seconds */
	data[0] = 0x80;
	data[1] = 0x0C;
	data[2] = 0x00;
	ret |= i2c_burst_write_dt(&config->i2c, XBR818_DELAY_TIME_1, data, 3);
	__ASSERT(ret == 0, "Error sending XBR818 defaults for XBR818_DELAY_TIME");

	/* 0.5 seconds */
	data[0] = 0x80;
	data[1] = 0x3E;
	data[2] = 0x00;
	ret |= i2c_burst_write_dt(&config->i2c, XBR818_LOCK_TIME_1, data, 3);
	__ASSERT(ret == 0, "Error sending XBR818 defaults for XBR818_LOCK_TIME");

	ret |= i2c_reg_write_byte_dt(&config->i2c, XBR818_PIN_SETTINGS, 0x0C);
	__ASSERT(ret == 0, "Error sending XBR818 defaults for XBR818_PIN_SETTINGS");

	ret |= i2c_reg_write_byte_dt(&config->i2c, XBR818_I2C_OUT, 0x1);
	__ASSERT(ret == 0, "Error sending XBR818 defaults for XBR818_I2C_OUT");

	return ret;
}

static int xbr818_init(const struct device *dev)
{
	const struct xbr818_config *config = dev->config;
	struct xbr818_data *data = dev->data;
	int ret;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C device not ready");
		return -ENODEV;
	}

	data->dev = dev;
	data->work.handler = xbr818_work;

	ret = gpio_pin_configure_dt(&config->io_val, GPIO_INPUT);
	if (ret != 0) {
		LOG_ERR("%s: could not configure io_val(int) pin", dev->name);
		return ret;
	}

	if (config->i2c_en.port) {
		ret = gpio_pin_configure_dt(&config->i2c_en, GPIO_OUTPUT);
		if (ret != 0) {
			LOG_ERR("%s: could not configure i2c_en pin", dev->name);
			return ret;
		}
	}

	ret = xbr818_enable_i2c(dev);
	if (ret != 0) {
		return ret;
	}

	ret = xbr818_init_defaults(dev);
	if (ret != 0) {
		LOG_ERR("%s: unable to configure", dev->name);
	}

	ret = xbr818_disable_i2c(dev);
	if (ret != 0) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->io_val, GPIO_INT_DISABLE);
	if (ret) {
		LOG_ERR("%s: failed to configure gpio interrupt: %d", dev->name, ret);
		return ret;
	}

	gpio_init_callback(&data->gpio_cb, xbr818_gpio_callback, BIT(config->io_val.pin));

	return ret;
}

static DEVICE_API(sensor, xbr818_api) = {
	.sample_fetch = xbr818_sample_fetch,
	.channel_get = xbr818_channel_get,
	.attr_set = xbr818_attr_set,
	.attr_get = xbr818_attr_get,
	.trigger_set = xbr818_trigger_set,
};

#define XBR818_INIT(inst)                                                                          \
	static const struct xbr818_config xbr818_##inst##_config = {                               \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.i2c_en = GPIO_DT_SPEC_GET_OR(DT_INST(inst, phosense_xbr818), i2c_en_gpios, {0}),  \
		.io_val = GPIO_DT_SPEC_GET(DT_INST(inst, phosense_xbr818), int_gpios),             \
	};                                                                                         \
                                                                                                   \
	static struct xbr818_data xbr818_##inst##_data;                                            \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, xbr818_init, NULL, &xbr818_##inst##_data,               \
				     &xbr818_##inst##_config, POST_KERNEL,                         \
				     CONFIG_SENSOR_INIT_PRIORITY, &xbr818_api);

DT_INST_FOREACH_STATUS_OKAY(XBR818_INIT);
