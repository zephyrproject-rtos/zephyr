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

#if defined(CONFIG_LSM6DSO_TILT)

static int lsm6dso_enable_tilt_int(const struct device *dev, int enable)
{
	const struct lsm6dso_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	int ret = 0;
	lsm6dso_emb_sens_t sens;

	sens.tilt = enable;

	ret += lsm6dso_embedded_sens_set(ctx, &sens);
	if (ret < 0) {
		LOG_ERR("Failed to enable tilt");
		return -EIO;
	}

	if (cfg->int_pin == 1) {

		lsm6dso_pin_int1_route_t route;

		lsm6dso_pin_int1_route_get(ctx, &route);
		route.tilt = enable;

		ret += lsm6dso_pin_int1_route_set(ctx, route);
		if (ret < 0) {
			LOG_ERR("Failed to set int1 route");
			return -EIO;
		}

	} else {

		lsm6dso_pin_int2_route_t route;

		lsm6dso_pin_int2_route_get(ctx, NULL, &route);
		route.tilt = enable;

		ret += lsm6dso_pin_int2_route_set(ctx, NULL, route);
		if (ret < 0) {
			LOG_ERR("Failed to set int2 route");
			return -EIO;
		}

	}

	return 0;
}

#endif

#if defined(CONFIG_LSM6DSO_TAP)

static int lsm6dso_enable_tap(const struct device *dev, int enable)
{
	const struct lsm6dso_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lsm6dso_odr_xl_t odr;

	if (lsm6dso_xl_data_rate_get(ctx, &odr) < 0) {
		LOG_ERR("Unable to read accelerometer ODR");
		return -EIO;
	}

	if (odr < LSM6DSO_XL_ODR_417Hz) {
		LOG_WRN("Minimum recommended accelerometer ODR is 417Hz for tap mode");
	}

	LOG_DBG("TAP: tap mode is %d", cfg->tap_mode);
	if (lsm6dso_tap_mode_set(ctx, cfg->tap_mode) < 0) {
		LOG_ERR("Failed to select tap trigger mode");
		return -EIO;
	}

	LOG_DBG("TAP: ths_x is %02x", cfg->tap_threshold[0]);
	if (lsm6dso_tap_threshold_x_set(ctx, cfg->tap_threshold[0]) < 0) {
		LOG_ERR("Failed to set tap X axis threshold");
		return -EIO;
	}

	LOG_DBG("TAP: ths_y is %02x", cfg->tap_threshold[1]);
	if (lsm6dso_tap_threshold_y_set(ctx, cfg->tap_threshold[1]) < 0) {
		LOG_ERR("Failed to set tap Y axis threshold");
		return -EIO;
	}

	LOG_DBG("TAP: ths_z is %02x", cfg->tap_threshold[2]);
	if (lsm6dso_tap_threshold_z_set(ctx, cfg->tap_threshold[2]) < 0) {
		LOG_ERR("Failed to set tap Z axis threshold");
		return -EIO;
	}

	if (cfg->tap_threshold[0] > 0) {
		LOG_DBG("TAP: tap_x enabled");
		if (lsm6dso_tap_detection_on_x_set(ctx, enable) < 0) {
			LOG_ERR("Failed to set tap detection on X axis");
			return -EIO;
		}
	}

	if (cfg->tap_threshold[1] > 0) {
		LOG_DBG("TAP: tap_y enabled");
		if (lsm6dso_tap_detection_on_y_set(ctx, enable) < 0) {
			LOG_ERR("Failed to set tap detection on Y axis");
			return -EIO;
		}
	}

	if (cfg->tap_threshold[2] > 0) {
		LOG_DBG("TAP: tap_z enabled");
		if (lsm6dso_tap_detection_on_z_set(ctx, enable) < 0) {
			LOG_ERR("Failed to set tap detection on Z axis");
			return -EIO;
		}
	}

	LOG_DBG("TAP: shock is %02x", cfg->tap_shock);
	if (lsm6dso_tap_shock_set(ctx, cfg->tap_shock) < 0) {
		LOG_ERR("Failed to set tap shock duration");
		return -EIO;
	}

	LOG_DBG("TAP: latency is %02x", cfg->tap_latency);
	if (lsm6dso_tap_dur_set(ctx, cfg->tap_latency) < 0) {
		LOG_ERR("Failed to set tap latency");
		return -EIO;
	}

	/* Set tap quiet */
	LOG_DBG("TAP: quiet time is %02x", cfg->tap_quiet);
	if (lsm6dso_tap_quiet_set(ctx, cfg->tap_quiet) < 0) {
		LOG_ERR("Failed to set tap quiet time");
		return -EIO;
	}

	return 0;
}

