/*
 * Copyright (c) 2025 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "max30210.h"
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(MAX30210, CONFIG_SENSOR_LOG_LEVEL);
#if defined(CONFIG_MAX30210_TRIGGER_OWN_THREAD) || defined(CONFIG_MAX30210_TRIGGER_GLOBAL_THREAD)
/**
 * @brief MAX30210 interrupt handling thread callback
 *
 * @param dev Device Pointer
 */
static int max30210_thread_cb(const struct device *dev)
{
	const struct max30210_config *config = dev->config;
	struct max30210_data *data = dev->data;
	int ret;
	uint8_t status;

	/*Clear the Status*/
	ret = max30210_reg_read(dev, STATUS, &status, 1);
	if (ret < 0) {
		LOG_ERR("Failed to read STATUS register: %d\n", ret);
		return -EFAULT;
	}

	if (data->temp_hi_handler && (status & TEMP_HI_MASK)) {
		data->temp_hi_handler(dev, data->temp_hi_trigger);
	}
	if (data->temp_lo_handler && (status & TEMP_LO_MASK)) {
		data->temp_lo_handler(dev, data->temp_lo_trigger);
	}
	if (data->temp_inc_fast_handler && (status & TEMP_INC_FAST_MASK)) {
		data->temp_inc_fast_handler(dev, data->temp_inc_fast_trigger);
	}
	if (data->temp_dec_fast_handler && (status & TEMP_DEC_FAST_MASK)) {
		data->temp_dec_fast_handler(dev, data->temp_dec_fast_trigger);
	}
	if (data->temp_rdy_handler && (status & TEMP_RDY_MASK)) {
		data->temp_rdy_handler(dev, data->temp_rdy_trigger);
	}
	if (data->a_fifo_full_handler && (status & FIFO_FULL_MASK)) {
		data->a_fifo_full_handler(dev, data->a_fifo_full_trigger);
	}

	ret = gpio_pin_interrupt_configure_dt(&config->interrupt_gpio, GPIO_INT_EDGE_FALLING);
	if (ret < 0) {
		LOG_ERR("Failed to reconfigure GPIO interrupt: %d\n", ret);
		return -EFAULT;
	}
	return 0;
}

#endif /* CONFIG_MAX30210_TRIGGER_OWN_THREAD || CONFIG_MAX30210_TRIGGER_GLOBAL_THREAD */

/**
 * @brief MAX30210 GPIO interrupt callback
 *
 * @param dev Device Pointer
 * @param cb GPIO Callback Pointer
 * @param pins Pin Mask
 */
static void max30210_gpio_callback(const struct device *dev, struct gpio_callback *cb,
				   uint32_t pins)
{
	struct max30210_data *data = CONTAINER_OF(cb, struct max30210_data, gpio_cb);
	const struct max30210_config *config = data->dev->config;

	gpio_pin_interrupt_configure_dt(&config->interrupt_gpio, GPIO_INT_DISABLE);

#if defined(CONFIG_MAX30210_TRIGGER_OWN_THREAD)
	k_sem_give(&data->gpio_sem);
#elif defined(CONFIG_MAX30210_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

#if defined(CONFIG_MAX30210_TRIGGER_OWN_THREAD)
static void max30210_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct max30210_data *data = p1;
	const struct device *dev = data->dev;
	int ret;

	while (1) {
		k_sem_take(&data->gpio_sem, K_FOREVER);
		ret = max30210_thread_cb(dev);
		if (ret < 0) {
			LOG_ERR("Error in interrupt handling thread: %d\n", ret);
		}
	}
}
#elif defined(CONFIG_MAX30210_TRIGGER_GLOBAL_THREAD)
static void max30210_work_cb(struct k_work *work)
{
	struct max30210_data *data = CONTAINER_OF(work, struct max30210_data, work);
	const struct device *dev = data->dev;
	int ret;

	ret = max30210_thread_cb(dev);
	if (ret < 0) {
		LOG_ERR("Error in interrupt handling work: %d\n", ret);
	}
}
#endif

/**
 * @brief Set MAX30210 sensor trigger
 *
 * @param dev Device Pointer
 * @param trig Trigger Pointer
 * @param handler Trigger Handler
 * @return int 0 on success, negative error code on failure
 */
