/*
 * Copyright (c) 2025 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT we_wsen_tids_2521020222501

#include <stdlib.h>

#include <zephyr/logging/log.h>

#include "wsen_tids_2521020222501.h"

LOG_MODULE_DECLARE(WSEN_TIDS_2521020222501, CONFIG_SENSOR_LOG_LEVEL);

/* Enable/disable interrupt handling */
static inline void tids_2521020222501_setup_interrupt(const struct device *dev, bool enable)
{
	const struct tids_2521020222501_config *cfg = dev->config;
	unsigned int flags = enable ? GPIO_INT_EDGE_TO_ACTIVE : GPIO_INT_DISABLE;

	gpio_pin_interrupt_configure_dt(&cfg->interrupt_gpio, flags);
}

/*
 * Is called when an interrupt has occurred.
 */
static void tids_2521020222501_handle_interrupt(const struct device *dev)
{
	struct tids_2521020222501_data *data = dev->data;

	/* Disable interrupt handling until the interrupt has been processed */
	tids_2521020222501_setup_interrupt(dev, false);

#if defined(CONFIG_WSEN_TIDS_2521020222501_TRIGGER_OWN_THREAD)
	k_sem_give(&data->sem);
#elif defined(CONFIG_WSEN_TIDS_2521020222501_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

/*
 * Calls trigger handles.
 */
static void tids_2521020222501_process_interrupt(const struct device *dev)
{
	struct tids_2521020222501_data *data = dev->data;

	if (data->temperature_high_handler != NULL || data->temperature_low_handler != NULL) {
		/*
		 * Read the sensor's status register - this also causes the interrupt pin
		 * to be de-asserted
		 */
		TIDS_status_t status;

		if (TIDS_getStatusRegister(&data->sensor_interface, &status) != WE_SUCCESS) {
			LOG_ERR("Failed to read status register");
			return;
		}

		if (data->temperature_high_handler != NULL &&
		    status.upperLimitExceeded == TIDS_enable) {
			data->temperature_high_handler(dev, data->temperature_high_trigger);
		} else if (data->temperature_low_handler != NULL &&
			   status.lowerLimitExceeded == TIDS_enable) {
			data->temperature_low_handler(dev, data->temperature_low_trigger);
		}
	}

	tids_2521020222501_setup_interrupt(dev, true);
}

/* Enables/disables processing of the "threshold exceeded" interrupt. */
int tids_2521020222501_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
				   sensor_trigger_handler_t handler)
{
	struct tids_2521020222501_data *data = dev->data;
	const struct tids_2521020222501_config *cfg = dev->config;

	switch (trig->chan) {
	case SENSOR_CHAN_ALL:
	case SENSOR_CHAN_AMBIENT_TEMP:
		break;
	default:
		LOG_ERR("Unsupported sensor trigger");
		return -ENOTSUP;
	}

	struct sensor_value interruptOFF = {.val1 = -40, .val2 = -320000};
	struct sensor_value threshold;

	switch ((int)trig->type) {
	case SENSOR_TRIG_WSEN_TIDS_2521020222501_THRESHOLD_LOWER: {

		threshold.val1 = data->sensor_low_threshold / 1000;
		threshold.val2 = ((int32_t)data->sensor_low_threshold % 1000) * (1000000 / 1000);

		if (tids_2521020222501_threshold_lower_set(
			    dev, (handler == NULL) ? &interruptOFF : &threshold) < 0) {
			LOG_ERR("Failed to set low temp threshold");
		}
		data->temperature_low_handler = handler;
		data->temperature_low_trigger = trig;
		break;
	}
	case SENSOR_TRIG_WSEN_TIDS_2521020222501_THRESHOLD_UPPER: {

		threshold.val1 = data->sensor_high_threshold / 1000;
		threshold.val2 = ((int32_t)data->sensor_high_threshold % 1000) * (1000000 / 1000);

		if (tids_2521020222501_threshold_upper_set(
			    dev, (handler == NULL) ? &interruptOFF : &threshold) < 0) {
			LOG_ERR("Failed to set high temp threshold");
		}
		data->temperature_high_handler = handler;
		data->temperature_high_trigger = trig;
		break;
	}
	default:
		LOG_ERR("Unsupported sensor trigger");
		return -ENOTSUP;
	}

	tids_2521020222501_setup_interrupt(dev, data->temperature_high_handler ||
							data->temperature_low_handler);

	if (gpio_pin_get_dt(&cfg->interrupt_gpio) > 0) {
		tids_2521020222501_handle_interrupt(dev);
	}

	return 0;
}

static void tids_2521020222501_callback(const struct device *dev, struct gpio_callback *cb,
					uint32_t pins)
{
	struct tids_2521020222501_data *data =
		CONTAINER_OF(cb, struct tids_2521020222501_data, interrupt_cb);

	ARG_UNUSED(pins);

	tids_2521020222501_handle_interrupt(data->dev);
}

#ifdef CONFIG_WSEN_TIDS_2521020222501_TRIGGER_OWN_THREAD
static void tids_2521020222501_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct tids_2521020222501_data *tids_2521020222501 = p1;

	while (true) {
		k_sem_take(&tids_2521020222501->sem, K_FOREVER);
		tids_2521020222501_process_interrupt(tids_2521020222501->dev);
	}
}
#endif /* CONFIG_WSEN_TIDS_2521020222501_TRIGGER_OWN_THREAD */

