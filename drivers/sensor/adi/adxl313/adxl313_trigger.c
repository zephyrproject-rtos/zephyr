/*
 * Copyright (c) 2025 Lothar Rubusch <l.rubusch@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_adxl313

#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/logging/log.h>

#include "adxl313.h"

LOG_MODULE_DECLARE(ADXL313, CONFIG_SENSOR_LOG_LEVEL);

int adxl313_set_gpios_en(const struct device *dev, bool en)
{
	const struct adxl313_dev_config *cfg = dev->config;
	int state = en ? GPIO_INT_EDGE_TO_ACTIVE : GPIO_INT_DISABLE;

	if (!cfg->gpio_int1.port && !cfg->gpio_int2.port) {
		LOG_WRN("Neither gpio1, nor gpio2 is configured in devicetree");
		return -ENOTSUP;
	}

	if (cfg->drdy_pad == 1) {
		return gpio_pin_interrupt_configure_dt(&cfg->gpio_int1, state);
	} else if (cfg->drdy_pad == 2) {
		return gpio_pin_interrupt_configure_dt(&cfg->gpio_int2, state);
	}

	return -ENOTSUP;
}

#if defined(CONFIG_ADXL313_TRIGGER_OWN_THREAD) || defined(CONFIG_ADXL313_TRIGGER_GLOBAL_THREAD)
static void adxl313_thread_cb(const struct device *dev)
{
	struct adxl313_dev_data *data = dev->data;
	uint64_t cycles;
	uint8_t status;
	int rc = sensor_clock_get_cycles(&cycles);

	if (rc) {
		LOG_ERR("Failed to get sensor clock cycles with %d", rc);
		return;
	}
	data->timestamp = sensor_clock_cycles_to_ns(cycles);

	/* INT_SOURCE register parsing */
	k_mutex_lock(&data->trigger_mutex, K_FOREVER);
	rc = adxl313_get_status(dev, &data->reg_int_source);
	__ASSERT(rc == 0, "Interrupt configuration failed");
	status = data->reg_int_source;
	k_mutex_unlock(&data->trigger_mutex);

	if (FIELD_GET(ADXL313_INT_ACT, status)) {
		if (data->act_handler) {
			/* Optionally call external activity handler. */
			data->act_handler(dev, data->act_trigger);
		}
	}

	if (FIELD_GET(ADXL313_INT_DATA_RDY, status)) {
		if (data->drdy_handler) {
			/*
			 * A handler needs to flush FIFO, i.e. fetch and get
			 * samples to get new events.
			 */
			data->drdy_handler(dev, data->drdy_trigger);
		}
	}

	if (data->fifo_config.fifo_mode == ADXL313_FIFO_BYPASSED) {
		/* FIFO BYPASSED: Skip the rest. */
		return;
	}

	if (FIELD_GET(ADXL313_INT_WATERMARK, status)) {
		if (data->wm_handler) {
			/*
			 * A handler needs to implement fetch, then get FIFO
			 * entries according to configured watermark in order
			 * to obtain new sensor events.
			 */
			data->wm_handler(dev, data->wm_trigger);
		}
	}

	/* handle FIFO: overrun */
	if (FIELD_GET(ADXL313_INT_OVERRUN, status)) {
		if (data->overrun_handler) {
			/*
			 * A handler may handle read outs, the fallback flushes
			 * the fifo and interrupt status register.
			 */
			data->overrun_handler(dev, data->overrun_trigger);
		}

		/*
		 * If overrun handling is enabled, reset status register and
		 * fifo here, if not handled before in any way.
		 */
		adxl313_flush_fifo(dev);
	}
}
#endif

