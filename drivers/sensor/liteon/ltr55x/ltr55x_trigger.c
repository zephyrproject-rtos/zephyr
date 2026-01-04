/*
 * Copyright (c) 2025 TOKITA Hiroshi
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ltr55x.h"
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_DECLARE(LTR55X, CONFIG_SENSOR_LOG_LEVEL);

static void ltr55x_handle_cb(struct ltr55x_data *data)
{
	const struct ltr55x_config *config = data->dev->config;

	gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_DISABLE);

#if defined(CONFIG_LTR55X_TRIGGER_OWN_THREAD)
	k_sem_give(&data->trig_sem);
#elif defined(CONFIG_LTR55X_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

static void ltr55x_gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pin_mask)
{
	struct ltr55x_data *data =
		CONTAINER_OF(cb, struct ltr55x_data, gpio_cb);
	const struct ltr55x_config *config = data->dev->config;

	ARG_UNUSED(dev);

	if ((pin_mask & BIT(config->int_gpio.pin)) == 0U) {
		return;
	}

	ltr55x_handle_cb(data);
}

static int ltr55x_handle_als_int(const struct device *dev)
{
	struct ltr55x_data *data = dev->data;

	if (data->als_handler != NULL) {
		data->als_handler(dev, data->als_trigger);
	}

	return 0;
}

static int ltr55x_handle_ps_int(const struct device *dev)
{
	struct ltr55x_data *data = dev->data;

	if (data->ps_handler != NULL) {
		data->ps_handler(dev, data->ps_trigger);
	}

	return 0;
}

static void ltr55x_handle_int(const struct device *dev)
{
	const struct ltr55x_config *config = dev->config;
	struct ltr55x_data *data = dev->data;
	uint8_t status;
	int rc;

	rc = i2c_reg_read_byte_dt(&config->bus, LTR55X_ALS_PS_STATUS, &status);
	if (rc < 0) {
		LOG_ERR("Failed to read interrupt status");
		return;
	}

	if ((status & LTR55X_ALS_PS_STATUS_ALS_INTR_STATUS_MASK) &&
	    (status & LTR55X_ALS_PS_STATUS_PS_INTR_STATUS_MASK)) {
		ltr55x_handle_als_int(dev);
		ltr55x_handle_ps_int(dev);
		ltr55x_read_data(config, SENSOR_CHAN_ALL, data);
	} else if (status & LTR55X_ALS_PS_STATUS_ALS_INTR_STATUS_MASK) {
		ltr55x_handle_als_int(dev);
		ltr55x_read_data(config, SENSOR_CHAN_LIGHT, data);
	} else if (status & LTR55X_ALS_PS_STATUS_PS_INTR_STATUS_MASK) {
		ltr55x_handle_ps_int(dev);
		ltr55x_read_data(config, SENSOR_CHAN_PROX, data);
	} else {
		LOG_WRN("Unknown interrupt source");
		return;
	}

	gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
}

#ifdef CONFIG_LTR55X_TRIGGER_OWN_THREAD
static void ltr55x_thread_main(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct ltr55x_data *data = p1;

	while (true) {
		k_sem_take(&data->trig_sem, K_FOREVER);
		ltr55x_handle_int(data->dev);
	}
}
#endif

#ifdef CONFIG_LTR55X_TRIGGER_GLOBAL_THREAD
static void ltr55x_work_handler(struct k_work *work)
{
	struct ltr55x_data *data = CONTAINER_OF(work, struct ltr55x_data, work);

	ltr55x_handle_int(data->dev);
}
#endif

int ltr55x_attr_set(const struct device *dev, enum sensor_channel chan, enum sensor_attribute attr,
		    const struct sensor_value *val)
{
	const struct ltr55x_config *config = dev->config;
	struct ltr55x_data *data = dev->data;
	uint16_t value;
	uint16_t offset;
	uint8_t buf[2];
	uint8_t reg;
	int rc;

	if (config->part_id != LTR55X_PART_ID_VALUE) {
		return -ENOTSUP;
	}

	if (chan == SENSOR_CHAN_PROX) {
		if (attr == SENSOR_ATTR_UPPER_THRESH) {
			reg = LTR55X_PS_THRES_UP_0;
		} else if (attr == SENSOR_ATTR_LOWER_THRESH) {
			reg = LTR55X_PS_THRES_LOW_0;
		} else if (attr == SENSOR_ATTR_OFFSET) {
			reg = LTR55X_PS_OFFSET;
		} else {
			return -ENOTSUP;
		}

		if (val->val1 < 0 || val->val1 > LTR55X_PS_DATA_MAX) {
			return -EINVAL;
		}
		value = (uint16_t)val->val1;

		sys_put_le16(value, buf);
		rc = i2c_burst_write_dt(&config->bus, reg, buf, sizeof(buf));

		if (rc < 0) {
			return rc;
		}

		if (attr == SENSOR_ATTR_UPPER_THRESH) {
			data->ps_upper_threshold = value;
		} else if (attr == SENSOR_ATTR_LOWER_THRESH) {
			data->ps_lower_threshold = value;
		} else if (attr == SENSOR_ATTR_OFFSET) {
			data->ps_offset = value;
		}
	} else if (chan == SENSOR_CHAN_LIGHT) {
		if (attr == SENSOR_ATTR_UPPER_THRESH) {
			reg = LTR55X_ALS_THRES_UP_0;
		} else if (attr == SENSOR_ATTR_LOWER_THRESH) {
			reg = LTR55X_ALS_THRES_LOW_0;
		} else {
			return -ENOTSUP;
		}

		if (val->val1 < 0 || val->val1 > UINT16_MAX) {
			return -EINVAL;
		}
		threshold = (uint16_t)val->val1;
		sys_put_le16(threshold, buf);
		rc = i2c_burst_write_dt(&config->bus, reg, buf, sizeof(buf));

		if (rc < 0) {
			return rc;
		}

		if (attr == SENSOR_ATTR_UPPER_THRESH) {
			data->als_upper_threshold = threshold;
		} else {
			data->als_lower_threshold = threshold;
		}
	} else {
		return -ENOTSUP;
	}

	return 0;
}

int ltr55x_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
	const struct ltr55x_config *config = dev->config;
	struct ltr55x_data *data = dev->data;
	uint8_t interrupt = 0;
	int rc;

	if (config->int_gpio.port == NULL) {
		return -ENOTSUP;
	}

	if (config->part_id != LTR55X_PART_ID_VALUE) {
		return -ENOTSUP;
	}

	if (trig->type != SENSOR_TRIG_THRESHOLD) {
		return -ENOTSUP;
	}

	switch (trig->chan) {
	case SENSOR_CHAN_LIGHT:
		if (!config->als_interrupt) {
			return -EINVAL;
		}
		data->als_handler = handler;
		data->als_trigger = handler ? trig : NULL;
		break;
	case SENSOR_CHAN_PROX:
		if (!config->ps_interrupt) {
			return -EINVAL;
		}
		data->ps_handler = handler;
		data->ps_trigger = handler ? trig : NULL;
		break;
	default:
		return -ENOTSUP;
	}

	rc = gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_DISABLE);
	if (rc < 0) {
		return rc;
	}

	if ((data->ps_handler != NULL) && config->ps_interrupt) {
		interrupt |= LTR55X_INTERRUPT_PS_MASK;
	}
	if ((data->als_handler != NULL) && config->als_interrupt) {
		interrupt |= LTR55X_INTERRUPT_ALS_MASK;
	}

	rc = i2c_reg_update_byte_dt(&config->bus, LTR55X_INTERRUPT, LTR55X_INTERRUPT_PS_MASK | LTR55X_INTERRUPT_ALS_MASK, interrupt);
	if (rc < 0) {
		return rc;
	}

	if (interrupt & (LTR55X_INTERRUPT_PS_MASK | LTR55X_INTERRUPT_ALS_MASK)) {
		rc = gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
		if (rc < 0) {
			return rc;
		}

		if (gpio_pin_get_dt(&config->int_gpio) > 0) {
			ltr55x_handle_cb(data);
		}
	}

	return 0;
}

int ltr55x_trigger_init(const struct device *dev)
{
	const struct ltr55x_config *config = dev->config;
	struct ltr55x_data *data = dev->data;
	int rc;

	data->dev = dev;

	if (config->int_gpio.port == NULL) {
		LOG_DBG("instance '%s' doesn't support trigger mode", dev->name);
		return 0;
	}

	/* Get the GPIO device */
	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_ERR("%s: device %s is not ready", dev->name,
				config->int_gpio.port->name);
		return -ENODEV;
	}

