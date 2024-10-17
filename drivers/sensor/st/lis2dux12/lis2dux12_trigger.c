/* ST Microelectronics LIS2DUX12 3-axis accelerometer driver
 *
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lis2dux12.pdf
 */

#include <zephyr/logging/log.h>
#include "lis2dux12.h"

LOG_MODULE_DECLARE(LIS2DUX12, CONFIG_SENSOR_LOG_LEVEL);

static void lis2dux12_gpio_callback(const struct device *dev, struct gpio_callback *cb,
				    uint32_t pins)
{
	struct lis2dux12_data *data = CONTAINER_OF(cb, struct lis2dux12_data, gpio_cb);
	int ret;

	ARG_UNUSED(pins);

	ret = gpio_pin_interrupt_configure_dt(data->drdy_gpio, GPIO_INT_DISABLE);
	if (ret < 0) {
		LOG_ERR("%s: Not able to configure pin_int", dev->name);
	}

#if defined(CONFIG_LIS2DUX12_TRIGGER_OWN_THREAD)
	k_sem_give(&data->trig_sem);
#elif defined(CONFIG_LIS2DUX12_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

#ifdef CONFIG_LIS2DUX12_TRIGGER_OWN_THREAD
static void lis2dux12_thread(struct lis2dux12_data *data)
{
	const struct device *dev = data->dev;
	const struct lis2dux12_config *const cfg = dev->config;
	const struct lis2dux12_chip_api *chip_api = cfg->chip_api;

	while (1) {
		k_sem_take(&data->trig_sem, K_FOREVER);
		chip_api->handle_interrupt(dev);
	}
}
#endif

#ifdef CONFIG_LIS2DUX12_TRIGGER_GLOBAL_THREAD
static void lis2dux12_work_cb(struct k_work *work)
{
	struct lis2dux12_data *data = CONTAINER_OF(work, struct lis2dux12_data, work);
	const struct device *dev = data->dev;
	const struct lis2dux12_config *const cfg = dev->config;
	const struct lis2dux12_chip_api *chip_api = cfg->chip_api;

	chip_api->handle_interrupt(dev);
}
#endif

int lis2dux12_trigger_init(const struct device *dev)
{
	struct lis2dux12_data *data = dev->data;
	const struct lis2dux12_config *cfg = dev->config;
	int ret;

	data->drdy_gpio = (cfg->drdy_pin == 1) ? (struct gpio_dt_spec *)&cfg->int1_gpio
							 : (struct gpio_dt_spec *)&cfg->int2_gpio;

	/* setup data ready gpio interrupt (INT1 or INT2) */
	if (!gpio_is_ready_dt(data->drdy_gpio)) {
		LOG_ERR("Cannot get pointer to drdy_gpio device");
		return -ENODEV;
	}

	data->dev = dev;

	ret = gpio_pin_configure_dt(data->drdy_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure gpio");
		return ret;
	}

	gpio_init_callback(&data->gpio_cb, lis2dux12_gpio_callback, BIT(data->drdy_gpio->pin));

	ret = gpio_add_callback(data->drdy_gpio->port, &data->gpio_cb);
	if (ret < 0) {
		LOG_ERR("Could not set gpio callback");
		return ret;
	}

#if defined(CONFIG_LIS2DUX12_TRIGGER_OWN_THREAD)
	k_sem_init(&data->trig_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&data->thread, data->thread_stack, CONFIG_LIS2DUX12_THREAD_STACK_SIZE,
			(k_thread_entry_t)lis2dux12_thread, data, NULL, NULL,
			K_PRIO_COOP(CONFIG_LIS2DUX12_THREAD_PRIORITY), 0, K_NO_WAIT);
	k_thread_name_set(&data->thread, dev->name);
#elif defined(CONFIG_LIS2DUX12_TRIGGER_GLOBAL_THREAD)
	data->work.handler = lis2dux12_work_cb;
#endif

	return gpio_pin_interrupt_configure_dt(data->drdy_gpio, GPIO_INT_EDGE_TO_ACTIVE);
}

int lis2dux12_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler)
{
	struct lis2dux12_data *data = dev->data;
	const struct lis2dux12_config *cfg = dev->config;
	const struct lis2dux12_chip_api *chip_api = cfg->chip_api;
	int ret;

	if (!cfg->trig_enabled) {
		LOG_ERR("trigger_set op not supported");
		return -ENOTSUP;
	}

	if (data->drdy_gpio->port == NULL) {
		LOG_ERR("trigger_set is not supported");
		return -ENOTSUP;
	}

	ret = gpio_pin_interrupt_configure_dt(data->drdy_gpio, GPIO_INT_DISABLE);
	if (ret < 0) {
		LOG_ERR("%s: Not able to configure pin_int", dev->name);
		return ret;
	}

	data->data_ready_handler = handler;
	if (handler == NULL) {
		LOG_WRN("lis2dux12: no handler");
		return 0;
	}

	/* re-trigger lost interrupt */
	chip_api->sample_fetch_accel(dev);

	data->data_ready_trigger = trig;

	ret = chip_api->init_interrupt(dev);
	if (ret < 0) {
		LOG_ERR("%s: Not able to initialize device interrupt", dev->name);
		return ret;
	}

	return gpio_pin_interrupt_configure_dt(data->drdy_gpio, GPIO_INT_EDGE_TO_ACTIVE);
}
