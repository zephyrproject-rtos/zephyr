/*
 * Copyright (c) 2023 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT we_wsen_pads

#include <zephyr/logging/log.h>

#include "wsen_pads.h"

LOG_MODULE_DECLARE(WSEN_PADS, CONFIG_SENSOR_LOG_LEVEL);

/* Enable/disable data-ready interrupt handling */
static inline int pads_setup_drdy_interrupt(const struct device *dev, bool enable)
{
	const struct pads_config *cfg = dev->config;
	unsigned int flags = enable ? GPIO_INT_EDGE_TO_ACTIVE : GPIO_INT_DISABLE;

	return gpio_pin_interrupt_configure_dt(&cfg->gpio_drdy, flags);
}

/*
 * Is called when a data-ready interrupt has occurred. Triggers
 * asynchronous processing of the interrupt in pads_process_drdy_interrupt().
 */
static inline void pads_handle_drdy_interrupt(const struct device *dev)
{
	struct pads_data *data = dev->data;

	/* Disable interrupt handling until the interrupt has been processed */
	pads_setup_drdy_interrupt(dev, false);

#if defined(CONFIG_WSEN_PADS_TRIGGER_OWN_THREAD)
	k_sem_give(&data->drdy_sem);
#elif defined(CONFIG_WSEN_PADS_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

/* Calls data-ready trigger handler (if any) */
static void pads_process_drdy_interrupt(const struct device *dev)
{
	struct pads_data *data = dev->data;

	if (data->data_ready_handler != NULL) {
		data->data_ready_handler(dev, data->data_ready_triggerP);
		pads_setup_drdy_interrupt(dev, true);
	}
}

int pads_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
		     sensor_trigger_handler_t handler)
{
	struct pads_data *data = dev->data;
	const struct pads_config *cfg = dev->config;
	int32_t pressure_dummy;

	if (trig->type != SENSOR_TRIG_DATA_READY) {
		LOG_ERR("Unsupported sensor trigger");
		return -ENOTSUP;
	}

	pads_setup_drdy_interrupt(dev, false);

	data->data_ready_handler = handler;
	if (handler == NULL) {
		/* Disable data-ready interrupt */
		if (PADS_enableDataReadyInterrupt(&data->sensor_interface, PADS_disable) !=
		    WE_SUCCESS) {
			LOG_ERR("Failed to disable data-ready interrupt.");
			return -EIO;
		}
		return 0;
	}

	data->data_ready_triggerP = trig;

	pads_setup_drdy_interrupt(dev, true);

	/* Read pressure to retrigger interrupt */
	if (PADS_getPressure_int(&data->sensor_interface, &pressure_dummy) != WE_SUCCESS) {
		LOG_ERR("Failed to read sample");
		return -EIO;
	}

	/* Enable data-ready interrupt */
	if (PADS_enableDataReadyInterrupt(&data->sensor_interface, PADS_enable) != WE_SUCCESS) {
		LOG_ERR("Failed to enable data-ready interrupt.");
		return -EIO;
	}

	/*
	 * If data-ready is active we probably won't get the rising edge, so
	 * invoke the handler manually.
	 */
	if (gpio_pin_get_dt(&cfg->gpio_drdy) > 0) {
		pads_handle_drdy_interrupt(dev);
	}

	return 0;
}

static void pads_drdy_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct pads_data *data = CONTAINER_OF(cb, struct pads_data, data_ready_cb);

	ARG_UNUSED(pins);

	pads_handle_drdy_interrupt(data->dev);
}

#ifdef CONFIG_WSEN_PADS_TRIGGER_OWN_THREAD
static void pads_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct pads_data *data = p1;

	while (true) {
		k_sem_take(&data->drdy_sem, K_FOREVER);
		pads_process_drdy_interrupt(data->dev);
	}
}
#endif /* CONFIG_WSEN_PADS_TRIGGER_OWN_THREAD */

#ifdef CONFIG_WSEN_PADS_TRIGGER_GLOBAL_THREAD
static void pads_work_cb(struct k_work *work)
{
	struct pads_data *data = CONTAINER_OF(work, struct pads_data, work);

	pads_process_drdy_interrupt(data->dev);
}
#endif /* CONFIG_WSEN_PADS_TRIGGER_GLOBAL_THREAD */

int pads_init_interrupt(const struct device *dev)
{
	struct pads_data *data = dev->data;
	const struct pads_config *cfg = dev->config;
	int status;

	data->dev = dev;

	if (cfg->gpio_drdy.port == NULL) {
		LOG_ERR("drdy-gpios is not defined in the device tree.");
		return -EINVAL;
	}

	if (!gpio_is_ready_dt(&cfg->gpio_drdy)) {
		LOG_ERR("Device %s is not ready", cfg->gpio_drdy.port->name);
		return -ENODEV;
	}

	/* Setup data-ready gpio interrupt */
	status = gpio_pin_configure_dt(&cfg->gpio_drdy, GPIO_INPUT);
	if (status < 0) {
		LOG_ERR("Failed to configure %s.%02u", cfg->gpio_drdy.port->name,
			cfg->gpio_drdy.pin);
		return status;
	}

	gpio_init_callback(&data->data_ready_cb, pads_drdy_callback, BIT(cfg->gpio_drdy.pin));

	status = gpio_add_callback(cfg->gpio_drdy.port, &data->data_ready_cb);
	if (status < 0) {
		LOG_ERR("Failed to set gpio callback.");
		return status;
	}

#if defined(CONFIG_WSEN_PADS_TRIGGER_OWN_THREAD)
	k_sem_init(&data->drdy_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&data->thread, data->thread_stack, CONFIG_WSEN_PADS_THREAD_STACK_SIZE,
			pads_thread, data, NULL, NULL,
			K_PRIO_COOP(CONFIG_WSEN_PADS_THREAD_PRIORITY), 0, K_NO_WAIT);
#elif defined(CONFIG_WSEN_PADS_TRIGGER_GLOBAL_THREAD)
	data->work.handler = pads_work_cb;
#endif

	return pads_setup_drdy_interrupt(dev, true);
}
