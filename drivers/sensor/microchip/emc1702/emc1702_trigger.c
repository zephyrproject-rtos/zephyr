/*
 * Copyright (c) 2026  Microchip Technology Inc. and its subsidiaries
 * SPDX-License-Identifier: Apache-2.0
 *
 * Trigger implementation for EMC1702. One global handler,
 * one ALERT pin, one trigger event delivered per chip-side alert.
 *
 * Uses the system workqueue.
 */

#include <zephyr/drivers/sensor/emc1702.h>
#include "emc1702.h"

LOG_MODULE_DECLARE(emc1702, CONFIG_SENSOR_LOG_LEVEL);

static void emc1702_work_handler(struct k_work *work)
{
	struct emc1702_data *data = CONTAINER_OF(work, struct emc1702_data, work);
	const struct emc1702_config *cfg = data->dev->config;
	uint8_t hi_status = 0, lo_status = 0, fault_status = 0;
	int rc;

	/* Read & clear the chip's alert status. In interrupt mode this
	 * releases the ALERT pin (assuming the condition has resolved).
	 * If it hasn't, the chip will re-latch on the next conversion
	 * and the level-triggered IRQ will re-fire.
	 */

	rc = emc1702_read8(data->dev, EMC1702_REG_HIGH_LIMIT_STATUS, &hi_status);
	if (rc) {
		LOG_ERR("clear HIGH_LIMIT_STATUS: %d", rc);
	}
	rc = emc1702_read8(data->dev, EMC1702_REG_LOW_LIMIT_STATUS, &lo_status);
	if (rc) {
		LOG_ERR("clear LOW_LIMIT_STATUS: %d", rc);
	}

	/*
	 * Read the chip's fault status. Only meaningful when a diode is wired
	 * to DP/DN: with an open external channel the fault bit is always set,
	 * which would report a fault on every single alert.
	 */
	if (cfg->external_diode) {
		rc = emc1702_read8(data->dev, EMC1702_REG_EXT_DIODE_FAULT, &fault_status);
		if (rc) {
			LOG_ERR("clear EXT_DIODE_FAULT: %d", rc);
		}
	}

	if (hi_status) {
		if (hi_status & EMC1702_REG_HIGH_LIMIT_STATUS_INT) {
			if (data->die_handler) {
				data->die_handler(data->dev, data->die_trigger);
			}
		}
		if ((hi_status & EMC1702_REG_HIGH_LIMIT_STATUS_EXT) && cfg->external_diode) {
			if (data->amb_handler) {
				data->amb_handler(data->dev, data->amb_trigger);
			}
		}
		if (hi_status & EMC1702_REG_HIGH_LIMIT_STATUS_VSENSE) {
			if (data->curr_handler) {
				data->curr_handler(data->dev, data->curr_trigger);
			}
		}
		if (hi_status & EMC1702_REG_HIGH_LIMIT_STATUS_VSOURCE) {
			if (data->volt_handler) {
				data->volt_handler(data->dev, data->volt_trigger);
			}
		}
	}

	if (lo_status) {
		if (lo_status & EMC1702_REG_LOW_LIMIT_STATUS_INT) {
			if (data->die_handler) {
				data->die_handler(data->dev, data->die_trigger);
			}
		}
		if ((lo_status & EMC1702_REG_LOW_LIMIT_STATUS_EXT) && cfg->external_diode) {
			if (data->amb_handler) {
				data->amb_handler(data->dev, data->amb_trigger);
			}
		}
		if (lo_status & EMC1702_REG_LOW_LIMIT_STATUS_VSENSE) {
			if (data->curr_handler) {
				data->curr_handler(data->dev, data->curr_trigger);
			}
		}
		if (lo_status & EMC1702_REG_LOW_LIMIT_STATUS_VSOURCE) {
			if (data->volt_handler) {
				data->volt_handler(data->dev, data->volt_trigger);
			}
		}
	}

	if (fault_status) {
		if (data->fault_handler) {
			data->fault_handler(data->dev, data->fault_trigger);
		}
	}

	/* Re-arm the GPIO interrupt. */
	rc = gpio_pin_interrupt_configure_dt(&cfg->alert_gpio, GPIO_INT_LEVEL_ACTIVE);
	if (rc) {
		LOG_ERR("could not re-arm gpio interrupt: %d", rc);
	}
}

