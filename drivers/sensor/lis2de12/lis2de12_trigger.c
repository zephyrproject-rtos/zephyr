/* ST Microelectronics LIS2DE12 3-axis accelerometer sensor driver
 *
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lis2de12.pdf
 */

#define DT_DRV_COMPAT st_lis2de12

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "lis2de12.h"

LOG_MODULE_DECLARE(LIS2DE12, CONFIG_SENSOR_LOG_LEVEL);

/**
 * lis2de12_enable_xl_int - XL enable selected int pin to generate interrupt
 */
static int lis2de12_enable_xl_int(const struct device *dev, int enable)
{
	const struct lis2de12_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lis2de12_ctrl_reg3_t val = {0};
	int ret;

	if (enable) {
		int16_t xl_data[3];

		/* dummy read: re-trigger interrupt */
		lis2de12_acceleration_raw_get(ctx, xl_data);
	}

	/* set interrupt */
	ret = lis2de12_pin_int1_config_get(ctx, &val);
	if (ret < 0) {
		LOG_ERR("pint_int1_route_get error");
		return ret;
	}

	val.i1_zyxda = 1;

	return lis2de12_pin_int1_config_set(ctx, &val);
}

/**
 * lis2de12_trigger_set - link external trigger to event data ready
 */
int lis2de12_trigger_set(const struct device *dev,
			 const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler)
{
	const struct lis2de12_config *cfg = dev->config;
	struct lis2de12_data *lis2de12 = dev->data;

	if (!cfg->trig_enabled) {
		LOG_ERR("trigger_set op not supported");
		return -ENOTSUP;
	}

	switch (trig->chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		lis2de12->handler_drdy_acc = handler;
		lis2de12->trig_drdy_acc = trig;
		if (handler) {
			return lis2de12_enable_xl_int(dev, LIS2DE12_EN_BIT);
		}

		return lis2de12_enable_xl_int(dev, LIS2DE12_DIS_BIT);

	default:
		return -ENOTSUP;
	}

}

/**
 * lis2de12_handle_interrupt - handle the drdy event
 * read data and call handler if registered any
 */
static void lis2de12_handle_interrupt(const struct device *dev)
{
	struct lis2de12_data *lis2de12 = dev->data;
	const struct lis2de12_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lis2de12_status_reg_t status;

	while (1) {
		if (lis2de12_status_get(ctx, &status) < 0) {
			LOG_ERR("failed reading status reg");
			return;
		}

		if (status.zyxda == 0) {
			/* spurious interrupt */
			break;
		}

		if ((status.zyxda) && (lis2de12->handler_drdy_acc != NULL)) {
			lis2de12->handler_drdy_acc(dev, lis2de12->trig_drdy_acc);
		}
	}

	gpio_pin_interrupt_configure_dt(lis2de12->drdy_gpio,
					GPIO_INT_EDGE_TO_ACTIVE);
}

static void lis2de12_gpio_callback(const struct device *dev,
				   struct gpio_callback *cb, uint32_t pins)
{
	struct lis2de12_data *lis2de12 =
		CONTAINER_OF(cb, struct lis2de12_data, gpio_cb);

	ARG_UNUSED(pins);

	gpio_pin_interrupt_configure_dt(lis2de12->drdy_gpio, GPIO_INT_DISABLE);

#if defined(CONFIG_LIS2DE12_TRIGGER_OWN_THREAD)
	k_sem_give(&lis2de12->gpio_sem);
#elif defined(CONFIG_LIS2DE12_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&lis2de12->work);
#endif /* CONFIG_LIS2DE12_TRIGGER_OWN_THREAD */
}

#ifdef CONFIG_LIS2DE12_TRIGGER_OWN_THREAD
static void lis2de12_thread(struct lis2de12_data *lis2de12)
{
	while (1) {
		k_sem_take(&lis2de12->gpio_sem, K_FOREVER);
		lis2de12_handle_interrupt(lis2de12->dev);
	}
}
#endif /* CONFIG_LIS2DE12_TRIGGER_OWN_THREAD */

#ifdef CONFIG_LIS2DE12_TRIGGER_GLOBAL_THREAD
static void lis2de12_work_cb(struct k_work *work)
{
	struct lis2de12_data *lis2de12 =
		CONTAINER_OF(work, struct lis2de12_data, work);

	lis2de12_handle_interrupt(lis2de12->dev);
}
#endif /* CONFIG_LIS2DE12_TRIGGER_GLOBAL_THREAD */

int lis2de12_init_interrupt(const struct device *dev)
{
	struct lis2de12_data *lis2de12 = dev->data;
	const struct lis2de12_config *cfg = dev->config;
	int ret;

	lis2de12->drdy_gpio = (struct gpio_dt_spec *)&cfg->int1_gpio;

	/* setup data ready gpio interrupt */
	if (!gpio_is_ready_dt(lis2de12->drdy_gpio)) {
		LOG_ERR("Cannot get pointer to drdy_gpio device (%p)",
			lis2de12->drdy_gpio);
		return -EINVAL;
	}

#if defined(CONFIG_LIS2DE12_TRIGGER_OWN_THREAD)
	k_sem_init(&lis2de12->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&lis2de12->thread, lis2de12->thread_stack,
			CONFIG_LIS2DE12_THREAD_STACK_SIZE,
			(k_thread_entry_t)lis2de12_thread, lis2de12,
			NULL, NULL, K_PRIO_COOP(CONFIG_LIS2DE12_THREAD_PRIORITY),
			0, K_NO_WAIT);
	k_thread_name_set(&lis2de12->thread, dev->name);
#elif defined(CONFIG_LIS2DE12_TRIGGER_GLOBAL_THREAD)
	lis2de12->work.handler = lis2de12_work_cb;
#endif /* CONFIG_LIS2DE12_TRIGGER_OWN_THREAD */

	ret = gpio_pin_configure_dt(lis2de12->drdy_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure gpio: %d", ret);
		return ret;
	}

	gpio_init_callback(&lis2de12->gpio_cb,
			   lis2de12_gpio_callback,
			   BIT(lis2de12->drdy_gpio->pin));

	if (gpio_add_callback(lis2de12->drdy_gpio->port, &lis2de12->gpio_cb) < 0) {
		LOG_ERR("Could not set gpio callback");
		return -EIO;
	}

	return gpio_pin_interrupt_configure_dt(lis2de12->drdy_gpio,
					       GPIO_INT_EDGE_TO_ACTIVE);
}
