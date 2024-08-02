/* ST Microelectronics LIS2DU12 3-axis accelerometer sensor driver
 *
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lis2du12.pdf
 */

#define DT_DRV_COMPAT st_lis2du12

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "lis2du12.h"

LOG_MODULE_DECLARE(LIS2DU12, CONFIG_SENSOR_LOG_LEVEL);

/**
 * lis2du12_enable_xl_int - XL enable selected int pin to generate interrupt
 */
static int lis2du12_enable_xl_int(const struct device *dev, int enable)
{
	const struct lis2du12_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	int ret;

	if (enable) {
		lis2du12_md_t md;
		lis2du12_data_t xl_data;

		/* dummy read: re-trigger interrupt */
		md.fs = cfg->accel_range;
		lis2du12_data_get(ctx, &md, &xl_data);
	}

	/* set interrupt */
	if (cfg->drdy_pin == 1) {
		lis2du12_pin_int_route_t val;

		ret = lis2du12_pin_int1_route_get(ctx, &val);
		if (ret < 0) {
			LOG_ERR("pint_int1_route_get error");
			return ret;
		}

		val.drdy_xl = 1;

		ret = lis2du12_pin_int1_route_set(ctx, &val);
	} else {
		lis2du12_pin_int_route_t val;

		ret = lis2du12_pin_int2_route_get(ctx, &val);
		if (ret < 0) {
			LOG_ERR("pint_int2_route_get error");
			return ret;
		}

		val.drdy_xl = 1;

		ret = lis2du12_pin_int2_route_set(ctx, &val);
	}

	return ret;
}

/**
 * lis2du12_trigger_set - link external trigger to event data ready
 */
int lis2du12_trigger_set(const struct device *dev,
			 const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler)
{
	const struct lis2du12_config *cfg = dev->config;
	struct lis2du12_data *lis2du12 = dev->data;

	if (!cfg->trig_enabled) {
		LOG_ERR("trigger_set op not supported");
		return -ENOTSUP;
	}

	switch (trig->chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		lis2du12->handler_drdy_acc = handler;
		lis2du12->trig_drdy_acc = trig;
		if (handler) {
			return lis2du12_enable_xl_int(dev, LIS2DU12_EN_BIT);
		}

		return lis2du12_enable_xl_int(dev, LIS2DU12_DIS_BIT);

	default:
		return -ENOTSUP;
	}

}

/**
 * lis2du12_handle_interrupt - handle the drdy event
 * read data and call handler if registered any
 */
static void lis2du12_handle_interrupt(const struct device *dev)
{
	struct lis2du12_data *lis2du12 = dev->data;
	const struct lis2du12_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lis2du12_status_t status;

	while (1) {
		if (lis2du12_status_get(ctx, &status) < 0) {
			LOG_ERR("failed reading status reg");
			return;
		}

		if (status.drdy_xl == 0) {
			break;
		}

		if ((status.drdy_xl) && (lis2du12->handler_drdy_acc != NULL)) {
			lis2du12->handler_drdy_acc(dev, lis2du12->trig_drdy_acc);
		}
	}

	gpio_pin_interrupt_configure_dt(lis2du12->drdy_gpio,
					GPIO_INT_EDGE_TO_ACTIVE);
}

static void lis2du12_gpio_callback(const struct device *dev,
				   struct gpio_callback *cb, uint32_t pins)
{
	struct lis2du12_data *lis2du12 =
		CONTAINER_OF(cb, struct lis2du12_data, gpio_cb);

	ARG_UNUSED(pins);

	gpio_pin_interrupt_configure_dt(lis2du12->drdy_gpio, GPIO_INT_DISABLE);

#if defined(CONFIG_LIS2DU12_TRIGGER_OWN_THREAD)
	k_sem_give(&lis2du12->gpio_sem);
#elif defined(CONFIG_LIS2DU12_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&lis2du12->work);
#endif /* CONFIG_LIS2DU12_TRIGGER_OWN_THREAD */
}

#ifdef CONFIG_LIS2DU12_TRIGGER_OWN_THREAD
static void lis2du12_thread(struct lis2du12_data *lis2du12)
{
	while (1) {
		k_sem_take(&lis2du12->gpio_sem, K_FOREVER);
		lis2du12_handle_interrupt(lis2du12->dev);
	}
}
#endif /* CONFIG_LIS2DU12_TRIGGER_OWN_THREAD */

#ifdef CONFIG_LIS2DU12_TRIGGER_GLOBAL_THREAD
static void lis2du12_work_cb(struct k_work *work)
{
	struct lis2du12_data *lis2du12 =
		CONTAINER_OF(work, struct lis2du12_data, work);

	lis2du12_handle_interrupt(lis2du12->dev);
}
#endif /* CONFIG_LIS2DU12_TRIGGER_GLOBAL_THREAD */

int lis2du12_init_interrupt(const struct device *dev)
{
	struct lis2du12_data *lis2du12 = dev->data;
	const struct lis2du12_config *cfg = dev->config;
	int ret;

	lis2du12->drdy_gpio = (cfg->drdy_pin == 1) ?
			(struct gpio_dt_spec *)&cfg->int1_gpio :
			(struct gpio_dt_spec *)&cfg->int2_gpio;

	/* setup data ready gpio interrupt (INT1 or INT2) */
	if (!gpio_is_ready_dt(lis2du12->drdy_gpio)) {
		LOG_ERR("Cannot get pointer to drdy_gpio device (%p)",
			lis2du12->drdy_gpio);
		return -EINVAL;
	}

#if defined(CONFIG_LIS2DU12_TRIGGER_OWN_THREAD)
	k_sem_init(&lis2du12->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&lis2du12->thread, lis2du12->thread_stack,
			CONFIG_LIS2DU12_THREAD_STACK_SIZE,
			(k_thread_entry_t)lis2du12_thread, lis2du12,
			NULL, NULL, K_PRIO_COOP(CONFIG_LIS2DU12_THREAD_PRIORITY),
			0, K_NO_WAIT);
	k_thread_name_set(&lis2du12->thread, dev->name);
#elif defined(CONFIG_LIS2DU12_TRIGGER_GLOBAL_THREAD)
	lis2du12->work.handler = lis2du12_work_cb;
#endif /* CONFIG_LIS2DU12_TRIGGER_OWN_THREAD */

	ret = gpio_pin_configure_dt(lis2du12->drdy_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure gpio: %d", ret);
		return ret;
	}

	gpio_init_callback(&lis2du12->gpio_cb,
			   lis2du12_gpio_callback,
			   BIT(lis2du12->drdy_gpio->pin));

	if (gpio_add_callback(lis2du12->drdy_gpio->port, &lis2du12->gpio_cb) < 0) {
		LOG_ERR("Could not set gpio callback");
		return -EIO;
	}

	return gpio_pin_interrupt_configure_dt(lis2du12->drdy_gpio,
					       GPIO_INT_EDGE_TO_ACTIVE);
}
