/*
 * Copyright (c) 2024 deveritec GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT vishay_vcnl36825t

#include "vcnl36825t.h"

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/check.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(VCNL36825T, CONFIG_SENSOR_LOG_LEVEL);

int vcnl36825t_trigger_attr_set(const struct device *dev, enum sensor_channel chan,
				enum sensor_attribute attr, const struct sensor_value *val)
{
	CHECKIF(dev == NULL) {
		return -EINVAL;
	}

	CHECKIF(val == NULL) {
		return -EINVAL;
	}

	const struct vcnl36825t_config *config = dev->config;
	int rc;
	uint16_t reg_addr;

	switch (attr) {
	case SENSOR_ATTR_UPPER_THRESH:
		reg_addr = VCNL36825T_REG_PS_THDH;
		break;
	case SENSOR_ATTR_LOWER_THRESH:
		reg_addr = VCNL36825T_REG_PS_THDL;
		break;
	default:
		LOG_ERR("unknown attribute %d", (int)attr);
		return -ENOTSUP;
	}

	rc = vcnl36825t_write(&config->i2c, reg_addr, (uint16_t)val->val1);
	if (rc < 0) {
		LOG_ERR("error writing attribute %d", attr);
		return rc;
	}

	return 0;
}

/**
 * @brief callback called if a GPIO-interrupt is registered
 *
 * @param port device struct for the GPIO device
 * @param cb @ref struct gpio_callback owning this handler
 * @param pins mask of pins that triggered the callback handler
 */
static void vcnl36825t_gpio_callback(const struct device *port, struct gpio_callback *cb,
				     uint32_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	CHECKIF(!cb) {
		LOG_ERR("cb: NULL");
		return;
	}
	struct vcnl36825t_data *data = CONTAINER_OF(cb, struct vcnl36825t_data, int_gpio_handler);
	int rc;

	rc = gpio_pin_interrupt_configure_dt(data->int_gpio, GPIO_INT_DISABLE);
	if (rc != 0) {
		LOG_ERR("error deactivating SoC interrupt %d", rc);
	}

#if CONFIG_VCNL36825T_TRIGGER_OWN_THREAD
	k_sem_give(&data->int_gpio_sem);
#elif CONFIG_VCNL36825T_TRIGGER_GLOBAL_THREAD
	k_work_submit(&data->int_work);
#endif
}

static void vcnl36825t_thread_cb(const struct device *dev)
{
	const struct vcnl36825t_config *config = dev->config;
	struct vcnl36825t_data *data = dev->data;
	int rc;
	uint16_t reg_value;

	if (data->int_handler != NULL) {
		data->int_handler(dev, data->int_trigger);
	}

	rc = gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_FALLING);
	if (rc < 0) {
		LOG_ERR("error activating SoC interrupt %d", rc);
		return;
	}

	rc = vcnl36825t_read(&config->i2c, VCNL36825T_REG_INT_FLAG, &reg_value);
	if (rc < 0) {
		LOG_ERR("error reading interrupt flag register %d", rc);
		return;
	}

	if (FIELD_GET(VCNL36825T_PS_IF_AWAY_MSK, reg_value) == 1) {
		LOG_INF("\"away\" trigger (PS below THDL)");
	} else if (FIELD_GET(VCNL36825T_PS_IF_CLOSE_MSK, reg_value) == 1) {
		LOG_INF("\"close\" trigger (PS above THDH)");
	} else if (FIELD_GET(VCNL36825T_PS_SPFLAG_MSK, reg_value) == 1) {
		LOG_INF("enter protection mode trigger");
	} else if (FIELD_GET(VCNL36825T_PS_ACFLAG_MSK, reg_value) == 1) {
		LOG_INF("finished auto calibration trigger");
	}
}

#if CONFIG_VCNL36825T_TRIGGER_OWN_THREAD
static void vcnl36825t_thread_main(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct vcnl36825t_data *data = p1;

	while (1) {
		k_sem_take(&data->int_gpio_sem, K_FOREVER);
		vcnl36825t_thread_cb(data->dev);
	}
}
#endif

#if CONFIG_VCNL36825T_TRIGGER_GLOBAL_THREAD
static void vcnl36825t_work_cb(struct k_work *work)
{
	struct vcnl36825t_data *data = CONTAINER_OF(work, struct vcnl36825t_data, int_work);

	vcnl36825t_thread_cb(data->dev);
}
#endif

