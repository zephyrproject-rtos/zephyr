/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adxl38x.h"
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(ADXL38X);

/**
 * @brief Select the INT_MAP0 register for the routed interrupt pin
 *
 * @param data Driver data pointer
 * @return uint8_t INT0_MAP0 or INT1_MAP0 register address
 */
static inline uint8_t adxl38x_int_map0_reg(const struct adxl38x_data *data)
{
	return data->use_int1 ? ADXL38X_INT1_MAP0 : ADXL38X_INT0_MAP0;
}

/**
 * @brief Select the INT_MAP1 register for the routed interrupt pin
 *
 * @param data Driver data pointer
 * @return uint8_t INT0_MAP1 or INT1_MAP1 register address
 */
static inline uint8_t adxl38x_int_map1_reg(const struct adxl38x_data *data)
{
	return data->use_int1 ? ADXL38X_INT1_MAP1 : ADXL38X_INT0_MAP1;
}

#if defined(CONFIG_ADXL38X_TRIGGER_OWN_THREAD) || defined(CONFIG_ADXL38X_TRIGGER_GLOBAL_THREAD)
/**
 * @brief Service data-ready and activity triggers
 *
 * @param dev Device pointer
 */
static void adxl38x_thread_cb(const struct device *dev)
{
	const struct adxl38x_dev_config *cfg = dev->config;
	struct adxl38x_data *data = dev->data;
	uint8_t status1;
	uint8_t status3;
	int ret;

	if (data->drdy_handler != NULL) {
		ret = adxl38x_reg_read(dev, ADXL38X_STATUS3, &status3, 1);
		if (ret == 0 && (status3 & ADXL38X_STATUS3_DATA_READY_MSK)) {
			data->drdy_handler(dev, data->drdy_trigger);
		}
	}

	if (data->act_handler != NULL) {
		ret = adxl38x_reg_read(dev, ADXL38X_STATUS1, &status1, 1);
		if (ret == 0 && (status1 & ADXL38X_STATUS1_ACT_MSK)) {
			data->act_handler(dev, data->act_trigger);
		}
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->interrupt_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret) {
		LOG_ERR("Failed to re-enable interrupt: %d", ret);
	}
}
#endif

/**
 * @brief GPIO interrupt callback
 *
 * @param dev GPIO device pointer
 * @param cb GPIO callback pointer
 * @param pins Mask of triggered pins
 */
static void adxl38x_gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	int ret;
	struct adxl38x_data *data = CONTAINER_OF(cb, struct adxl38x_data, gpio_cb);
	const struct adxl38x_dev_config *cfg = data->dev->config;

	ret = gpio_pin_interrupt_configure_dt(&cfg->interrupt_gpio, GPIO_INT_DISABLE);
	if (ret) {
		LOG_ERR("Failed to disable interrupt: %d", ret);
		return;
	}

#ifdef CONFIG_ADXL38X_STREAM
	adxl38x_stream_irq_handler(data->dev);
#endif

#if defined(CONFIG_ADXL38X_TRIGGER_OWN_THREAD)
	k_sem_give(&data->gpio_sem);
#elif defined(CONFIG_ADXL38X_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

#if defined(CONFIG_ADXL38X_TRIGGER_OWN_THREAD)
/**
 * @brief Trigger handling thread
 *
 * @param p1 Device data pointer
 * @param p2 Unused
 * @param p3 Unused
 */
static void adxl38x_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct adxl38x_data *data = p1;

	while (true) {
		k_sem_take(&data->gpio_sem, K_FOREVER);
		adxl38x_thread_cb(data->dev);
	}
}
#elif defined(CONFIG_ADXL38X_TRIGGER_GLOBAL_THREAD)
/**
 * @brief Trigger handling work callback
 *
 * @param work Work item pointer
 */
static void adxl38x_work_cb(struct k_work *work)
{
	struct adxl38x_data *data = CONTAINER_OF(work, struct adxl38x_data, work);

	adxl38x_thread_cb(data->dev);
}
#endif

/**
 * @brief Set ADXL38X trigger
 *
 * @param dev Device pointer
 * @param trig Trigger pointer
 * @param handler Trigger handler
 * @return int 0 on success, negative error code otherwise
 */