#if defined(CONFIG_LTR55X_TRIGGER_OWN_THREAD)
	k_sem_init(&data->trig_sem, 0, K_SEM_MAX_LIMIT);
	k_thread_create(&data->thread, data->thread_stack,
			CONFIG_LTR55X_THREAD_STACK_SIZE,
			ltr55x_thread_main,
			data, NULL, NULL,
			K_PRIO_COOP(CONFIG_LTR55X_THREAD_PRIORITY),
			0, K_NO_WAIT);
	k_thread_name_set(&data->thread, "ltr55x_trigger");
#elif defined(CONFIG_LTR55X_TRIGGER_GLOBAL_THREAD)
	k_work_init(&data->work, ltr55x_work_handler);
#endif

	rc = 
	gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
	if (rc < 0) {
		LOG_ERR("Failed to configure interrupt pin");
		return rc;
	}

	gpio_init_callback(&data->gpio_cb, ltr55x_gpio_callback,
			   BIT(config->int_gpio.pin));

	if (gpio_add_callback(config->int_gpio.port, &data->gpio_cb) < 0) {
		LOG_ERR("Failed to set gpio callback");
		return -EIO;
	}

	rc = 
	gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (rc < 0) {
		LOG_ERR("Failed to enable interrupt pin");
		return rc;
	}

	if (gpio_pin_get_dt(&config->int_gpio) > 0) {
		ltr55x_handle_cb(data);
	}

	return 0;
}
