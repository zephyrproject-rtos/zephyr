/* ST Microelectronics LSM6DSO16IS 6-axis IMU sensor driver
 *
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lsm6dso16is.pdf
 */

#define DT_DRV_COMPAT st_lsm6dso16is

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "lsm6dso16is.h"

LOG_MODULE_DECLARE(LSM6DSO16IS, CONFIG_SENSOR_LOG_LEVEL);

#if defined(CONFIG_LSM6DSO16IS_ENABLE_TEMP)
/**
 * lsm6dso16is_enable_t_int - TEMP enable selected int pin to generate interrupt
 */
static int lsm6dso16is_enable_t_int(const struct device *dev, int enable)
{
	const struct lsm6dso16is_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lsm6dso16is_pin_int2_route_t val;
	int ret;

	if (enable) {
		int16_t buf;

		/* dummy read: re-trigger interrupt */
		lsm6dso16is_temperature_raw_get(ctx, &buf);
	}

	/* set interrupt (TEMP DRDY interrupt is only on INT2) */
	if (cfg->drdy_pin == 1) {
		return -EIO;
	}

	ret = lsm6dso16is_pin_int2_route_get(ctx, &val);
	if (ret < 0) {
		LOG_ERR("pint_int2_route_get error");
		return ret;
	}

	val.drdy_temp = 1;

	return lsm6dso16is_pin_int2_route_set(ctx, val);
}
#endif

/**
 * lsm6dso16is_enable_xl_int - XL enable selected int pin to generate interrupt
 */
static int lsm6dso16is_enable_xl_int(const struct device *dev, int enable)
{
	const struct lsm6dso16is_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	int ret;

	if (enable) {
		int16_t buf[3];

		/* dummy read: re-trigger interrupt */
		lsm6dso16is_acceleration_raw_get(ctx, buf);
	}

	/* set interrupt */
	if (cfg->drdy_pin == 1) {
		lsm6dso16is_pin_int1_route_t val;

		ret = lsm6dso16is_pin_int1_route_get(ctx, &val);
		if (ret < 0) {
			LOG_ERR("pint_int1_route_get error");
			return ret;
		}

		val.drdy_xl = 1;

		ret = lsm6dso16is_pin_int1_route_set(ctx, val);
	} else {
		lsm6dso16is_pin_int2_route_t val;

		ret = lsm6dso16is_pin_int2_route_get(ctx, &val);
		if (ret < 0) {
			LOG_ERR("pint_int2_route_get error");
			return ret;
		}

		val.drdy_xl = 1;

		ret = lsm6dso16is_pin_int2_route_set(ctx, val);
	}

	return ret;
}

/**
 * lsm6dso16is_enable_g_int - Gyro enable selected int pin to generate interrupt
 */
static int lsm6dso16is_enable_g_int(const struct device *dev, int enable)
{
	const struct lsm6dso16is_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	int ret;

	if (enable) {
		int16_t buf[3];

		/* dummy read: re-trigger interrupt */
		lsm6dso16is_angular_rate_raw_get(ctx, buf);
	}

	/* set interrupt */
	if (cfg->drdy_pin == 1) {
		lsm6dso16is_pin_int1_route_t val;

		ret = lsm6dso16is_pin_int1_route_get(ctx, &val);
		if (ret < 0) {
			LOG_ERR("pint_int1_route_get error");
			return ret;
		}

		val.drdy_gy = 1;

		ret = lsm6dso16is_pin_int1_route_set(ctx, val);
	} else {
		lsm6dso16is_pin_int2_route_t val;

		ret = lsm6dso16is_pin_int2_route_get(ctx, &val);
		if (ret < 0) {
			LOG_ERR("pint_int2_route_get error");
			return ret;
		}

		val.drdy_gy = 1;

		ret = lsm6dso16is_pin_int2_route_set(ctx, val);
	}

	return ret;
}

/**
 * lsm6dso16is_trigger_set - link external trigger to event data ready
 */
