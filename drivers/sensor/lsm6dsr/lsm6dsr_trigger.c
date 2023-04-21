/* ST Microelectronics LSM6DSR 6-axis IMU sensor driver
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lsm6dsr.pdf
 */

#define DT_DRV_COMPAT st_lsm6dsr

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/sys/__assert.h>

#include "lsm6dsr.h"

LOG_MODULE_DECLARE(LSM6DSR, CONFIG_SENSOR_LOG_LEVEL);

#if defined(CONFIG_LSM6DSR_ENABLE_TEMP)
/**
 * lsm6dsr_enable_t_int - TEMP enable selected int pin to generate interrupt
 */
static int lsm6dsr_enable_t_int(const struct device *dev, int enable)
{
	const struct lsm6dsr_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lsm6dsr_int2_ctrl_t int2_ctrl;

	if (enable) {
		int16_t buf;

		/* dummy read: re-trigger interrupt */
		lsm6dsr_temperature_raw_get(ctx, &buf);
	}

	/* set interrupt (TEMP DRDY interrupt is only on INT2) */
	if (cfg->int_pin == 1)
		return -EIO;

	lsm6dsr_read_reg(ctx, LSM6DSR_INT2_CTRL, (uint8_t *)&int2_ctrl, 1);
	int2_ctrl.int2_drdy_temp = enable;
	return lsm6dsr_write_reg(ctx, LSM6DSR_INT2_CTRL,
				 (uint8_t *)&int2_ctrl, 1);
}
#endif

/**
 * lsm6dsr_enable_xl_int - XL enable selected int pin to generate interrupt
 */
static int lsm6dsr_enable_xl_int(const struct device *dev, int enable)
{
	const struct lsm6dsr_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;

	if (enable) {
		int16_t buf[3];

		/* dummy read: re-trigger interrupt */
		lsm6dsr_acceleration_raw_get(ctx, buf);
	}

	/* set interrupt */
	if (cfg->int_pin == 1) {
		lsm6dsr_int1_ctrl_t int1_ctrl;

		lsm6dsr_read_reg(ctx, LSM6DSR_INT1_CTRL,
				 (uint8_t *)&int1_ctrl, 1);

		int1_ctrl.int1_drdy_xl = enable;
		return lsm6dsr_write_reg(ctx, LSM6DSR_INT1_CTRL,
					 (uint8_t *)&int1_ctrl, 1);
	} else {
		lsm6dsr_int2_ctrl_t int2_ctrl;

		lsm6dsr_read_reg(ctx, LSM6DSR_INT2_CTRL,
				 (uint8_t *)&int2_ctrl, 1);
		int2_ctrl.int2_drdy_xl = enable;
		return lsm6dsr_write_reg(ctx, LSM6DSR_INT2_CTRL,
					 (uint8_t *)&int2_ctrl, 1);
	}
}

/**
 * lsm6dsr_enable_g_int - Gyro enable selected int pin to generate interrupt
 */
static int lsm6dsr_enable_g_int(const struct device *dev, int enable)
{
	const struct lsm6dsr_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;

	if (enable) {
		int16_t buf[3];

		/* dummy read: re-trigger interrupt */
		lsm6dsr_angular_rate_raw_get(ctx, buf);
	}

	/* set interrupt */
	if (cfg->int_pin == 1) {
		lsm6dsr_int1_ctrl_t int1_ctrl;

		lsm6dsr_read_reg(ctx, LSM6DSR_INT1_CTRL,
				 (uint8_t *)&int1_ctrl, 1);
		int1_ctrl.int1_drdy_g = enable;
		return lsm6dsr_write_reg(ctx, LSM6DSR_INT1_CTRL,
					 (uint8_t *)&int1_ctrl, 1);
	} else {
		lsm6dsr_int2_ctrl_t int2_ctrl;

		lsm6dsr_read_reg(ctx, LSM6DSR_INT2_CTRL,
				 (uint8_t *)&int2_ctrl, 1);
		int2_ctrl.int2_drdy_g = enable;
		return lsm6dsr_write_reg(ctx, LSM6DSR_INT2_CTRL,
					 (uint8_t *)&int2_ctrl, 1);
	}
}

