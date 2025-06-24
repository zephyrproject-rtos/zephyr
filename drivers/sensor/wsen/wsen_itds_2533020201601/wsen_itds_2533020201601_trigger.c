/*
 * Copyright (c) 2025 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT we_wsen_itds_2533020201601

#include <zephyr/logging/log.h>

#include "wsen_itds_2533020201601.h"

LOG_MODULE_DECLARE(WSEN_ITDS_2533020201601, CONFIG_SENSOR_LOG_LEVEL);

/* Enable/disable interrupt handling for tap, free-fall, delta/wake-up */
static inline int itds_2533020201601_setup_interrupt(const struct device *dev,
						     const struct gpio_dt_spec *pin, bool enable)
{
	unsigned int flags = enable ? GPIO_INT_EDGE_TO_ACTIVE : GPIO_INT_DISABLE;

	return gpio_pin_interrupt_configure_dt(pin, flags);
}

static inline void itds_2533020201601_handle_interrupt_1(const struct device *dev)
{
	struct itds_2533020201601_data *data = dev->data;
	const struct itds_2533020201601_config *cfg = dev->config;

	/* Disable interrupt handling until the interrupt has been processed */
	itds_2533020201601_setup_interrupt(data->dev, &cfg->drdy_interrupt_gpio, false);

#if defined(CONFIG_WSEN_ITDS_2533020201601_TRIGGER_OWN_THREAD)
	k_sem_give(&data->drdy_sem);
#elif defined(CONFIG_WSEN_ITDS_2533020201601_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->drdy_work);
#endif
}

#ifdef CONFIG_WSEN_ITDS_2533020201601_EVENTS
static inline void itds_2533020201601_handle_interrupt_0(const struct device *dev)
{
	struct itds_2533020201601_data *data = dev->data;
	const struct itds_2533020201601_config *cfg = dev->config;

	/* Disable interrupt handling until the interrupt has been processed */
	itds_2533020201601_setup_interrupt(data->dev, &cfg->events_interrupt_gpio, false);

#if defined(CONFIG_WSEN_ITDS_2533020201601_TRIGGER_OWN_THREAD)
	k_sem_give(&data->events_sem);
#elif defined(CONFIG_WSEN_ITDS_2533020201601_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->events_work);
#endif
}

/* Asynchronous handling of interrupt triggered in itds_2533020201601_gpio_callback() */
static void itds_2533020201601_process_interrupt_0(const struct device *dev)
{
	struct itds_2533020201601_data *data = dev->data;
	const struct itds_2533020201601_config *cfg = dev->config;
	ITDS_status_t itds_2533020201601_status;

	/* Read status register to check for data-ready - if not using soft trigger, the
	 * status register is also used to find out which interrupt occurred.
	 */
	if (ITDS_getStatusRegister(&data->sensor_interface, &itds_2533020201601_status) !=
	    WE_SUCCESS) {
		LOG_ERR("Failed to read status register");
		return;
	}

#ifdef CONFIG_WSEN_ITDS_2533020201601_TAP

	if (data->single_tap_handler && itds_2533020201601_status.singleTap) {
		data->single_tap_handler(dev, data->single_tap_trigger);
	}

	if (data->double_tap_handler && itds_2533020201601_status.doubleTap) {
		data->double_tap_handler(dev, data->double_tap_trigger);
	}
#endif /* CONFIG_WSEN_ITDS_2533020201601_TAP */

#ifdef CONFIG_WSEN_ITDS_2533020201601_FREEFALL
	if (data->freefall_handler && itds_2533020201601_status.freeFall) {
		data->freefall_handler(dev, data->freefall_trigger);
	}
#endif /* CONFIG_WSEN_ITDS_2533020201601_FREEFALL */

#ifdef CONFIG_WSEN_ITDS_2533020201601_DELTA
	if (data->delta_handler && itds_2533020201601_status.wakeUp) {
		data->delta_handler(dev, data->delta_trigger);
	}
#endif /* CONFIG_WSEN_ITDS_2533020201601_DELTA */

	/* Re-enable interrupt handling */
	itds_2533020201601_setup_interrupt(dev, &cfg->events_interrupt_gpio, true);
}