int lsm6dso16is_trigger_set(const struct device *dev,
			  const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler)
{
	const struct lsm6dso16is_config *cfg = dev->config;
	struct lsm6dso16is_data *lsm6dso16is = dev->data;

	if (!cfg->trig_enabled) {
		LOG_ERR("trigger_set op not supported");
		return -ENOTSUP;
	}

	if (trig->chan == SENSOR_CHAN_ACCEL_XYZ) {
		lsm6dso16is->handler_drdy_acc = handler;
		lsm6dso16is->trig_drdy_acc = trig;
		if (handler) {
			return lsm6dso16is_enable_xl_int(dev, LSM6DSO16IS_EN_BIT);
		} else {
			return lsm6dso16is_enable_xl_int(dev, LSM6DSO16IS_DIS_BIT);
		}
	} else if (trig->chan == SENSOR_CHAN_GYRO_XYZ) {
		lsm6dso16is->handler_drdy_gyr = handler;
		lsm6dso16is->trig_drdy_gyr = trig;
		if (handler) {
			return lsm6dso16is_enable_g_int(dev, LSM6DSO16IS_EN_BIT);
		} else {
			return lsm6dso16is_enable_g_int(dev, LSM6DSO16IS_DIS_BIT);
		}
	}
#if defined(CONFIG_LSM6DSO16IS_ENABLE_TEMP)
	else if (trig->chan == SENSOR_CHAN_DIE_TEMP) {
		lsm6dso16is->handler_drdy_temp = handler;
		lsm6dso16is->trig_drdy_temp = trig;
		if (handler) {
			return lsm6dso16is_enable_t_int(dev, LSM6DSO16IS_EN_BIT);
		} else {
			return lsm6dso16is_enable_t_int(dev, LSM6DSO16IS_DIS_BIT);
		}
	}
#endif

	return -ENOTSUP;
}

/**
 * lsm6dso16is_handle_interrupt - handle the drdy event
 * read data and call handler if registered any
 */
static void lsm6dso16is_handle_interrupt(const struct device *dev)
{
	struct lsm6dso16is_data *lsm6dso16is = dev->data;
	const struct lsm6dso16is_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lsm6dso16is_status_reg_t status;

	while (1) {
		if (lsm6dso16is_status_reg_get(ctx, &status) < 0) {
			LOG_DBG("failed reading status reg");
			return;
		}

		if ((status.xlda == 0) && (status.gda == 0)
#if defined(CONFIG_LSM6DSO16IS_ENABLE_TEMP)
					&& (status.tda == 0)
#endif
					) {
			break;
		}

		if ((status.xlda) && (lsm6dso16is->handler_drdy_acc != NULL)) {
			lsm6dso16is->handler_drdy_acc(dev, lsm6dso16is->trig_drdy_acc);
		}

		if ((status.gda) && (lsm6dso16is->handler_drdy_gyr != NULL)) {
			lsm6dso16is->handler_drdy_gyr(dev, lsm6dso16is->trig_drdy_gyr);
		}

#if defined(CONFIG_LSM6DSO16IS_ENABLE_TEMP)
		if ((status.tda) && (lsm6dso16is->handler_drdy_temp != NULL)) {
			lsm6dso16is->handler_drdy_temp(dev, lsm6dso16is->trig_drdy_temp);
		}
#endif
	}

	gpio_pin_interrupt_configure_dt(&cfg->gpio_drdy,
					GPIO_INT_EDGE_TO_ACTIVE);
}

static void lsm6dso16is_gpio_callback(const struct device *dev,
				    struct gpio_callback *cb, uint32_t pins)
{
	struct lsm6dso16is_data *lsm6dso16is =
		CONTAINER_OF(cb, struct lsm6dso16is_data, gpio_cb);
	const struct lsm6dso16is_config *cfg = lsm6dso16is->dev->config;

	ARG_UNUSED(pins);

