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
	if (cfg->int_pin == 1) {
		return -EIO;
	}

	lsm6dso_read_reg(ctx, LSM6DSO_INT2_CTRL, (uint8_t *)&int2_ctrl, 1);
	int2_ctrl.int2_drdy_temp = enable;
	return lsm6dso_write_reg(ctx, LSM6DSO_INT2_CTRL,
				 (uint8_t *)&int2_ctrl, 1);
}
#endif

/**
 * lsm6dso_enable_xl_int - XL enable selected int pin to generate interrupt
 * when data is ready.
 */
static int lsm6dso_enable_xl_drdy_int(const struct device *dev, int enable)
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
 * lsm6dso_enable_xl_motion_int - XL enable selected int pin to generate
 * interrupt when any motion is detected.
 */
static int lsm6dso_enable_xl_delta_int(const struct device *dev, int enable)
{
	const struct lsm6dso_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;

	int rc;

	/* Only configure the tap registers when we're enabling, as we'll
	 * otherwise get a spurious interrupt when this returns!
	 */
	if (enable) {
		lsm6dso_tap_cfg0_t tap_cfg0;
		lsm6dso_tap_cfg2_t tap_cfg2;

		rc = lsm6dso_read_reg(ctx, LSM6DSO_TAP_CFG0, (uint8_t *)&tap_cfg0, 1);
		if (rc < 0) {
			return rc;
		}

		/* Output state changes, and use the slope filter. */
		tap_cfg0.sleep_status_on_int = 0;
		tap_cfg0.slope_fds = 0;

		rc = lsm6dso_write_reg(ctx, LSM6DSO_TAP_CFG0, (uint8_t *)&tap_cfg0, 1);
		if (rc < 0) {
			return rc;
		}

		/* Enable motion/stationary detection interrupts, and don't change the
		 * ODR when stationary (i.e. not using active/inactive mode).
		 */
		rc = lsm6dso_read_reg(ctx, LSM6DSO_TAP_CFG2, (uint8_t *)&tap_cfg2, 1);
		if (rc < 0) {
			return rc;
		}

		tap_cfg2.interrupts_enable = enable;
		tap_cfg2.inact_en = 0;

		rc = lsm6dso_write_reg(ctx, LSM6DSO_TAP_CFG2, (uint8_t *)&tap_cfg2, 1);
		if (rc < 0) {
			return rc;
		}
	}

	/* Route the motion/stationary detection interrupt to the correct pin. */
	if (cfg->int_pin == 1) {
		lsm6dso_md1_cfg_t md1_cfg;

		rc = lsm6dso_read_reg(ctx, LSM6DSO_MD1_CFG, (uint8_t *)&md1_cfg, 1);
		if (rc < 0) {
			return rc;
		}

		md1_cfg.int1_sleep_change = enable;

		rc = lsm6dso_write_reg(ctx, LSM6DSO_MD1_CFG, (uint8_t *)&md1_cfg, 1);
		if (rc < 0) {
			return rc;
		}
	} else {
		lsm6dso_md2_cfg_t md2_cfg;

		rc = lsm6dso_read_reg(ctx, LSM6DSO_MD2_CFG, (uint8_t *)&md2_cfg, 1);
		if (rc < 0) {
			return rc;
		}

		md2_cfg.int2_sleep_change = enable;

		rc = lsm6dso_write_reg(ctx, LSM6DSO_MD2_CFG, (uint8_t *)&md2_cfg, 1);
		if (rc < 0) {
			return rc;
		}
	}

	return 0;
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
		int enable = handler ? LSM6DSO_EN_BIT : LSM6DSO_DIS_BIT;

		if (trig->type == SENSOR_TRIG_DATA_READY) {
			lsm6dso->handler_drdy_acc = handler;
			lsm6dso->trig_drdy_acc = trig;
			return lsm6dso_enable_xl_drdy_int(dev, enable);
		} else if (trig->type == SENSOR_TRIG_DELTA) {
			lsm6dso->handler_delta_acc = handler;
			lsm6dso->trig_delta_acc = trig;
			return lsm6dso_enable_xl_delta_int(dev, enable);
		}
	} else if (trig->chan == SENSOR_CHAN_GYRO_XYZ) {
		lsm6dso->handler_drdy_gyr = handler;
		lsm6dso->trig_drdy_gyr = trig;
		if (handler) {
			return lsm6dso_enable_g_int(dev, LSM6DSO_EN_BIT);
		} else {
			return lsm6dso_enable_g_int(dev, LSM6DSO_DIS_BIT);
		}
	}