#endif /* CONFIG_WSEN_ITDS_2533020201601_EVENTS */

static void itds_2533020201601_process_interrupt_1(const struct device *dev)
{
	struct itds_2533020201601_data *data = dev->data;
	const struct itds_2533020201601_config *cfg = dev->config;
	ITDS_statusDetect_t status_detect;

	/* Read status register to check for data-ready - if not using soft trigger, the
	 * status register is also used to find out which interrupt occurred.
	 */
	if (ITDS_getStatusDetectRegister(&data->sensor_interface, &status_detect) != WE_SUCCESS) {
		LOG_ERR("Failed to read status register");
		return;
	}

	if (data->accel_data_ready_handler && status_detect.dataReady) {
		data->accel_data_ready_handler(dev, data->accel_data_ready_trigger);
	}

	if (data->temp_data_ready_handler && status_detect.temperatureDataReady) {
		data->temp_data_ready_handler(dev, data->temp_data_ready_trigger);
	}

	/* Re-enable interrupt handling */
	itds_2533020201601_setup_interrupt(dev, &cfg->drdy_interrupt_gpio, true);
}

/*
 * Is called when interrupt on INT1.
 * Triggers asynchronous handling of interrupt in itds_2533020201601_process_interrupt().
 */
static void itds_2533020201601_interrupt_1_gpio_callback(const struct device *dev,
							 struct gpio_callback *cb, uint32_t pins)
{
	struct itds_2533020201601_data *data =
		CONTAINER_OF(cb, struct itds_2533020201601_data, drdy_interrupt_cb);

	ARG_UNUSED(pins);

	itds_2533020201601_handle_interrupt_1(data->dev);
}

#ifdef CONFIG_WSEN_ITDS_2533020201601_EVENTS
/*
 * Is called when interrupt on INT0.
 * Triggers asynchronous handling of interrupt in itds_2533020201601_process_interrupt().
 */
static void itds_2533020201601_interrupt_0_gpio_callback(const struct device *dev,
							 struct gpio_callback *cb, uint32_t pins)
{
	struct itds_2533020201601_data *data =
		CONTAINER_OF(cb, struct itds_2533020201601_data, events_interrupt_cb);

	ARG_UNUSED(pins);

	itds_2533020201601_handle_interrupt_0(data->dev);
}
#endif

#ifdef CONFIG_WSEN_ITDS_2533020201601_TRIGGER_OWN_THREAD
static void itds_2533020201601_drdy_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct itds_2533020201601_data *data = p1;

	while (true) {
		k_sem_take(&data->drdy_sem, K_FOREVER);
		itds_2533020201601_process_interrupt_1(data->dev);
	}
}
#ifdef CONFIG_WSEN_ITDS_2533020201601_EVENTS
static void itds_2533020201601_events_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct itds_2533020201601_data *data = p1;

	while (true) {
		k_sem_take(&data->events_sem, K_FOREVER);
		itds_2533020201601_process_interrupt_0(data->dev);
	}
}
#endif /* CONFIG_WSEN_ITDS_2533020201601_EVENTS */
#endif /* CONFIG_WSEN_ITDS_2533020201601_TRIGGER_OWN_THREAD */

#ifdef CONFIG_WSEN_ITDS_2533020201601_TRIGGER_GLOBAL_THREAD
static void itds_2533020201601_drdy_work_cb(struct k_work *work)
{
	struct itds_2533020201601_data *itds_2533020201601 =
		CONTAINER_OF(work, struct itds_2533020201601_data, drdy_work);

	itds_2533020201601_process_interrupt_1(itds_2533020201601->dev);
}
#ifdef CONFIG_WSEN_ITDS_2533020201601_EVENTS
static void itds_2533020201601_events_work_cb(struct k_work *work)
{
	struct itds_2533020201601_data *itds_2533020201601 =
		CONTAINER_OF(work, struct itds_2533020201601_data, events_work);

	itds_2533020201601_process_interrupt_0(itds_2533020201601->dev);
}
#endif /* CONFIG_WSEN_ITDS_2533020201601_EVENTS */
#endif /* CONFIG_WSEN_ITDS_2533020201601_TRIGGER_GLOBAL_THREAD */

