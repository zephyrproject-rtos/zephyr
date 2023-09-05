/*
 * Copyright (c) 2025 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT we_wsen_pads_2511020213301

#include <zephyr/logging/log.h>

#include "wsen_pads_2511020213301.h"

LOG_MODULE_DECLARE(WSEN_PADS_2511020213301, CONFIG_SENSOR_LOG_LEVEL);

/* Enable/disable data-ready interrupt handling */
static inline int pads_2511020213301_setup_interrupt(const struct device *dev, bool enable)
{
	const struct pads_2511020213301_config *cfg = dev->config;
	unsigned int flags = enable ? GPIO_INT_EDGE_TO_ACTIVE : GPIO_INT_DISABLE;

	return gpio_pin_interrupt_configure_dt(&cfg->interrupt_gpio, flags);
}

/*
 * Is called when an interrupt has occurred.
 */
static inline void pads_2511020213301_handle_interrupt(const struct device *dev)
{
	struct pads_2511020213301_data *data = dev->data;

	/* Disable interrupt handling until the interrupt has been processed */
	pads_2511020213301_setup_interrupt(dev, false);

#if defined(CONFIG_WSEN_PADS_2511020213301_TRIGGER_OWN_THREAD)
	k_sem_give(&data->sem);
#elif defined(CONFIG_WSEN_PADS_2511020213301_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

/* Calls data-ready trigger handler (if any) */
static void pads_2511020213301_process_interrupt(const struct device *dev)
{
	struct pads_2511020213301_data *data = dev->data;

#ifdef CONFIG_WSEN_PADS_2511020213301_PRESSURE_THRESHOLD
	if (data->pressure_low_trigger_handler != NULL ||
	    data->pressure_high_trigger_handler != NULL) {
		PADS_state_t pressure_high_state, pressure_low_state;

		if (PADS_getHighPressureInterruptStatus(&data->sensor_interface,
							&pressure_high_state) != WE_SUCCESS) {
			LOG_ERR("Failed to read pressure high state");
			return;
		}

		if (PADS_getLowPressureInterruptStatus(&data->sensor_interface,
						       &pressure_low_state) != WE_SUCCESS) {
			LOG_ERR("Failed to read pressure high state");
			return;
		}

		if (data->pressure_high_trigger_handler != NULL &&
		    pressure_high_state == PADS_enable) {
			data->pressure_high_trigger_handler(dev, data->pressure_high_trigger);
		} else if (data->pressure_low_trigger_handler != NULL &&
			   pressure_low_state == PADS_enable) {
			data->pressure_low_trigger_handler(dev, data->pressure_low_trigger);
		}
	}
#else
	if (data->data_ready_trigger_handler != NULL) {
		data->data_ready_trigger_handler(dev, data->data_ready_trigger);
	}
#endif /* CONFIG_WSEN_PADS_2511020213301_PRESSURE_THRESHOLD */

	pads_2511020213301_setup_interrupt(dev, true);
}

int pads_2511020213301_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
				   sensor_trigger_handler_t handler)
{
	struct pads_2511020213301_data *data = dev->data;
	const struct pads_2511020213301_config *cfg = dev->config;

	if (trig->chan != SENSOR_CHAN_PRESS) {
		LOG_ERR("Unsupported sensor trigger");
		return -ENOTSUP;
	}

	switch ((int)trig->type) {
#ifdef CONFIG_WSEN_PADS_2511020213301_PRESSURE_THRESHOLD
	case SENSOR_TRIG_WSEN_PADS_2511020213301_THRESHOLD_LOWER:
	case SENSOR_TRIG_WSEN_PADS_2511020213301_THRESHOLD_UPPER: {
		switch ((int)trig->type) {
		case SENSOR_TRIG_WSEN_PADS_2511020213301_THRESHOLD_LOWER:
			data->pressure_low_trigger_handler = handler;
			data->pressure_low_trigger = trig;
			break;
		case SENSOR_TRIG_WSEN_PADS_2511020213301_THRESHOLD_UPPER:
			data->pressure_high_trigger_handler = handler;
			data->pressure_high_trigger = trig;
			break;
		default:
			break;
		}

		if (PADS_setInterruptEventControl(&data->sensor_interface,
						  PADS_pressureHighOrLow) != WE_SUCCESS) {
			LOG_ERR("Failed to set interrupt event control to pressure high or low");
			return -EIO;
		}

		if (PADS_enableDiffPressureInterrupt(&data->sensor_interface,
						     (data->pressure_high_trigger_handler ||
						      data->pressure_low_trigger_handler)
							     ? PADS_enable
							     : PADS_disable) != WE_SUCCESS) {
			LOG_ERR("Failed to enable pressure diff interrupt.");
			return -EIO;
		}

		if (PADS_enableLowPressureInterrupt(&data->sensor_interface,
						    (data->pressure_low_trigger_handler == NULL)
							    ? PADS_disable
							    : PADS_enable) != WE_SUCCESS) {
			LOG_ERR("Failed to enable low pressure interrupt.");
			return -EIO;
		}
		if (PADS_enableHighPressureInterrupt(&data->sensor_interface,
						     (data->pressure_high_trigger_handler == NULL)
							     ? PADS_disable
							     : PADS_enable) != WE_SUCCESS) {
			LOG_ERR("Failed to enable high pressure interrupt.");
			return -EIO;
		}

		pads_2511020213301_setup_interrupt(dev, data->pressure_high_trigger_handler ||
								data->pressure_low_trigger_handler);
		break;
	}
#else
	case SENSOR_TRIG_DATA_READY: {
		int32_t pressure_dummy;
		/* Read pressure to retrigger interrupt */
		if (PADS_getPressure_int(&data->sensor_interface, &pressure_dummy) != WE_SUCCESS) {
			LOG_ERR("Failed to read sample");
			return -EIO;
		}

		if (PADS_setInterruptEventControl(&data->sensor_interface, PADS_dataReady)) {
			LOG_ERR("Failed to set interrupt event control to data ready");
			return -EIO;
		}

		/* Enable data-ready interrupt */
		if (PADS_enableDataReadyInterrupt(&data->sensor_interface,
						  (handler == NULL) ? PADS_disable : PADS_enable) !=
		    WE_SUCCESS) {
			LOG_ERR("Failed to enable data-ready interrupt.");
			return -EIO;
		}

		data->data_ready_trigger_handler = handler;
		data->data_ready_trigger = trig;

		pads_2511020213301_setup_interrupt(dev, data->data_ready_trigger_handler);

		break;
	}
#endif /* CONFIG_WSEN_PADS_2511020213301_PRESSURE_THRESHOLD */
	default:
		LOG_ERR("Unsupported sensor trigger");
		return -ENOTSUP;
	}

	if (gpio_pin_get_dt(&cfg->interrupt_gpio) > 0) {
		pads_2511020213301_handle_interrupt(dev);
	}

	return 0;
}

