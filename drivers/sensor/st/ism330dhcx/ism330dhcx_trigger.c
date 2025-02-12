/* ST Microelectronics ISM330DHCX 6-axis IMU sensor driver
 *
 * Copyright (c) 2020 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/ism330dhcx.pdf
 */

#define DT_DRV_COMPAT st_ism330dhcx

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "ism330dhcx.h"

LOG_MODULE_DECLARE(ISM330DHCX, CONFIG_SENSOR_LOG_LEVEL);

#if defined(CONFIG_ISM330DHCX_ENABLE_TEMP)
/**
 * ism330dhcx_enable_t_int - TEMP enable selected int pin to generate interrupt
 */
static int ism330dhcx_enable_t_int(const struct device *dev, int enable)
{
	const struct ism330dhcx_config *cfg = dev->config;
	struct ism330dhcx_data *ism330dhcx = dev->data;
	ism330dhcx_pin_int2_route_t int2_route;

	if (enable) {
		int16_t buf;

		/* dummy read: re-trigger interrupt */
		ism330dhcx_temperature_raw_get(ism330dhcx->ctx, &buf);
	}

	/* set interrupt (TEMP DRDY interrupt is only on INT2) */
	if (cfg->int_pin == 1)
		return -EIO;

	ism330dhcx_read_reg(ism330dhcx->ctx, ISM330DHCX_INT2_CTRL,
			    (uint8_t *)&int2_route.int2_ctrl, 1);
	int2_route.int2_ctrl.int2_drdy_temp = enable;
	return ism330dhcx_write_reg(ism330dhcx->ctx, ISM330DHCX_INT2_CTRL,
				    (uint8_t *)&int2_route.int2_ctrl, 1);
}
#endif

/**
 * ism330dhcx_enable_xl_int - XL enable selected int pin to generate interrupt
 */
static int ism330dhcx_enable_xl_int(const struct device *dev, int enable)
{
	const struct ism330dhcx_config *cfg = dev->config;
	struct ism330dhcx_data *ism330dhcx = dev->data;

	if (enable) {
		int16_t buf[3];

		/* dummy read: re-trigger interrupt */
		ism330dhcx_acceleration_raw_get(ism330dhcx->ctx, buf);
	}

	/* set interrupt */
	if (cfg->int_pin == 1) {
		ism330dhcx_pin_int1_route_t int1_route;

		ism330dhcx_read_reg(ism330dhcx->ctx, ISM330DHCX_INT1_CTRL,
				    (uint8_t *)&int1_route.int1_ctrl, 1);

		int1_route.int1_ctrl.int1_drdy_xl = enable;
		return ism330dhcx_write_reg(ism330dhcx->ctx, ISM330DHCX_INT1_CTRL,
					    (uint8_t *)&int1_route.int1_ctrl, 1);
	} else {
		ism330dhcx_pin_int2_route_t int2_route;

		ism330dhcx_read_reg(ism330dhcx->ctx, ISM330DHCX_INT2_CTRL,
				    (uint8_t *)&int2_route.int2_ctrl, 1);
		int2_route.int2_ctrl.int2_drdy_xl = enable;
		return ism330dhcx_write_reg(ism330dhcx->ctx, ISM330DHCX_INT2_CTRL,
					    (uint8_t *)&int2_route.int2_ctrl, 1);
	}
}

/**
 * ism330dhcx_enable_g_int - Gyro enable selected int pin to generate interrupt
 */
