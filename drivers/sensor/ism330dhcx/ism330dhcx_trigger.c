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

#include <kernel.h>
#include <drivers/sensor.h>
#include <drivers/gpio.h>
#include <logging/log.h>

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
		union axis1bit16_t buf;

		/* dummy read: re-trigger interrupt */
		ism330dhcx_temperature_raw_get(ism330dhcx->ctx, buf.u8bit);
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
		union axis3bit16_t buf;

		/* dummy read: re-trigger interrupt */
		ism330dhcx_acceleration_raw_get(ism330dhcx->ctx, buf.u8bit);
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
		union axis3bit16_t buf;

		/* dummy read: re-trigger interrupt */
		ism330dhcx_angular_rate_raw_get(ism330dhcx->ctx, buf.u8bit);
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

	if (trig->chan == SENSOR_CHAN_ACCEL_XYZ) {
		ism330dhcx->handler_drdy_acc = handler;
		if (handler) {
			return ism330dhcx_enable_xl_int(dev, ISM330DHCX_EN_BIT);
		} else {
			return ism330dhcx_enable_xl_int(dev, ISM330DHCX_DIS_BIT);
		}
	} else if (trig->chan == SENSOR_CHAN_GYRO_XYZ) {
		ism330dhcx->handler_drdy_gyr = handler;
		if (handler) {
			return ism330dhcx_enable_g_int(dev, ISM330DHCX_EN_BIT);
		} else {
			return ism330dhcx_enable_g_int(dev, ISM330DHCX_DIS_BIT);
		}
	}
#if defined(CONFIG_ISM330DHCX_ENABLE_TEMP)
	else if (trig->chan == SENSOR_CHAN_DIE_TEMP) {
		ism330dhcx->handler_drdy_temp = handler;
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
	struct sensor_trigger drdy_trigger = {
		.type = SENSOR_TRIG_DATA_READY,
	};
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
			ism330dhcx->handler_drdy_acc(dev, &drdy_trigger);
		}

		if ((status.gda) && (ism330dhcx->handler_drdy_gyr != NULL)) {
			ism330dhcx->handler_drdy_gyr(dev, &drdy_trigger);
		}

#if defined(CONFIG_ISM330DHCX_ENABLE_TEMP)
		if ((status.tda) && (ism330dhcx->handler_drdy_temp != NULL)) {
			ism330dhcx->handler_drdy_temp(dev, &drdy_trigger);
		}
#endif
	}

	gpio_pin_interrupt_configure(ism330dhcx->gpio, cfg->int_gpio_pin,
				     GPIO_INT_EDGE_TO_ACTIVE);
}

static void ism330dhcx_gpio_callback(const struct device *dev,
				     struct gpio_callback *cb, uint32_t pins)
{
	struct ism330dhcx_data *ism330dhcx =
		CONTAINER_OF(cb, struct ism330dhcx_data, gpio_cb);
	const struct ism330dhcx_config *cfg = ism330dhcx->dev->config;

	ARG_UNUSED(pins);

	gpio_pin_interrupt_configure(ism330dhcx->gpio, cfg->int_gpio_pin,
				     GPIO_INT_DISABLE);

#if defined(CONFIG_ISM330DHCX_TRIGGER_OWN_THREAD)
	k_sem_give(&ism330dhcx->gpio_sem);
#elif defined(CONFIG_ISM330DHCX_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&ism330dhcx->work);
#endif /* CONFIG_ISM330DHCX_TRIGGER_OWN_THREAD */
}

#ifdef CONFIG_ISM330DHCX_TRIGGER_OWN_THREAD
static void ism330dhcx_thread(struct ism330dhcx_data *ism330dhcx)
{
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

	/* setup data ready gpio interrupt (INT1 or INT2) */
	ism330dhcx->gpio = device_get_binding(cfg->int_gpio_port);
	if (ism330dhcx->gpio == NULL) {
		LOG_ERR("Cannot get pointer to %s device", cfg->int_gpio_port);
		return -EINVAL;
	}

#if defined(CONFIG_ISM330DHCX_TRIGGER_OWN_THREAD)
	k_sem_init(&ism330dhcx->gpio_sem, 0, UINT_MAX);

	k_thread_create(&ism330dhcx->thread, ism330dhcx->thread_stack,
			CONFIG_ISM330DHCX_THREAD_STACK_SIZE,
			(k_thread_entry_t)ism330dhcx_thread,
			ism330dhcx, NULL, NULL,
			K_PRIO_COOP(CONFIG_ISM330DHCX_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif defined(CONFIG_ISM330DHCX_TRIGGER_GLOBAL_THREAD)
	ism330dhcx->work.handler = ism330dhcx_work_cb;
#endif /* CONFIG_ISM330DHCX_TRIGGER_OWN_THREAD */

	ret = gpio_pin_configure(ism330dhcx->gpio, cfg->int_gpio_pin,
				 GPIO_INPUT | cfg->int_gpio_flags);
	if (ret < 0) {
		LOG_ERR("Could not configure gpio");
		return ret;
	}

	gpio_init_callback(&ism330dhcx->gpio_cb,
			   ism330dhcx_gpio_callback,
			   BIT(cfg->int_gpio_pin));

	if (gpio_add_callback(ism330dhcx->gpio, &ism330dhcx->gpio_cb) < 0) {
		LOG_ERR("Could not set gpio callback");
		return -EIO;
	}

	/* enable interrupt on int1/int2 in pulse mode */
	if (ism330dhcx_int_notification_set(ism330dhcx->ctx,
					    ISM330DHCX_ALL_INT_PULSED) < 0) {
		LOG_ERR("Could not set pulse mode");
		return -EIO;
	}

	return gpio_pin_interrupt_configure(ism330dhcx->gpio, cfg->int_gpio_pin,
					    GPIO_INT_EDGE_TO_ACTIVE);
}
