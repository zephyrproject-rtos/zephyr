/* ST Microelectronics LSM6DSO 6-axis IMU sensor driver
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lsm6dso.pdf
 */

#define DT_DRV_COMPAT st_lsm6dso

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "lsm6dso.h"

LOG_MODULE_DECLARE(LSM6DSO, CONFIG_SENSOR_LOG_LEVEL);

#if defined(CONFIG_LSM6DSO_ENABLE_TEMP)
/**
 * lsm6dso_enable_t_int - TEMP enable selected int pin to generate interrupt
 */
static int lsm6dso_enable_t_int(const struct device *dev, int enable)
{
	const struct lsm6dso_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lsm6dso_int2_ctrl_t int2_ctrl;

	if (enable) {
		int16_t buf;

		/* dummy read: re-trigger interrupt */
		lsm6dso_temperature_raw_get(ctx, &buf);
	}

	/* set interrupt (TEMP DRDY interrupt is only on INT2) */
	if (cfg->int_pin == 1)
		return -EIO;

	lsm6dso_read_reg(ctx, LSM6DSO_INT2_CTRL, (uint8_t *)&int2_ctrl, 1);
	int2_ctrl.int2_drdy_temp = enable;
	return lsm6dso_write_reg(ctx, LSM6DSO_INT2_CTRL,
				 (uint8_t *)&int2_ctrl, 1);
}
#endif

/**
 * lsm6dso_enable_xl_int - XL enable selected int pin to generate interrupt
 */
static int lsm6dso_enable_xl_int(const struct device *dev, int enable)
{
	const struct lsm6dso_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;

	if (enable) {
		int16_t buf[3];

		/* dummy read: re-trigger interrupt */
		lsm6dso_acceleration_raw_get(ctx, buf);
	}

	/* set interrupt */
	if (cfg->int_pin == 1) {
		lsm6dso_int1_ctrl_t int1_ctrl;

		lsm6dso_read_reg(ctx, LSM6DSO_INT1_CTRL,
				 (uint8_t *)&int1_ctrl, 1);

		int1_ctrl.int1_drdy_xl = enable;
		return lsm6dso_write_reg(ctx, LSM6DSO_INT1_CTRL,
					 (uint8_t *)&int1_ctrl, 1);
	} else {
		lsm6dso_int2_ctrl_t int2_ctrl;

		lsm6dso_read_reg(ctx, LSM6DSO_INT2_CTRL,
				 (uint8_t *)&int2_ctrl, 1);
		int2_ctrl.int2_drdy_xl = enable;
		return lsm6dso_write_reg(ctx, LSM6DSO_INT2_CTRL,
					 (uint8_t *)&int2_ctrl, 1);
	}
}

/**
 * lsm6dso_enable_g_int - Gyro enable selected int pin to generate interrupt
 */
static int lsm6dso_enable_g_int(const struct device *dev, int enable)
{
	const struct lsm6dso_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;

	if (enable) {
		int16_t buf[3];

		/* dummy read: re-trigger interrupt */
		lsm6dso_angular_rate_raw_get(ctx, buf);
	}

	/* set interrupt */
	if (cfg->int_pin == 1) {
		lsm6dso_int1_ctrl_t int1_ctrl;

		lsm6dso_read_reg(ctx, LSM6DSO_INT1_CTRL,
				 (uint8_t *)&int1_ctrl, 1);
		int1_ctrl.int1_drdy_g = enable;
		return lsm6dso_write_reg(ctx, LSM6DSO_INT1_CTRL,
					 (uint8_t *)&int1_ctrl, 1);
	} else {
		lsm6dso_int2_ctrl_t int2_ctrl;

		lsm6dso_read_reg(ctx, LSM6DSO_INT2_CTRL,
				 (uint8_t *)&int2_ctrl, 1);
		int2_ctrl.int2_drdy_g = enable;
		return lsm6dso_write_reg(ctx, LSM6DSO_INT2_CTRL,
					 (uint8_t *)&int2_ctrl, 1);
	}
}

/**
 * lsm6dso_trigger_set - link external trigger to event data ready
 */
int lsm6dso_trigger_set(const struct device *dev,
			  const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler)
{
	const struct lsm6dso_config *cfg = dev->config;
	struct lsm6dso_data *lsm6dso = dev->data;

	if (!cfg->trig_enabled) {
		LOG_ERR("trigger_set op not supported");
		return -ENOTSUP;
	}

	if (trig->chan == SENSOR_CHAN_ACCEL_XYZ) {
		lsm6dso->handler_drdy_acc = handler;
		if (handler) {
			return lsm6dso_enable_xl_int(dev, LSM6DSO_EN_BIT);
		} else {
			return lsm6dso_enable_xl_int(dev, LSM6DSO_DIS_BIT);
		}
	} else if (trig->chan == SENSOR_CHAN_GYRO_XYZ) {
		lsm6dso->handler_drdy_gyr = handler;
		if (handler) {
			return lsm6dso_enable_g_int(dev, LSM6DSO_EN_BIT);
		} else {
			return lsm6dso_enable_g_int(dev, LSM6DSO_DIS_BIT);
		}
	}
#if defined(CONFIG_LSM6DSO_ENABLE_TEMP)
	else if (trig->chan == SENSOR_CHAN_DIE_TEMP) {
		lsm6dso->handler_drdy_temp = handler;
		if (handler) {
			return lsm6dso_enable_t_int(dev, LSM6DSO_EN_BIT);
		} else {
			return lsm6dso_enable_t_int(dev, LSM6DSO_DIS_BIT);
		}
	}
#endif

	return -ENOTSUP;
}