int vcnl36825t_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			   sensor_trigger_handler_t handler)
{
	CHECKIF(dev == NULL) {
		LOG_ERR("dev: NULL");
		return -EINVAL;
	}

	CHECKIF(trig == NULL) {
		LOG_ERR("trig: NULL");
		return -EINVAL;
	}

	const struct vcnl36825t_config *config = dev->config;
	struct vcnl36825t_data *data = dev->data;

	int rc;
	uint16_t regdata;

	if (trig->chan != SENSOR_CHAN_PROX) {
		LOG_ERR("invalid channel %d", (int)trig->chan);
		return -ENOTSUP;
	}

	if (trig->type != SENSOR_TRIG_THRESHOLD) {
		LOG_ERR("invalid trigger type %d", (int)trig->type);
		return -ENOTSUP;
	}

	rc = gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_DISABLE);
	if (rc < 0) {
		LOG_ERR("error configuring SoC interrupt %d", rc);
		return rc;
	}

	data->int_trigger = trig;
	data->int_handler = handler;

	if (handler == NULL) {
		regdata = VCNL36825T_PS_INT_DISABLE;
	} else {
		switch (config->int_mode) {
		case VCNL36825T_INT_MODE_NORMAL:
			regdata = VCNL36825T_PS_INT_MODE_NORMAL;
			break;
		case VCNL36825T_INT_MODE_FIRST_HIGH:
			regdata = VCNL36825T_PS_INT_MODE_FIRST_HIGH;
			break;
		case VCNL36825T_INT_MODE_LOGIC_HIGH_LOW:
			regdata = VCNL36825T_PS_INT_MODE_LOGIC_HIGH_LOW;
			break;
		default:
			LOG_ERR("unknown interrupt mode");
			return -ENOTSUP;
		}
	}

	rc = vcnl36825t_update(&config->i2c, VCNL36825T_REG_PS_CONF2, VCNL36825T_PS_INT_MSK,
			       regdata);
	if (rc < 0) {
		LOG_ERR("error updating interrupt %d", rc);
		return rc;
	}

	if (handler) {
		rc = gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_FALLING);
		if (rc < 0) {
			LOG_ERR("error configuring SoC interrupt %d", rc);
			return rc;
		}

		/* read interrupt register one time to clear any pending interrupt */
		rc = vcnl36825t_read(&config->i2c, VCNL36825T_REG_INT_FLAG, &regdata);
		if (rc < 0) {
			LOG_ERR("error clearing interrupt flag register %d", rc);
			return rc;
		}
	}

	return 0;
}

int vcnl36825t_trigger_init(const struct device *dev)
{
	CHECKIF(dev == NULL) {
		LOG_ERR("dev: NULL");
		return -EINVAL;
	}

	const struct vcnl36825t_config *config = dev->config;
	struct vcnl36825t_data *data = dev->data;

	int rc;
	uint8_t reg_value;

	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_ERR("interrupt GPIO not ready");
		return -ENODEV;
	}

	data->dev = dev;
	data->int_gpio = &config->int_gpio;

	rc = gpio_pin_configure_dt(data->int_gpio, GPIO_INPUT);
	if (rc < 0) {
		LOG_ERR("error setting interrupt gpio configuration %d", rc);
		return rc;
	}

	/* PS_CONF2 */
	reg_value = VCNL36825T_PS_INT_DISABLE;

	if (config->int_smart_persistence) {
		reg_value |= VCNL36825T_PS_SMART_PERS_ENABLED;
	}

	switch (config->int_proximity_count) {
	case 1:
		reg_value |= VCNL36825T_PS_PERS_1;
		break;
	case 2:
		reg_value |= VCNL36825T_PS_PERS_2;
		break;
	case 3:
		reg_value |= VCNL36825T_PS_PERS_3;
		break;
	case 4:
		__fallthrough;
	default:
		reg_value |= VCNL36825T_PS_PERS_4;
	}

	rc = vcnl36825t_update(
		&config->i2c, VCNL36825T_REG_PS_CONF2,
		(VCNL36825T_PS_SMART_PERS_MSK | VCNL36825T_PS_INT_MSK | VCNL36825T_PS_PERS_MSK),
		reg_value);
	if (rc < 0) {
		LOG_ERR("could not write interrupt configuration %d", rc);
		return rc;
	}

#if CONFIG_VCNL36825T_TRIGGER_OWN_THREAD
	k_sem_init(&data->int_gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&data->int_thread, data->int_thread_stack,
			CONFIG_VCNL36825T_THREAD_STACK_SIZE, vcnl36825t_thread_main, data, NULL,
			NULL, K_PRIO_COOP(CONFIG_VCNL36825T_THREAD_PRIORITY), 0, K_NO_WAIT);
#elif CONFIG_VCNL36825T_TRIGGER_GLOBAL_THREAD
	k_work_init(&data->int_work, vcnl36825t_work_cb);
#else
#error "invalid interrupt threading configuration"
#endif

	gpio_init_callback(&data->int_gpio_handler, vcnl36825t_gpio_callback,
			   BIT(config->int_gpio.pin));

	rc = gpio_add_callback(config->int_gpio.port, &data->int_gpio_handler);
	if (rc < 0) {
		LOG_ERR("could not set gpio callback %d", rc);
		return rc;
	}

	rc = gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_DISABLE);
	if (rc < 0) {
		LOG_ERR("could not set SoC interrupt configuration %d", rc);
		return rc;
	}

	return 0;
}