#if defined(CONFIG_LSM6DSO_ENABLE_TEMP)
	else if (trig->chan == SENSOR_CHAN_DIE_TEMP) {
		lsm6dso->handler_drdy_temp = handler;
		lsm6dso->trig_drdy_temp = trig;
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
	const struct lsm6dso_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lsm6dso_all_sources_t all_sources;

	while (1) {
		if (lsm6dso_all_sources_get(ctx, &all_sources) < 0) {
			LOG_ERR("failed reading all int sources");
			return;
		}

		if ((all_sources.drdy_xl == 0) && (all_sources.drdy_g == 0)
#if defined(CONFIG_LSM6DSO_ENABLE_TEMP)
					&& (all_sources.drdy_temp == 0)
#endif
					&& (all_sources.sleep_change == 0)
					) {
			break;
		}

		if ((all_sources.drdy_xl) && (lsm6dso->handler_drdy_acc != NULL)) {
			lsm6dso->handler_drdy_acc(dev, lsm6dso->trig_drdy_acc);
		}

		if ((all_sources.drdy_g) && (lsm6dso->handler_drdy_gyr != NULL)) {
			lsm6dso->handler_drdy_gyr(dev, lsm6dso->trig_drdy_gyr);
		}

#if defined(CONFIG_LSM6DSO_ENABLE_TEMP)
		if ((all_sources.drdy_temp) && (lsm6dso->handler_drdy_temp != NULL)) {
			lsm6dso->handler_drdy_temp(dev, lsm6dso->trig_drdy_temp);
		}
#endif

		if ((all_sources.sleep_change) &&
					(lsm6dso->handler_delta_acc != NULL)) {
			lsm6dso->handler_delta_acc(dev, lsm6dso->trig_delta_acc);
		}
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
static void lsm6dso_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct lsm6dso_data *lsm6dso = p1;

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
	if (!gpio_is_ready_dt(&cfg->gpio_drdy)) {
		LOG_ERR("Cannot get pointer to drdy_gpio device");
		return -EINVAL;
	}

#if defined(CONFIG_LSM6DSO_TRIGGER_OWN_THREAD)
	k_sem_init(&lsm6dso->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&lsm6dso->thread, lsm6dso->thread_stack,
			CONFIG_LSM6DSO_THREAD_STACK_SIZE,
			lsm6dso_thread, lsm6dso,
			NULL, NULL, K_PRIO_COOP(CONFIG_LSM6DSO_THREAD_PRIORITY),
			0, K_NO_WAIT);
	k_thread_name_set(&lsm6dso->thread, "lsm6dso");
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


	/* set data ready mode on int1/int2 */
	LOG_DBG("drdy_pulsed is %d", (int)cfg->drdy_pulsed);
	lsm6dso_dataready_pulsed_t mode = cfg->drdy_pulsed ? LSM6DSO_DRDY_PULSED :
							     LSM6DSO_DRDY_LATCHED;

	ret = lsm6dso_data_ready_mode_set(ctx, mode);
	if (ret < 0) {
		LOG_ERR("drdy_pulsed config error %d", (int)cfg->drdy_pulsed);
		return ret;
	}

	return gpio_pin_interrupt_configure_dt(&cfg->gpio_drdy,
					       GPIO_INT_EDGE_TO_ACTIVE);
}