/* (Un)register trigger handler and enable/disable the corresponding sensor interrupt */
int itds_2533020201601_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
				   sensor_trigger_handler_t handler)
{
	struct itds_2533020201601_data *data = dev->data;
	const struct itds_2533020201601_config *cfg = dev->config;

	ITDS_state_t state = (handler != NULL) ? ITDS_enable : ITDS_disable;

	switch (trig->type) {
	case SENSOR_TRIG_DATA_READY: {
		int16_t x, y, z;

		switch (trig->chan) {
		case SENSOR_CHAN_ACCEL_X:
		case SENSOR_CHAN_ACCEL_Y:
		case SENSOR_CHAN_ACCEL_Z:
		case SENSOR_CHAN_ACCEL_XYZ: {
			data->accel_data_ready_handler = handler;
			data->accel_data_ready_trigger = trig;
			itds_2533020201601_setup_interrupt(dev, &cfg->drdy_interrupt_gpio,
							   data->accel_data_ready_handler ||
								   data->temp_data_ready_handler);
			if (state) {
				/* Dummy read: re-trigger interrupt */
				ITDS_getRawAccelerationX(&data->sensor_interface, &x);
				ITDS_getRawAccelerationY(&data->sensor_interface, &y);
				ITDS_getRawAccelerationZ(&data->sensor_interface, &z);
			}
			if (ITDS_enableDataReadyINT1(&data->sensor_interface, state) !=
			    WE_SUCCESS) {
				return -EIO;
			}
			break;
		}
		case SENSOR_CHAN_AMBIENT_TEMP: {
			data->temp_data_ready_handler = handler;
			data->temp_data_ready_trigger = trig;
			itds_2533020201601_setup_interrupt(dev, &cfg->drdy_interrupt_gpio,
							   data->accel_data_ready_handler ||
								   data->temp_data_ready_handler);
			if (state) {
				/* Dummy read: re-trigger interrupt */
				ITDS_getRawTemperature12bit(&data->sensor_interface, &x);
			}
			if (ITDS_enableTempDataReadyINT1(&data->sensor_interface, state) !=
			    WE_SUCCESS) {
				return -EIO;
			}
			break;
		}
		default:
			goto error;
		}
		return 0;
	}
#ifdef CONFIG_WSEN_ITDS_2533020201601_TAP
	case SENSOR_TRIG_TAP:
		if (trig->chan != SENSOR_CHAN_ALL) {
			break;
		}
		data->single_tap_handler = handler;
		data->single_tap_trigger = trig;
		itds_2533020201601_setup_interrupt(
			dev, &cfg->events_interrupt_gpio,
			data->single_tap_handler || data->double_tap_handler ||
				data->freefall_handler || data->delta_handler);
		if (ITDS_enableSingleTapINT0(&data->sensor_interface, state) != WE_SUCCESS) {
			return -EIO;
		}
		return 0;
	case SENSOR_TRIG_DOUBLE_TAP:
		if (trig->chan != SENSOR_CHAN_ALL) {
			break;
		}
		data->double_tap_handler = handler;
		data->double_tap_trigger = trig;
		itds_2533020201601_setup_interrupt(
			dev, &cfg->events_interrupt_gpio,
			data->single_tap_handler || data->double_tap_handler ||
				data->freefall_handler || data->delta_handler);
		if (ITDS_enableDoubleTapINT0(&data->sensor_interface, state) != WE_SUCCESS) {
			return -EIO;
		}
		return 0;
#endif /* CONFIG_WSEN_ITDS_2533020201601_TAP */

#ifdef CONFIG_WSEN_ITDS_2533020201601_FREEFALL
	case SENSOR_TRIG_FREEFALL:
		if (trig->chan != SENSOR_CHAN_ALL) {
			break;
		}
		data->freefall_handler = handler;
		data->freefall_trigger = trig;
		itds_2533020201601_setup_interrupt(
			dev, &cfg->events_interrupt_gpio,
			data->single_tap_handler || data->double_tap_handler ||
				data->freefall_handler || data->delta_handler);
		if (ITDS_enableFreeFallINT0(&data->sensor_interface, state) != WE_SUCCESS) {
			return -EIO;
		}
		return 0;
#endif /* CONFIG_WSEN_ITDS_2533020201601_FREEFALL */

#ifdef CONFIG_WSEN_ITDS_2533020201601_DELTA
	case SENSOR_TRIG_DELTA:
		if (trig->chan != SENSOR_CHAN_ALL) {
			break;
		}
		data->delta_handler = handler;
		data->delta_trigger = trig;
		itds_2533020201601_setup_interrupt(
			dev, &cfg->events_interrupt_gpio,
			data->single_tap_handler || data->double_tap_handler ||
				data->freefall_handler || data->delta_handler);
		if (ITDS_enableWakeUpOnINT0(&data->sensor_interface, state) != WE_SUCCESS) {
			return -EIO;
		}
		return 0;
#endif /* CONFIG_WSEN_ITDS_2533020201601_DELTA */
	default:
		break;
	}

error:
	LOG_ERR("Unsupported sensor trigger");
	return -ENOTSUP;
}