static int lsm6dso_enable_single_tap_int(const struct device *dev, int enable)
{
	const struct lsm6dso_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;

	if (cfg->int_pin == 1) {

		lsm6dso_pin_int1_route_t route;

		lsm6dso_pin_int1_route_get(ctx, &route);
		route.single_tap = enable;
		return lsm6dso_pin_int1_route_set(ctx, route);

	} else {

		lsm6dso_pin_int2_route_t route;

		lsm6dso_pin_int2_route_get(ctx, NULL, &route);
		route.single_tap = enable;
		return lsm6dso_pin_int2_route_set(ctx, NULL, route);
	}

}

static int lsm6dso_enable_double_tap_int(const struct device *dev, int enable)
{
	const struct lsm6dso_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;

	if (cfg->int_pin == 1) {

		lsm6dso_pin_int1_route_t route;

		lsm6dso_pin_int1_route_get(ctx, &route);
		route.double_tap = enable;
		return lsm6dso_pin_int1_route_set(ctx, route);

	} else {

		lsm6dso_pin_int2_route_t route;

		lsm6dso_pin_int2_route_get(ctx, NULL, &route);
		route.double_tap = enable;
		return lsm6dso_pin_int2_route_set(ctx, NULL, route);
	}
}

#endif /* CONFIG_LSM6DSO_TAP */

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

	if (trig->type == SENSOR_TRIG_DATA_READY) {

		if (trig->chan == SENSOR_CHAN_ACCEL_XYZ) {
			lsm6dso->handler_drdy_acc = handler;
			lsm6dso->trig_drdy_acc = trig;
			if (handler) {
				return lsm6dso_enable_xl_int(dev, LSM6DSO_EN_BIT);
			} else {
				return lsm6dso_enable_xl_int(dev, LSM6DSO_DIS_BIT);
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
		else {

			return -ENOTSUP;

		}

	}
#if defined(CONFIG_LSM6DSO_TILT)
	else if (trig->type == SENSOR_TRIG_TILT) {

		lsm6dso->handler_tilt = handler;
		lsm6dso->trig_tilt = trig;
		if (handler) {
			return lsm6dso_enable_tilt_int(dev, LSM6DSO_EN_BIT);
		} else {
			return lsm6dso_enable_tilt_int(dev, LSM6DSO_DIS_BIT);
		}

	}
#endif
#if defined(CONFIG_LSM6DSO_TAP)
	else if (trig->type == SENSOR_TRIG_TAP || trig->type == SENSOR_TRIG_DOUBLE_TAP) {

		int ret = lsm6dso_enable_tap(dev, handler ? LSM6DSO_EN_BIT : LSM6DSO_DIS_BIT);

		if (ret < 0) {
			return ret;
		}

		/* Set interrupt */
		if (trig->type == SENSOR_TRIG_TAP) {

			lsm6dso->handler_tap = handler;
			lsm6dso->trig_tap = trig;
			if (handler) {
				return lsm6dso_enable_single_tap_int(dev, LSM6DSO_EN_BIT);
			} else {
				return lsm6dso_enable_single_tap_int(dev, LSM6DSO_DIS_BIT);
			}

		} else if (trig->type == SENSOR_TRIG_DOUBLE_TAP) {

			lsm6dso->handler_double_tap = handler;
			lsm6dso->trig_double_tap = trig;
			if (handler) {
				return lsm6dso_enable_double_tap_int(dev, LSM6DSO_EN_BIT);
			} else {
				return lsm6dso_enable_double_tap_int(dev, LSM6DSO_DIS_BIT);
			}
		}
	}
#endif /* CONFIG_LSM6DSO_TAP */
	return -ENOTSUP;

}

/**
 * lsm6dso_handle_interrupt - handle the drdy event
 * read data and call handler if registered any
 */
#if defined(CONFIG_LSM6DSO_TILT) || defined(CONFIG_LSM6DSO_TAP)

static void lsm6dso_handle_interrupt(const struct device *dev)
{
	struct lsm6dso_data *lsm6dso = dev->data;
	const struct lsm6dso_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lsm6dso_all_sources_t sources;
	bool pending_status = false;

	while (1) {

		if (lsm6dso_all_sources_get(ctx, &sources) < 0) {
			LOG_ERR("failed reading all sources");
			return;
		}

		if ((sources.drdy_xl && lsm6dso->handler_drdy_acc != NULL) ||
		    (sources.drdy_g && lsm6dso->handler_drdy_gyr != NULL)
#if defined(CONFIG_LSM6DSO_ENABLE_TEMP)
		    || (sources.drdy_temp && lsm6dso->handler_drdy_temp != NULL)
#endif
		) {
			pending_status = true;
		} else {
			pending_status = false;
		}

		if ((sources.drdy_xl) && (lsm6dso->handler_drdy_acc != NULL)) {
			lsm6dso->handler_drdy_acc(dev, lsm6dso->trig_drdy_acc);
		}

		if ((sources.drdy_g) && (lsm6dso->handler_drdy_gyr != NULL)) {
			lsm6dso->handler_drdy_gyr(dev, lsm6dso->trig_drdy_gyr);
		}

#if defined(CONFIG_LSM6DSO_ENABLE_TEMP)
		if ((sources.drdy_temp) && (lsm6dso->handler_drdy_temp != NULL)) {
			lsm6dso->handler_drdy_temp(dev, lsm6dso->trig_drdy_temp);
		}
#endif

		if (!pending_status) {
			break;
		}

	}

#if defined(CONFIG_LSM6DSO_TILT)
	if (sources.tilt && (lsm6dso->handler_tilt != NULL)) {
		lsm6dso->handler_tilt(dev, lsm6dso->trig_tilt);
	}
#endif /* CONFIG_LSM6DSO_TILT */
#if defined(CONFIG_LSM6DSO_TAP)

	if (sources.single_tap && lsm6dso->handler_tap != NULL) {
		lsm6dso->handler_tap(dev, lsm6dso->trig_tap);
	}

	if (sources.double_tap && lsm6dso->handler_double_tap != NULL) {
		lsm6dso->handler_double_tap(dev, lsm6dso->trig_double_tap);
	}
#endif /* CONFIG_LSM6DSO_TAP */

	gpio_pin_interrupt_configure_dt(&cfg->gpio_drdy,
					GPIO_INT_EDGE_TO_ACTIVE);
}

#else /* defined(CONFIG_LSM6DSO_TILT) || defined(CONFIG_LSM6DSO_TAP) */

static void lsm6dso_handle_interrupt(const struct device *dev)
{
	struct lsm6dso_data *lsm6dso = dev->data;
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
			lsm6dso->handler_drdy_acc(dev, lsm6dso->trig_drdy_acc);
		}

		if ((status.gda) && (lsm6dso->handler_drdy_gyr != NULL)) {
			lsm6dso->handler_drdy_gyr(dev, lsm6dso->trig_drdy_gyr);
		}

#if defined(CONFIG_LSM6DSO_ENABLE_TEMP)
		if ((status.tda) && (lsm6dso->handler_drdy_temp != NULL)) {
			lsm6dso->handler_drdy_temp(dev, lsm6dso->trig_drdy_temp);
		}
#endif
	}

	gpio_pin_interrupt_configure_dt(&cfg->gpio_drdy, GPIO_INT_EDGE_TO_ACTIVE);
}

#endif /* defined(CONFIG_LSM6DSO_TILT) || defined(CONFIG_LSM6DSO_TAP) */

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
