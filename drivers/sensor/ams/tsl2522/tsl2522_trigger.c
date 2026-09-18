/*
 * Copyright (c) 2026 Carl Zeiss Meditec AG
 * SPDX-License-Identifier: Apache-2.0
 */

#include "tsl2522.h"
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(tsl2522, CONFIG_SENSOR_LOG_LEVEL);

static int setup_gpio_int(const struct device *dev, bool enable)
{
	const struct tsl2522_dts_config *cfg = dev->config;
	gpio_flags_t flags = enable ? GPIO_INT_EDGE_TO_ACTIVE : GPIO_INT_DISABLE;

	int rc = gpio_pin_interrupt_configure_dt(&cfg->int_gpio, flags);

	if (rc) {
		LOG_ERR("Could not configure gpio interrupt: %d", rc);
	}

	return rc;
}

static void handle_gpio_int(const struct device *dev)
{
	struct tsl2522_data *data = dev->data;

	setup_gpio_int(dev, false);

#if defined(CONFIG_TSL2522_TRIGGER_OWN_THREAD)
	k_sem_give(&data->trig_sem);
#elif defined(CONFIG_TSL2522_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

static void gpio_callback_handler(const struct device *dev, struct gpio_callback *cb,
				  uint32_t pin_mask)
{
	struct tsl2522_data *data = CONTAINER_OF(cb, struct tsl2522_data, gpio_cb);

	handle_gpio_int(data->dev);
}

static void process_int(const struct device *dev)
{
	const struct tsl2522_dts_config *cfg = dev->config;
	struct tsl2522_data *data = dev->data;
	uint8_t status = 0U;
	uint8_t status2 = 0U;
	struct {
		uint8_t status4;
		uint8_t status5;
	} status4_5 = {0U};
	const struct sensor_trigger *data_ready = NULL;
	sensor_trigger_handler_t data_ready_handler = NULL;
	const struct sensor_trigger *overflow = NULL;
	sensor_trigger_handler_t overflow_handler = NULL;
	bool handlers_registered = false;
	int rc = 0;

	BUILD_ASSERT(sizeof(status4_5) == 2);

	k_mutex_lock(&data->mutex, K_FOREVER);

	rc = i2c_reg_read_byte_dt(&cfg->i2c, TSL2522_REG_STATUS, &status);
	if (rc) {
		LOG_ERR("Could not read status register (%#x), errno: %d", TSL2522_REG_STATUS, rc);
		goto unlock;
	}

	if (FIELD_GET(TSL2522_STATUS_MINT, status)) { /* modulator interrupt */
		/* Save saturation information, they will be discarded when interrupts are cleared
		 * by writing to the status register (either further down in this function or in
		 * clear_modulator_saturation() in tsl2522.c).
		 */
		rc = i2c_reg_read_byte_dt(&cfg->i2c, TSL2522_REG_STATUS2, &status2);
		if (rc) {
			LOG_ERR("Could not read status2 register (%#x), errno: %d",
				TSL2522_REG_STATUS2, rc);
			/* No goto here, we continue without the status2 information. */
		}

		data->saved_status2 |=
			status2; /* Used in sample fetch. Keep previously fetch status. */
		overflow = data->als_overflow;
		overflow_handler = data->als_overflow_handler;
	}

	if (FIELD_GET(TSL2522_STATUS_SINT, status)) {
		rc = i2c_burst_read_dt(&cfg->i2c, TSL2522_REG_STATUS4, (uint8_t *)&status4_5,
				       sizeof(status4_5));
		if (rc) {
			LOG_ERR("Could not read status4/5 register, errno: %d", rc);
			/* No goto here, we continue without the status4/5 information. */
		}

		if (status4_5.status5 != 0) {
			/* Clear interrupt. */
			rc = i2c_reg_write_byte_dt(&cfg->i2c, TSL2522_REG_STATUS5,
						   status4_5.status5);
			if (rc) {
				LOG_ERR("Could not write status5 register (%#x), errno: %d",
					TSL2522_REG_STATUS5, rc);
				/* No goto here, we only log the failure. */
			}
		}

		data_ready = data->als_data_ready;
		data_ready_handler = data->als_data_ready_handler;
	}

	if (status != 0) {
		/*
		 * Clear the bits AFTER handling the interrupts, otherwise the information in the
		 * modulator status registers may be discarded.
		 */
		rc = i2c_reg_write_byte_dt(&cfg->i2c, TSL2522_REG_STATUS, status);
		if (rc) {
			LOG_ERR("Could not write status register (%#x), errno: %d",
				TSL2522_REG_STATUS, rc);
		}
	}

unlock:
	k_mutex_unlock(&data->mutex);

	/* Call the registered handlers after unlocking the mutex. */
	if (overflow_handler != NULL && (FIELD_GET(TSL2522_STATUS2_ALS_DIG_SAT, status2) ||
					 FIELD_GET(TSL2522_STATUS2_MOD_ANA_SAT1, status2) ||
					 FIELD_GET(TSL2522_STATUS2_MOD_ANA_SAT0, status2))) {
		overflow_handler(dev, overflow);
	}

	if (data_ready_handler != NULL &&
	    !FIELD_GET(TSL2522_STATUS4_MOD_SAMPLE_TRIGGER_ERROR, status4_5.status4) &&
	    FIELD_GET(TSL2522_STATUS5_SINT_MEASUREMENT_SEQUENCER, status4_5.status5)) {
		data_ready_handler(dev, data_ready);
	}

	k_mutex_lock(&data->mutex, K_FOREVER);
	handlers_registered = data->als_data_ready_handler || data->als_overflow_handler;
	k_mutex_unlock(&data->mutex);

	/* In case a handler has been removed in a callback, get the current state of
	 * handlers.
	 */
	if (handlers_registered) {
		setup_gpio_int(dev, true);

		/* Check for pin that may be asserted while we were busy */
		int pv = gpio_pin_get_dt(&cfg->int_gpio);

		if (pv > 0) {
			handle_gpio_int(dev);
		}
	}
}

#ifdef CONFIG_TSL2522_TRIGGER_OWN_THREAD
static void thread_main(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct tsl2522_data *data = p1;

	while (true) {
		k_sem_take(&data->trig_sem, K_FOREVER);
		process_int(data->dev);
	}
}
#endif

#ifdef CONFIG_TSL2522_TRIGGER_GLOBAL_THREAD
static void work_handler(struct k_work *work)
{
	struct tsl2522_data *data = CONTAINER_OF(work, struct tsl2522_data, work);

	process_int(data->dev);
}
#endif

int tsl2522_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	const struct tsl2522_dts_config *cfg = dev->config;
	struct tsl2522_data *data = dev->data;
	bool enable_interrupt = false;
	int rc;

	if (cfg->int_gpio.port == NULL) {
		LOG_ERR("Interrupt GPIO not configured for %s", dev->name);
		return -ENOTSUP;
	}

	if (trig->type != SENSOR_TRIG_DATA_READY && trig->type != SENSOR_TRIG_OVERFLOW) {
		LOG_ERR("Unsupported sensor trigger type: %d", trig->type);
		return -ENOTSUP;
	}

	if (trig->chan != SENSOR_CHAN_ALL && trig->chan != SENSOR_CHAN_AMBIENT_LIGHT &&
	    trig->chan != SENSOR_CHAN_LIGHT && trig->chan != SENSOR_CHAN_IR) {
		LOG_ERR("Unsupported sensor trigger channel: %d", trig->chan);
		return -ENOTSUP;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);

	/* Disable gpio interrupt. */
	rc = setup_gpio_int(dev, false);
	if (rc != 0) {
		LOG_ERR("Could not disable gpio interrupt: %d", rc);
		goto unlock;
	}

	if (trig->type == SENSOR_TRIG_DATA_READY) {
		if (handler != NULL) {
			/* Clear measurement sequencer system interrupt bit. */
			(void)i2c_reg_write_byte_dt(&cfg->i2c, TSL2522_REG_STATUS5,
						    TSL2522_STATUS5_SINT_MEASUREMENT_SEQUENCER);

			/* Clear system interrupt bit before enabling the interrupt. */
			(void)i2c_reg_write_byte_dt(&cfg->i2c, TSL2522_REG_STATUS,
						    TSL2522_STATUS_SINT);

			rc = i2c_reg_update_byte_dt(&cfg->i2c, TSL2522_REG_CFG4,
						    TSL2522_CFG4_MEAS_SEQ_SINT_PER_STEP,
						    TSL2522_CFG4_MEAS_SEQ_SINT_PER_STEP);
			if (rc) {
				LOG_ERR("%#x: I/O error: %d", TSL2522_REG_CFG4, rc);
				goto unlock;
			}

			rc = i2c_reg_update_byte_dt(&cfg->i2c, TSL2522_REG_SIEN,
						    TSL2522_SIEN_MEASUREMENT_SEQUENCER,
						    TSL2522_SIEN_MEASUREMENT_SEQUENCER);
			if (rc) {
				LOG_ERR("%#x: I/O error: %d", TSL2522_REG_SIEN, rc);
				goto unlock;
			}

			/* Enable interrupt. */
			rc = i2c_reg_update_byte_dt(&cfg->i2c, TSL2522_REG_INTENAB,
						    TSL2522_INTENAB_SIEN, TSL2522_INTENAB_SIEN);
		} else {
			/* Disable interrupt. */
			rc = i2c_reg_update_byte_dt(&cfg->i2c, TSL2522_REG_INTENAB,
						    TSL2522_INTENAB_SIEN, 0);
		}
		if (rc) {
			LOG_ERR("%#x: I/O error: %d", TSL2522_REG_INTENAB, rc);
			goto unlock;
		}

		data->als_data_ready_handler = handler;
		data->als_data_ready = trig;
	} else {
		if (handler != NULL) {
			/* Enable interrupt. */
			rc = i2c_reg_update_byte_dt(&cfg->i2c, TSL2522_REG_INTENAB,
						    TSL2522_INTENAB_MIEN, TSL2522_INTENAB_MIEN);
		} else {
			/* Disable interrupt. */
			rc = i2c_reg_update_byte_dt(&cfg->i2c, TSL2522_REG_INTENAB,
						    TSL2522_INTENAB_MIEN, 0);
		}
		if (rc) {
			LOG_ERR("%#x: I/O error: %d", TSL2522_REG_INTENAB, rc);
			goto unlock;
		}

		data->als_overflow_handler = handler;
		data->als_overflow = trig;
	}

unlock:
	enable_interrupt =
		data->als_overflow_handler != NULL || data->als_data_ready_handler != NULL;

	k_mutex_unlock(&data->mutex);

	int rc_int = setup_gpio_int(dev, enable_interrupt);

	if (rc == 0) {
		rc = rc_int;
	}

	if (enable_interrupt) {
		/* Check whether already asserted */
		int pv = gpio_pin_get_dt(&cfg->int_gpio);

		if (pv > 0) {
			handle_gpio_int(dev);
		}
	}

	return rc;
}

