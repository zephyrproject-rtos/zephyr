/* ST Microelectronics LIS2DUXS12 3-axis accelerometer sensor driver
 *
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lis2duxs12.pdf
 */

#define DT_DRV_COMPAT st_lis2duxs12

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "lis2duxs12.h"

LOG_MODULE_DECLARE(LIS2DUXS12, CONFIG_SENSOR_LOG_LEVEL);

#if defined(CONFIG_LIS2DUXS12_ENABLE_TEMP)
/**
 * lis2duxs12_enable_t_int - TEMP enable selected int pin to generate interrupt
 */
static int lis2duxs12_enable_t_int(const struct device *dev, int enable)
{
	const struct lis2duxs12_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lis2duxs12_pin_int2_route_t val;
	int ret;

	if (enable) {
		int16_t buf;

		/* dummy read: re-trigger interrupt */
		lis2duxs12_temperature_raw_get(ctx, &buf);
	}

	/* set interrupt (TEMP DRDY interrupt is only on INT2) */
	if (cfg->drdy_pin == 1) {
		return -EIO;
	}

	ret = lis2duxs12_pin_int2_route_get(ctx, &val);
	if (ret < 0) {
		LOG_ERR("pint_int2_route_get error");
		return ret;
	}

	val.drdy_temp = 1;

	return lis2duxs12_pin_int2_route_set(ctx, val);
}
#endif

/**
 * lis2duxs12_enable_xl_int - XL enable selected int pin to generate interrupt
 */
static int lis2duxs12_enable_xl_int(const struct device *dev, int enable)
{
	const struct lis2duxs12_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	int ret;

	if (enable) {
		lis2duxs12_md_t md;
		lis2duxs12_xl_data_t xl_data;

		/* dummy read: re-trigger interrupt */
		md.fs = cfg->accel_range;
		lis2duxs12_xl_data_get(ctx, &md, &xl_data);
	}

	/* set interrupt */
	if (cfg->drdy_pin == 1) {
		lis2duxs12_pin_int_route_t val;

		ret = lis2duxs12_pin_int1_route_get(ctx, &val);
		if (ret < 0) {
			LOG_ERR("pint_int1_route_get error");
			return ret;
		}

		val.drdy = 1;

		ret = lis2duxs12_pin_int1_route_set(ctx, &val);
	} else {
		lis2duxs12_pin_int_route_t val;

		ret = lis2duxs12_pin_int2_route_get(ctx, &val);
		if (ret < 0) {
			LOG_ERR("pint_int2_route_get error");
			return ret;
		}

		val.drdy = 1;

		ret = lis2duxs12_pin_int2_route_set(ctx, &val);
	}

	return ret;
}

/**
 * lis2duxs12_trigger_set - link external trigger to event data ready
 */
int lis2duxs12_trigger_set(const struct device *dev,
			  const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler)
{
	const struct lis2duxs12_config *cfg = dev->config;
	struct lis2duxs12_data *lis2duxs12 = dev->data;

	if (!cfg->trig_enabled) {
		LOG_ERR("trigger_set op not supported");
		return -ENOTSUP;
	}

	if (trig->chan == SENSOR_CHAN_ACCEL_XYZ) {
		lis2duxs12->handler_drdy_acc = handler;
		lis2duxs12->trig_drdy_acc = trig;
		if (handler) {
			return lis2duxs12_enable_xl_int(dev, LIS2DUXS12_EN_BIT);
		} else {
			return lis2duxs12_enable_xl_int(dev, LIS2DUXS12_DIS_BIT);
		}
	}
#if defined(CONFIG_LIS2DUXS12_ENABLE_TEMP)
	else if (trig->chan == SENSOR_CHAN_DIE_TEMP) {
		lis2duxs12->handler_drdy_temp = handler;
		lis2duxs12->trig_drdy_temp = trig;
		if (handler) {
			return lis2duxs12_enable_t_int(dev, LIS2DUXS12_EN_BIT);
		} else {
			return lis2duxs12_enable_t_int(dev, LIS2DUXS12_DIS_BIT);
		}
	}
#endif

	return -ENOTSUP;
}

/**
 * lis2duxs12_handle_interrupt - handle the drdy event
 * read data and call handler if registered any
 */
static void lis2duxs12_handle_interrupt(const struct device *dev)
{
	struct lis2duxs12_data *lis2duxs12 = dev->data;
	const struct lis2duxs12_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lis2duxs12_status_t status;

	while (1) {
		if (lis2duxs12_status_get(ctx, &status) < 0) {
			LOG_DBG("failed reading status reg");
			return;
		}

		if ((status.drdy) && (lis2duxs12->handler_drdy_acc != NULL)) {
			lis2duxs12->handler_drdy_acc(dev, lis2duxs12->trig_drdy_acc);
		}
	}

	gpio_pin_interrupt_configure_dt(&cfg->gpio_drdy,
					GPIO_INT_EDGE_TO_ACTIVE);
}