static int ism330dhcx_enable_g_int(const struct device *dev, int enable)
{
	const struct ism330dhcx_config *cfg = dev->config;
	struct ism330dhcx_data *ism330dhcx = dev->data;

	if (enable) {
		int16_t buf[3];

		/* dummy read: re-trigger interrupt */
		ism330dhcx_angular_rate_raw_get(ism330dhcx->ctx, buf);
	}

	/* set interrupt */
	if (cfg->int_pin == 1) {
		ism330dhcx_pin_int1_route_t int1_route;

		ism330dhcx_read_reg(ism330dhcx->ctx, ISM330DHCX_INT1_CTRL,
				 (uint8_t *)&int1_route.int1_ctrl, 1);
		int1_route.int1_ctrl.int1_drdy_g = enable;
		return ism330dhcx_write_reg(ism330dhcx->ctx, ISM330DHCX_INT1_CTRL,
					    (uint8_t *)&int1_route.int1_ctrl, 1);
	} else {
		ism330dhcx_pin_int2_route_t int2_route;

		ism330dhcx_read_reg(ism330dhcx->ctx, ISM330DHCX_INT2_CTRL,
				 (uint8_t *)&int2_route.int2_ctrl, 1);
		int2_route.int2_ctrl.int2_drdy_g = enable;
		return ism330dhcx_write_reg(ism330dhcx->ctx, ISM330DHCX_INT2_CTRL,
					    (uint8_t *)&int2_route.int2_ctrl, 1);
	}
}

/**
 * ism330dhcx_trigger_set - link external trigger to event data ready
 */
int ism330dhcx_trigger_set(const struct device *dev,
			   const struct sensor_trigger *trig,
			   sensor_trigger_handler_t handler)
{
	struct ism330dhcx_data *ism330dhcx = dev->data;
	const struct ism330dhcx_config *cfg = dev->config;

	if (!cfg->drdy_gpio.port) {
		return -ENOTSUP;
	}

	if (trig->chan == SENSOR_CHAN_ACCEL_XYZ) {
		ism330dhcx->handler_drdy_acc = handler;
		ism330dhcx->trig_drdy_acc = trig;
		if (handler) {
			return ism330dhcx_enable_xl_int(dev, ISM330DHCX_EN_BIT);
		} else {
			return ism330dhcx_enable_xl_int(dev, ISM330DHCX_DIS_BIT);
		}
	} else if (trig->chan == SENSOR_CHAN_GYRO_XYZ) {
		ism330dhcx->handler_drdy_gyr = handler;
		ism330dhcx->trig_drdy_gyr = trig;
		if (handler) {
			return ism330dhcx_enable_g_int(dev, ISM330DHCX_EN_BIT);
		} else {
			return ism330dhcx_enable_g_int(dev, ISM330DHCX_DIS_BIT);
		}
	}
#if defined(CONFIG_ISM330DHCX_ENABLE_TEMP)
	else if (trig->chan == SENSOR_CHAN_DIE_TEMP) {
		ism330dhcx->handler_drdy_temp = handler;
		ism330dhcx->trig_drdy_temp = trig;
		if (handler) {
			return ism330dhcx_enable_t_int(dev, ISM330DHCX_EN_BIT);
		} else {
			return ism330dhcx_enable_t_int(dev, ISM330DHCX_DIS_BIT);
		}
	}
#endif

	return -ENOTSUP;
}

/**
 * ism330dhcx_handle_interrupt - handle the drdy event
 * read data and call handler if registered any
 */
static void ism330dhcx_handle_interrupt(const struct device *dev)
{
	struct ism330dhcx_data *ism330dhcx = dev->data;
	const struct ism330dhcx_config *cfg = dev->config;
	ism330dhcx_status_reg_t status;

	while (1) {
		if (ism330dhcx_status_reg_get(ism330dhcx->ctx, &status) < 0) {
			LOG_DBG("failed reading status reg");
			return;
		}

		if ((status.xlda == 0) && (status.gda == 0)
#if defined(CONFIG_ISM330DHCX_ENABLE_TEMP)
					&& (status.tda == 0)
#endif
					) {
			break;
		}

		if ((status.xlda) && (ism330dhcx->handler_drdy_acc != NULL)) {
			ism330dhcx->handler_drdy_acc(dev, ism330dhcx->trig_drdy_acc);
		}

		if ((status.gda) && (ism330dhcx->handler_drdy_gyr != NULL)) {
			ism330dhcx->handler_drdy_gyr(dev, ism330dhcx->trig_drdy_gyr);
		}

#if defined(CONFIG_ISM330DHCX_ENABLE_TEMP)
		if ((status.tda) && (ism330dhcx->handler_drdy_temp != NULL)) {
			ism330dhcx->handler_drdy_temp(dev, ism330dhcx->trig_drdy_temp);
		}
#endif
	}

	gpio_pin_interrupt_configure_dt(&cfg->drdy_gpio, GPIO_INT_EDGE_TO_ACTIVE);
}