int adxl38x_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	const struct adxl38x_dev_config *cfg = dev->config;
	struct adxl38x_data *data = dev->data;
	enum adxl38x_op_mode saved_mode = data->op_mode;
	uint8_t int_mask;
	uint8_t int_en = 1;
	int ret;

	/* Resolve and validate the trigger before changing any device state. */
	switch (trig->type) {
	case SENSOR_TRIG_DATA_READY:
		data->drdy_handler = handler;
		data->drdy_trigger = trig;
		int_mask = ADXL38X_INT_MAP0_DATA_RDY_MSK;
		break;
	case SENSOR_TRIG_MOTION:
		data->act_handler = handler;
		data->act_trigger = trig;
		int_mask = ADXL38X_INT_MAP0_ACT_MSK;
		break;
	default:
		LOG_ERR("Unsupported trigger type: %d", trig->type);
		return -ENOTSUP;
	}

	if (handler == NULL) {
		int_en = 0;
	}

	/* Interrupt config must be done in standby; restore the prior mode on exit. */
	ret = adxl38x_set_op_mode(dev, ADXL38X_STANDBY_MODE);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->interrupt_gpio, GPIO_INT_DISABLE);
	if (ret) {
		LOG_ERR("Failed to disable interrupt: %d", ret);
		goto restore;
	}

	ret = adxl38x_reg_update(dev, adxl38x_int_map0_reg(data), int_mask, int_en);
	if (ret) {
		LOG_ERR("Failed to update interrupt map: %d", ret);
		goto restore;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->interrupt_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret) {
		LOG_ERR("Failed to enable interrupt: %d", ret);
		goto restore;
	}

	return adxl38x_set_op_mode(dev, saved_mode);

restore:
	adxl38x_set_op_mode(dev, saved_mode); /* best-effort; report the original error */
	return ret;
}

/**
 * @brief Initialize ADXL38X interrupt
 *
 * @param dev Device pointer
 * @return int 0 on success, negative error code otherwise
 */
int adxl38x_init_interrupt(const struct device *dev)
{
	int ret;
	const struct adxl38x_dev_config *cfg = dev->config;
	struct adxl38x_data *data = dev->data;

	data->dev = dev;
	data->use_int1 = cfg->use_int1;

	if (!gpio_is_ready_dt(&cfg->interrupt_gpio)) {
		LOG_ERR("GPIO device not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&cfg->interrupt_gpio, GPIO_INPUT);
	if (ret != 0) {
		LOG_ERR("Failed to configure interrupt pin: %d", ret);
		return ret;
	}

	gpio_init_callback(&data->gpio_cb, adxl38x_gpio_callback, BIT(cfg->interrupt_gpio.pin));

	ret = gpio_add_callback(cfg->interrupt_gpio.port, &data->gpio_cb);
	if (ret != 0) {
		LOG_ERR("Failed to add GPIO callback: %d", ret);
		return ret;
	}

	/* Clear the reset-default NVM routing on the pin we drive so it is not
	 * asserted spuriously (INT0_MAP0 resets to 0x80, INT1_MAP1 to 0x80).
	 */
	uint8_t clear = 0;

	ret = adxl38x_reg_write(dev, adxl38x_int_map0_reg(data), &clear, 1);
	if (ret != 0) {
		return ret;
	}
	ret = adxl38x_reg_write(dev, adxl38x_int_map1_reg(data), &clear, 1);
	if (ret != 0) {
		return ret;
	}

#if defined(CONFIG_ADXL38X_STREAM)
	ret = adxl38x_reg_update(dev, adxl38x_int_map0_reg(data),
				 ADXL38X_INT_MAP0_FIFO_WATERMARK_MSK, 1);
	if (ret != 0) {
		LOG_ERR("Failed to map FIFO watermark interrupt: %d", ret);
		return ret;
	}
#endif

#if defined(CONFIG_ADXL38X_TRIGGER_OWN_THREAD)
	k_sem_init(&data->gpio_sem, 0, K_SEM_MAX_LIMIT);
	k_thread_create(&data->thread, data->thread_stack, CONFIG_ADXL38X_THREAD_STACK_SIZE,
			adxl38x_thread, data, NULL, NULL,
			K_PRIO_COOP(CONFIG_ADXL38X_THREAD_PRIORITY), 0, K_NO_WAIT);
	k_thread_name_set(&data->thread, dev->name);
#elif defined(CONFIG_ADXL38X_TRIGGER_GLOBAL_THREAD)
	data->work.handler = adxl38x_work_cb;
#endif

	ret = gpio_pin_interrupt_configure_dt(&cfg->interrupt_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret) {
		LOG_ERR("Failed to enable interrupt: %d", ret);
		return ret;
	}

	LOG_INF("ADXL38X interrupt initialized on INT%d", data->use_int1 ? 1 : 0);
	return 0;
}