int tsl2522_trigger_init(const struct device *dev)
{
	const struct tsl2522_dts_config *cfg = dev->config;
	struct tsl2522_data *data = dev->data;
	int rc;

	data->dev = dev;

	/* Configure gpio interrupt if it a gpio has been assigned. */
	if (cfg->int_gpio.port != NULL) {
		/* Get the GPIO device */
		if (!gpio_is_ready_dt(&cfg->int_gpio)) {
			LOG_ERR_DEVICE_NOT_READY(cfg->int_gpio.port);
			return -ENODEV;
		}

		rc = gpio_pin_configure_dt(&cfg->int_gpio, GPIO_INPUT);
		if (rc < 0) {
			LOG_ERR("Could not configure gpio interrupt: %d", rc);
			return rc;
		}

		gpio_init_callback(&data->gpio_cb, gpio_callback_handler, BIT(cfg->int_gpio.pin));

		if (gpio_add_callback(cfg->int_gpio.port, &data->gpio_cb) < 0) {
			LOG_ERR("Failed to set gpio callback!");
			return -EIO;
		}
	}

#if defined(CONFIG_TSL2522_TRIGGER_OWN_THREAD)
	k_sem_init(&data->trig_sem, 0, K_SEM_MAX_LIMIT);
	k_thread_create(&data->thread, data->thread_stack, CONFIG_TSL2522_THREAD_STACK_SIZE,
			thread_main, data, NULL, NULL, K_PRIO_COOP(CONFIG_TSL2522_THREAD_PRIORITY),
			0, K_NO_WAIT);
	k_thread_name_set(&data->thread, "TSL2522 trigger");
#elif defined(CONFIG_TSL2522_TRIGGER_GLOBAL_THREAD)
	k_work_init(&data->work, work_handler);
#endif

	return 0;
}