static void ism330dhcx_gpio_callback(const struct device *dev,
				     struct gpio_callback *cb, uint32_t pins)
{
	struct ism330dhcx_data *ism330dhcx =
		CONTAINER_OF(cb, struct ism330dhcx_data, gpio_cb);
	const struct ism330dhcx_config *cfg = ism330dhcx->dev->config;

	ARG_UNUSED(pins);

	gpio_pin_interrupt_configure_dt(&cfg->drdy_gpio, GPIO_INT_DISABLE);

#if defined(CONFIG_ISM330DHCX_TRIGGER_OWN_THREAD)
	k_sem_give(&ism330dhcx->gpio_sem);
#elif defined(CONFIG_ISM330DHCX_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&ism330dhcx->work);
#endif /* CONFIG_ISM330DHCX_TRIGGER_OWN_THREAD */
}

#ifdef CONFIG_ISM330DHCX_TRIGGER_OWN_THREAD
static void ism330dhcx_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct ism330dhcx_data *ism330dhcx = p1;

	while (1) {
		k_sem_take(&ism330dhcx->gpio_sem, K_FOREVER);
		ism330dhcx_handle_interrupt(ism330dhcx->dev);
	}
}
#endif /* CONFIG_ISM330DHCX_TRIGGER_OWN_THREAD */

#ifdef CONFIG_ISM330DHCX_TRIGGER_GLOBAL_THREAD
static void ism330dhcx_work_cb(struct k_work *work)
{
	struct ism330dhcx_data *ism330dhcx =
		CONTAINER_OF(work, struct ism330dhcx_data, work);

	ism330dhcx_handle_interrupt(ism330dhcx->dev);
}
#endif /* CONFIG_ISM330DHCX_TRIGGER_GLOBAL_THREAD */

int ism330dhcx_init_interrupt(const struct device *dev)
{
	struct ism330dhcx_data *ism330dhcx = dev->data;
	const struct ism330dhcx_config *cfg = dev->config;
	int ret;

	if (!gpio_is_ready_dt(&cfg->drdy_gpio)) {
		LOG_ERR("GPIO device not ready");
		return -ENODEV;
	}

#if defined(CONFIG_ISM330DHCX_TRIGGER_OWN_THREAD)
	k_sem_init(&ism330dhcx->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&ism330dhcx->thread, ism330dhcx->thread_stack,
			CONFIG_ISM330DHCX_THREAD_STACK_SIZE,
			ism330dhcx_thread,
			ism330dhcx, NULL, NULL,
			K_PRIO_COOP(CONFIG_ISM330DHCX_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif defined(CONFIG_ISM330DHCX_TRIGGER_GLOBAL_THREAD)
	ism330dhcx->work.handler = ism330dhcx_work_cb;
#endif /* CONFIG_ISM330DHCX_TRIGGER_OWN_THREAD */

	ret = gpio_pin_configure_dt(&cfg->drdy_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure gpio");
		return ret;
	}

	gpio_init_callback(&ism330dhcx->gpio_cb, ism330dhcx_gpio_callback, BIT(cfg->drdy_gpio.pin));

	if (gpio_add_callback(cfg->drdy_gpio.port, &ism330dhcx->gpio_cb) < 0) {
		LOG_ERR("Could not set gpio callback");
		return -EIO;
	}

	/* enable interrupt on int1/int2 in pulse mode */
	if (ism330dhcx_data_ready_mode_set(ism330dhcx->ctx,
					   ISM330DHCX_DRDY_PULSED) < 0) {
		LOG_ERR("Could not set pulse mode");
		return -EIO;
	}

	return gpio_pin_interrupt_configure_dt(&cfg->drdy_gpio, GPIO_INT_EDGE_TO_ACTIVE);
}
