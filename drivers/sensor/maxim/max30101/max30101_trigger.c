/*
 * Copyright (c) 2025, CATIE
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include "max30101.h"

LOG_MODULE_DECLARE(MAX30101, CONFIG_SENSOR_LOG_LEVEL);

#if CONFIG_MAX30101_TRIGGER_OWN_THREAD
K_THREAD_STACK_DEFINE(max30101_workqueue_stack, CONFIG_MAX30101_THREAD_SIZE);
static struct k_work_q max30101_workqueue;

static int max30101_workqueue_init(void)
{
	k_work_queue_init(&max30101_workqueue);
	k_work_queue_start(&max30101_workqueue, max30101_workqueue_stack,
			   K_THREAD_STACK_SIZEOF(max30101_workqueue_stack),
			   CONFIG_MAX30101_THREAD_PRIORITY, NULL);

	return 0;
}

/* The work-queue is shared across all instances, hence it is initialized separately */
SYS_INIT(max30101_workqueue_init, POST_KERNEL, CONFIG_I2C_INIT_PRIORITY);
#endif /* CONFIG_MAX30101_TRIGGER_OWN_THREAD */

static void max30101_gpio_callback_handler(const struct device *p_port, struct gpio_callback *p_cb,
					   uint32_t pins)
{
	ARG_UNUSED(p_port);
	ARG_UNUSED(pins);

	struct max30101_data *data = CONTAINER_OF(p_cb, struct max30101_data, gpio_cb);

	/* Using work queue to exit isr context */
#if CONFIG_MAX30101_TRIGGER_OWN_THREAD
	k_work_submit_to_queue(&max30101_workqueue, &data->cb_work);
#else
	k_work_submit(&data->cb_work);
#endif /* CONFIG_MAX30101_TRIGGER_OWN_THREAD */
}

static void max30101_work_cb(struct k_work *p_work)
{
	struct max30101_data *data = CONTAINER_OF(p_work, struct max30101_data, cb_work);
	const struct max30101_config *config = data->dev->config;
	uint8_t reg;

	/* Read INTERRUPT status */
	if (i2c_reg_read_byte_dt(&config->i2c, MAX30101_REG_INT_STS1, &reg)) {
		LOG_ERR("Trigger worker I2C read STS1 FLAGS error");
		return;
	}

	if ((reg & MAX30101_INT_FULL_MASK) &&
	    (data->trigger_handler[MAX30101_FULL_CB_INDEX] != NULL)) {
		data->trigger_handler[MAX30101_FULL_CB_INDEX](
			data->dev, data->trigger[MAX30101_FULL_CB_INDEX]);
	}
	if ((reg & MAX30101_INT_PPG_MASK) &&
	    (data->trigger_handler[MAX30101_PPG_CB_INDEX] != NULL)) {
		data->trigger_handler[MAX30101_PPG_CB_INDEX](data->dev,
							     data->trigger[MAX30101_PPG_CB_INDEX]);
	}
	if ((reg & MAX30101_INT_ALC_OVF_MASK) &&
	    (data->trigger_handler[MAX30101_ALC_CB_INDEX] != NULL)) {
		data->trigger_handler[MAX30101_ALC_CB_INDEX](data->dev,
							     data->trigger[MAX30101_ALC_CB_INDEX]);
	}

#if CONFIG_MAX30101_DIE_TEMPERATURE
	/* Read INTERRUPT status */
	if (i2c_reg_read_byte_dt(&config->i2c, MAX30101_REG_INT_STS2, &reg)) {
		LOG_ERR("Trigger worker I2C read STS2 FLAGS error");
		return;
	}

	if ((reg & MAX30101_INT_TEMP_MASK) &&
	    (data->trigger_handler[MAX30101_TEMP_CB_INDEX] != NULL)) {
		data->trigger_handler[MAX30101_TEMP_CB_INDEX](
			data->dev, data->trigger[MAX30101_TEMP_CB_INDEX]);
	}
#endif /* CONFIG_MAX30101_DIE_TEMPERATURE */
}

