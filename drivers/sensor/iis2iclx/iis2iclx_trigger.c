/* ST Microelectronics IIS2ICLX 2-axis accelerometer sensor driver
 *
 * Copyright (c) 2020 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/iis2iclx.pdf
 */

#define DT_DRV_COMPAT st_iis2iclx

#include <kernel.h>
#include <drivers/sensor.h>
#include <drivers/gpio.h>
#include <logging/log.h>

#include "iis2iclx.h"

LOG_MODULE_DECLARE(IIS2ICLX, CONFIG_SENSOR_LOG_LEVEL);

#if defined(CONFIG_IIS2ICLX_ENABLE_TEMP)
/**
 * iis2iclx_enable_t_int - TEMP enable selected int pin to generate interrupt
 */
static int iis2iclx_enable_t_int(const struct device *dev, int enable)
{
	const struct iis2iclx_config *cfg = dev->config;
	struct iis2iclx_data *iis2iclx = dev->data;
	iis2iclx_pin_int2_route_t int2_route;

	if (enable) {
		int16_t buf;

		/* dummy read: re-trigger interrupt */
		iis2iclx_temperature_raw_get(iis2iclx->ctx, &buf);
	}

	/* set interrupt (TEMP DRDY interrupt is only on INT2) */
	if (cfg->int_pin == 1)
		return -EIO;

	iis2iclx_read_reg(iis2iclx->ctx, IIS2ICLX_INT2_CTRL,
			    (uint8_t *)&int2_route.int2_ctrl, 1);
	int2_route.int2_ctrl.int2_drdy_temp = enable;
	return iis2iclx_write_reg(iis2iclx->ctx, IIS2ICLX_INT2_CTRL,
				    (uint8_t *)&int2_route.int2_ctrl, 1);
}
#endif

/**
 * iis2iclx_enable_xl_int - XL enable selected int pin to generate interrupt
 */
static int iis2iclx_enable_xl_int(const struct device *dev, int enable)
{
	const struct iis2iclx_config *cfg = dev->config;
	struct iis2iclx_data *iis2iclx = dev->data;

	if (enable) {
		int16_t buf[3];

		/* dummy read: re-trigger interrupt */
		iis2iclx_acceleration_raw_get(iis2iclx->ctx, buf);
	}

	/* set interrupt */
	if (cfg->int_pin == 1) {
		iis2iclx_pin_int1_route_t int1_route;

		iis2iclx_read_reg(iis2iclx->ctx, IIS2ICLX_INT1_CTRL,
				    (uint8_t *)&int1_route.int1_ctrl, 1);

		int1_route.int1_ctrl.int1_drdy_xl = enable;
		return iis2iclx_write_reg(iis2iclx->ctx, IIS2ICLX_INT1_CTRL,
					    (uint8_t *)&int1_route.int1_ctrl, 1);
	} else {
		iis2iclx_pin_int2_route_t int2_route;

		iis2iclx_read_reg(iis2iclx->ctx, IIS2ICLX_INT2_CTRL,
				    (uint8_t *)&int2_route.int2_ctrl, 1);
		int2_route.int2_ctrl.int2_drdy_xl = enable;
		return iis2iclx_write_reg(iis2iclx->ctx, IIS2ICLX_INT2_CTRL,
					    (uint8_t *)&int2_route.int2_ctrl, 1);
	}
}

/**
 * iis2iclx_trigger_set - link external trigger to event data ready
 */
int iis2iclx_trigger_set(const struct device *dev,
			   const struct sensor_trigger *trig,
			   sensor_trigger_handler_t handler)
{
	struct iis2iclx_data *iis2iclx = dev->data;

	/* If drdy_gpio is not configured in DT just return error */
	if (!iis2iclx->gpio) {
		LOG_ERR("triggers not supported");
		return -ENOTSUP;
	}

	if (trig->chan == SENSOR_CHAN_ACCEL_XYZ) {
		iis2iclx->handler_drdy_acc = handler;
		if (handler) {
			return iis2iclx_enable_xl_int(dev, IIS2ICLX_EN_BIT);
		} else {
			return iis2iclx_enable_xl_int(dev, IIS2ICLX_DIS_BIT);
		}
	}
#if defined(CONFIG_IIS2ICLX_ENABLE_TEMP)
	else if (trig->chan == SENSOR_CHAN_DIE_TEMP) {
		iis2iclx->handler_drdy_temp = handler;
		if (handler) {
			return iis2iclx_enable_t_int(dev, IIS2ICLX_EN_BIT);
		} else {
			return iis2iclx_enable_t_int(dev, IIS2ICLX_DIS_BIT);
		}
	}
#endif

	return -ENOTSUP;
}

/**
 * iis2iclx_handle_interrupt - handle the drdy event
 * read data and call handler if registered any
 */
