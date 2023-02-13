/*
 * Copyright (c) 2020 Richard Osterloh
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vcnl4040.h"
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(vcnl4040, CONFIG_SENSOR_LOG_LEVEL);

static void vcnl4040_handle_cb(struct vcnl4040_data *data)
{
	const struct vcnl4040_config *config = data->dev->config;

	gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_DISABLE);

#if defined(CONFIG_VCNL4040_TRIGGER_OWN_THREAD)
	k_sem_give(&data->trig_sem);
#elif defined(CONFIG_VCNL4040_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

static void vcnl4040_gpio_callback(const struct device *dev,
				   struct gpio_callback *cb,
				   uint32_t pin_mask)
{
	struct vcnl4040_data *data =
		CONTAINER_OF(cb, struct vcnl4040_data, gpio_cb);
	const struct vcnl4040_config *config = data->dev->config;

	if ((pin_mask & BIT(config->int_gpio.pin)) == 0U) {
		return;
	}

	vcnl4040_handle_cb(data);
}

static int vcnl4040_handle_proxy_int(const struct device *dev)
{
	struct vcnl4040_data *data = dev->data;
	struct sensor_trigger proxy_trig = {
		.type = SENSOR_TRIG_THRESHOLD,
		.chan = SENSOR_CHAN_PROX,
	};

	if (data->proxy_handler != NULL) {
		data->proxy_handler(dev, &proxy_trig);
	}

	return 0;
}

static int vcnl4040_handle_als_int(const struct device *dev)
{
	struct vcnl4040_data *data = dev->data;
	struct sensor_trigger als_trig = {
		.type = SENSOR_TRIG_THRESHOLD,
		.chan = SENSOR_CHAN_LIGHT,
	};

	if (data->als_handler != NULL) {
		data->als_handler(dev, &als_trig);
	}

	return 0;
}

static void vcnl4040_handle_int(const struct device *dev)
{
	const struct vcnl4040_config *config = dev->config;
	struct vcnl4040_data *data = dev->data;
	uint16_t int_source;

	if (vcnl4040_read(dev, VCNL4040_REG_INT_FLAG, &int_source)) {
		LOG_ERR("Could not read interrupt source");
		int_source = 0U;
	}

	data->int_type = int_source >> 8;

	if (data->int_type == VCNL4040_PROXIMITY_AWAY ||
	    data->int_type == VCNL4040_PROXIMITY_CLOSE) {
		vcnl4040_handle_proxy_int(dev);
	} else if (data->int_type == VCNL4040_AMBIENT_HIGH ||
		   data->int_type == VCNL4040_AMBIENT_LOW) {
		vcnl4040_handle_als_int(dev);
	} else {
		LOG_ERR("Unknown interrupt source %d", data->int_type);
	}

	gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
}

#ifdef CONFIG_VCNL4040_TRIGGER_OWN_THREAD
static void vcnl4040_thread_main(struct vcnl4040_data *data)
{
	while (true) {
		k_sem_take(&data->trig_sem, K_FOREVER);
		vcnl4040_handle_int(data->dev);
	}
}
#endif

#ifdef CONFIG_VCNL4040_TRIGGER_GLOBAL_THREAD
static void vcnl4040_work_handler(struct k_work *work)
{
	struct vcnl4040_data *data =
		CONTAINER_OF(work, struct vcnl4040_data, work);

	vcnl4040_handle_int(data->dev);
}
#endif

int vcnl4040_attr_set(const struct device *dev,
		      enum sensor_channel chan,
		      enum sensor_attribute attr,
		      const struct sensor_value *val)
{
	struct vcnl4040_data *data = dev->data;
	const struct vcnl4040_config *config = dev->config;
	int ret = 0;

	if (config->int_gpio.port == NULL) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);

	if (chan == SENSOR_CHAN_PROX) {
		if (attr == SENSOR_ATTR_UPPER_THRESH) {
			if (vcnl4040_write(dev, VCNL4040_REG_PS_THDH,
					   (uint16_t)val->val1)) {
				ret = -EIO;
				goto exit;
			}

			ret = 0;
			goto exit;
		}
		if (attr == SENSOR_ATTR_LOWER_THRESH) {
			if (vcnl4040_write(dev, VCNL4040_REG_PS_THDL,
					   (uint16_t)val->val1)) {
				ret = -EIO;
				goto exit;
			}

			ret = 0;
			goto exit;
		}
	}
#ifdef CONFIG_VCNL4040_ENABLE_ALS
	if (chan == SENSOR_CHAN_LIGHT) {
		if (attr == SENSOR_ATTR_UPPER_THRESH) {
			if (vcnl4040_write(dev, VCNL4040_REG_ALS_THDH,
					   (uint16_t)val->val1)) {
				ret = -EIO;
				goto exit;
			}

			ret = 0;
			goto exit;
		}
		if (attr == SENSOR_ATTR_LOWER_THRESH) {
			if (vcnl4040_write(dev, VCNL4040_REG_ALS_THDL,
					   (uint16_t)val->val1)) {
				ret = -EIO;
				goto exit;
			}

			ret = 0;
			goto exit;
		}
	}
#endif
	ret = -ENOTSUP;
exit:
	k_mutex_unlock(&data->mutex);

	return ret;
}

int vcnl4040_trigger_set(const struct device *dev,
			 const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler)
{
	const struct vcnl4040_config *config = dev->config;
	struct vcnl4040_data *data = dev->data;
	uint16_t conf;
	int ret = 0;

	if (config->int_gpio.port == NULL) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);

	switch (trig->type) {
	case SENSOR_TRIG_THRESHOLD:
		if (trig->chan == SENSOR_CHAN_PROX) {
			if (vcnl4040_read(dev, VCNL4040_REG_PS_CONF, &conf)) {
				ret = -EIO;
				goto exit;
			}

			/* Set interrupt bits 1:0 of high register */
			conf |= config->proxy_type << 8;

			if (vcnl4040_write(dev, VCNL4040_REG_PS_CONF, conf)) {
				ret = -EIO;
				goto exit;
			}

			data->proxy_handler = handler;
		} else if (trig->chan == SENSOR_CHAN_LIGHT) {
#ifdef CONFIG_VCNL4040_ENABLE_ALS
			if (vcnl4040_read(dev, VCNL4040_REG_ALS_CONF, &conf)) {
				ret = -EIO;
				goto exit;
			}

			/* ALS interrupt enable */
			conf |= VCNL4040_ALS_INT_EN_MASK;

			if (vcnl4040_write(dev, VCNL4040_REG_ALS_CONF, conf)) {
				ret = -EIO;
				goto exit;
			}

			data->als_handler = handler;
#else
			ret = -ENOTSUP;
			goto exit;
#endif
		} else {
			ret = -ENOTSUP;
			goto exit;
		}
		break;
	default:
		LOG_ERR("Unsupported sensor trigger");
		ret = -ENOTSUP;
		goto exit;
	}
