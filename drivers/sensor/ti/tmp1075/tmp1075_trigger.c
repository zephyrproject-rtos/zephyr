/*
 * Copyright (c) 2024 Arrow Electronics.
 * Copyright (c) 2026 Antmicro <antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

#include "tmp1075.h"

LOG_MODULE_DECLARE(TMP1075, CONFIG_SENSOR_LOG_LEVEL);

void tmp1075_gpio_callback(const struct device *dev, struct gpio_callback *cb,
			   gpio_port_pins_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pins);

	struct tmp1075_data *data = CONTAINER_OF(cb, struct tmp1075_data, temp_alert_gpio_cb);

#if defined(CONFIG_TMP1075_TRIGGER_OWN_THREAD)
	k_sem_give(&data->trigger_sem);
#elif defined(CONFIG_TMP1075_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif /* CONFIG_TMP1075_TRIGGER_OWN_THREAD */
}

static void tmp1075_trigger_handle_alert(const struct tmp1075_data *drv_data)
{
	uint16_t conf_reg;
	int res;
	const struct device *dev = drv_data->tmp1075_dev;

	if (drv_data->temp_alert_handler) {
		drv_data->temp_alert_handler(drv_data->tmp1075_dev, drv_data->temp_alert_trigger);
	}

	/* 7.4.4.2 - Register read clears interrupts */
	res = tmp1075_reg_read(dev->config, TMP1075_REG_CONFIG, &conf_reg);
	if (res < 0) {
		LOG_ERR("Failed to read configuration register");
	}
}

#if defined(CONFIG_TMP1075_TRIGGER_OWN_THREAD)
static void tmp1075_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct tmp1075_data *data = p1;

	while (1) {
		k_sem_take(&data->trigger_sem, K_FOREVER);
		tmp1075_trigger_handle_alert(data);
	}
}
#elif defined(CONFIG_TMP1075_TRIGGER_GLOBAL_THREAD)
static void tmp1075_work_callback(struct k_work *work)
{
	struct tmp1075_data *data = CONTAINER_OF(work, struct tmp1075_data, work);

	tmp1075_trigger_handle_alert(data);
}
#endif /* CONFIG_TMP1075_TRIGGER_GLOBAL_THREAD */

int tmp1075_setup_trigger(const struct device *dev)
{
	struct tmp1075_data *drv_data = dev->data;
	const struct tmp1075_config *config = dev->config;
	const struct gpio_dt_spec *alert_gpio = &config->alert_gpio;
	int result;

	if (!device_is_ready(alert_gpio->port)) {
		LOG_ERR_DEVICE_NOT_READY(alert_gpio->port);
		return -ENODEV;
	}

	result = gpio_pin_configure_dt(alert_gpio, GPIO_INPUT);

	if (result < 0) {
		return result;
	}

	gpio_init_callback(&drv_data->temp_alert_gpio_cb, tmp1075_gpio_callback,
			   BIT(alert_gpio->pin));

	result = gpio_add_callback(alert_gpio->port, &drv_data->temp_alert_gpio_cb);

	if (result < 0) {
		return result;
	}

#if defined(CONFIG_TMP1075_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->trigger_sem, 0, K_SEM_MAX_LIMIT);
#elif defined(CONFIG_TMP1075_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = tmp1075_work_callback;
#endif
	result = gpio_pin_interrupt_configure_dt(alert_gpio, GPIO_INT_EDGE_BOTH);

	if (result < 0) {
		return result;
	}

#if defined(CONFIG_TMP1075_TRIGGER_OWN_THREAD)
	k_thread_create(&drv_data->trigger_thread, drv_data->trigger_thread_stack,
			CONFIG_TMP1075_THREAD_STACK_SIZE, tmp1075_thread, drv_data, NULL, NULL,
			K_PRIO_COOP(CONFIG_TMP1075_THREAD_PRIORITY), 0, K_NO_WAIT);
	k_thread_name_set(&drv_data->trigger_thread, dev->name);
#endif
	return 0;
}

/*
 * @brief callback implementation for setting the custom trigger handler in the userspace
 * @param dev - sensor device struct pointer
 * @param trig - trigger struct pointer to be set up
 * @param handler - pointer to custom callback handler which the user would like to use
 * @return 0 if ok - -ENOTSUP in case of err
 */
int tmp1075_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	struct tmp1075_data *drv_data = dev->data;

	if (trig->type == SENSOR_TRIG_THRESHOLD) {
		drv_data->temp_alert_handler = handler;
		drv_data->temp_alert_trigger = trig;
		return 0;
	}

	return -ENOTSUP;
}
