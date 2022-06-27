/*
 * Copyright (c) 2022 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT we_wsen_itds

#include <zephyr/logging/log.h>

#include "itds.h"

LOG_MODULE_DECLARE(ITDS, CONFIG_SENSOR_LOG_LEVEL);

/* Enable/disable interrupt handling for data-ready, tap, free-fall, delta/wake-up */
static inline int itds_setup_interrupt(const struct device *dev, bool enable)
{
	const struct itds_config *cfg = dev->config;
	unsigned int flags = enable ? GPIO_INT_EDGE_TO_ACTIVE : GPIO_INT_DISABLE;

	return gpio_pin_interrupt_configure_dt(&cfg->gpio_interrupts, flags);
}

static inline void itds_handle_interrupt(const struct device *dev)
{
	struct itds_data *data = dev->data;

	/* Disable interrupt handling until the interrupt has been processed */
	itds_setup_interrupt(data->dev, false);

#if defined(CONFIG_ITDS_TRIGGER_OWN_THREAD)
	k_sem_give(&data->interrupt_sem);
#elif defined(CONFIG_ITDS_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

static int itds_process_drdy_interrupt(const struct device *dev)
{
	struct itds_data *data = dev->data;

	struct sensor_trigger drdy_trig = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ALL,
	};

	if (data->data_ready_handler) {
		data->data_ready_handler(dev, &drdy_trig);
	}

	return 0;
}

#ifdef CONFIG_ITDS_TAP
static int itds_process_single_tap_interrupt(const struct device *dev)
{
	struct itds_data *data = dev->data;
	sensor_trigger_handler_t handler = data->single_tap_handler;

	struct sensor_trigger tap_trig = {
		.type = SENSOR_TRIG_TAP,
		.chan = SENSOR_CHAN_ALL,
	};

	if (handler) {
		handler(dev, &tap_trig);
	}

	return 0;
}

static int itds_process_double_tap_interrupt(const struct device *dev)
{
	struct itds_data *data = dev->data;
	sensor_trigger_handler_t handler = data->double_tap_handler;

	struct sensor_trigger tap_trig = {
		.type = SENSOR_TRIG_DOUBLE_TAP,
		.chan = SENSOR_CHAN_ALL,
	};

	if (handler) {
		handler(dev, &tap_trig);
	}

	return 0;
}
#endif /* CONFIG_ITDS_TAP */

#ifdef CONFIG_ITDS_FREEFALL
static int itds_process_freefall_interrupt(const struct device *dev)
{
	struct itds_data *data = dev->data;
	sensor_trigger_handler_t handler = data->freefall_handler;

	struct sensor_trigger freefall_trig = {
		.type = SENSOR_TRIG_FREEFALL,
		.chan = SENSOR_CHAN_ALL,
	};

	if (handler) {
		handler(dev, &freefall_trig);
	}

	return 0;
}
#endif /* CONFIG_ITDS_FREEFALL */

#ifdef CONFIG_ITDS_DELTA
static int itds_process_delta_interrupt(const struct device *dev)
{
	struct itds_data *data = dev->data;
	sensor_trigger_handler_t handler = data->delta_handler;

	struct sensor_trigger delta_trig = {
		.type = SENSOR_TRIG_DELTA,
		.chan = SENSOR_CHAN_ALL,
	};

	if (handler) {
		handler(dev, &delta_trig);
	}

	return 0;
}
#endif /* CONFIG_ITDS_DELTA */

/* Asynchronous handling of interrupt triggered in itds_gpio_callback() */
static void itds_process_interrupt(const struct device *dev)
{
	struct itds_data *data = dev->data;
	ITDS_status_t itds_status;

	/* Read status register to find out which interrupt occurred. */
	if (WE_SUCCESS != ITDS_getStatusRegister(&data->sensor_interface, &itds_status)) {
		LOG_ERR("Failed to read status register");
		return;
	}

	if (itds_status.dataReady) {
		itds_process_drdy_interrupt(dev);
	}

#ifdef CONFIG_ITDS_TAP
	if (itds_status.singleTap) {
		itds_process_single_tap_interrupt(dev);
	}

	if (itds_status.doubleTap) {
		itds_process_double_tap_interrupt(dev);
	}
#endif /* CONFIG_ITDS_TAP */

#ifdef CONFIG_ITDS_FREEFALL
	if (itds_status.freeFall) {
		itds_process_freefall_interrupt(dev);
	}
#endif /* CONFIG_ITDS_FREEFALL */

#ifdef CONFIG_ITDS_DELTA
	if (itds_status.wakeUp) {
		itds_process_delta_interrupt(dev);
	}
#endif /* CONFIG_ITDS_DELTA */

	/* Re-enable interrupt handling */
	itds_setup_interrupt(dev, true);
}