/**
 * lsm6dsr_trigger_set - link external trigger to event data ready
 */
int lsm6dsr_trigger_set(const struct device *dev,
			  const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler)
{
	const struct lsm6dsr_config *cfg = dev->config;
	struct lsm6dsr_data *lsm6dsr = dev->data;

	__ASSERT_NO_MSG(trig->type == SENSOR_TRIG_DATA_READY);

	if (!cfg->trig_enabled) {
		LOG_ERR("trigger_set op not supported");
		return -ENOTSUP;
	}

	if (trig->chan == SENSOR_CHAN_ACCEL_XYZ) {
		lsm6dsr->handler_drdy_acc = handler;
		lsm6dsr->trig_drdy_acc = trig;
		if (handler) {
			return lsm6dsr_enable_xl_int(dev, LSM6DSR_EN_BIT);
		} else {
			return lsm6dsr_enable_xl_int(dev, LSM6DSR_DIS_BIT);
		}
	} else if (trig->chan == SENSOR_CHAN_GYRO_XYZ) {
		lsm6dsr->handler_drdy_gyr = handler;
		lsm6dsr->trig_drdy_gyr = trig;
		if (handler) {
			return lsm6dsr_enable_g_int(dev, LSM6DSR_EN_BIT);
		} else {
			return lsm6dsr_enable_g_int(dev, LSM6DSR_DIS_BIT);
		}
	}
#if defined(CONFIG_LSM6DSR_ENABLE_TEMP)
	else if (trig->chan == SENSOR_CHAN_DIE_TEMP) {
		lsm6dsr->handler_drdy_temp = handler;
		lsm6dsr->trig_drdy_temp = trig;
		if (handler) {
			return lsm6dsr_enable_t_int(dev, LSM6DSR_EN_BIT);
		} else {
			return lsm6dsr_enable_t_int(dev, LSM6DSR_DIS_BIT);
		}
	}
#endif

	return -ENOTSUP;
}

/**
 * lsm6dsr_handle_interrupt - handle the drdy event
 * read data and call handler if registered any
 */
static void lsm6dsr_handle_interrupt(const struct device *dev)
{
	struct lsm6dsr_data *lsm6dsr = dev->data;
	const struct lsm6dsr_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lsm6dsr_status_reg_t status;

	while (1) {
		if (lsm6dsr_status_reg_get(ctx, &status) < 0) {
			LOG_DBG("failed reading status reg");
			return;
		}

		if ((status.xlda == 0) && (status.gda == 0)
#if defined(CONFIG_LSM6DSR_ENABLE_TEMP)
			&& (status.tda == 0)
#endif
		) {
			break;
		}

		if ((status.xlda) && (lsm6dsr->handler_drdy_acc != NULL)) {
			lsm6dsr->handler_drdy_acc(dev, lsm6dsr->trig_drdy_acc);
		}

		if ((status.gda) && (lsm6dsr->handler_drdy_gyr != NULL)) {
			lsm6dsr->handler_drdy_gyr(dev, lsm6dsr->trig_drdy_gyr);
		}

		if (IS_ENABLED(CONFIG_LSM6DSR_ENABLE_TEMP)) {
			if ((status.tda) && (lsm6dsr->handler_drdy_temp != NULL)) {
				lsm6dsr->handler_drdy_temp(dev, lsm6dsr->trig_drdy_temp);
			}
		}
	}

	gpio_pin_interrupt_configure_dt(&cfg->gpio_drdy,
					GPIO_INT_EDGE_TO_ACTIVE);
}

static void handle_irq(const struct device *dev)
{
	struct lsm6dsr_data *lsm6dsr = dev->data;
	const struct lsm6dsr_config *cfg = dev->config;

	gpio_pin_interrupt_configure_dt(&cfg->gpio_drdy, GPIO_INT_DISABLE);

#if defined(CONFIG_LSM6DSR_TRIGGER_OWN_THREAD)
		k_sem_give(&lsm6dsr->gpio_sem);
#elif defined(CONFIG_LSM6DSR_TRIGGER_GLOBAL_THREAD)
		k_work_submit(&lsm6dsr->work);
#endif
}