int itds_2533020201601_init_interrupt(const struct device *dev)
{
	struct itds_2533020201601_data *data = dev->data;
	const struct itds_2533020201601_config *cfg = dev->config;

	data->dev = dev;

	if (cfg->drdy_interrupt_gpio.port == NULL) {
		LOG_DBG("drdy-interrupt-gpios is not defined in the device tree.");
		return -EINVAL;
	}

	if (!gpio_is_ready_dt(&cfg->drdy_interrupt_gpio)) {
		LOG_ERR("Device %s is not ready", cfg->drdy_interrupt_gpio.port->name);
		return -ENODEV;
	}

	/* Setup interrupt gpio */
	if (gpio_pin_configure_dt(&cfg->drdy_interrupt_gpio, GPIO_INPUT) < 0) {
		LOG_ERR("Failed to configure %s.%02u", cfg->drdy_interrupt_gpio.port->name,
			cfg->drdy_interrupt_gpio.pin);
		return -EIO;
	}

	gpio_init_callback(&data->drdy_interrupt_cb, itds_2533020201601_interrupt_1_gpio_callback,
			   BIT(cfg->drdy_interrupt_gpio.pin));

	if (gpio_add_callback(cfg->drdy_interrupt_gpio.port, &data->drdy_interrupt_cb) < 0) {
		LOG_ERR("Failed to set gpio callback");
		return -EIO;
	}

#if defined(CONFIG_WSEN_ITDS_2533020201601_EVENTS)

	if (cfg->events_interrupt_gpio.port == NULL) {
		LOG_DBG("events-interrupt-gpios is not defined in the device tree.");
		return -EINVAL;
	}

	if (!gpio_is_ready_dt(&cfg->events_interrupt_gpio)) {
		LOG_ERR("Device %s is not ready", cfg->events_interrupt_gpio.port->name);
		return -ENODEV;
	}

	/* Setup interrupt gpio */
	if (gpio_pin_configure_dt(&cfg->events_interrupt_gpio, GPIO_INPUT) < 0) {
		LOG_ERR("Failed to configure %s.%02u", cfg->events_interrupt_gpio.port->name,
			cfg->events_interrupt_gpio.pin);
		return -EIO;
	}

	gpio_init_callback(&data->events_interrupt_cb, itds_2533020201601_interrupt_0_gpio_callback,
			   BIT(cfg->events_interrupt_gpio.pin));

	if (gpio_add_callback(cfg->events_interrupt_gpio.port, &data->events_interrupt_cb) < 0) {
		LOG_ERR("Failed to set gpio callback");
		return -EIO;
	}

#endif

#if defined(CONFIG_WSEN_ITDS_2533020201601_TRIGGER_OWN_THREAD)
	k_sem_init(&data->drdy_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&data->drdy_thread, data->drdy_thread_stack,
			CONFIG_WSEN_ITDS_2533020201601_THREAD_STACK_SIZE,
			itds_2533020201601_drdy_thread, data, NULL, NULL,
			K_PRIO_COOP(CONFIG_WSEN_ITDS_2533020201601_THREAD_PRIORITY), 0, K_NO_WAIT);
#if defined(CONFIG_WSEN_ITDS_2533020201601_EVENTS)
	k_sem_init(&data->events_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&data->events_thread, data->events_thread_stack,
			CONFIG_WSEN_ITDS_2533020201601_THREAD_STACK_SIZE,
			itds_2533020201601_events_thread, data, NULL, NULL,
			K_PRIO_COOP(CONFIG_WSEN_ITDS_2533020201601_THREAD_PRIORITY), 0, K_NO_WAIT);
#endif
#elif defined(CONFIG_WSEN_ITDS_2533020201601_TRIGGER_GLOBAL_THREAD)
	data->drdy_work.handler = itds_2533020201601_drdy_work_cb;
#if defined(CONFIG_WSEN_ITDS_2533020201601_EVENTS)
	data->events_work.handler = itds_2533020201601_events_work_cb;

#endif
#endif

	/* Enable interrupt in pulsed mode */
	if (ITDS_enableLatchedInterrupt(&data->sensor_interface, ITDS_disable) != WE_SUCCESS) {
		LOG_ERR("Failed to disable latched mode");
		return -EIO;
	}

	/* Enable data-ready in pulsed mode */
	if (ITDS_setDataReadyPulsed(&data->sensor_interface, ITDS_pulsed) != WE_SUCCESS) {
		LOG_ERR("Failed to enable data-ready pulsed mode");
		return -EIO;
	}

	if (ITDS_enableInterrupts(&data->sensor_interface, ITDS_enable) != WE_SUCCESS) {
		LOG_ERR("Failed to enable interrupts");
		return -EIO;
	}

#ifdef CONFIG_WSEN_ITDS_2533020201601_TAP
	if (!(cfg->op_mode == ITDS_highPerformance && cfg->odr >= ITDS_odr7)) {
		LOG_WRN("A minimum output data rate of 400 Hz is recommended when using the tap "
			"recognition feature");
	}

	if (ITDS_enableDoubleTapEvent(&data->sensor_interface,
				      (cfg->tap_mode == 1) ? ITDS_enable : ITDS_disable) !=
	    WE_SUCCESS) {
		LOG_ERR("Failed to enable/disable double tap event");
		return -EIO;
	}

	if (ITDS_setTapThresholdX(&data->sensor_interface, cfg->tap_threshold[0]) != WE_SUCCESS) {
		LOG_ERR("Failed to set X axis tap threshold");
		return -EIO;
	}

	if (ITDS_setTapThresholdY(&data->sensor_interface, cfg->tap_threshold[1]) != WE_SUCCESS) {
		LOG_ERR("Failed to set Y axis tap threshold");
		return -EIO;
	}

	if (ITDS_setTapThresholdZ(&data->sensor_interface, cfg->tap_threshold[2]) != WE_SUCCESS) {
		LOG_ERR("Failed to set Z axis tap threshold");
		return -EIO;
	}

	if (cfg->tap_threshold[0] > 0) {
		if (ITDS_enableTapX(&data->sensor_interface, ITDS_enable) != WE_SUCCESS) {
			LOG_ERR("Failed to enable tap recognition in X direction");
			return -EIO;
		}
	}

	if (cfg->tap_threshold[1] > 0) {
		if (ITDS_enableTapY(&data->sensor_interface, ITDS_enable) != WE_SUCCESS) {
			LOG_ERR("Failed to enable tap recognition in Y direction");
			return -EIO;
		}
	}

	if (cfg->tap_threshold[2] > 0) {
		if (ITDS_enableTapZ(&data->sensor_interface, ITDS_enable) != WE_SUCCESS) {
			LOG_ERR("Failed to enable tap recognition in Z direction");
			return -EIO;
		}
	}

	if (ITDS_setTapShockTime(&data->sensor_interface, cfg->tap_shock) != WE_SUCCESS) {
		LOG_ERR("Failed to set tap shock duration");
		return -EIO;
	}

	if (ITDS_setTapLatencyTime(&data->sensor_interface, cfg->tap_latency) != WE_SUCCESS) {
		LOG_ERR("Failed to set tap latency");
		return -EIO;
	}

	if (ITDS_setTapQuietTime(&data->sensor_interface, cfg->tap_quiet) != WE_SUCCESS) {
		LOG_ERR("Failed to set tap quiet time");
		return -EIO;
	}
#endif /* CONFIG_WSEN_ITDS_2533020201601_TAP */

#ifdef CONFIG_WSEN_ITDS_2533020201601_FREEFALL
	if (ITDS_setFreeFallDuration(&data->sensor_interface, cfg->freefall_duration) !=
	    WE_SUCCESS) {
		LOG_ERR("Failed to set free-fall duration");
		return -EIO;
	}

	if (ITDS_setFreeFallThreshold(&data->sensor_interface, cfg->freefall_threshold) !=
	    WE_SUCCESS) {
		LOG_ERR("Failed to set free-fall threshold");
		return -EIO;
	}
#endif /* CONFIG_WSEN_ITDS_2533020201601_FREEFALL */

#ifdef CONFIG_WSEN_ITDS_2533020201601_DELTA
	if (ITDS_setWakeUpDuration(&data->sensor_interface, cfg->delta_duration) != WE_SUCCESS) {
		LOG_ERR("Failed to set wake-up duration");
		return -EIO;
	}

	if (ITDS_setWakeUpThreshold(&data->sensor_interface, cfg->delta_threshold) != WE_SUCCESS) {
		LOG_ERR("Failed to set wake-up threshold");
		return -EIO;
	}

	if (cfg->delta_offsets[0] != 0 || cfg->delta_offsets[1] != 0 ||
	    cfg->delta_offsets[2] != 0) {
		if (ITDS_setOffsetWeight(&data->sensor_interface, (cfg->delta_offset_weight) != 0
									  ? ITDS_enable
									  : ITDS_disable) !=
		    WE_SUCCESS) {
			LOG_ERR("Failed to set wake-up offset weight");
			return -EIO;
		}

		if (ITDS_setOffsetValueX(&data->sensor_interface, cfg->delta_offsets[0]) !=
		    WE_SUCCESS) {
			LOG_ERR("Failed to set wake-up X offset");
			return -EIO;
		}

		if (ITDS_setOffsetValueY(&data->sensor_interface, cfg->delta_offsets[1]) !=
		    WE_SUCCESS) {
			LOG_ERR("Failed to set wake-up Y offset");
			return -EIO;
		}

		if (ITDS_setOffsetValueZ(&data->sensor_interface, cfg->delta_offsets[2]) !=
		    WE_SUCCESS) {
			LOG_ERR("Failed to set wake-up Z offset");
			return -EIO;
		}

		if (ITDS_enableApplyWakeUpOffset(&data->sensor_interface, ITDS_enable) !=
		    WE_SUCCESS) {
			LOG_ERR("Failed to enable wake-up offsets");
			return -EIO;
		}
	}
#endif /* CONFIG_WSEN_ITDS_2533020201601_DELTA */

	return 0;
}