static void iis2iclx_handle_interrupt(const struct device *dev)
{
	struct iis2iclx_data *iis2iclx = dev->data;
	struct sensor_trigger drdy_trigger = {
		.type = SENSOR_TRIG_DATA_READY,
	};
	const struct iis2iclx_config *cfg = dev->config;
	iis2iclx_status_reg_t status;

	while (1) {
		if (iis2iclx_status_reg_get(iis2iclx->ctx, &status) < 0) {
			LOG_DBG("failed reading status reg");
			return;
		}

		if ((status.xlda == 0)
#if defined(CONFIG_IIS2ICLX_ENABLE_TEMP)
					&& (status.tda == 0)
#endif
					) {
			break;
		}

		if ((status.xlda) && (iis2iclx->handler_drdy_acc != NULL)) {
			iis2iclx->handler_drdy_acc(dev, &drdy_trigger);
		}

#if defined(CONFIG_IIS2ICLX_ENABLE_TEMP)
		if ((status.tda) && (iis2iclx->handler_drdy_temp != NULL)) {
			iis2iclx->handler_drdy_temp(dev, &drdy_trigger);
		}
#endif
	}

	gpio_pin_interrupt_configure(iis2iclx->gpio, cfg->irq_pin,
				     GPIO_INT_EDGE_TO_ACTIVE);
}

static void iis2iclx_gpio_callback(const struct device *dev,
				     struct gpio_callback *cb, uint32_t pins)
{
	struct iis2iclx_data *iis2iclx =
		CONTAINER_OF(cb, struct iis2iclx_data, gpio_cb);
	const struct iis2iclx_config *cfg = iis2iclx->dev->config;

	ARG_UNUSED(pins);

	gpio_pin_interrupt_configure(iis2iclx->gpio, cfg->irq_pin,
				     GPIO_INT_DISABLE);

#if defined(CONFIG_IIS2ICLX_TRIGGER_OWN_THREAD)
	k_sem_give(&iis2iclx->gpio_sem);
#elif defined(CONFIG_IIS2ICLX_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&iis2iclx->work);
#endif /* CONFIG_IIS2ICLX_TRIGGER_OWN_THREAD */
}

#ifdef CONFIG_IIS2ICLX_TRIGGER_OWN_THREAD
static void iis2iclx_thread(struct iis2iclx_data *iis2iclx)
{
	while (1) {
		k_sem_take(&iis2iclx->gpio_sem, K_FOREVER);
		iis2iclx_handle_interrupt(iis2iclx->dev);
	}
}
#endif /* CONFIG_IIS2ICLX_TRIGGER_OWN_THREAD */

#ifdef CONFIG_IIS2ICLX_TRIGGER_GLOBAL_THREAD
static void iis2iclx_work_cb(struct k_work *work)
{
	struct iis2iclx_data *iis2iclx =
		CONTAINER_OF(work, struct iis2iclx_data, work);

	iis2iclx_handle_interrupt(iis2iclx->dev);
}
#endif /* CONFIG_IIS2ICLX_TRIGGER_GLOBAL_THREAD */

int iis2iclx_init_interrupt(const struct device *dev)
{
	struct iis2iclx_data *iis2iclx = dev->data;
	const struct iis2iclx_config *cfg = dev->config;
	int ret;

	/* setup data ready gpio interrupt (INT1 or INT2) */
	iis2iclx->gpio = device_get_binding(cfg->irq_dev_name);
	if (iis2iclx->gpio == NULL) {
		LOG_INF("Cannot get pointer to irq_dev_name");
		goto end;
	}

#if defined(CONFIG_IIS2ICLX_TRIGGER_OWN_THREAD)
	k_sem_init(&iis2iclx->gpio_sem, 0, UINT_MAX);

	k_thread_create(&iis2iclx->thread, iis2iclx->thread_stack,
			CONFIG_IIS2ICLX_THREAD_STACK_SIZE,
			(k_thread_entry_t)iis2iclx_thread,
			iis2iclx, NULL, NULL,
			K_PRIO_COOP(CONFIG_IIS2ICLX_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif defined(CONFIG_IIS2ICLX_TRIGGER_GLOBAL_THREAD)
	iis2iclx->work.handler = iis2iclx_work_cb;
#endif /* CONFIG_IIS2ICLX_TRIGGER_OWN_THREAD */

	ret = gpio_pin_configure(iis2iclx->gpio, cfg->irq_pin,
				 GPIO_INPUT | cfg->irq_flags);
	if (ret < 0) {
		LOG_ERR("Could not configure gpio");
		return ret;
	}

	gpio_init_callback(&iis2iclx->gpio_cb,
			   iis2iclx_gpio_callback,
			   BIT(cfg->irq_pin));

	if (gpio_add_callback(iis2iclx->gpio, &iis2iclx->gpio_cb) < 0) {
		LOG_ERR("Could not set gpio callback");
		return -EIO;
	}

	/* enable interrupt on int1/int2 in pulse mode */
	if (iis2iclx_int_notification_set(iis2iclx->ctx,
					    IIS2ICLX_ALL_INT_PULSED) < 0) {
		LOG_ERR("Could not set pulse mode");
		return -EIO;
	}

	if (gpio_pin_interrupt_configure(iis2iclx->gpio, cfg->irq_pin,
					    GPIO_INT_EDGE_TO_ACTIVE) < 0) {
		LOG_ERR("Could not configure interrupt");
		return -EIO;
	}

end:
	return 0;
}
