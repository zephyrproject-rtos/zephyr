/* ST Microelectronics LSM6DSV16X 6-axis IMU sensor driver
 *
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lsm6dsv16x.pdf
 */

#define DT_DRV_COMPAT st_lsm6dsv16x

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "lsm6dsv16x.h"

LOG_MODULE_DECLARE(LSM6DSV16X, CONFIG_SENSOR_LOG_LEVEL);

/**
 * lsm6dsv16x_enable_xl_int - XL enable selected int pin to generate interrupt
 */
static int lsm6dsv16x_enable_xl_int(const struct device *dev, int enable)
{
	const struct lsm6dsv16x_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	int ret;

	if (enable) {
		int16_t buf[3];

		/* dummy read: re-trigger interrupt */
		lsm6dsv16x_acceleration_raw_get(ctx, buf);
	}

	/* set interrupt */
	if (cfg->drdy_pin == 1) {
		lsm6dsv16x_pin_int_route_t val;

		ret = lsm6dsv16x_pin_int1_route_get(ctx, &val);
		if (ret < 0) {
			LOG_ERR("pint_int1_route_get error");
			return ret;
		}

		val.drdy_xl = 1;

		ret = lsm6dsv16x_pin_int1_route_set(ctx, &val);
	} else {
		lsm6dsv16x_pin_int_route_t val;

		ret = lsm6dsv16x_pin_int2_route_get(ctx, &val);
		if (ret < 0) {
			LOG_ERR("pint_int2_route_get error");
			return ret;
		}

		val.drdy_xl = 1;

		ret = lsm6dsv16x_pin_int2_route_set(ctx, &val);
	}

	return ret;
}

/**
 * lsm6dsv16x_enable_g_int - Gyro enable selected int pin to generate interrupt
 */
static int lsm6dsv16x_enable_g_int(const struct device *dev, int enable)
{
	const struct lsm6dsv16x_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	int ret;

	if (enable) {
		int16_t buf[3];

		/* dummy read: re-trigger interrupt */
		lsm6dsv16x_angular_rate_raw_get(ctx, buf);
	}

	/* set interrupt */
	if (cfg->drdy_pin == 1) {
		lsm6dsv16x_pin_int_route_t val;

		ret = lsm6dsv16x_pin_int1_route_get(ctx, &val);
		if (ret < 0) {
			LOG_ERR("pint_int1_route_get error");
			return ret;
		}

		val.drdy_g = 1;

		ret = lsm6dsv16x_pin_int1_route_set(ctx, &val);
	} else {
		lsm6dsv16x_pin_int_route_t val;

		ret = lsm6dsv16x_pin_int2_route_get(ctx, &val);
		if (ret < 0) {
			LOG_ERR("pint_int2_route_get error");
			return ret;
		}

		val.drdy_g = 1;

		ret = lsm6dsv16x_pin_int2_route_set(ctx, &val);
	}

	return ret;
}

/**
 * lsm6dsv16x_trigger_set - link external trigger to event data ready
 */
int lsm6dsv16x_trigger_set(const struct device *dev,
			  const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler)
{
	const struct lsm6dsv16x_config *cfg = dev->config;
	struct lsm6dsv16x_data *lsm6dsv16x = dev->data;

	if (!cfg->trig_enabled) {
		LOG_ERR("trigger_set op not supported");
		return -ENOTSUP;
	}

	if (trig->chan == SENSOR_CHAN_ACCEL_XYZ) {
		lsm6dsv16x->handler_drdy_acc = handler;
		lsm6dsv16x->trig_drdy_acc = trig;
		if (handler) {
			return lsm6dsv16x_enable_xl_int(dev, LSM6DSV16X_EN_BIT);
		} else {
			return lsm6dsv16x_enable_xl_int(dev, LSM6DSV16X_DIS_BIT);
		}
	} else if (trig->chan == SENSOR_CHAN_GYRO_XYZ) {
		lsm6dsv16x->handler_drdy_gyr = handler;
		lsm6dsv16x->trig_drdy_gyr = trig;
		if (handler) {
			return lsm6dsv16x_enable_g_int(dev, LSM6DSV16X_EN_BIT);
		} else {
			return lsm6dsv16x_enable_g_int(dev, LSM6DSV16X_DIS_BIT);
		}
	}

	return -ENOTSUP;
}

/**
 * lsm6dsv16x_handle_interrupt - handle the drdy event
 * read data and call handler if registered any
 */
static void lsm6dsv16x_handle_interrupt(const struct device *dev)
{
	struct lsm6dsv16x_data *lsm6dsv16x = dev->data;
	const struct lsm6dsv16x_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lsm6dsv16x_data_ready_t status;

	while (1) {
		if (lsm6dsv16x_flag_data_ready_get(ctx, &status) < 0) {
			LOG_DBG("failed reading status reg");
			return;
		}

		if ((status.drdy_xl == 0) && (status.drdy_gy == 0)) {
			break;
		}

		if ((status.drdy_xl) && (lsm6dsv16x->handler_drdy_acc != NULL)) {
			lsm6dsv16x->handler_drdy_acc(dev, lsm6dsv16x->trig_drdy_acc);
		}

		if ((status.drdy_gy) && (lsm6dsv16x->handler_drdy_gyr != NULL)) {
			lsm6dsv16x->handler_drdy_gyr(dev, lsm6dsv16x->trig_drdy_gyr);
		}

	}

	gpio_pin_interrupt_configure_dt(lsm6dsv16x->drdy_gpio,
					GPIO_INT_EDGE_TO_ACTIVE);
}