static void lsm6dsr_gpio_callback(const struct device *dev,
				    struct gpio_callback *cb, uint32_t pins)
{
	struct lsm6dsr_data *lsm6dsr =
		CONTAINER_OF(cb, struct lsm6dsr_data, gpio_cb);

	ARG_UNUSED(pins);

	handle_irq(lsm6dsr->dev);
}

#if defined(CONFIG_LSM6DSR_TRIGGER_OWN_THREAD)
static void lsm6dsr_thread(struct lsm6dsr_data *lsm6dsr)
{
	while (1) {
		k_sem_take(&lsm6dsr->gpio_sem, K_FOREVER);
		lsm6dsr_handle_interrupt(lsm6dsr->dev);
	}
}
#endif

#if defined(CONFIG_LSM6DSR_TRIGGER_GLOBAL_THREAD)
static void lsm6dsr_work_cb(struct k_work *work)
{
	struct lsm6dsr_data *lsm6dsr =
		CONTAINER_OF(work, struct lsm6dsr_data, work);

	lsm6dsr_handle_interrupt(lsm6dsr->dev);
}
#endif /* defined(CONFIG_LSM6DSR_TRIGGER_GLOBAL_THREAD) */

void lsm6dsr_trigger_init(const struct device *dev)
{
	struct lsm6dsr_data *lsm6dsr = dev->data;
#if defined(CONFIG_LSM6DSR_TRIGGER_OWN_THREAD)
	k_sem_init(&lsm6dsr->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&lsm6dsr->thread, lsm6dsr->thread_stack,
			CONFIG_LSM6DSR_THREAD_STACK_SIZE,
			(k_thread_entry_t)lsm6dsr_thread, lsm6dsr,
			NULL, NULL, K_PRIO_COOP(CONFIG_LSM6DSR_THREAD_PRIORITY),
			0, K_NO_WAIT);
	k_thread_name_set(&lsm6dsr->thread, "lsm6dsr");
#elif defined(CONFIG_LSM6DSR_TRIGGER_GLOBAL_THREAD)
	lsm6dsr->work.handler = lsm6dsr_work_cb;
#endif
}

int lsm6dsr_init_interrupt(const struct device *dev)
{
	struct lsm6dsr_data *lsm6dsr = dev->data;
	const struct lsm6dsr_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	int ret;

	/* setup data ready gpio interrupt (INT1 or INT2) */
	if (!device_is_ready(cfg->gpio_drdy.port)) {
		LOG_ERR("Cannot get pointer to drdy_gpio device");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_LSM6DSR_TRIGGER)) {
		lsm6dsr_trigger_init(dev);
	}

	ret = gpio_pin_configure_dt(&cfg->gpio_drdy, GPIO_INPUT);
	if (ret < 0) {
		LOG_DBG("Could not configure gpio");
		return ret;
	}

	gpio_init_callback(&lsm6dsr->gpio_cb,
			   lsm6dsr_gpio_callback,
			   BIT(cfg->gpio_drdy.pin));

	if (gpio_add_callback(cfg->gpio_drdy.port, &lsm6dsr->gpio_cb) < 0) {
		LOG_DBG("Could not set gpio callback.");
		return -EIO;
	}


	/* set data ready mode on int1/int2 */
	LOG_DBG("drdy_pulsed is %d", (int)cfg->drdy_pulsed);
	lsm6dsr_dataready_pulsed_t mode = cfg->drdy_pulsed ? LSM6DSR_DRDY_PULSED :
							     LSM6DSR_DRDY_LATCHED;

	ret = lsm6dsr_data_ready_mode_set(ctx, mode);
	if (ret < 0) {
		LOG_ERR("drdy_pulsed config error %d", (int)cfg->drdy_pulsed);
		return ret;
	}

	return gpio_pin_interrupt_configure_dt(&cfg->gpio_drdy,
					       GPIO_INT_EDGE_TO_ACTIVE);
}
