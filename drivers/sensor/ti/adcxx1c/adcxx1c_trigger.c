/*
 * Copyright (c) 2024 Bert Outtier
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/sensor.h>

#include "adcxx1c.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ADCXX1C, CONFIG_SENSOR_LOG_LEVEL);

static int adcxx1c_read_reg(const struct device *dev, uint8_t reg, uint8_t *out)
{
	const struct adcxx1c_config *cfg = dev->config;

	return i2c_reg_read_byte_dt(&cfg->bus, reg, out);
}

static int adcxx1c_write_regs(const struct device *dev, uint8_t reg, int16_t val)
{
	const struct adcxx1c_config *cfg = dev->config;
	uint8_t tx_buf[2];

	sys_put_be16(val, tx_buf);

	struct i2c_msg msgs[2] = {
		{
			.buf = &reg,
			.len = 1,
			.flags = I2C_MSG_WRITE,
		},
		{
			.buf = tx_buf,
			.len = sizeof(tx_buf),
			.flags = I2C_MSG_WRITE | I2C_MSG_STOP,
		},
	};

	return i2c_transfer_dt(&cfg->bus, msgs, 2);
}

int adcxx1c_attr_get(const struct device *dev, enum sensor_channel chan, enum sensor_attribute attr,
		     struct sensor_value *val)
{
	const struct adcxx1c_data *data = dev->data;
	int16_t sval;
	int ret;

	switch (attr) {
	case SENSOR_ATTR_LOWER_THRESH:
		ret = adcxx1c_read_regs(dev, ADCXX1C_LOW_LIMIT_ADDR, &sval);
		if (ret < 0) {
			return ret;
		}
		sval >>= (ADCXX1C_RES_12BITS - data->bits);
		break;
	case SENSOR_ATTR_UPPER_THRESH:
		ret = adcxx1c_read_regs(dev, ADCXX1C_HIGH_LIMIT_ADDR, &sval);
		if (ret < 0) {
			return ret;
		}
		sval >>= (ADCXX1C_RES_12BITS - data->bits);
		break;
	case SENSOR_ATTR_HYSTERESIS:
		ret = adcxx1c_read_regs(dev, ADCXX1C_HYSTERESIS_ADDR, &sval);
		if (ret < 0) {
			return ret;
		}
		sval >>= (ADCXX1C_RES_12BITS - data->bits);
		break;
	case SENSOR_ATTR_ALERT: {
		uint8_t conf;

		ret = adcxx1c_read_reg(dev, ADCXX1C_CONF_ADDR, &conf);
		if (ret < 0) {
			return ret;
		}
		sval = (conf & ADCXX1C_CONF_ALERT_FLAG_EN) ? 1 : 0;
		break;
	}
	default:
		LOG_ERR("ADCXX1C attribute not supported.");
		return -ENOTSUP;
	}

	val->val1 = sval;
	val->val2 = 0;

	return 0;
}

int adcxx1c_attr_set(const struct device *dev, enum sensor_channel chan, enum sensor_attribute attr,
		     const struct sensor_value *val)
{
	const struct adcxx1c_config *cfg = dev->config;
	struct adcxx1c_data *data = dev->data;
	int16_t sval = val->val1;

	switch (attr) {
	case SENSOR_ATTR_LOWER_THRESH:
		return adcxx1c_write_regs(dev, ADCXX1C_LOW_LIMIT_ADDR,
					  sval << (ADCXX1C_RES_12BITS - data->bits));
	case SENSOR_ATTR_UPPER_THRESH:
		return adcxx1c_write_regs(dev, ADCXX1C_HIGH_LIMIT_ADDR,
					  sval << (ADCXX1C_RES_12BITS - data->bits));
	case SENSOR_ATTR_HYSTERESIS:
		return adcxx1c_write_regs(dev, ADCXX1C_HYSTERESIS_ADDR,
					  sval << (ADCXX1C_RES_12BITS - data->bits));
	case SENSOR_ATTR_ALERT:
		data->conf = (cfg->cycle << 5);
		if (sval) {
			data->conf |= (ADCXX1C_CONF_ALERT_PIN_EN | ADCXX1C_CONF_ALERT_FLAG_EN);
		}
		return adcxx1c_write_reg(dev, ADCXX1C_CONF_ADDR, data->conf);
	default:
		LOG_ERR("ADCXX1C attribute not supported.");
		return -ENOTSUP;
	}
}

static inline void setup_alert(const struct device *dev, bool enable)
{
	const struct adcxx1c_config *cfg = dev->config;
	unsigned int flags = enable ? GPIO_INT_EDGE_TO_ACTIVE : GPIO_INT_DISABLE;

	gpio_pin_interrupt_configure_dt(&cfg->alert_gpio, flags);
}

static inline void handle_alert(const struct device *dev)
{
	setup_alert(dev, false);

#if defined(CONFIG_ADCXX1C_TRIGGER_OWN_THREAD)
	struct adcxx1c_data *data = dev->data;

	k_sem_give(&data->gpio_sem);
#elif defined(CONFIG_ADCXX1C_TRIGGER_GLOBAL_THREAD)
	struct adcxx1c_data *data = dev->data;

	k_work_submit(&data->work);
#endif
}

int adcxx1c_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	struct adcxx1c_data *data = dev->data;
	const struct adcxx1c_config *cfg = dev->config;

	setup_alert(dev, false);

	if (trig->type != SENSOR_TRIG_THRESHOLD) {
		return -ENOTSUP;
	}

	data->handler = handler;
	if (handler == NULL) {
		return 0;
	}

	data->trigger = trig;

	setup_alert(dev, true);

	/* If ALERT is active we probably won't get the rising edge,
	 * so invoke the callback manually.
	 */
	if (gpio_pin_get_dt(&cfg->alert_gpio)) {
		handle_alert(dev);
	}

	return 0;
}

