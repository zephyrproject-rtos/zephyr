/* ST Microelectronics IIS3DWB 3-axis accelerometer driver
 *
 * Copyright (c) 2020 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/iis3dwb.pdf
 */

#define DT_DRV_COMPAT st_iis3dwb

#include <kernel.h>
#include <drivers/sensor.h>
#include <drivers/gpio.h>
#include <logging/log.h>

#include "iis3dwb.h"

LOG_MODULE_DECLARE(IIS3DWB, CONFIG_SENSOR_LOG_LEVEL);

/**
 * iis3dwb_enable_int - enable selected int pin to generate interrupt
 */
static int iis3dwb_enable_int(const struct device *dev,
			       enum sensor_trigger_type type, int enable)
{
	const struct iis3dwb_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	iis3dwb_pin_int1_route_t int1_route;
	iis3dwb_pin_int2_route_t int2_route;
	int ret;

	switch (type) {
	case SENSOR_TRIG_DATA_READY:
		if (cfg->drdy_int == 1) {
			/* set interrupt for pin INT1 */
			iis3dwb_pin_int1_route_get(ctx, &int1_route);
			int1_route.drdy_xl = enable;

			ret = iis3dwb_pin_int1_route_set(ctx, &int1_route);
		} else {
			/* set interrupt for pin INT2 */
			iis3dwb_pin_int2_route_get(ctx, &int2_route);
			int2_route.drdy_xl = enable;

			ret = iis3dwb_pin_int2_route_set(ctx, &int2_route);
		}

		return ret;
	default:
		LOG_ERR("Unsupported trigger interrupt route %d", type);
		return -ENOTSUP;
	}
}

/**
 * iis3dwb_trigger_set - link external trigger to event data ready
 */
int iis3dwb_trigger_set(const struct device *dev,
			  const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler)
{
	const struct iis3dwb_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	struct iis3dwb_data *iis3dwb = dev->data;
	int16_t raw[3];
	int state = (handler != NULL) ? PROPERTY_ENABLE : PROPERTY_DISABLE;

	switch (trig->type) {
	case SENSOR_TRIG_DATA_READY:
		iis3dwb->drdy_handler = handler;
		if (state) {
			/* dummy read: re-trigger interrupt */
			iis3dwb_acceleration_raw_get(ctx, raw);
		}
		return iis3dwb_enable_int(dev, SENSOR_TRIG_DATA_READY, state);
	default:
		LOG_ERR("Unsupported sensor trigger");
		return -ENOTSUP;
	}
}

static int iis3dwb_handle_drdy_int(const struct device *dev)
{
	struct iis3dwb_data *data = dev->data;

	struct sensor_trigger drdy_trig = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ALL,
	};

	if (data->drdy_handler) {
		data->drdy_handler(dev, &drdy_trig);
	}

	return 0;
}

/**
 * iis3dwb_handle_interrupt - handle the drdy event
 * read data and call handler if registered any
 */
static void iis3dwb_handle_interrupt(const struct device *dev)
{
	const struct iis3dwb_config *cfg = dev->config;

	iis3dwb_handle_drdy_int(dev);

	gpio_pin_interrupt_configure_dt(&cfg->gpio_drdy,
					GPIO_INT_EDGE_TO_ACTIVE);
}

static void iis3dwb_gpio_callback(const struct device *dev,
				    struct gpio_callback *cb, uint32_t pins)
{
	struct iis3dwb_data *iis3dwb =
		CONTAINER_OF(cb, struct iis3dwb_data, gpio_cb);
	const struct iis3dwb_config *cfg = iis3dwb->dev->config;

	ARG_UNUSED(pins);

	gpio_pin_interrupt_configure_dt(&cfg->gpio_drdy, GPIO_INT_DISABLE);

#if defined(CONFIG_IIS3DWB_TRIGGER_OWN_THREAD)
	k_sem_give(&iis3dwb->gpio_sem);
#elif defined(CONFIG_IIS3DWB_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&iis3dwb->work);
#endif /* CONFIG_IIS3DWB_TRIGGER_OWN_THREAD */
}

#ifdef CONFIG_IIS3DWB_TRIGGER_OWN_THREAD
static void iis3dwb_thread(struct iis3dwb_data *iis3dwb)
{
	while (1) {
		k_sem_take(&iis3dwb->gpio_sem, K_FOREVER);
		iis3dwb_handle_interrupt(iis3dwb->dev);
	}
}
#endif /* CONFIG_IIS3DWB_TRIGGER_OWN_THREAD */

#ifdef CONFIG_IIS3DWB_TRIGGER_GLOBAL_THREAD
static void iis3dwb_work_cb(struct k_work *work)
{
	struct iis3dwb_data *iis3dwb =
		CONTAINER_OF(work, struct iis3dwb_data, work);

	iis3dwb_handle_interrupt(iis3dwb->dev);
}
#endif /* CONFIG_IIS3DWB_TRIGGER_GLOBAL_THREAD */

int iis3dwb_init_interrupt(const struct device *dev)
{
	struct iis3dwb_data *iis3dwb = dev->data;
	const struct iis3dwb_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	int ret;

	/* setup data ready gpio interrupt (INT1 or INT2) */
	if (!device_is_ready(cfg->gpio_drdy.port)) {
		LOG_ERR("Cannot get pointer to drdy_gpio device");
		return -EINVAL;
	}

#if defined(CONFIG_IIS3DWB_TRIGGER_OWN_THREAD)
	k_sem_init(&iis3dwb->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&iis3dwb->thread, iis3dwb->thread_stack,
		       CONFIG_IIS3DWB_THREAD_STACK_SIZE,
		       (k_thread_entry_t)iis3dwb_thread, iis3dwb,
		       NULL, NULL, K_PRIO_COOP(CONFIG_IIS3DWB_THREAD_PRIORITY),
		       0, K_NO_WAIT);
#elif defined(CONFIG_IIS3DWB_TRIGGER_GLOBAL_THREAD)
	iis3dwb->work.handler = iis3dwb_work_cb;
#endif /* CONFIG_IIS3DWB_TRIGGER_OWN_THREAD */

	ret = gpio_pin_configure_dt(&cfg->gpio_drdy, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure gpio");
		return ret;
	}

	gpio_init_callback(&iis3dwb->gpio_cb,
			   iis3dwb_gpio_callback,
			   BIT(cfg->gpio_drdy.pin));

	if (gpio_add_callback(cfg->gpio_drdy.port, &iis3dwb->gpio_cb) < 0) {
		LOG_ERR("Could not set gpio callback");
		return -EIO;
	}

	/* enable interrupt on int1/int2 in latched mode */
	if (iis3dwb_data_ready_mode_set(ctx, IIS3DWB_DRDY_LATCHED)) {
		LOG_ERR("Could not set latched mode");
		return -EIO;
	}

	return gpio_pin_interrupt_configure_dt(&cfg->gpio_drdy,
					       GPIO_INT_EDGE_TO_ACTIVE);
}