/*
 * Is called when any interrupt has occurred (data-ready, tap, free-fall, delta/wake-up).
 * Triggers asynchronous handling of interrupt in itds_process_interrupt().
 */
static void itds_gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct itds_data *data = CONTAINER_OF(cb, struct itds_data, interrupt_cb);

	ARG_UNUSED(pins);

	itds_handle_interrupt(data->dev);
}

#ifdef CONFIG_ITDS_TRIGGER_OWN_THREAD
static void itds_thread(struct itds_data *data)
{
	while (1) {
		k_sem_take(&data->interrupt_sem, K_FOREVER);
		itds_process_interrupt(data->dev);
	}
}
#endif /* CONFIG_ITDS_TRIGGER_OWN_THREAD */

#ifdef CONFIG_ITDS_TRIGGER_GLOBAL_THREAD
static void itds_work_cb(struct k_work *work)
{
	struct itds_data *itds = CONTAINER_OF(work, struct itds_data, work);

	itds_process_interrupt(itds->dev);
}
#endif /* CONFIG_ITDS_TRIGGER_GLOBAL_THREAD */

/* Enables or disables an interrupt in the sensor. */
static int itds_enable_interrupt(const struct device *dev, enum sensor_trigger_type type,
				 ITDS_state_t enable)
{
	const struct itds_config *cfg = dev->config;
	struct itds_data *data = dev->data;

	/* Enable interrupt handling if any trigger handler has been registered */
	itds_setup_interrupt(dev, (NULL != data->data_ready_handler ||
				   NULL != data->single_tap_handler ||
				   NULL != data->double_tap_handler ||
				   NULL != data->freefall_handler ||
				   NULL != data->delta_handler));

	switch (type) {
	case SENSOR_TRIG_DATA_READY:
		if (cfg->drdy_int == 0) {
			/* Enable/disable interrupt for pin INT_0 */
			if (WE_SUCCESS !=
			    ITDS_enableDataReadyINT0(&data->sensor_interface, enable)) {
				return -EIO;
			}
		} else {
			/* Enable/disable interrupt for pin INT_1 */
			if (WE_SUCCESS !=
			    ITDS_enableDataReadyINT1(&data->sensor_interface, enable)) {
				return -EIO;
			}
		}
		break;

#ifdef CONFIG_ITDS_TAP
	case SENSOR_TRIG_TAP:
		/* Enable/disable interrupt for pin INT_0 */
		if (WE_SUCCESS != ITDS_enableSingleTapINT0(&data->sensor_interface, enable)) {
			return -EIO;
		}
		break;

	case SENSOR_TRIG_DOUBLE_TAP:
		/* Enable/disable interrupt for pin INT_0 */
		if (WE_SUCCESS != ITDS_enableDoubleTapINT0(&data->sensor_interface, enable)) {
			return -EIO;
		}
		break;
#endif /* CONFIG_ITDS_TAP */

#ifdef CONFIG_ITDS_FREEFALL
	case SENSOR_TRIG_FREEFALL:
		/* Enable/disable interrupt for pin INT_0 */
		if (WE_SUCCESS != ITDS_enableFreeFallINT0(&data->sensor_interface, enable)) {
			return -EIO;
		}
		break;
#endif /* CONFIG_ITDS_FREEFALL */

#ifdef CONFIG_ITDS_DELTA
	case SENSOR_TRIG_DELTA:
		/* Enable/disable interrupt for pin INT_0 */
		if (WE_SUCCESS != ITDS_enableWakeUpOnINT0(&data->sensor_interface, enable)) {
			return -EIO;
		}
		break;
#endif /* CONFIG_ITDS_DELTA */

	default:
		LOG_ERR("Unsupported trigger interrupt route %d", type);
		return -ENOTSUP;
	}

	return 0;
}