int max30210_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler)
{
	const struct max30210_config *config = dev->config;
	struct max30210_data *data = dev->data;
	int ret = 0;
	uint8_t int_mask = 0, int_en;
	uint8_t status;

	if (!config->interrupt_gpio.port) {
		LOG_ERR("Interrupt GPIO not configured\n");
		return -ENODEV;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->interrupt_gpio, GPIO_INT_DISABLE);
	if (ret < 0) {
		LOG_ERR("Failed to disable GPIO interrupt: %d\n", ret);
		return ret;
	}

	switch ((int)trig->type) {
	case SENSOR_TRIG_THRESHOLD: {
		uint8_t temp_hi_setup[2];

		max30210_reg_read(dev, TEMP_ALARM_HIGH_MSB, temp_hi_setup, 2);
		data->temp_alarm_high_setup = (temp_hi_setup[0] << 8) | temp_hi_setup[1];

		if (data->temp_alarm_high_setup >= 0x7FFF) {
			LOG_ERR("Temperature high threshold not set\n");
		}

		else {
			data->temp_hi_handler = handler;
			data->temp_hi_trigger = trig;
			int_mask |= TEMP_HI_MASK;
		}

		uint8_t temp_lo_setup[2];

		max30210_reg_read(dev, TEMP_ALARM_LOW_MSB, temp_lo_setup, 2);
		data->temp_alarm_low_setup = (temp_lo_setup[0] << 8) | temp_lo_setup[1];
		if (data->temp_alarm_low_setup == 0x8000) {
			LOG_ERR("Temperature low threshold not set\n");
		}

		else {
			data->temp_lo_handler = handler;
			data->temp_lo_trigger = trig;
			int_mask |= TEMP_LO_MASK;
		}
		break;
	}

	case SENSOR_TRIG_TEMP_INC_FAST: {
		uint8_t temp_inc_fast_thresh;

		ret = max30210_reg_read(dev, TEMP_INC_FAST_THRESH, &temp_inc_fast_thresh, 1);
		if (ret < 0) {
			LOG_ERR("Failed to read TEMP_INC_FAST_THRESH: %d\n", ret);
			return ret;
		}
		data->temp_inc_fast_thresh = temp_inc_fast_thresh;
		if (data->temp_inc_fast_thresh == 0) {
			LOG_ERR("Temperature increase fast threshold not set\n");
			return -EINVAL;
		}

		else {
			data->temp_inc_fast_handler = handler;
			data->temp_inc_fast_trigger = trig;
			int_mask |= TEMP_INC_FAST_MASK;
		}
		break;
	}

	case SENSOR_TRIG_TEMP_DEC_FAST: {
		uint8_t temp_dec_fast_thresh;

		ret = max30210_reg_read(dev, TEMP_DEC_FAST_THRESH, &temp_dec_fast_thresh, 1);
		if (ret < 0) {
			LOG_ERR("Failed to read TEMP_DEC_FAST_THRESH: %d\n", ret);
			return ret;
		}
		data->temp_dec_fast_thresh = temp_dec_fast_thresh;
		if (data->temp_dec_fast_thresh == 0) {
			LOG_ERR("Temperature decrease fast threshold not set\n");
			return -EINVAL;
		}

		else {
			data->temp_dec_fast_handler = handler;
			data->temp_dec_fast_trigger = trig;
			int_mask |= TEMP_DEC_FAST_MASK;
		}
		break;
	}

	case SENSOR_TRIG_DATA_READY:
		data->temp_rdy_handler = handler;
		data->temp_rdy_trigger = trig;
		int_mask |= TEMP_RDY_MASK;
		break;

	case SENSOR_TRIG_FIFO_FULL:
		data->a_fifo_full_handler = handler;
		data->a_fifo_full_trigger = trig;
		int_mask |= FIFO_FULL_MASK;
		break;
	default:
		LOG_ERR("Unsupported trigger type: %d\n", trig->type);
		return -ENOTSUP;
	}
	if (handler) {
		int_en = 1;
	} else {
		int_en = 0;
	}

	if (sys_count_bits(&int_mask, sizeof(int_mask)) > 1) {
		int_en = 0x3;
	}

	ret = max30210_reg_update(dev, INTERRUPT_ENABLE, int_mask, int_en);
	if (ret < 0) {
		LOG_ERR("Failed to update interrupt enable register: %d\n", ret);
		return ret;
	}
	/* Clear Status */
	ret = max30210_reg_read(dev, STATUS, &status, 1);
	if (ret < 0) {
		LOG_ERR("Failed to read STATUS register: %d\n", ret);
		return ret;
	}
	ret = gpio_pin_interrupt_configure_dt(&config->interrupt_gpio, GPIO_INT_EDGE_FALLING);
	if (ret < 0) {
		LOG_ERR("Failed to configure GPIO interrupt: %d\n", ret);
		return ret;
	}

	return 0;
}

/**
 * @brief Initialize MAX30210 interrupt
 *
 * @param dev Device Pointer
 * @return int 0 on success, negative error code on failure
 */
int max30210_init_interrupt(const struct device *dev)
{
	struct max30210_data *data = dev->data;
	const struct max30210_config *config = dev->config;
	int ret;

	if (!gpio_is_ready_dt(&config->interrupt_gpio)) {
		LOG_ERR("Interrupt GPIO not ready\n");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->interrupt_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure interrupt GPIO: %d\n", ret);
		return ret;
	}

	gpio_init_callback(&data->gpio_cb, max30210_gpio_callback, BIT(config->interrupt_gpio.pin));

	ret = gpio_add_callback(config->interrupt_gpio.port, &data->gpio_cb);
	if (ret < 0) {
		LOG_ERR("Failed to add GPIO callback: %d\n", ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->interrupt_gpio, GPIO_INT_EDGE_FALLING);
	if (ret < 0) {
		LOG_ERR("Failed to configure GPIO interrupt: %d\n", ret);
		return ret;
	}
	data->dev = dev;

#if defined(CONFIG_MAX30210_TRIGGER_OWN_THREAD)
	k_sem_init(&data->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&data->thread, data->thread_stack, CONFIG_MAX30210_THREAD_STACK_SIZE,
			(k_thread_entry_t)max30210_thread, data, NULL, NULL,
			K_PRIO_COOP(CONFIG_MAX30210_THREAD_PRIORITY), 0, K_NO_WAIT);

	k_thread_name_set(&data->thread, dev->name);
#elif defined(CONFIG_MAX30210_TRIGGER_GLOBAL_THREAD)
	data->work.handler = max30210_work_cb;
#endif
	return 0;
}
