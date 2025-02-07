/*
 * Copyright (c) 2025 Paul Timke <ptimkec@live.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT pixart_paj7620

#include "paj7620.h"
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

LOG_MODULE_REGISTER(paj7620, CONFIG_SENSOR_LOG_LEVEL);

static void paj7620_gpio_callback(const struct device *dev,
				struct gpio_callback *cb,
				uint32_t pin_mask)
{
	struct paj7620_data *data =
		CONTAINER_OF(cb, struct paj7620_data, gpio_cb);
	const struct paj7620_config *config = data->dev->config;

	if ((pin_mask & BIT(config->int_gpio.pin)) == 0U) {
		return;
	}

#ifdef CONFIG_PAJ7620_TRIGGER_OWN_THREAD
	k_sem_give(&data->trig_sem);
#elif CONFIG_PAJ7620_TRIGGER_GLOBAL_THREAD
	k_work_submit(&data->work);
#endif
}

static void paj7620_handle_int(const struct device *dev)
{
	struct paj7620_data *data = dev->data;

	if (data->motion_handler) {
		data->motion_handler(dev, data->motion_trig);
	}
}

#ifdef CONFIG_PAJ7620_TRIGGER_OWN_THREAD
static void paj7620_thread_main(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);

	struct paj7620_data *data = p1;

	while (1) {
		k_sem_take(&data->trig_sem, K_FOREVER);
		paj7620_handle_int(data->dev);
	}
}
#endif

#ifdef CONFIG_PAJ7620_TRIGGER_GLOBAL_THREAD
static void paj7620_work_handler(struct k_work *work)
{
	struct paj7620_data *data =
		CONTAINER_OF(work, struct paj7620_data, work);

	paj7620_handle_int(data->dev);
}
#endif

int paj7620_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	struct paj7620_data *data = dev->data;
	const struct paj7620_config *cfg = dev->config;

	if (cfg->int_gpio.port == NULL) {
		return -ENOTSUP;
	}

	if (trig->type != SENSOR_TRIG_MOTION) {
		LOG_ERR("Unsupported sensor trigger");
		return -ENOTSUP;
	}

	data->motion_handler = handler;
	data->motion_trig = trig;

	if (handler == NULL) {
		return gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_DISABLE);
	}

	return gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_EDGE_FALLING);
}

int paj7620_trigger_init(const struct device *dev)
{
	int ret;
	const struct paj7620_config *config = dev->config;
	struct paj7620_data *data = dev->data;

	data->dev = dev;

#ifdef CONFIG_PAJ7620_TRIGGER_OWN_THREAD
	k_sem_init(&data->trig_sem, 0, K_SEM_MAX_LIMIT);
	k_thread_create(&data->thread,
			data->thread_stack,
			K_KERNEL_STACK_SIZEOF(data->thread_stack),
			paj7620_thread_main,
			data,
			NULL,
			NULL,
			K_PRIO_COOP(CONFIG_PAJ7620_THREAD_PRIORITY),
			0,
			K_NO_WAIT);
#elif CONFIG_PAJ7620_TRIGGER_GLOBAL_THREAD
	data->work.handler = paj7620_work_handler;
#endif

	/* Configure GPIO */
	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_ERR("GPIO device not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
	if (ret < 0) {
		return ret;
	}

	gpio_init_callback(&data->gpio_cb, paj7620_gpio_callback, BIT(config->int_gpio.pin));

	ret = gpio_add_callback(config->int_gpio.port, &data->gpio_cb);
	if (ret < 0) {
		return ret;
	}

	return 0;
}
