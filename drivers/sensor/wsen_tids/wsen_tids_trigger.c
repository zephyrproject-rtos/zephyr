/*
 * Copyright (c) 2022 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT we_wsen_tids

#include <stdlib.h>

#include <zephyr/logging/log.h>

#include "wsen_tids.h"

LOG_MODULE_DECLARE(WSEN_TIDS, CONFIG_SENSOR_LOG_LEVEL);

#define THRESHOLD_TEMPERATURE2REGISTER_OFFSET (double)63.
#define THRESHOLD_TEMPERATURE2REGISTER_STEP   (double)0.64

/* Enable/disable threshold interrupt handling */
static inline void tids_setup_threshold_interrupt(const struct device *dev, bool enable)
{
	const struct tids_config *cfg = dev->config;
	unsigned int flags = enable ? GPIO_INT_EDGE_TO_ACTIVE : GPIO_INT_DISABLE;

	gpio_pin_interrupt_configure_dt(&cfg->gpio_threshold, flags);
}

/*
 * Is called when a "threshold exceeded" interrupt occurred. Triggers
 * asynchronous processing of the interrupt in tids_process_threshold_interrupt().
 */
static void tids_handle_threshold_interrupt(const struct device *dev)
{
	struct tids_data *data = dev->data;

	/* Disable interrupt handling until the interrupt has been processed */
	tids_setup_threshold_interrupt(dev, false);

#if defined(CONFIG_WSEN_TIDS_TRIGGER_OWN_THREAD)
	k_sem_give(&data->threshold_sem);
#elif defined(CONFIG_WSEN_TIDS_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

/*
 * Is called after a "threshold exceeded" interrupt occurred.
 * Checks the sensor's status register for the limit exceeded flags and
 * calls the trigger handler if one of the flags is set.
 */
static void tids_process_threshold_interrupt(const struct device *dev)
{
	struct tids_data *data = dev->data;
	TIDS_status_t status;

	/*
	 * Read the sensor's status register - this also causes the interrupt pin
	 * to be de-asserted
	 */
	if (TIDS_getStatusRegister(&data->sensor_interface, &status) != WE_SUCCESS) {
		LOG_ERR("Failed to read status register");
		return;
	}

	if (data->threshold_handler != NULL &&
	    (status.upperLimitExceeded != 0 || status.lowerLimitExceeded != 0)) {
		data->threshold_handler(dev, data->threshold_trigger);
	}

	if (data->threshold_handler != NULL) {
		tids_setup_threshold_interrupt(dev, true);
	}
}

/* Enables/disables processing of the "threshold exceeded" interrupt. */
int tids_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
		     sensor_trigger_handler_t handler)
{
	struct tids_data *data = dev->data;
	const struct tids_config *cfg = dev->config;

	if (trig->type != SENSOR_TRIG_THRESHOLD) {
		LOG_ERR("Unsupported sensor trigger");
		return -ENOTSUP;
	}

	tids_setup_threshold_interrupt(dev, false);

	data->threshold_handler = handler;
	if (handler == NULL) {
		return 0;
	}

	data->threshold_trigger = trig;

	tids_setup_threshold_interrupt(dev, true);

	/*
	 * If threshold interrupt is active we probably won't get the rising edge, so
	 * invoke the callback manually.
	 */
	if (gpio_pin_get_dt(&cfg->gpio_threshold) > 0) {
		tids_handle_threshold_interrupt(dev);
	}

	return 0;
}

static void tids_threshold_callback(const struct device *dev, struct gpio_callback *cb,
				    uint32_t pins)
{
	struct tids_data *data = CONTAINER_OF(cb, struct tids_data, threshold_cb);

	ARG_UNUSED(pins);

	tids_handle_threshold_interrupt(data->dev);
}

#ifdef CONFIG_WSEN_TIDS_TRIGGER_OWN_THREAD
static void tids_thread(struct tids_data *tids)
{
	while (true) {
		k_sem_take(&tids->threshold_sem, K_FOREVER);
		tids_process_threshold_interrupt(tids->dev);
	}
}
#endif /* CONFIG_WSEN_TIDS_TRIGGER_OWN_THREAD */