	gpio_pin_interrupt_configure_dt(&cfg->gpio_drdy, GPIO_INT_DISABLE);

#if defined(CONFIG_LSM6DSO16IS_TRIGGER_OWN_THREAD)
	k_sem_give(&lsm6dso16is->gpio_sem);
#elif defined(CONFIG_LSM6DSO16IS_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&lsm6dso16is->work);
#endif /* CONFIG_LSM6DSO16IS_TRIGGER_OWN_THREAD */
}

#ifdef CONFIG_LSM6DSO16IS_TRIGGER_OWN_THREAD
static void lsm6dso16is_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct lsm6dso16is_data *lsm6dso16is = p1;

	while (1) {
		k_sem_take(&lsm6dso16is->gpio_sem, K_FOREVER);
		lsm6dso16is_handle_interrupt(lsm6dso16is->dev);
	}
}
#endif /* CONFIG_LSM6DSO16IS_TRIGGER_OWN_THREAD */

#ifdef CONFIG_LSM6DSO16IS_TRIGGER_GLOBAL_THREAD
static void lsm6dso16is_work_cb(struct k_work *work)
{
	struct lsm6dso16is_data *lsm6dso16is =
		CONTAINER_OF(work, struct lsm6dso16is_data, work);

	lsm6dso16is_handle_interrupt(lsm6dso16is->dev);
}
#endif /* CONFIG_LSM6DSO16IS_TRIGGER_GLOBAL_THREAD */

int lsm6dso16is_init_interrupt(const struct device *dev)
{
	struct lsm6dso16is_data *lsm6dso16is = dev->data;
	const struct lsm6dso16is_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	int ret;

	/* setup data ready gpio interrupt (INT1 or INT2) */
	if (!gpio_is_ready_dt(&cfg->gpio_drdy)) {
		LOG_ERR("Cannot get pointer to drdy_gpio device");
		return -EINVAL;
	}

#if defined(CONFIG_LSM6DSO16IS_TRIGGER_OWN_THREAD)
	k_sem_init(&lsm6dso16is->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&lsm6dso16is->thread, lsm6dso16is->thread_stack,
			CONFIG_LSM6DSO16IS_THREAD_STACK_SIZE,
			lsm6dso16is_thread, lsm6dso16is,
			NULL, NULL, K_PRIO_COOP(CONFIG_LSM6DSO16IS_THREAD_PRIORITY),
			0, K_NO_WAIT);
	k_thread_name_set(&lsm6dso16is->thread, "lsm6dso16is");
#elif defined(CONFIG_LSM6DSO16IS_TRIGGER_GLOBAL_THREAD)
	lsm6dso16is->work.handler = lsm6dso16is_work_cb;
#endif /* CONFIG_LSM6DSO16IS_TRIGGER_OWN_THREAD */

	ret = gpio_pin_configure_dt(&cfg->gpio_drdy, GPIO_INPUT);
	if (ret < 0) {
		LOG_DBG("Could not configure gpio");
		return ret;
	}

	gpio_init_callback(&lsm6dso16is->gpio_cb,
			   lsm6dso16is_gpio_callback,
			   BIT(cfg->gpio_drdy.pin));

	if (gpio_add_callback(cfg->gpio_drdy.port, &lsm6dso16is->gpio_cb) < 0) {
		LOG_DBG("Could not set gpio callback");
		return -EIO;
	}


	/* set data ready mode on int1/int2 */
	LOG_DBG("drdy_pulsed is %d", (int)cfg->drdy_pulsed);
	lsm6dso16is_data_ready_mode_t mode = cfg->drdy_pulsed ? LSM6DSO16IS_DRDY_PULSED :
							     LSM6DSO16IS_DRDY_LATCHED;

	ret = lsm6dso16is_data_ready_mode_set(ctx, mode);
	if (ret < 0) {
		LOG_ERR("drdy_pulsed config error %d", (int)cfg->drdy_pulsed);
		return ret;
	}

	return gpio_pin_interrupt_configure_dt(&cfg->gpio_drdy,
					       GPIO_INT_EDGE_TO_ACTIVE);
}