static void adxl313_int1_gpio_callback(const struct device *dev, struct gpio_callback *cb,
				       uint32_t pins)
{
	struct adxl313_dev_data *data = CONTAINER_OF(cb, struct adxl313_dev_data, int1_cb);

	ARG_UNUSED(pins);

#if defined(CONFIG_ADXL313_TRIGGER_OWN_THREAD)
	k_sem_give(&data->gpio_sem);
#elif defined(CONFIG_ADXL313_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

static void adxl313_int2_gpio_callback(const struct device *dev, struct gpio_callback *cb,
				       uint32_t pins)
{
	struct adxl313_dev_data *data = CONTAINER_OF(cb, struct adxl313_dev_data, int2_cb);

	ARG_UNUSED(pins);

#if defined(CONFIG_ADXL313_TRIGGER_OWN_THREAD)
	k_sem_give(&data->gpio_sem);
#elif defined(CONFIG_ADXL313_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

#if defined(CONFIG_ADXL313_TRIGGER_OWN_THREAD)
static void adxl313_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct adxl313_dev_data *data = p1;

	while (true) {
		k_sem_take(&data->gpio_sem, K_FOREVER);
		adxl313_thread_cb(data->dev);
	}
}

#elif defined(CONFIG_ADXL313_TRIGGER_GLOBAL_THREAD)
static void adxl313_work_cb(struct k_work *work)
{
	struct adxl313_dev_data *data = CONTAINER_OF(work, struct adxl313_dev_data, work);

	adxl313_thread_cb(data->dev);
}
#endif

/**
 * Register an application callback for sensor triggers.
 *
 * This function allows the application to register interrupt service routines
 * (ISRs) for specific sensor events. Supported triggers include:
 *
 * - SENSOR_TRIG_MOTION: Activity detection
 * - SENSOR_TRIG_FIFO_WATERMARK: FIFO watermark reached
 * - SENSOR_TRIG_DATA_READY: New FIFO data available
 * - SENSOR_TRIG_FIFO_FULL: FIFO overrun
 *
 * Note:
 * - FIFO data handling is typically done via the FIFO Watermark trigger,
 *   which usually coincides with data ready events.
 * - FIFO overrun is handled internally by the driver; register a handler only
 *   if the application needs to be notified of this condition.
 *
 * @param dev Pointer to the device structure.
 * @param trig Pointer to the sensor_trigger specifying the event to handle.
 * @param handler Callback function to invoke when the specified trigger occurs.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int adxl313_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	const struct adxl313_dev_config *cfg = dev->config;
	struct adxl313_dev_data *data = dev->data;
	int ret;

	if (!cfg->gpio_int1.port && !cfg->gpio_int2.port) {
		/* might be in FIFO BYPASS mode */
		goto done;
	}

	/* generally turn off interrupts */
	ret = adxl313_set_gpios_en(dev, false);
	if (ret) {
		goto done;
	}

	if (!handler) {
		goto done;
	}

	switch (trig->type) {
	case SENSOR_TRIG_MOTION:
		/* Register optional activity event handler. */
		data->act_handler = handler;
		data->act_trigger = trig;
		break;
	case SENSOR_TRIG_DATA_READY:
		data->drdy_handler = handler;
		data->drdy_trigger = trig;
		ret = adxl313_reg_assign_bits(dev, ADXL313_REG_INT_ENABLE, ADXL313_INT_DATA_RDY,
					      true);
		if (ret) {
			return ret;
		}
		break;
	case SENSOR_TRIG_FIFO_WATERMARK:
		if (data->fifo_config.fifo_mode == ADXL313_FIFO_BYPASSED) {
			/*
			 * FIFO and its watermark are optional to event handling
			 * other sensor events do not imply a running FIFO, but
			 * require a configured interrupt line.
			 */
			break;
		}
		data->wm_handler = handler;
		data->wm_trigger = trig;

		ret = adxl313_reg_assign_bits(dev, ADXL313_REG_INT_ENABLE, ADXL313_INT_WATERMARK,
					      true);
		if (ret) {
			return ret;
		}
		break;
	case SENSOR_TRIG_FIFO_FULL:
		data->overrun_handler = handler;
		data->overrun_trigger = trig;
		ret = adxl313_reg_assign_bits(dev, ADXL313_REG_INT_ENABLE, ADXL313_INT_OVERRUN,
					      true);
		if (ret) {
			return ret;
		}
		break;
	default:
		LOG_ERR("Unsupported sensor trigger");
		return -ENOTSUP;
	}

done:
	ret = adxl313_set_gpios_en(dev, true);
	if (ret) {
		return ret;
	}

	return adxl313_flush_fifo(dev);
}

int adxl313_init_interrupt(const struct device *dev)
{
	const struct adxl313_dev_config *cfg = dev->config;
	struct adxl313_dev_data *data = dev->data;
	int ret;

	k_mutex_init(&data->trigger_mutex);

	/* TRIGGER is set, but no INT line was defined in DT */
	if (!cfg->gpio_int1.port && !cfg->gpio_int2.port) {
		return -ENOTSUP;
	}

	if (cfg->gpio_int1.port) {
		if (!gpio_is_ready_dt(&cfg->gpio_int1)) {
			LOG_ERR("INT_1 line defined, but not ready");
			return -ENODEV;
		}
	}

	if (cfg->gpio_int2.port) {
		if (!gpio_is_ready_dt(&cfg->gpio_int2)) {
			LOG_ERR("INT_2 line defined, but not ready");
			return -ENODEV;
		}
	}

	data->dev = dev;

#if defined(CONFIG_ADXL313_TRIGGER_OWN_THREAD)
	k_sem_init(&data->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&data->thread, data->thread_stack, CONFIG_ADXL313_THREAD_STACK_SIZE,
			adxl313_thread, data, NULL, NULL,
			K_PRIO_COOP(CONFIG_ADXL313_THREAD_PRIORITY), 0, K_NO_WAIT);

	k_thread_name_set(&data->thread, dev->name);
#elif defined(CONFIG_ADXL313_TRIGGER_GLOBAL_THREAD)
	data->work.handler = adxl313_work_cb;
#endif
	if (cfg->gpio_int1.port) {
		ret = gpio_pin_configure_dt(&cfg->gpio_int1, GPIO_INPUT);
		if (ret) {
			LOG_WRN("GPIO INT_1 configuring to GPIO_INPUT failed");
			return ret;
		}

		gpio_init_callback(&data->int1_cb, adxl313_int1_gpio_callback,
				   BIT(cfg->gpio_int1.pin));

		ret = gpio_add_callback(cfg->gpio_int1.port, &data->int1_cb);
		if (ret) {
			LOG_ERR("Failed to set INT_1 gpio callback!");
			return -EIO;
		}
	}

	if (cfg->gpio_int2.port) {
		ret = gpio_pin_configure_dt(&cfg->gpio_int2, GPIO_INPUT);
		if (ret) {
			LOG_WRN("GPIO INT_2 configuring to GPIO_INPUT failed");
			return ret;
		}

		gpio_init_callback(&data->int2_cb, adxl313_int2_gpio_callback,
				   BIT(cfg->gpio_int2.pin));

		ret = gpio_add_callback(cfg->gpio_int2.port, &data->int2_cb);
		if (ret) {
			LOG_ERR("Failed to set INT_2 gpio callback!");
			return -EIO;
		}
	}

	return 0;
}