static void lis2duxs12_gpio_callback(const struct device *dev,
				    struct gpio_callback *cb, uint32_t pins)
{
	struct lis2duxs12_data *lis2duxs12 =
		CONTAINER_OF(cb, struct lis2duxs12_data, gpio_cb);
	const struct lis2duxs12_config *cfg = lis2duxs12->dev->config;

	ARG_UNUSED(pins);

	gpio_pin_interrupt_configure_dt(&cfg->gpio_drdy, GPIO_INT_DISABLE);

#if defined(CONFIG_LIS2DUXS12_TRIGGER_OWN_THREAD)
	k_sem_give(&lis2duxs12->gpio_sem);
#elif defined(CONFIG_LIS2DUXS12_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&lis2duxs12->work);
#endif /* CONFIG_LIS2DUXS12_TRIGGER_OWN_THREAD */
}

#ifdef CONFIG_LIS2DUXS12_TRIGGER_OWN_THREAD
static void lis2duxs12_thread(struct lis2duxs12_data *lis2duxs12)
{
	while (1) {
		k_sem_take(&lis2duxs12->gpio_sem, K_FOREVER);
		lis2duxs12_handle_interrupt(lis2duxs12->dev);
	}
}
#endif /* CONFIG_LIS2DUXS12_TRIGGER_OWN_THREAD */

#ifdef CONFIG_LIS2DUXS12_TRIGGER_GLOBAL_THREAD
static void lis2duxs12_work_cb(struct k_work *work)
{
	struct lis2duxs12_data *lis2duxs12 =
		CONTAINER_OF(work, struct lis2duxs12_data, work);

	lis2duxs12_handle_interrupt(lis2duxs12->dev);
}
#endif /* CONFIG_LIS2DUXS12_TRIGGER_GLOBAL_THREAD */

int lis2duxs12_init_interrupt(const struct device *dev)
{
	struct lis2duxs12_data *lis2duxs12 = dev->data;
	const struct lis2duxs12_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	int ret;

	/* setup data ready gpio interrupt (INT1 or INT2) */
	if (!device_is_ready(cfg->gpio_drdy.port)) {
		LOG_ERR("Cannot get pointer to drdy_gpio device");
		return -EINVAL;
	}

#if defined(CONFIG_LIS2DUXS12_TRIGGER_OWN_THREAD)
	k_sem_init(&lis2duxs12->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&lis2duxs12->thread, lis2duxs12->thread_stack,
			CONFIG_LIS2DUXS12_THREAD_STACK_SIZE,
			(k_thread_entry_t)lis2duxs12_thread, lis2duxs12,
			NULL, NULL, K_PRIO_COOP(CONFIG_LIS2DUXS12_THREAD_PRIORITY),
			0, K_NO_WAIT);
	k_thread_name_set(&lis2duxs12->thread, "lis2duxs12");
#elif defined(CONFIG_LIS2DUXS12_TRIGGER_GLOBAL_THREAD)
	lis2duxs12->work.handler = lis2duxs12_work_cb;
#endif /* CONFIG_LIS2DUXS12_TRIGGER_OWN_THREAD */

	ret = gpio_pin_configure_dt(&cfg->gpio_drdy, GPIO_INPUT);
	if (ret < 0) {
		LOG_DBG("Could not configure gpio");
		return ret;
	}

	gpio_init_callback(&lis2duxs12->gpio_cb,
			   lis2duxs12_gpio_callback,
			   BIT(cfg->gpio_drdy.pin));

	if (gpio_add_callback(cfg->gpio_drdy.port, &lis2duxs12->gpio_cb) < 0) {
		LOG_DBG("Could not set gpio callback");
		return -EIO;
	}


	/* set data ready mode on int1/int2 */
	LOG_DBG("drdy_pulsed is %d", (int)cfg->drdy_pulsed);
	lis2duxs12_data_ready_mode_t mode = cfg->drdy_pulsed ? LIS2DUXS12_DRDY_PULSED :
							     LIS2DUXS12_DRDY_LATCHED;

	ret = lis2duxs12_data_ready_mode_set(ctx, mode);
	if (ret < 0) {
		LOG_ERR("drdy_pulsed config error %d", (int)cfg->drdy_pulsed);
		return ret;
	}

	return gpio_pin_interrupt_configure_dt(&cfg->gpio_drdy,
					       GPIO_INT_EDGE_TO_ACTIVE);
}