exit:
	k_mutex_unlock(&data->mutex);

	return ret;
}

int vcnl4040_trigger_init(const struct device *dev)
{
	const struct vcnl4040_config *config = dev->config;
	struct vcnl4040_data *data = dev->data;

	data->dev = dev;

	if (config->int_gpio.port == NULL) {
		LOG_DBG("instance '%s' doesn't support trigger mode", dev->name);
		return 0;
	}

	/* Get the GPIO device */
	if (!device_is_ready(config->int_gpio.port)) {
		LOG_ERR("%s: device %s is not ready", dev->name,
				config->int_gpio.port->name);
		return -ENODEV;
	}

#if defined(CONFIG_VCNL4040_TRIGGER_OWN_THREAD)
	k_sem_init(&data->trig_sem, 0, K_SEM_MAX_LIMIT);
	k_thread_create(&data->thread, data->thread_stack,
			CONFIG_VCNL4040_THREAD_STACK_SIZE,
			(k_thread_entry_t)vcnl4040_thread_main,
			data, NULL, NULL,
			K_PRIO_COOP(CONFIG_VCNL4040_THREAD_PRIORITY),
			0, K_NO_WAIT);
	k_thread_name_set(&data->thread, "VCNL4040 trigger");
#elif defined(CONFIG_VCNL4040_TRIGGER_GLOBAL_THREAD)
	data->work.handler = vcnl4040_work_handler;
#endif

	gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);

	gpio_init_callback(&data->gpio_cb, vcnl4040_gpio_callback,
			   BIT(config->int_gpio.pin));

	if (gpio_add_callback(config->int_gpio.port, &data->gpio_cb) < 0) {
		LOG_ERR("Failed to set gpio callback");
		return -EIO;
	}

	gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);

	if (gpio_pin_get_dt(&config->int_gpio) > 0) {
		vcnl4040_handle_cb(data);
	}

	return 0;
}