#ifdef CONFIG_WSEN_TIDS_TRIGGER_GLOBAL_THREAD
static void tids_work_cb(struct k_work *work)
{
	struct tids_data *tids = CONTAINER_OF(work, struct tids_data, work);

	tids_process_threshold_interrupt(tids->dev);
}
#endif /* CONFIG_WSEN_TIDS_TRIGGER_GLOBAL_THREAD */

int tids_threshold_set(const struct device *dev, const struct sensor_value *thresh_value,
		       bool upper)
{
	struct tids_data *data = dev->data;
	double thresh = (sensor_value_to_double(thresh_value) / THRESHOLD_TEMPERATURE2REGISTER_STEP)
					+ THRESHOLD_TEMPERATURE2REGISTER_OFFSET;

	if (thresh < 0) {
		thresh = 0;
	} else if (thresh > 255) {
		thresh = 255;
	}

	if (upper) {
		if (TIDS_setTempHighLimit(&data->sensor_interface, (uint8_t)thresh) != WE_SUCCESS) {
			LOG_ERR("Failed to set high temperature threshold to %d.%d (%d).",
				thresh_value->val1, abs(thresh_value->val2), (uint8_t)thresh);
			return -EIO;
		}
	} else {
		if (TIDS_setTempLowLimit(&data->sensor_interface, (uint8_t)thresh) != WE_SUCCESS) {
			LOG_ERR("Failed to set low temperature threshold to %d.%d (%d).",
				thresh_value->val1, abs(thresh_value->val2), (uint8_t)thresh);
			return -EIO;
		}
	}

	return 0;
}

int tids_init_interrupt(const struct device *dev)
{
	struct tids_data *data = dev->data;
	const struct tids_config *cfg = dev->config;
	int status;
	struct sensor_value upper_limit;
	struct sensor_value lower_limit;

	if (cfg->gpio_threshold.port == NULL) {
		LOG_ERR("int-gpios is not defined in the device tree.");
		return -EINVAL;
	}

	if (!device_is_ready(cfg->gpio_threshold.port)) {
		LOG_ERR("Device %s is not ready", cfg->gpio_threshold.port->name);
		return -ENODEV;
	}

	data->dev = dev;

	/* Setup threshold gpio interrupt */
	status = gpio_pin_configure_dt(&cfg->gpio_threshold, GPIO_INPUT);
	if (status < 0) {
		LOG_ERR("Failed to configure %s.%02u", cfg->gpio_threshold.port->name,
			cfg->gpio_threshold.pin);
		return status;
	}

	gpio_init_callback(&data->threshold_cb, tids_threshold_callback,
			   BIT(cfg->gpio_threshold.pin));

	status = gpio_add_callback(cfg->gpio_threshold.port, &data->threshold_cb);
	if (status < 0) {
		LOG_ERR("Failed to set gpio callback.");
		return status;
	}

	/*
	 * Enable interrupt on high/low temperature (interrupt generation is enabled if at
	 * least one threshold is non-zero)
	 */
	upper_limit.val1 = cfg->high_threshold;
	upper_limit.val2 = 0;
	lower_limit.val1 = cfg->low_threshold;
	lower_limit.val2 = 0;

	status = tids_threshold_set(dev, &upper_limit, true);
	if (status < 0) {
		return status;
	}

	status = tids_threshold_set(dev, &lower_limit, false);
	if (status < 0) {
		return status;
	}

#if defined(CONFIG_WSEN_TIDS_TRIGGER_OWN_THREAD)
	k_sem_init(&data->threshold_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&data->thread, data->thread_stack, CONFIG_WSEN_TIDS_THREAD_STACK_SIZE,
			(k_thread_entry_t)tids_thread, data, NULL, NULL,
			K_PRIO_COOP(CONFIG_WSEN_TIDS_THREAD_PRIORITY), 0, K_NO_WAIT);
#elif defined(CONFIG_WSEN_TIDS_TRIGGER_GLOBAL_THREAD)
	data->work.handler = tids_work_cb;
#endif

	tids_setup_threshold_interrupt(dev, true);

	return 0;
}