int max30101_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler)
{
	const struct max30101_config *config = dev->config;
	struct max30101_data *data = dev->data;
	uint8_t mask, index, enable = 0x00;

	switch (trig->type) {
	case SENSOR_TRIG_FIFO_WATERMARK:
		mask = MAX30101_INT_FULL_MASK;
		index = MAX30101_FULL_CB_INDEX;
		break;

	case SENSOR_TRIG_OVERFLOW:
		if (trig->chan == SENSOR_CHAN_AMBIENT_LIGHT) {
			mask = MAX30101_INT_ALC_OVF_MASK;
			index = MAX30101_ALC_CB_INDEX;
		} else {
			LOG_ERR("Only SENSOR_CHAN_AMBIENT_LIGHT is supported for overflow trigger");
			return -EINVAL;
		}
		break;

	case SENSOR_TRIG_DATA_READY:
		switch (trig->chan) {
		case SENSOR_CHAN_DIE_TEMP:
#if CONFIG_MAX30101_DIE_TEMPERATURE
			mask = MAX30101_INT_TEMP_MASK;
			index = MAX30101_TEMP_CB_INDEX;
#else
			LOG_ERR("SENSOR_CHAN_DIE_TEMP needs CONFIG_MAX30101_DIE_TEMPERATURE");
			return -EINVAL;
#endif /* CONFIG_MAX30101_DIE_TEMPERATURE */
			break;

		case SENSOR_CHAN_LIGHT:
		case SENSOR_CHAN_IR:
		case SENSOR_CHAN_RED:
		case SENSOR_CHAN_GREEN:
			mask = MAX30101_INT_PPG_MASK;
			index = MAX30101_PPG_CB_INDEX;
			break;

		default:
			LOG_ERR("Only SENSOR_CHAN_DIE_TEMP and SENSOR_CHAN_LIGHT/IR/RED/GREEN are "
				"supported for data ready trigger");
			return -EINVAL;
		}
		break;

	default:
		LOG_ERR("Unsupported trigger type");
		return -EINVAL;
	}

	if (handler != NULL) {
		enable = 0xFF;
	}

	/* Write the Interrupt enable register */
	LOG_DBG("Writing Interrupt enable register: [0x%02X][0x%02X]", mask, enable);
	if (i2c_reg_update_byte_dt(&config->i2c, MAX30101_REG_INT_EN1, mask, enable)) {
		LOG_ERR("Could not set interrupt enable register");
		return -EIO;
	}

#if CONFIG_MAX30101_DIE_TEMPERATURE
	if (i2c_reg_update_byte_dt(&config->i2c, MAX30101_REG_INT_EN2, mask, enable)) {
		LOG_ERR("Could not set interrupt enable register");
		return -EIO;
	}

	/* Start die temperature acquisition */
	if (i2c_reg_write_byte_dt(&config->i2c, MAX30101_REG_TEMP_CFG, 1)) {
		LOG_ERR("Could not start die temperature acquisition");
		return -EIO;
	}
#endif /* CONFIG_MAX30101_DIE_TEMPERATURE */

	/* CLEAR ALL INTERRUPT STATUS */
	uint8_t int_status;

	if (i2c_reg_read_byte_dt(&config->i2c, MAX30101_REG_INT_STS1, &int_status)) {
		LOG_ERR("Could not get interrupt STATUS register");
		return -EIO;
	}

	if (!!enable) {
		data->trigger_handler[index] = handler;
		data->trigger[index] = trig;
	}
	LOG_DBG("TRIGGER %sset [%d][%d]", !!enable ? "" : "un", trig->type, trig->chan);
	return 0;
}

int max30101_init_interrupts(const struct device *dev)
{
	const struct max30101_config *config = dev->config;
	struct max30101_data *data = dev->data;

	if (!gpio_is_ready_dt(&config->irq_gpio)) {
		LOG_ERR("GPIO is not ready");
		return -ENODEV;
	}

	if (gpio_pin_configure_dt(&config->irq_gpio, GPIO_INPUT)) {
		LOG_ERR("Failed to configure GPIO");
		return -EIO;
	}

	if (gpio_pin_interrupt_configure_dt(&config->irq_gpio, GPIO_INT_EDGE_TO_ACTIVE)) {
		LOG_ERR("Failed to configure interrupt");
		return -EIO;
	}

	gpio_init_callback(&data->gpio_cb, max30101_gpio_callback_handler,
			   BIT(config->irq_gpio.pin));

	if (gpio_add_callback_dt(&config->irq_gpio, &data->gpio_cb)) {
		LOG_ERR("Failed to add GPIO callback");
		return -EIO;
	}
	LOG_DBG("GPIO callback configured");

	data->dev = dev;
	memset(&(data->trigger_handler[0]), 0, sizeof(data->trigger_handler));
	memset(&(data->trigger[0]), 0, sizeof(data->trigger));
	k_work_init(&data->cb_work, max30101_work_cb);

	return 0;
}
