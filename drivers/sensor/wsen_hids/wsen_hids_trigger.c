/*
 * Copyright (c) 2022 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT we_wsen_hids

#include <zephyr/logging/log.h>

#include "wsen_hids.h"

LOG_MODULE_DECLARE(WSEN_HIDS, CONFIG_SENSOR_LOG_LEVEL);

static inline void hids_setup_drdy_interrupt(const struct device *dev, bool enable)
{
	const struct hids_config *cfg = dev->config;
	unsigned int flags = enable ? GPIO_INT_EDGE_TO_ACTIVE : GPIO_INT_DISABLE;

	gpio_pin_interrupt_configure_dt(&cfg->gpio_drdy, flags);
}

static inline void hids_handle_drdy_interrupt(const struct device *dev)
{
	struct hids_data *data = dev->data;

	hids_setup_drdy_interrupt(dev, false);

#if defined(CONFIG_WSEN_HIDS_TRIGGER_OWN_THREAD)
	k_sem_give(&data->drdy_sem);
#elif defined(CONFIG_WSEN_HIDS_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

static void hids_process_drdy_interrupt(const struct device *dev)
{
	struct hids_data *data = dev->data;

	if (data->data_ready_handler != NULL) {
		data->data_ready_handler(dev, data->data_ready_trigger);
	}

	if (data->data_ready_handler != NULL) {
		hids_setup_drdy_interrupt(dev, true);
	}
}

int hids_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
		     sensor_trigger_handler_t handler)
{
	struct hids_data *data = dev->data;
	const struct hids_config *cfg = dev->config;

	if (trig->type != SENSOR_TRIG_DATA_READY) {
		LOG_ERR("Unsupported sensor trigger");
		return -ENOTSUP;
	}

	hids_setup_drdy_interrupt(dev, false);

	data->data_ready_handler = handler;
	if (handler == NULL) {
		return 0;
	}

	data->data_ready_trigger = trig;

	hids_setup_drdy_interrupt(dev, true);

	/*
	 * If DRDY is active we probably won't get the rising edge, so
	 * invoke the callback manually.
	 */
	if (gpio_pin_get_dt(&cfg->gpio_drdy) > 0) {
		hids_handle_drdy_interrupt(dev);
	}

	return 0;
}

static void hids_drdy_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct hids_data *data = CONTAINER_OF(cb, struct hids_data, data_ready_cb);

	ARG_UNUSED(pins);

	hids_handle_drdy_interrupt(data->dev);
}

#ifdef CONFIG_WSEN_HIDS_TRIGGER_OWN_THREAD
static void hids_thread(struct hids_data *data)
{
	while (true) {
		k_sem_take(&data->drdy_sem, K_FOREVER);
		hids_process_drdy_interrupt(data->dev);
	}
}
#endif /* CONFIG_WSEN_HIDS_TRIGGER_OWN_THREAD */

#ifdef CONFIG_WSEN_HIDS_TRIGGER_GLOBAL_THREAD
static void hids_work_cb(struct k_work *work)
{
	struct hids_data *data = CONTAINER_OF(work, struct hids_data, work);

	hids_process_drdy_interrupt(data->dev);
}
#endif /* CONFIG_WSEN_HIDS_TRIGGER_GLOBAL_THREAD */

int hids_init_interrupt(const struct device *dev)
{
	struct hids_data *data = dev->data;
	const struct hids_config *cfg = dev->config;
	int status;

	data->dev = dev;

	if (cfg->gpio_drdy.port == NULL) {
		LOG_ERR("drdy-gpios is not defined in the device tree.");
		return -EINVAL;
	}

	if (!device_is_ready(cfg->gpio_drdy.port)) {
		LOG_ERR("Device %s is not ready", cfg->gpio_drdy.port->name);
		return -ENODEV;
	}

	/* Setup data-ready gpio interrupt */
	status = gpio_pin_configure_dt(&cfg->gpio_drdy, GPIO_INPUT);
	if (status < 0) {
		LOG_ERR("Could not configure %s.%02u", cfg->gpio_drdy.port->name,
			cfg->gpio_drdy.pin);
		return status;
	}

	gpio_init_callback(&data->data_ready_cb, hids_drdy_callback, BIT(cfg->gpio_drdy.pin));

	status = gpio_add_callback(cfg->gpio_drdy.port, &data->data_ready_cb);
	if (status < 0) {
		LOG_ERR("Could not set gpio callback.");
		return status;
	}

	/* Enable data-ready interrupt */
	if (HIDS_enableDataReadyInterrupt(&data->sensor_interface, HIDS_enable) != WE_SUCCESS) {
		LOG_ERR("Could not enable data-ready interrupt.");
		return -EIO;
	}

#if defined(CONFIG_WSEN_HIDS_TRIGGER_OWN_THREAD)
	k_sem_init(&data->drdy_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&data->thread, data->thread_stack, CONFIG_WSEN_HIDS_THREAD_STACK_SIZE,
			(k_thread_entry_t)hids_thread, data, NULL, NULL,
			K_PRIO_COOP(CONFIG_WSEN_HIDS_THREAD_PRIORITY), 0, K_NO_WAIT);
#elif defined(CONFIG_WSEN_HIDS_TRIGGER_GLOBAL_THREAD)
	data->work.handler = hids_work_cb;
#endif

	hids_setup_drdy_interrupt(dev, true);

	return 0;
}