static void pads_2511020213301_callback(const struct device *dev, struct gpio_callback *cb,
					uint32_t pins)
{
	struct pads_2511020213301_data *data =
		CONTAINER_OF(cb, struct pads_2511020213301_data, interrupt_cb);

	ARG_UNUSED(pins);

	pads_2511020213301_handle_interrupt(data->dev);
}

#ifdef CONFIG_WSEN_PADS_2511020213301_TRIGGER_OWN_THREAD
static void pads_2511020213301_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct pads_2511020213301_data *data = p1;

	while (true) {
		k_sem_take(&data->sem, K_FOREVER);
		pads_2511020213301_process_interrupt(data->dev);
	}
}
#endif /* CONFIG_WSEN_PADS_2511020213301_TRIGGER_OWN_THREAD */

#ifdef CONFIG_WSEN_PADS_2511020213301_TRIGGER_GLOBAL_THREAD
static void pads_2511020213301_work_cb(struct k_work *work)
{
	struct pads_2511020213301_data *data =
		CONTAINER_OF(work, struct pads_2511020213301_data, work);

	pads_2511020213301_process_interrupt(data->dev);
}
#endif /* CONFIG_WSEN_PADS_2511020213301_TRIGGER_GLOBAL_THREAD */

#ifdef CONFIG_WSEN_PADS_2511020213301_PRESSURE_THRESHOLD
/* Set threshold for the differential pressure interrupt. */
int pads_2511020213301_threshold_set(const struct device *dev, const struct sensor_value *threshold)
{
	struct pads_2511020213301_data *data = dev->data;

	if (PADS_setPressureThreshold(&data->sensor_interface, (uint32_t)threshold->val1) !=
	    WE_SUCCESS) {
		LOG_ERR("Failed to set threshold");
		return -EIO;
	}

	return 0;
}

