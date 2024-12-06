/*
 * Copyright (c) 2024 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT phosense_xbr818

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(XBR818, CONFIG_SENSOR_LOG_LEVEL);

#include "xbr818.h"

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
	uint32_t tmp;
	uint64_t tmpf;

	if (chan != SENSOR_CHAN_PROX) {
		LOG_ERR("%s: requesting unsupported channel %i", dev->name, chan);
		return -ENOTSUP;
	}

	if (val->val1 < 0) {
		return -EINVAL;
	}

	gpio_pin_set_dt(&config->i2c_en, 1);

	switch ((int)attr) {
	case SENSOR_ATTR_LOWER_THRESH:
		if (val->val1 > 0xFFFF) {
			return -EINVAL;
		}
		tmp = val->val1 & 0xFF;
		ret = i2c_reg_write_byte_dt(&config->i2c, XBR818_THRESHOLD_1, tmp);
		if (ret != 0) {
			break;
		}
		tmp = (val->val1 & 0xFF00) >> 8;
		ret = i2c_reg_write_byte_dt(&config->i2c, XBR818_THRESHOLD_2, tmp);
		break;
	case SENSOR_ATTR_XBR818_NOISE_FLOOR:
		if (val->val1 > 0xFFFF) {
			return -EINVAL;
		}
		tmp = val->val1 & 0xFF;
		ret = i2c_reg_write_byte_dt(&config->i2c, XBR818_THRESHOLD_NOISE_1, tmp);
		if (ret != 0) {
			break;
		}
		tmp = (val->val1 & 0xFF00) >> 8;
		ret = i2c_reg_write_byte_dt(&config->i2c, XBR818_THRESHOLD_NOISE_2, tmp);
		break;
	case SENSOR_ATTR_XBR818_DELAY_TIME:
		tmpf = (uint64_t)val->val1 * 1000000 + (uint64_t)val->val2;
		tmpf = (tmpf * SENSOR_XBR818_CLOCKRATE) / 1000000;
		if (tmpf > 0xFFFFFF) {
			return -EINVAL;
		}
		tmp = tmpf & 0xFF;
		ret = i2c_reg_write_byte_dt(&config->i2c, XBR818_DELAY_TIME_1, tmp);
		if (ret != 0) {
			break;
		}
		tmp = (tmpf & 0xFF00) >> 8;
		ret = i2c_reg_write_byte_dt(&config->i2c, XBR818_DELAY_TIME_2, tmp);
		if (ret != 0) {
			break;
		}
		tmp = (tmpf & 0xFF0000) >> 16;
		ret = i2c_reg_write_byte_dt(&config->i2c, XBR818_DELAY_TIME_3, tmp);
		break;
	case SENSOR_ATTR_XBR818_LOCK_TIME:
		tmpf = (uint64_t)val->val1 * 1000000 + (uint64_t)val->val2;
		tmpf = (tmpf * SENSOR_XBR818_CLOCKRATE) / 1000000;
		if (tmpf > 0xFFFFFF) {
			return -EINVAL;
		}
		tmp = tmpf & 0xFF;
		ret = i2c_reg_write_byte_dt(&config->i2c, XBR818_LOCK_TIME_1, tmp);
		if (ret != 0) {
			break;
		}
		tmp = (tmpf & 0xFF00) >> 8;
		ret = i2c_reg_write_byte_dt(&config->i2c, XBR818_LOCK_TIME_2, tmp);
		if (ret != 0) {
			break;
		}
		tmp = (tmpf & 0xFF0000) >> 16;
		ret = i2c_reg_write_byte_dt(&config->i2c, XBR818_LOCK_TIME_3, tmp);
		break;
	case SENSOR_ATTR_XBR818_RF_POWER:
		if (val->val1 > 0x7) {
			return -EINVAL;
		}
		tmp = val->val1 & 0x7;
		ret = i2c_reg_write_byte_dt(&config->i2c, XBR818_RF_POWER, tmp);
		break;
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		if (val->val1 > SENSOR_XBR818_CLOCKRATE) {
			return -EINVAL;
		}
		tmp = SENSOR_XBR818_CLOCKRATE / val->val1;
		if (tmp > 0xFF) {
			return -EINVAL;
		}
		ret = i2c_reg_write_byte_dt(&config->i2c, XBR818_SAMPLE_RATE_DIVIDER, tmp);
		break;
	default:
		ret = -ENODEV;

		break;
	}

	gpio_pin_set_dt(&config->i2c_en, 0);

	return ret;
}

static int xbr818_attr_get(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, struct sensor_value *val)
{
	const struct xbr818_config *config = dev->config;
	int ret;
	uint8_t tmp = 0;
	uint64_t tmpf;

	if (chan != SENSOR_CHAN_PROX) {
		LOG_ERR("%s: requesting unsupported channel %i", dev->name, chan);
		return -ENOTSUP;
	}

	gpio_pin_set_dt(&config->i2c_en, 1);

	switch ((int)attr) {
	case SENSOR_ATTR_LOWER_THRESH:
		ret = i2c_reg_read_byte_dt(&config->i2c, XBR818_THRESHOLD_1, &tmp);
		if (ret != 0) {
			break;
		}
		val->val1 = tmp & 0xFF;
		ret = i2c_reg_read_byte_dt(&config->i2c, XBR818_THRESHOLD_2, &tmp);
		if (ret != 0) {
			break;
		}
		val->val1 |= tmp << 8;
		break;
	case SENSOR_ATTR_XBR818_NOISE_FLOOR:
		ret = i2c_reg_read_byte_dt(&config->i2c, XBR818_THRESHOLD_NOISE_1, &tmp);
		if (ret != 0) {
			break;
		}
		val->val1 = tmp & 0xFF;
		ret = i2c_reg_read_byte_dt(&config->i2c, XBR818_THRESHOLD_NOISE_2, &tmp);
		if (ret != 0) {
			break;
		}
		val->val1 |= tmp << 8;
		break;
	case SENSOR_ATTR_XBR818_DELAY_TIME:
		ret = i2c_reg_read_byte_dt(&config->i2c, XBR818_DELAY_TIME_1, &tmp);
		if (ret != 0) {
			break;
		}
		val->val1 = tmp & 0xFF;
		ret = i2c_reg_read_byte_dt(&config->i2c, XBR818_DELAY_TIME_2, &tmp);
		if (ret != 0) {
			break;
		}
		val->val1 |= tmp << 8;
		ret = i2c_reg_read_byte_dt(&config->i2c, XBR818_DELAY_TIME_3, &tmp);
		if (ret != 0) {
			break;
		}
		val->val1 |= tmp << 16;
		tmpf = (uint64_t)val->val1 * 1000000;
		tmpf /= SENSOR_XBR818_CLOCKRATE;
		val->val1 = tmpf / 1000000;
		val->val2 = tmpf - val->val1 * 1000000;
		break;
	case SENSOR_ATTR_XBR818_LOCK_TIME:
		ret = i2c_reg_read_byte_dt(&config->i2c, XBR818_LOCK_TIME_1, &tmp);
		if (ret != 0) {
			break;
		}
		val->val1 = tmp & 0xFF;
		ret = i2c_reg_read_byte_dt(&config->i2c, XBR818_LOCK_TIME_2, &tmp);
		if (ret != 0) {
			break;
		}
		val->val1 |= tmp << 8;
		ret = i2c_reg_read_byte_dt(&config->i2c, XBR818_LOCK_TIME_3, &tmp);
		if (ret != 0) {
			break;
		}
		val->val1 |= tmp << 16;
		tmpf = (uint64_t)val->val1 * 1000000;
		tmpf /= SENSOR_XBR818_CLOCKRATE;
		val->val1 = tmpf / 1000000;
		val->val2 = tmpf - val->val1 * 1000000;
		break;
	case SENSOR_ATTR_XBR818_RF_POWER:
		ret = i2c_reg_read_byte_dt(&config->i2c, XBR818_RF_POWER, &tmp);
		if (ret != 0) {
			break;
		}
		val->val1 = tmp & 0x7;
		break;
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		ret = i2c_reg_read_byte_dt(&config->i2c, XBR818_SAMPLE_RATE_DIVIDER, &tmp);
		if (ret != 0) {
			break;
		}
		val->val1 = SENSOR_XBR818_CLOCKRATE / tmp;
		break;
	default:
		ret = -ENODEV;

		break;
	}

	gpio_pin_set_dt(&config->i2c_en, 0);

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
	int ret;

	ret = i2c_reg_write_byte_dt(&config->i2c, XBR818_IO_ACTIVE_VALUE_REG, 0x03);
	if (ret != 0) {
		return ret;
	}
	ret = i2c_reg_write_byte_dt(&config->i2c, XBR818_RF_EN_SEL, 0x20);
	if (ret != 0) {
		return ret;
	}
	ret = i2c_reg_write_byte_dt(&config->i2c, XBR818_SAMPLE_RATE_DIVIDER, 0x20);
	if (ret != 0) {
		return ret;
	}
	ret = i2c_reg_write_byte_dt(&config->i2c, XBR818_RF_POWER, 0x45);
	if (ret != 0) {
		return ret;
	}
	ret = i2c_reg_write_byte_dt(&config->i2c, XBR818_TIMER_CTRL, 0x21);
	if (ret != 0) {
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&config->i2c, XBR818_THRESHOLD_1, 0x5a);
	if (ret != 0) {
		return ret;
	}
	ret = i2c_reg_write_byte_dt(&config->i2c, XBR818_THRESHOLD_2, 0x01);
	if (ret != 0) {
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&config->i2c, XBR818_THRESHOLD_NOISE_1, 0x55);
	if (ret != 0) {
		return ret;
	}
	ret = i2c_reg_write_byte_dt(&config->i2c, XBR818_THRESHOLD_NOISE_2, 0x01);
	if (ret != 0) {
		return ret;
	}

	/* 0.1 seconds */
	ret = i2c_reg_write_byte_dt(&config->i2c, XBR818_DELAY_TIME_1, 0x80);
	if (ret != 0) {
		return ret;
	}
	ret = i2c_reg_write_byte_dt(&config->i2c, XBR818_DELAY_TIME_2, 0x0C);
	if (ret != 0) {
		return ret;
	}
	ret = i2c_reg_write_byte_dt(&config->i2c, XBR818_DELAY_TIME_3, 0x00);
	if (ret != 0) {
		return ret;
	}

	/* 0.5 seconds */
	ret = i2c_reg_write_byte_dt(&config->i2c, XBR818_LOCK_TIME_1, 0x80);
	if (ret != 0) {
		return ret;
	}
	ret = i2c_reg_write_byte_dt(&config->i2c, XBR818_LOCK_TIME_2, 0x3E);
	if (ret != 0) {
		return ret;
	}
	ret = i2c_reg_write_byte_dt(&config->i2c, XBR818_LOCK_TIME_3, 0x00);
	if (ret != 0) {
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&config->i2c, XBR818_PIN_SETTINGS, 0x0C);
	if (ret != 0) {
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&config->i2c, XBR818_I2C_OUT, 0x1);
	if (ret != 0) {
		return ret;
	}

	return 0;
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
		return ret;
	}

	ret = gpio_pin_configure_dt(&config->i2c_en, GPIO_OUTPUT);
	if (ret != 0) {
		return ret;
	}

	gpio_pin_set_dt(&config->i2c_en, 1);

	ret = xbr818_init_defaults(dev);

	gpio_pin_set_dt(&config->i2c_en, 0);

	if (ret != 0) {
		LOG_ERR("%s: unable to configure", dev->name);
	}

	ret = gpio_pin_interrupt_configure_dt(&config->io_val, GPIO_INT_DISABLE);
	if (ret) {
		LOG_ERR("failed to configure gpio interrupt: %d", ret);
		return ret;
	}

	gpio_init_callback(&data->gpio_cb, xbr818_gpio_callback, BIT(config->io_val.pin));

	return ret;
}

static const struct sensor_driver_api xbr818_api = {
	.sample_fetch = xbr818_sample_fetch,
	.channel_get = xbr818_channel_get,
	.attr_set = xbr818_attr_set,
	.attr_get = xbr818_attr_get,
	.trigger_set = xbr818_trigger_set,
};

#define XBR818_INIT(inst)                                                                          \
	static const struct xbr818_config xbr818_##inst##_config = {                               \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.i2c_en = GPIO_DT_SPEC_GET(DT_INST(inst, phosense_xbr818), i2c_en_gpios),          \
		.io_val = GPIO_DT_SPEC_GET(DT_INST(inst, phosense_xbr818), int_gpios),             \
	};                                                                                         \
                                                                                                   \
	static struct xbr818_data xbr818_##inst##_data;                                            \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, xbr818_init, NULL, &xbr818_##inst##_data,               \
				     &xbr818_##inst##_config, POST_KERNEL,                         \
				     CONFIG_SENSOR_INIT_PRIORITY, &xbr818_api);

DT_INST_FOREACH_STATUS_OKAY(XBR818_INIT);