static void lsm6dsv16x_gpio_callback(const struct device *dev,
				    struct gpio_callback *cb, uint32_t pins)
{
	struct lsm6dsv16x_data *lsm6dsv16x =
		CONTAINER_OF(cb, struct lsm6dsv16x_data, gpio_cb);

	ARG_UNUSED(pins);

	gpio_pin_interrupt_configure_dt(lsm6dsv16x->drdy_gpio, GPIO_INT_DISABLE);

#if defined(CONFIG_LSM6DSV16X_TRIGGER_OWN_THREAD)
	k_sem_give(&lsm6dsv16x->gpio_sem);
#elif defined(CONFIG_LSM6DSV16X_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&lsm6dsv16x->work);
#endif /* CONFIG_LSM6DSV16X_TRIGGER_OWN_THREAD */
}

#ifdef CONFIG_LSM6DSV16X_TRIGGER_OWN_THREAD
static void lsm6dsv16x_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct lsm6dsv16x_data *lsm6dsv16x = p1;

	while (1) {
		k_sem_take(&lsm6dsv16x->gpio_sem, K_FOREVER);
		lsm6dsv16x_handle_interrupt(lsm6dsv16x->dev);
	}
}
#endif /* CONFIG_LSM6DSV16X_TRIGGER_OWN_THREAD */

#ifdef CONFIG_LSM6DSV16X_TRIGGER_GLOBAL_THREAD
static void lsm6dsv16x_work_cb(struct k_work *work)
{
	struct lsm6dsv16x_data *lsm6dsv16x =
		CONTAINER_OF(work, struct lsm6dsv16x_data, work);

	lsm6dsv16x_handle_interrupt(lsm6dsv16x->dev);
}
#endif /* CONFIG_LSM6DSV16X_TRIGGER_GLOBAL_THREAD */

int lsm6dsv16x_init_interrupt(const struct device *dev)
{
	struct lsm6dsv16x_data *lsm6dsv16x = dev->data;
	const struct lsm6dsv16x_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	int ret;

	lsm6dsv16x->drdy_gpio = (cfg->drdy_pin == 1) ?
			(struct gpio_dt_spec *)&cfg->int1_gpio :
			(struct gpio_dt_spec *)&cfg->int2_gpio;

	/* setup data ready gpio interrupt (INT1 or INT2) */
	if (!gpio_is_ready_dt(lsm6dsv16x->drdy_gpio)) {
		LOG_ERR("Cannot get pointer to drdy_gpio device");
		return -EINVAL;
	}

#if defined(CONFIG_LSM6DSV16X_TRIGGER_OWN_THREAD)
	k_sem_init(&lsm6dsv16x->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&lsm6dsv16x->thread, lsm6dsv16x->thread_stack,
			CONFIG_LSM6DSV16X_THREAD_STACK_SIZE,
			lsm6dsv16x_thread, lsm6dsv16x,
			NULL, NULL, K_PRIO_COOP(CONFIG_LSM6DSV16X_THREAD_PRIORITY),
			0, K_NO_WAIT);
	k_thread_name_set(&lsm6dsv16x->thread, "lsm6dsv16x");
#elif defined(CONFIG_LSM6DSV16X_TRIGGER_GLOBAL_THREAD)
	lsm6dsv16x->work.handler = lsm6dsv16x_work_cb;
#endif /* CONFIG_LSM6DSV16X_TRIGGER_OWN_THREAD */

	ret = gpio_pin_configure_dt(lsm6dsv16x->drdy_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_DBG("Could not configure gpio");
		return ret;
	}

	gpio_init_callback(&lsm6dsv16x->gpio_cb,
			   lsm6dsv16x_gpio_callback,
			   BIT(lsm6dsv16x->drdy_gpio->pin));

	if (gpio_add_callback(lsm6dsv16x->drdy_gpio->port, &lsm6dsv16x->gpio_cb) < 0) {
		LOG_DBG("Could not set gpio callback");
		return -EIO;
	}


	/* set data ready mode on int1/int2 */
	LOG_DBG("drdy_pulsed is %d", (int)cfg->drdy_pulsed);
	lsm6dsv16x_data_ready_mode_t mode = cfg->drdy_pulsed ? LSM6DSV16X_DRDY_PULSED :
							     LSM6DSV16X_DRDY_LATCHED;

	ret = lsm6dsv16x_data_ready_mode_set(ctx, mode);
	if (ret < 0) {
		LOG_ERR("drdy_pulsed config error %d", (int)cfg->drdy_pulsed);
		return ret;
	}

	return gpio_pin_interrupt_configure_dt(lsm6dsv16x->drdy_gpio,
					       GPIO_INT_EDGE_TO_ACTIVE);
}