/* Get threshold for the differential pressure interrupt. */
int pads_2511020213301_threshold_get(const struct device *dev, struct sensor_value *threshold)
{

	struct pads_2511020213301_data *data = dev->data;

	if (PADS_getPressureThreshold(&data->sensor_interface, (uint32_t *)&threshold->val1) !=
	    WE_SUCCESS) {
		LOG_ERR("Failed to get threshold");
		return -EIO;
	}

	return 0;
}

/* Set reference point to current measured pressure state. */
int pads_2511020213301_reference_point_set(const struct device *dev,
					   const struct sensor_value *reference_point)
{
	struct pads_2511020213301_data *data = dev->data;

	if (reference_point != NULL) {
		LOG_ERR("Sensor value should be null");
		return -EIO;
	}

	if (PADS_enableAutoRefp(&data->sensor_interface, PADS_enable) != WE_SUCCESS) {
		LOG_ERR("Failed to set additional low pass filter");
		return -EIO;
	}

	return 0;
}

/* Get reference point from registers. */
int pads_2511020213301_reference_point_get(const struct device *dev,
					   struct sensor_value *reference_point)
{

	struct pads_2511020213301_data *data = dev->data;

	if (PADS_getReferencePressure(&data->sensor_interface,
				      (uint32_t *)&reference_point->val1) != WE_SUCCESS) {
		LOG_ERR("Failed to get reference point");
		return -EIO;
	}

	return 0;
}
#endif /* CONFIG_WSEN_PADS_2511020213301_PRESSURE_THRESHOLD */

int pads_2511020213301_init_interrupt(const struct device *dev)
{
	struct pads_2511020213301_data *data = dev->data;
	const struct pads_2511020213301_config *cfg = dev->config;

	data->dev = dev;

	if (cfg->interrupt_gpio.port == NULL) {
		LOG_ERR("interrupt-gpio is not defined in the device tree.");
		return -EINVAL;
	}

	if (!gpio_is_ready_dt(&cfg->interrupt_gpio)) {
		LOG_ERR("Device %s is not ready", cfg->interrupt_gpio.port->name);
		return -ENODEV;
	}

	if (gpio_pin_configure_dt(&cfg->interrupt_gpio, GPIO_INPUT) < 0) {
		LOG_ERR("Failed to configure %s.%02u", cfg->interrupt_gpio.port->name,
			cfg->interrupt_gpio.pin);
		return -EIO;
	}

	gpio_init_callback(&data->interrupt_cb, pads_2511020213301_callback,
			   BIT(cfg->interrupt_gpio.pin));

	if (gpio_add_callback(cfg->interrupt_gpio.port, &data->interrupt_cb) < 0) {
		LOG_ERR("Failed to set gpio callback.");
		return -EIO;
	}
#ifdef CONFIG_WSEN_PADS_2511020213301_PRESSURE_THRESHOLD

	struct sensor_value threshold;

	threshold.val1 = cfg->threshold;
	threshold.val2 = 0;
	if (pads_2511020213301_threshold_set(dev, &threshold) < 0) {
		LOG_ERR("Failed to set threshold.");
		return -EIO;
	}

#endif /* CONFIG_WSEN_PADS_2511020213301_PRESSURE_THRESHOLD */

#if defined(CONFIG_WSEN_PADS_2511020213301_TRIGGER_OWN_THREAD)
	k_sem_init(&data->sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&data->thread, data->thread_stack,
			CONFIG_WSEN_PADS_2511020213301_THREAD_STACK_SIZE, pads_2511020213301_thread,
			data, NULL, NULL,
			K_PRIO_COOP(CONFIG_WSEN_PADS_2511020213301_THREAD_PRIORITY), 0, K_NO_WAIT);
#elif defined(CONFIG_WSEN_PADS_2511020213301_TRIGGER_GLOBAL_THREAD)
	data->work.handler = pads_2511020213301_work_cb;
#endif

	return 0;
}