static void adcxx1c_gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct adcxx1c_data *data = CONTAINER_OF(cb, struct adcxx1c_data, alert_cb);

	handle_alert(data->dev);
}

static void adcxx1c_thread_cb(const struct device *dev)
{
	struct adcxx1c_data *data = dev->data;

	if (data->handler != NULL) {
		data->handler(dev, data->trigger);
	}

	setup_alert(dev, true);
}

#ifdef CONFIG_ADCXX1C_TRIGGER_OWN_THREAD
static void adcxx1c_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct adcxx1c_data *data = p1;

	while (1) {
		k_sem_take(&data->gpio_sem, K_FOREVER);
		adcxx1c_thread_cb(data->dev);
	}
}
#endif

#ifdef CONFIG_ADCXX1C_TRIGGER_GLOBAL_THREAD
static void adcxx1c_work_cb(struct k_work *work)
{
	struct adcxx1c_data *data = CONTAINER_OF(work, struct adcxx1c_data, work);

	adcxx1c_thread_cb(data->dev);
}
#endif

int adcxx1c_init_interrupt(const struct device *dev)
{
	struct adcxx1c_data *data = dev->data;
	const struct adcxx1c_config *cfg = dev->config;
	int rc;

	if (!gpio_is_ready_dt(&cfg->alert_gpio)) {
		LOG_ERR("GPIO device not ready");
		return -ENODEV;
	}

	rc = gpio_pin_configure_dt(&cfg->alert_gpio, GPIO_INPUT);
	if (rc != 0) {
		LOG_ERR("Failed to configure alert pin %u!", cfg->alert_gpio.pin);
		return -EIO;
	}

	gpio_init_callback(&data->alert_cb, adcxx1c_gpio_callback, BIT(cfg->alert_gpio.pin));
	rc = gpio_add_callback(cfg->alert_gpio.port, &data->alert_cb);
	if (rc < 0) {
		LOG_ERR("Failed to set gpio callback!");
		return -EIO;
	}

	/* clear alert parameters */
	if (adcxx1c_write_regs(dev, ADCXX1C_LOW_LIMIT_ADDR, 0) < 0) {
		LOG_ERR("Failed to clear low limit");
		return -EIO;
	}

	if (adcxx1c_write_regs(dev, ADCXX1C_HIGH_LIMIT_ADDR, 0) < 0) {
		LOG_ERR("Failed to clear high limit");
		return -EIO;
	}

	if (adcxx1c_write_regs(dev, ADCXX1C_HYSTERESIS_ADDR, 0) < 0) {
		LOG_ERR("Failed to clear hysteresis");
		return -EIO;
	}

#if defined(CONFIG_ADCXX1C_TRIGGER_OWN_THREAD)
	k_sem_init(&data->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&data->thread, data->thread_stack, CONFIG_ADCXX1C_THREAD_STACK_SIZE,
			adcxx1c_thread, data, NULL, NULL,
			K_PRIO_COOP(CONFIG_ADCXX1C_THREAD_PRIORITY), 0, K_NO_WAIT);
#elif defined(CONFIG_ADCXX1C_TRIGGER_GLOBAL_THREAD)
	data->work.handler = adcxx1c_work_cb;
#endif

	return 0;
}
