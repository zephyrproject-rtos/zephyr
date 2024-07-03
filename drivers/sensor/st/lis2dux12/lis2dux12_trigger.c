/* ST Microelectronics LIS2DUX12 3-axis accelerometer driver
 *
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lis2dux12.pdf
 */

#define DT_DRV_COMPAT st_lis2dux12

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

static void lis2dux12_handle_drdy_int(const struct device *dev)
{
	struct lis2dux12_data *data = dev->data;

	if (data->data_ready_handler != NULL) {
		data->data_ready_handler(dev, data->data_ready_trigger);
	}
}

static void lis2dux12_handle_int(const struct device *dev)
{
	struct lis2dux12_data *lis2dux12 = dev->data;
	const struct lis2dux12_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lis2dux12_all_sources_t sources;
	int ret;

	lis2dux12_all_sources_get(ctx, &sources);

	if (sources.drdy) {
		lis2dux12_handle_drdy_int(dev);
	}

	ret = gpio_pin_interrupt_configure_dt(lis2dux12->drdy_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("%s: Not able to configure pin_int", dev->name);
	}
}

#ifdef CONFIG_LIS2DUX12_TRIGGER_OWN_THREAD
static void lis2dux12_thread(struct lis2dux12_data *data)
{
	while (1) {
		k_sem_take(&data->trig_sem, K_FOREVER);
		lis2dux12_handle_int(data->dev);
	}
}
#endif

#ifdef CONFIG_LIS2DUX12_TRIGGER_GLOBAL_THREAD
static void lis2dux12_work_cb(struct k_work *work)
{
	struct lis2dux12_data *data = CONTAINER_OF(work, struct lis2dux12_data, work);

	lis2dux12_handle_int(data->dev);
}
#endif

static int lis2dux12_init_interrupt(const struct device *dev)
{
	const struct lis2dux12_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lis2dux12_pin_int_route_t route;
	int err;

	/* Enable pulsed mode */
	err = lis2dux12_data_ready_mode_set(ctx, LIS2DUX12_DRDY_PULSED);
	if (err < 0) {
		return err;
	}

	/* route data-ready interrupt on int1 */
	err = lis2dux12_pin_int1_route_get(ctx, &route);
	if (err < 0) {
		return err;
	}

	route.drdy = 1;

	err = lis2dux12_pin_int1_route_set(ctx, &route);
	if (err < 0) {
		return err;
	}

	return 0;
}

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
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lis2dux12_xl_data_t xldata;
	lis2dux12_md_t mode = {.fs = cfg->range};
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
	lis2dux12_xl_data_get(ctx, &mode, &xldata);

	data->data_ready_trigger = trig;

	lis2dux12_init_interrupt(dev);
	return gpio_pin_interrupt_configure_dt(data->drdy_gpio, GPIO_INT_EDGE_TO_ACTIVE);
}