/* (Un)register trigger handler and enable/disable the corresponding sensor interrupt */
int itds_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
		     sensor_trigger_handler_t handler)
{
	struct itds_data *data = dev->data;
	int16_t x, y, z;
	ITDS_state_t state = (handler != NULL) ? ITDS_enable : ITDS_disable;

	switch (trig->type) {
	case SENSOR_TRIG_DATA_READY:
		data->data_ready_handler = handler;
		if (state) {
			/* Dummy read: re-trigger interrupt */
			ITDS_getRawAccelerations(&data->sensor_interface, 1, &x, &y, &z);
		}
		return itds_enable_interrupt(dev, SENSOR_TRIG_DATA_READY, state);

#ifdef CONFIG_ITDS_TAP
	case SENSOR_TRIG_TAP:
		data->single_tap_handler = handler;
		return itds_enable_interrupt(dev, SENSOR_TRIG_TAP, state);

	case SENSOR_TRIG_DOUBLE_TAP:
		data->double_tap_handler = handler;
		return itds_enable_interrupt(dev, SENSOR_TRIG_DOUBLE_TAP, state);
#endif /* CONFIG_ITDS_TAP */

#ifdef CONFIG_ITDS_FREEFALL
	case SENSOR_TRIG_FREEFALL:
		data->freefall_handler = handler;
		return itds_enable_interrupt(dev, SENSOR_TRIG_FREEFALL, state);
#endif /* CONFIG_ITDS_FREEFALL */

#ifdef CONFIG_ITDS_DELTA
	case SENSOR_TRIG_DELTA:
		data->delta_handler = handler;
		return itds_enable_interrupt(dev, SENSOR_TRIG_DELTA, state);
#endif /* CONFIG_ITDS_DELTA */

	default:
		LOG_ERR("Unsupported sensor trigger");
		return -ENOTSUP;
	}
}

