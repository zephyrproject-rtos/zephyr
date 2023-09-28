/* ST Microelectronics IIS2MDC 3-axis magnetometer sensor
 *
 * Copyright (c) 2020 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/iis2mdc.pdf
 */

#define DT_DRV_COMPAT st_iis2mdc

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include "iis2mdc.h"

LOG_MODULE_DECLARE(IIS2MDC, CONFIG_SENSOR_LOG_LEVEL);

static int iis2mdc_enable_int(const struct device *dev, int enable)
{
	struct iis2mdc_data *iis2mdc = dev->data;

	/* set interrupt on mag */
	return iis2mdc_drdy_on_pin_set(iis2mdc->ctx, enable);
}

/* link external trigger to event data ready */
int iis2mdc_trigger_set(const struct device *dev,
			  const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler)
{
	struct iis2mdc_data *iis2mdc = dev->data;
	int16_t raw[3];

	if (trig->chan == SENSOR_CHAN_MAGN_XYZ) {
		iis2mdc->handler_drdy = handler;
		iis2mdc->trig_drdy = trig;
		if (handler) {
			/* fetch raw data sample: re-trigger lost interrupt */
			iis2mdc_magnetic_raw_get(iis2mdc->ctx, raw);

			return iis2mdc_enable_int(dev, 1);
		} else {
			return iis2mdc_enable_int(dev, 0);
		}
	}

	return -ENOTSUP;
}

/* handle the drdy event: read data and call handler if registered any */
static void iis2mdc_handle_interrupt(const struct device *dev)
{
	struct iis2mdc_data *iis2mdc = dev->data;
	const struct iis2mdc_dev_config *const config = dev->config;

	if (iis2mdc->handler_drdy != NULL) {
		iis2mdc->handler_drdy(dev, iis2mdc->trig_drdy);
	}

	gpio_pin_interrupt_configure_dt(&config->gpio_drdy,
					GPIO_INT_EDGE_TO_ACTIVE);
}

static void iis2mdc_gpio_callback(const struct device *dev,
				    struct gpio_callback *cb, uint32_t pins)
{
	struct iis2mdc_data *iis2mdc =
		CONTAINER_OF(cb, struct iis2mdc_data, gpio_cb);
	const struct iis2mdc_dev_config *const config = iis2mdc->dev->config;

	ARG_UNUSED(pins);

	gpio_pin_interrupt_configure_dt(&config->gpio_drdy, GPIO_INT_DISABLE);

#if defined(CONFIG_IIS2MDC_TRIGGER_OWN_THREAD)
	k_sem_give(&iis2mdc->gpio_sem);
#elif defined(CONFIG_IIS2MDC_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&iis2mdc->work);
#endif
}

#ifdef CONFIG_IIS2MDC_TRIGGER_OWN_THREAD
static void iis2mdc_thread(struct iis2mdc_data *iis2mdc)
{
	while (1) {
		k_sem_take(&iis2mdc->gpio_sem, K_FOREVER);
		iis2mdc_handle_interrupt(iis2mdc->dev);
	}
}
#endif

#ifdef CONFIG_IIS2MDC_TRIGGER_GLOBAL_THREAD
static void iis2mdc_work_cb(struct k_work *work)
{
	struct iis2mdc_data *iis2mdc =
		CONTAINER_OF(work, struct iis2mdc_data, work);

	iis2mdc_handle_interrupt(iis2mdc->dev);
}
#endif

int iis2mdc_init_interrupt(const struct device *dev)
{
	struct iis2mdc_data *iis2mdc = dev->data;
	const struct iis2mdc_dev_config *const config = dev->config;
	int ret;

	/* setup data ready gpio interrupt */
	if (!gpio_is_ready_dt(&config->gpio_drdy)) {
		LOG_ERR("Cannot get pointer to drdy_gpio device");
		return -ENODEV;
	}

#if defined(CONFIG_IIS2MDC_TRIGGER_OWN_THREAD)
	k_sem_init(&iis2mdc->gpio_sem, 0, K_SEM_MAX_LIMIT);
	k_thread_create(&iis2mdc->thread, iis2mdc->thread_stack,
			CONFIG_IIS2MDC_THREAD_STACK_SIZE,
			(k_thread_entry_t)iis2mdc_thread, iis2mdc,
			NULL, NULL, K_PRIO_COOP(CONFIG_IIS2MDC_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif defined(CONFIG_IIS2MDC_TRIGGER_GLOBAL_THREAD)
	iis2mdc->work.handler = iis2mdc_work_cb;
#endif

	ret = gpio_pin_configure_dt(&config->gpio_drdy, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure gpio");
		return ret;
	}

	gpio_init_callback(&iis2mdc->gpio_cb,
			   iis2mdc_gpio_callback,
			   BIT(config->gpio_drdy.pin));

	if (gpio_add_callback(config->gpio_drdy.port, &iis2mdc->gpio_cb) < 0) {
		LOG_DBG("Could not set gpio callback");
		return -EIO;
	}

	return gpio_pin_interrupt_configure_dt(&config->gpio_drdy,
					       GPIO_INT_EDGE_TO_ACTIVE);
}
