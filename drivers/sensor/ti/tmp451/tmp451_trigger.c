/*
 * Copyright (c) 2026 Open Device Partnership and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include "tmp451.h"

LOG_MODULE_DECLARE(TMP451, CONFIG_SENSOR_LOG_LEVEL);

int tmp451_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
	struct tmp451_data *data = dev->data;
	const struct tmp451_config *cfg = dev->config;
	uint8_t status;
	bool any_handler;
	int ret;

	if (!cfg->alert_gpio.port || trig->type != SENSOR_TRIG_THRESHOLD || cfg->therm2_mode) {
		return -ENOTSUP;
	}

	switch (trig->chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		data->trigger_handler_local = handler;
		data->trigger_local = trig;
		break;
	case SENSOR_CHAN_DIE_TEMP:
		data->trigger_handler_remote = handler;
		data->trigger_remote = trig;
		break;
	case SENSOR_CHAN_ALL:
		data->trigger_handler_local = handler;
		data->trigger_local = trig;
		data->trigger_handler_remote = handler;
		data->trigger_remote = trig;
		break;
	default:
		return -ENOTSUP;
	}

	/* Read the status reg just to clear any stale alert flags */
	ret = i2c_reg_read_byte_dt(&cfg->i2c, TMP451_REG_STATUS, &status);
	if (ret < 0) {
		LOG_ERR("Failed to read and clear status register");
		return ret;
	}

	/* If any handler is set, enable ALERT output.
	 * Otherwise, disable ALERT output to avoid unnecessary interrupts.
	 */
	any_handler = data->trigger_handler_local || data->trigger_handler_remote;
	ret = tmp451_reg_update(cfg, TMP451_REG_CONFIG_READ, TMP451_REG_CONFIG_WRITE,
				TMP451_CONFIG_MASK1, any_handler ? 0 : TMP451_CONFIG_MASK1);
	if (ret < 0) {
		LOG_ERR("Failed to update MASK1 (ALERT enable) bit");
	}

	return ret;
}

static void tmp451_handle_interrupt(const struct device *dev)
{
	struct tmp451_data *data = dev->data;
	const struct tmp451_config *cfg = dev->config;
	uint8_t status;

	/* We read the status reg to know which handler to call (also clears the alert flags)
	 * Note: The datasheet has a confusing statement on how the ALERT pin is reset:
	 *
	 * "Reading the status register clears the five flags, provided that the condition that
	 * caused the setting of the flags is not present anymore (that is, the value of the
	 * corresponding result register is within the limits, or the remote sensor is connected
	 * properly and functional).
	 * The ALERT interrupt latch (and the ALERT pin correspondingly) is not reset by reading
	 * the status register. The reset is done by the master reading
	 * the temperature sensor device address to service the interrupt."
	 *
	 * But in practice it seems just reading the status reg is sufficient.
	 * This might only apply to an SMBUS scenario where multiple sensors are sharing an alert
	 * line?
	 */
	if (i2c_reg_read_byte_dt(&cfg->i2c, TMP451_REG_STATUS, &status) < 0) {
		LOG_ERR("Failed to read status register");
	} else {
		if ((status & TMP451_STATUS_LOCAL_ALERT) && data->trigger_handler_local) {
			data->trigger_handler_local(dev, data->trigger_local);
		}

		if ((status & TMP451_STATUS_REMOTE_ALERT) && data->trigger_handler_remote) {
			data->trigger_handler_remote(dev, data->trigger_remote);
		}

		/* Note: It's possible we got here because the OPEN bit is set,
		 * which indicates an open-circuit fault at the remote junction.
		 * However, we choose not to trigger the remote handler because
		 * technically it's not a threshold trigger.
		 *
		 * If OPEN detection interrupt is desired, likely best to add
		 * a custom trigger for it in the future.
		 */
		if (status & TMP451_STATUS_OPEN) {
			LOG_ERR("Remote diode open-circuit fault detected");
		}
	}

	gpio_pin_interrupt_configure_dt(&cfg->alert_gpio, GPIO_INT_EDGE_TO_ACTIVE);
}

static void tmp451_gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct tmp451_data *data = CONTAINER_OF(cb, struct tmp451_data, trigger_cb);
	const struct tmp451_config *cfg = data->dev->config;

	ARG_UNUSED(dev);
	ARG_UNUSED(pins);

	gpio_pin_interrupt_configure_dt(&cfg->alert_gpio, GPIO_INT_DISABLE);

#if defined(CONFIG_TMP451_TRIGGER_OWN_THREAD)
	k_sem_give(&data->gpio_sem);
#elif defined(CONFIG_TMP451_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

#ifdef CONFIG_TMP451_TRIGGER_OWN_THREAD
static void tmp451_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct tmp451_data *data = p1;

	while (1) {
		k_sem_take(&data->gpio_sem, K_FOREVER);
		tmp451_handle_interrupt(data->dev);
	}
}
#endif /* CONFIG_TMP451_TRIGGER_OWN_THREAD */

#ifdef CONFIG_TMP451_TRIGGER_GLOBAL_THREAD
static void tmp451_work_cb(struct k_work *work)
{
	struct tmp451_data *data = CONTAINER_OF(work, struct tmp451_data, work);

	tmp451_handle_interrupt(data->dev);
}
#endif /* CONFIG_TMP451_TRIGGER_GLOBAL_THREAD */

int tmp451_init_interrupt(const struct device *dev)
{
	struct tmp451_data *data = dev->data;
	const struct tmp451_config *cfg = dev->config;
	int ret;

	/* If triggers are enabled, but we have multiple instances and this one doesn't
	 * have gpios attached, just return success but don't proceed with rest of setup.
	 */
	if (!cfg->alert_gpio.port) {
		return 0;
	}

	if (!gpio_is_ready_dt(&cfg->alert_gpio)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->alert_gpio.port);
		return -ENODEV;
	}

	data->dev = dev;

	ret = gpio_pin_configure_dt(&cfg->alert_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure alert-gpios");
		return ret;
	}

	gpio_init_callback(&data->trigger_cb, tmp451_gpio_callback, BIT(cfg->alert_gpio.pin));

	ret = gpio_add_callback(cfg->alert_gpio.port, &data->trigger_cb);
	if (ret < 0) {
		LOG_ERR("Failed to add alert-gpios callback");
		return ret;
	}

#if defined(CONFIG_TMP451_TRIGGER_OWN_THREAD)
	k_sem_init(&data->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&data->thread, data->thread_stack, CONFIG_TMP451_THREAD_STACK_SIZE,
			tmp451_thread, data, NULL, NULL, K_PRIO_COOP(CONFIG_TMP451_THREAD_PRIORITY),
			0, K_NO_WAIT);
	k_thread_name_set(&data->thread, dev->name);
#elif defined(CONFIG_TMP451_TRIGGER_GLOBAL_THREAD)
	data->work.handler = tmp451_work_cb;
#endif

	ret = gpio_pin_interrupt_configure_dt(&cfg->alert_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure alert-gpios interrupt");
	}

	return ret;
}
