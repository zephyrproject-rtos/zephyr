/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adxl355.h"
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(ADXL355);

#if defined(CONFIG_ADXL355_TRIGGER_OWN_THREAD) || defined(CONFIG_ADXL355_TRIGGER_GLOBAL_THREAD)
/**
 * @brief ADXL355 interrupt handling thread callback
 *
 * @param dev Device pointer
 */
static void adxl355_thread_cb(const struct device *dev)
{
	const struct adxl355_dev_config *cfg = dev->config;
	struct adxl355_data *data = dev->data;
	uint8_t status;
	int ret;

	/* Clear the status */
	ret = adxl355_reg_read(dev, ADXL355_STATUS, &status, 1);
	if (ret < 0) {
		LOG_ERR("Failed to read status register: %d", ret);
		return;
	}

	if ((data->drdy_handler != NULL) && (status & ADXL355_STATUS_DATA_RDY_MSK)) {
		data->drdy_handler(dev, data->drdy_trigger);
	}

	if ((data->act_handler != NULL) && (status & ADXL355_STATUS_ACTIVITY_MSK)) {
		data->act_handler(dev, data->act_trigger);
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->interrupt_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret) {
		LOG_ERR("Failed to enable interrupt: %d", ret);
		return;
	}
}
#endif
/**
 * @brief ADXL355 GPIO callback
 *
 * @param dev Device pointer
 * @param cb GPIO callback pointer
 * @param pins Pins
 */
static void adxl355_gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	int ret;
	struct adxl355_data *data = CONTAINER_OF(cb, struct adxl355_data, gpio_cb);
	const struct adxl355_dev_config *cfg = data->dev->config;

	ret = gpio_pin_interrupt_configure_dt(&cfg->interrupt_gpio, GPIO_INT_DISABLE);
	if (ret) {
		LOG_ERR("Failed to disable interrupt: %d", ret);
		return;
	}
#ifdef CONFIG_ADXL355_TRIGGER_OWN_THREAD
#ifdef CONFIG_ADXL355_STREAM
	adxl355_stream_irq_handler(data->dev);
#endif /* CONFIG_ADXL355_STREAM */
#endif /* !CONFIG_ADXL355_TRIGGER_OWN_THREAD */

#if defined(CONFIG_ADXL355_TRIGGER_OWN_THREAD)
	k_sem_give(&data->gpio_sem);
#elif defined(CONFIG_ADXL355_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

#if defined(CONFIG_ADXL355_TRIGGER_OWN_THREAD)
/**
 * @brief ADXL355 interrupt handling thread
 *
 * @param p1 Context pointer of type struct adxl355_data
 * @param p2 Unused
 * @param p3 Unused
 */
static void adxl355_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct adxl355_data *data = p1;

	while (true) {
		k_sem_take(&data->gpio_sem, K_FOREVER);
		adxl355_thread_cb(data->dev);
	}
}
#elif defined(CONFIG_ADXL355_TRIGGER_GLOBAL_THREAD)
static void adxl355_work_cb(struct k_work *work)
{
	struct adxl355_data *data = CONTAINER_OF(work, struct adxl355_data, work);

	adxl355_thread_cb(data->dev);
}
#endif /* CONFIG_ADXL355_TRIGGER_OWN_THREAD || CONFIG_ADXL355_TRIGGER_GLOBAL_THREAD */

/**
 * @brief Set ADXL355 trigger
 *
 * @param dev Device pointer
 * @param trig Trigger pointer
 * @param handler Trigger handler
 * @return int 0 on success, negative error code otherwise
 */
int adxl355_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)