int itds_init_interrupt(const struct device *dev)
{
	struct itds_data *data = dev->data;
	const struct itds_config *cfg = dev->config;
	int status;

	data->dev = dev;

	if (cfg->gpio_interrupts.port == NULL) {
		LOG_DBG("int-gpios is not defined in the device tree.");
		return -EINVAL;
	}

	if (!device_is_ready(cfg->gpio_interrupts.port)) {
		LOG_ERR("Device %s is not ready", cfg->gpio_interrupts.port->name);
		return -ENODEV;
	}

	/* Setup interrupt gpio */
	status = gpio_pin_configure_dt(&cfg->gpio_interrupts, GPIO_INPUT);
	if (status < 0) {
		LOG_ERR("Failed to configure %s.%02u", cfg->gpio_interrupts.port->name,
			cfg->gpio_interrupts.pin);
		return status;
	}

	gpio_init_callback(&data->interrupt_cb, itds_gpio_callback, BIT(cfg->gpio_interrupts.pin));

	status = gpio_add_callback(cfg->gpio_interrupts.port, &data->interrupt_cb);
	if (status < 0) {
		LOG_ERR("Failed to set gpio callback");
		return status;
	}

#if defined(CONFIG_ITDS_TRIGGER_OWN_THREAD)
	k_sem_init(&data->interrupt_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&data->thread, data->thread_stack, CONFIG_ITDS_THREAD_STACK_SIZE,
			(k_thread_entry_t)itds_thread, data, NULL, NULL,
			K_PRIO_COOP(CONFIG_ITDS_THREAD_PRIORITY), 0, K_NO_WAIT);
#elif defined(CONFIG_ITDS_TRIGGER_GLOBAL_THREAD)
	data->work.handler = itds_work_cb;
#endif

	/* Enable interrupt on INT_0/INT_1 in pulsed mode */
	if (WE_SUCCESS != ITDS_enableLatchedInterrupt(&data->sensor_interface, ITDS_disable)) {
		LOG_ERR("Failed to disable latched mode");
		return -EIO;
	}

	/* Enable data-ready interrupt on INT_0/INT_1 in pulsed mode */
	if (WE_SUCCESS != ITDS_setDataReadyPulsed(&data->sensor_interface, ITDS_enable)) {
		LOG_ERR("Failed to enable data-ready pulsed mode");
		return -EIO;
	}

	if (WE_SUCCESS != ITDS_enableInterrupts(&data->sensor_interface, ITDS_enable)) {
		LOG_ERR("Failed to enable interrupts");
		return -EIO;
	}

#ifdef CONFIG_ITDS_TAP
	if (cfg->op_mode != itds_op_mode_high_performance || cfg->odr < ITDS_odr7) {
		LOG_WRN("The tap recognition feature requires a minimum output data rate of 400 Hz");
	}

	if (WE_SUCCESS !=
	    ITDS_enableDoubleTapEvent(&data->sensor_interface,
				      (cfg->tap_mode == 1) ? ITDS_enable : ITDS_disable)) {
		LOG_ERR("Failed to enable/disable double tap event");
		return -EIO;
	}

	if (WE_SUCCESS != ITDS_setTapThresholdX(&data->sensor_interface, cfg->tap_threshold[0])) {
		LOG_ERR("Failed to set X axis tap threshold");
		return -EIO;
	}

	if (WE_SUCCESS != ITDS_setTapThresholdY(&data->sensor_interface, cfg->tap_threshold[1])) {
		LOG_ERR("Failed to set Y axis tap threshold");
		return -EIO;
	}

	if (WE_SUCCESS != ITDS_setTapThresholdZ(&data->sensor_interface, cfg->tap_threshold[2])) {
		LOG_ERR("Failed to set Z axis tap threshold");
		return -EIO;
	}

	if (cfg->tap_threshold[0] > 0) {
		if (WE_SUCCESS != ITDS_enableTapX(&data->sensor_interface, ITDS_enable)) {
			LOG_ERR("Failed to enable tap recognition in X direction");
			return -EIO;
		}
	}

	if (cfg->tap_threshold[1] > 0) {
		if (WE_SUCCESS != ITDS_enableTapY(&data->sensor_interface, ITDS_enable)) {
			LOG_ERR("Failed to enable tap recognition in Y direction");
			return -EIO;
		}
	}

	if (cfg->tap_threshold[2] > 0) {
		if (WE_SUCCESS != ITDS_enableTapZ(&data->sensor_interface, ITDS_enable)) {
			LOG_ERR("Failed to enable tap recognition in Z direction");
			return -EIO;
		}
	}

	if (WE_SUCCESS != ITDS_setTapShockTime(&data->sensor_interface, cfg->tap_shock)) {
		LOG_ERR("Failed to set tap shock duration");
		return -EIO;
	}

	if (WE_SUCCESS != ITDS_setTapLatencyTime(&data->sensor_interface, cfg->tap_latency)) {
		LOG_ERR("Failed to set tap latency");
		return -EIO;
	}

	if (WE_SUCCESS != ITDS_setTapQuietTime(&data->sensor_interface, cfg->tap_quiet)) {
		LOG_ERR("Failed to set tap quiet time");
		return -EIO;
	}
#endif /* CONFIG_ITDS_TAP */

#ifdef CONFIG_ITDS_FREEFALL
	if (WE_SUCCESS !=
	    ITDS_setFreeFallDuration(&data->sensor_interface, cfg->freefall_duration)) {
		LOG_ERR("Failed to set free-fall duration");
		return -EIO;
	}

	if (WE_SUCCESS !=
	    ITDS_setFreeFallThreshold(&data->sensor_interface, cfg->freefall_threshold)) {
		LOG_ERR("Failed to set free-fall threshold");
		return -EIO;
	}
#endif /* CONFIG_ITDS_FREEFALL */

#ifdef CONFIG_ITDS_DELTA
	if (WE_SUCCESS != ITDS_setWakeUpDuration(&data->sensor_interface, cfg->delta_duration)) {
		LOG_ERR("Failed to set wake-up duration");
		return -EIO;
	}

	if (WE_SUCCESS != ITDS_setWakeUpThreshold(&data->sensor_interface, cfg->delta_threshold)) {
		LOG_ERR("Failed to set wake-up threshold");
		return -EIO;
	}

	if (cfg->delta_offsets[0] != 0 || cfg->delta_offsets[1] != 0 ||
	    cfg->delta_offsets[2] != 0) {
		if (WE_SUCCESS !=
		    ITDS_setOffsetWeight(&data->sensor_interface, (cfg->delta_offset_weight) != 0 ?
									  ITDS_enable :
									  ITDS_disable)) {
			LOG_ERR("Failed to set wake-up offset weight");
			return -EIO;
		}

		if (WE_SUCCESS !=
		    ITDS_setOffsetValueX(&data->sensor_interface, cfg->delta_offsets[0])) {
			LOG_ERR("Failed to set wake-up X offset");
			return -EIO;
		}

		if (WE_SUCCESS !=
		    ITDS_setOffsetValueY(&data->sensor_interface, cfg->delta_offsets[1])) {
			LOG_ERR("Failed to set wake-up Y offset");
			return -EIO;
		}

		if (WE_SUCCESS !=
		    ITDS_setOffsetValueZ(&data->sensor_interface, cfg->delta_offsets[2])) {
			LOG_ERR("Failed to set wake-up Z offset");
			return -EIO;
		}

		if (WE_SUCCESS !=
		    ITDS_enableApplyWakeUpOffset(&data->sensor_interface, ITDS_enable)) {
			LOG_ERR("Failed to enable wake-up offsets");
			return -EIO;
		}
	}
#endif /* CONFIG_ITDS_DELTA */

	return 0;
}