#ifdef CONFIG_WSEN_TIDS_2521020222501_TRIGGER_GLOBAL_THREAD
static void tids_2521020222501_work_cb(struct k_work *work)
{
	struct tids_2521020222501_data *tids_2521020222501 =
		CONTAINER_OF(work, struct tids_2521020222501_data, work);

	tids_2521020222501_process_interrupt(tids_2521020222501->dev);
}
#endif /* CONFIG_WSEN_TIDS_2521020222501_TRIGGER_GLOBAL_THREAD */

int tids_2521020222501_threshold_upper_set(const struct device *dev,
					   const struct sensor_value *thresh_value)
{
	struct tids_2521020222501_data *data = dev->data;
	int32_t thresh = thresh_value->val1 * 1000 + thresh_value->val2 / 1000;

	if (TIDS_setTempHighLimit(&data->sensor_interface, thresh) != WE_SUCCESS) {
		LOG_ERR("Failed to set high temperature threshold.");
		return -EIO;
	}

	data->sensor_high_threshold = thresh;

	return 0;
}

int tids_2521020222501_threshold_upper_get(const struct device *dev,
					   struct sensor_value *thresh_value)
{
	struct tids_2521020222501_data *data = dev->data;
	int32_t thresh;

	if (TIDS_getTempHighLimit(&data->sensor_interface, &thresh) != WE_SUCCESS) {
		LOG_ERR("Failed to get high temperature threshold.");
		return -EIO;
	}

	thresh_value->val1 = thresh / 1000;
	thresh_value->val2 = (thresh % 1000) * (1000000 / 1000);

	return 0;
}

int tids_2521020222501_threshold_lower_set(const struct device *dev,
					   const struct sensor_value *thresh_value)
{
	struct tids_2521020222501_data *data = dev->data;
	int32_t thresh = thresh_value->val1 * 1000 + thresh_value->val2 / 1000;

	if (TIDS_setTempLowLimit(&data->sensor_interface, thresh) != WE_SUCCESS) {
		LOG_ERR("Failed to set low temperature threshold.");
		return -EIO;
	}

	data->sensor_low_threshold = thresh;

	return 0;
}

int tids_2521020222501_threshold_lower_get(const struct device *dev,
					   struct sensor_value *thresh_value)
{
	struct tids_2521020222501_data *data = dev->data;
	int32_t thresh;

	if (TIDS_getTempLowLimit(&data->sensor_interface, &thresh) != WE_SUCCESS) {
		LOG_ERR("Failed to get low temperature threshold.");
		return -EIO;
	}

	thresh_value->val1 = thresh / 1000;
	thresh_value->val2 = (thresh % 1000) * (1000000 / 1000);

	return 0;
}

int tids_2521020222501_init_interrupt(const struct device *dev)
{
	struct tids_2521020222501_data *data = dev->data;
	const struct tids_2521020222501_config *cfg = dev->config;
	struct sensor_value upper_limit, lower_limit;

	if (cfg->interrupt_gpio.port == NULL) {
		LOG_ERR("interrupt-gpios is not defined in the device tree.");
		return -EINVAL;
	}

	if (!gpio_is_ready_dt(&cfg->interrupt_gpio)) {
		LOG_ERR("Device %s is not ready", cfg->interrupt_gpio.port->name);
		return -ENODEV;
	}

	data->dev = dev;

	/* Setup threshold gpio interrupt */
	if (gpio_pin_configure_dt(&cfg->interrupt_gpio, GPIO_INPUT) < 0) {
		LOG_ERR("Failed to configure %s.%02u", cfg->interrupt_gpio.port->name,
			cfg->interrupt_gpio.pin);
		return -EIO;
	}

	gpio_init_callback(&data->interrupt_cb, tids_2521020222501_callback,
			   BIT(cfg->interrupt_gpio.pin));

	if (gpio_add_callback(cfg->interrupt_gpio.port, &data->interrupt_cb) < 0) {
		LOG_ERR("Failed to set gpio callback.");
		return -EIO;
	}

	/*
	 * Enable interrupt on high/low temperature (interrupt generation is enabled if at
	 * least one threshold is non-zero)
	 */

	upper_limit.val1 = cfg->high_threshold / 1000;
	upper_limit.val2 = ((int32_t)cfg->high_threshold % 1000) * (1000000 / 1000);

	lower_limit.val1 = cfg->low_threshold / 1000;
	lower_limit.val2 = ((int32_t)cfg->low_threshold % 1000) * (1000000 / 1000);

	if (tids_2521020222501_threshold_upper_set(dev, &upper_limit) < 0) {
		LOG_ERR("Failed to set upper threshold");
		return -EIO;
	}

	if (tids_2521020222501_threshold_lower_set(dev, &lower_limit) < 0) {
		LOG_ERR("Failed to set lower threshold");
		return -EIO;
	}

#if defined(CONFIG_WSEN_TIDS_2521020222501_TRIGGER_OWN_THREAD)
	k_sem_init(&data->sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&data->thread, data->thread_stack,
			CONFIG_WSEN_TIDS_2521020222501_THREAD_STACK_SIZE, tids_2521020222501_thread,
			data, NULL, NULL,
			K_PRIO_COOP(CONFIG_WSEN_TIDS_2521020222501_THREAD_PRIORITY), 0, K_NO_WAIT);
#elif defined(CONFIG_WSEN_TIDS_2521020222501_TRIGGER_GLOBAL_THREAD)
	data->work.handler = tids_2521020222501_work_cb;
#endif

	return 0;
}