{
	const struct adxl355_dev_config *cfg = dev->config;
	struct adxl355_data *data = dev->data;
	uint8_t int_mask;
	uint8_t int_en;
	int ret;

	ret = adxl355_set_op_mode(dev, ADXL355_STANDBY);
	if (ret < 0) {
		LOG_ERR("Failed to set standby mode: %d", ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->interrupt_gpio, GPIO_INT_DISABLE);
	if (ret) {
		LOG_ERR("Failed to disable interrupt: %d", ret);
		return ret;
	}

	switch (trig->type) {
	case SENSOR_TRIG_DATA_READY:
		data->drdy_handler = handler;
		data->drdy_trigger = trig;
		/** Enabling DRDY means not using FIFO interrupts as both
		 * are served by reading data-register: two clients can't be
		 * served simultaneously.
		 */
		int_mask = ADXL355_INT_MAP_DATA_RDY_EN1_MSK | ADXL355_INT_MAP_FIFO_FULL_EN1_MSK |
			   ADXL355_INT_MAP_FIFO_OVR_EN1_MSK;
		int_en = 1;
		break;
	case SENSOR_TRIG_MOTION:
		data->act_handler = handler;
		data->act_trigger = trig;
		int_mask = ADXL355_INT_MAP_ACTIVITY_EN1_MSK;
		int_en = 1;
		break;
	default:
		LOG_ERR("Unsupported trigger type: %d", trig->type);
		return -ENOTSUP;
	}

	if (handler == NULL) {
		int_en = 0U;
	}

	if (cfg->route_to_int2) {
		int_mask = int_mask << 4;
	}

	ret = adxl355_reg_update(dev, ADXL355_INT_MAP, int_mask, int_en);
	if (ret) {
		LOG_ERR("Failed to update interrupt map: %d", ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->interrupt_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret) {
		LOG_ERR("Failed to enable interrupt: %d", ret);
		return ret;
	}

	ret = adxl355_set_op_mode(dev, ADXL355_MEASURE);
	if (ret < 0) {
		LOG_ERR("Failed to set measurement mode: %d", ret);
		return ret;
	}

	return 0;
}

/**
 * @brief Initialize ADXL355 interrupt
 *
 * @param dev Device pointer
 * @return int 0 on success, negative error code otherwise
 */
int adxl355_init_interrupt(const struct device *dev)
{
	int ret;
	const struct adxl355_dev_config *cfg = dev->config;
	struct adxl355_data *data = dev->data;

	if (!gpio_is_ready_dt(&cfg->interrupt_gpio)) {
		LOG_ERR("GPIO device not ready");
		return -ENODEV;
	}

	/* Configure Interrupt Pin */
	ret = gpio_pin_configure_dt(&cfg->interrupt_gpio, GPIO_INPUT);
	if (ret != 0) {
		LOG_ERR("Failed to configure interrupt pin: %d", ret);
		return ret;
	}

	gpio_init_callback(&data->gpio_cb, adxl355_gpio_callback, BIT(cfg->interrupt_gpio.pin));

	ret = gpio_add_callback(cfg->interrupt_gpio.port, &data->gpio_cb);
	if (ret != 0) {
		LOG_ERR("Failed to add GPIO callback: %d", ret);
		return ret;
	}

	data->dev = dev;
#if defined(CONFIG_ADXL355_STREAM)
	ret = adxl355_reg_update(dev, ADXL355_INT_MAP,
				 cfg->route_to_int2 ? ADXL355_INT_MAP_FIFO_FULL_EN2_MSK
						    : ADXL355_INT_MAP_FIFO_FULL_EN1_MSK,
				 1);
	if (ret != 0) {
		LOG_ERR("Failed to enable FIFO Full interrupt: %d", ret);
		return ret;
	}
#endif
#if defined(CONFIG_ADXL355_TRIGGER_OWN_THREAD)
	k_sem_init(&data->gpio_sem, 0, K_SEM_MAX_LIMIT);
	k_thread_create(&data->thread, data->thread_stack, CONFIG_ADXL355_THREAD_STACK_SIZE,
			adxl355_thread, data, NULL, NULL,
			K_PRIO_COOP(CONFIG_ADXL355_THREAD_PRIORITY), 0, K_NO_WAIT);
	k_thread_name_set(&data->thread, dev->name);
#elif defined(CONFIG_ADXL355_TRIGGER_GLOBAL_THREAD)
	data->work.handler = adxl355_work_cb;
#endif
	ret = gpio_pin_interrupt_configure_dt(&cfg->interrupt_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret) {
		LOG_ERR("Failed to enable interrupt: %d", ret);
		return ret;
	}
	LOG_INF("ADXL355 interrupt initialized");
	return 0;
}