/**
 * lsm6dso_handle_interrupt - handle the drdy event
 * read data and call handler if registered any
 */
static void lsm6dso_handle_interrupt(const struct device *dev)
{
	struct lsm6dso_data *lsm6dso = dev->data;
	struct sensor_trigger drdy_trigger = {
		.type = SENSOR_TRIG_DATA_READY,
	};
	const struct lsm6dso_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lsm6dso_status_reg_t status;

	while (1) {
		if (lsm6dso_status_reg_get(ctx, &status) < 0) {
			LOG_DBG("failed reading status reg");
			return;
		}

		if ((status.xlda == 0) && (status.gda == 0)
#if defined(CONFIG_LSM6DSO_ENABLE_TEMP)
					&& (status.tda == 0)
#endif
					) {
			break;
		}

		if ((status.xlda) && (lsm6dso->handler_drdy_acc != NULL)) {
			lsm6dso->handler_drdy_acc(dev, &drdy_trigger);
		}

		if ((status.gda) && (lsm6dso->handler_drdy_gyr != NULL)) {
			lsm6dso->handler_drdy_gyr(dev, &drdy_trigger);
		}

#if defined(CONFIG_LSM6DSO_ENABLE_TEMP)
		if ((status.tda) && (lsm6dso->handler_drdy_temp != NULL)) {
			lsm6dso->handler_drdy_temp(dev, &drdy_trigger);
		}
#endif
	}

	gpio_pin_interrupt_configure_dt(&cfg->gpio_drdy,
					GPIO_INT_EDGE_TO_ACTIVE);
}

static void lsm6dso_gpio_callback(const struct device *dev,
				    struct gpio_callback *cb, uint32_t pins)
{
	struct lsm6dso_data *lsm6dso =
		CONTAINER_OF(cb, struct lsm6dso_data, gpio_cb);
	const struct lsm6dso_config *cfg = lsm6dso->dev->config;

	ARG_UNUSED(pins);

	gpio_pin_interrupt_configure_dt(&cfg->gpio_drdy, GPIO_INT_DISABLE);

#if defined(CONFIG_LSM6DSO_TRIGGER_OWN_THREAD)
	k_sem_give(&lsm6dso->gpio_sem);
#elif defined(CONFIG_LSM6DSO_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&lsm6dso->work);
#endif /* CONFIG_LSM6DSO_TRIGGER_OWN_THREAD */
}

#ifdef CONFIG_LSM6DSO_TRIGGER_OWN_THREAD
static void lsm6dso_thread(struct lsm6dso_data *lsm6dso)
{
	while (1) {
		k_sem_take(&lsm6dso->gpio_sem, K_FOREVER);
		lsm6dso_handle_interrupt(lsm6dso->dev);
	}
}
#endif /* CONFIG_LSM6DSO_TRIGGER_OWN_THREAD */

#ifdef CONFIG_LSM6DSO_TRIGGER_GLOBAL_THREAD
static void lsm6dso_work_cb(struct k_work *work)
{
	struct lsm6dso_data *lsm6dso =
		CONTAINER_OF(work, struct lsm6dso_data, work);

	lsm6dso_handle_interrupt(lsm6dso->dev);
}
#endif /* CONFIG_LSM6DSO_TRIGGER_GLOBAL_THREAD */

int lsm6dso_init_interrupt(const struct device *dev)
{
	struct lsm6dso_data *lsm6dso = dev->data;
	const struct lsm6dso_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	int ret;

	/* setup data ready gpio interrupt (INT1 or INT2) */
	if (!device_is_ready(cfg->gpio_drdy.port)) {
		LOG_ERR("Cannot get pointer to drdy_gpio device");
		return -EINVAL;
	}

#if defined(CONFIG_LSM6DSO_TRIGGER_OWN_THREAD)
	k_sem_init(&lsm6dso->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&lsm6dso->thread, lsm6dso->thread_stack,
			CONFIG_LSM6DSO_THREAD_STACK_SIZE,
			(k_thread_entry_t)lsm6dso_thread, lsm6dso,
			NULL, NULL, K_PRIO_COOP(CONFIG_LSM6DSO_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif defined(CONFIG_LSM6DSO_TRIGGER_GLOBAL_THREAD)
	lsm6dso->work.handler = lsm6dso_work_cb;
#endif /* CONFIG_LSM6DSO_TRIGGER_OWN_THREAD */

	ret = gpio_pin_configure_dt(&cfg->gpio_drdy, GPIO_INPUT);
	if (ret < 0) {
		LOG_DBG("Could not configure gpio");
		return ret;
	}

	gpio_init_callback(&lsm6dso->gpio_cb,
			   lsm6dso_gpio_callback,
			   BIT(cfg->gpio_drdy.pin));

	if (gpio_add_callback(cfg->gpio_drdy.port, &lsm6dso->gpio_cb) < 0) {
		LOG_DBG("Could not set gpio callback");
		return -EIO;
	}

	/* enable interrupt on int1/int2 in pulse mode */
	if (lsm6dso_int_notification_set(ctx, LSM6DSO_ALL_INT_PULSED) < 0) {
		LOG_DBG("Could not set pulse mode");
		return -EIO;
	}

	return gpio_pin_interrupt_configure_dt(&cfg->gpio_drdy,
					       GPIO_INT_EDGE_TO_ACTIVE);
}