static void emc1702_alert_isr(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	struct emc1702_data *data = CONTAINER_OF(cb, struct emc1702_data, alert_cb);
	const struct emc1702_config *cfg = data->dev->config;

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	/* Disable the interrupt before submitting work. Level-triggered
	 * IRQs would re-fire continuously otherwise.
	 */
	gpio_pin_interrupt_configure_dt(&cfg->alert_gpio, GPIO_INT_DISABLE);
	k_work_submit(&data->work);
}

int emc1702_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	const struct emc1702_config *cfg = dev->config;
	struct emc1702_data *data = dev->data;

	/* Triggers are unavailable when no ALERT pin is wired (see init). */
	if (cfg->alert_gpio.port == NULL) {
		return -ENOTSUP;
	}

	if (!trig || ((trig->type != SENSOR_TRIG_THRESHOLD) &&
		      (trig->type != (enum sensor_trigger_type)SENSOR_TRIG_EMC1702_FAULT))) {
		return -ENOTSUP;
	}

	switch ((int)trig->chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		/* External channel is masked when no diode is wired. */
		if (!cfg->external_diode) {
			return -ENOTSUP;
		}
		data->amb_handler = handler;
		data->amb_trigger = trig;
		break;
	case SENSOR_CHAN_DIE_TEMP:
		data->die_handler = handler;
		data->die_trigger = trig;
		break;
	case SENSOR_CHAN_CURRENT:
		data->curr_handler = handler;
		data->curr_trigger = trig;
		break;
	case SENSOR_CHAN_VOLTAGE:
		data->volt_handler = handler;
		data->volt_trigger = trig;
		break;
	case SENSOR_CHAN_EMC1702_FAULT:
		/* Diode faults can only be reported for an external diode. */
		if (!cfg->external_diode) {
			return -ENOTSUP;
		}
		data->fault_handler = handler;
		data->fault_trigger = trig;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

int emc1702_trigger_init(const struct device *dev)
{
	const struct emc1702_config *cfg = dev->config;
	struct emc1702_data *data = dev->data;
	int rc;

	data->dev = dev;
	data->die_handler = NULL;
	data->die_trigger = NULL;
	data->amb_handler = NULL;
	data->amb_trigger = NULL;
	data->curr_handler = NULL;
	data->curr_trigger = NULL;
	data->volt_handler = NULL;
	data->volt_trigger = NULL;
	data->fault_handler = NULL;
	data->fault_trigger = NULL;

	k_work_init(&data->work, emc1702_work_handler);

	if (cfg->alert_gpio.port == NULL) {
		/*
		 * No ALERT line wired for this instance: the sensor still works
		 * in polled mode but triggers are unavailable.
		 */
		LOG_WRN("%s: alert-gpios not defined, triggers disabled", dev->name);
		return 0;
	}
	if (!gpio_is_ready_dt(&cfg->alert_gpio)) {
		LOG_ERR("%s: alert gpio not ready", dev->name);
		return -ENODEV;
	}
	rc = gpio_pin_configure_dt(&cfg->alert_gpio, GPIO_INPUT);
	if (rc) {
		return rc;
	}

	/*
	 * Initialize the callback before enabling the interrupt. ALERT is
	 * level-triggered, so if it is already asserted at this point an
	 * interrupt enabled without a callback would fire with nothing to
	 * disable it again and the CPU would spin in the ISR forever.
	 */
	gpio_init_callback(&data->alert_cb, emc1702_alert_isr, BIT(cfg->alert_gpio.pin));
	rc = gpio_add_callback(cfg->alert_gpio.port, &data->alert_cb);
	if (rc) {
		return rc;
	}

	rc = gpio_pin_interrupt_configure_dt(&cfg->alert_gpio, GPIO_INT_LEVEL_ACTIVE);
	if (rc) {
		gpio_remove_callback(cfg->alert_gpio.port, &data->alert_cb);
		return rc;
	}

	return 0;
}
