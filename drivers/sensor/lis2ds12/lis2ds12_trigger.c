/* ST Microelectronics LIS2DS12 3-axis accelerometer driver
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lis2ds12.pdf
 */

#define DT_DRV_COMPAT st_lis2ds12

#include <zephyr/logging/log.h>
#include "lis2ds12.h"

LOG_MODULE_DECLARE(LIS2DS12, CONFIG_SENSOR_LOG_LEVEL);

static void lis2ds12_gpio_callback(const struct device *dev,
				   struct gpio_callback *cb, uint32_t pins)
{
	struct lis2ds12_data *data =
		CONTAINER_OF(cb, struct lis2ds12_data, gpio_cb);
	const struct lis2ds12_config *cfg = data->dev->config;
	int ret;

	ARG_UNUSED(pins);

	ret = gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_DISABLE);
	if (ret < 0) {
		LOG_ERR("%s: Not able to configure pin_int", dev->name);
	}

#if defined(CONFIG_LIS2DS12_TRIGGER_OWN_THREAD)
	k_sem_give(&data->trig_sem);
#elif defined(CONFIG_LIS2DS12_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

static void lis2ds12_handle_drdy_int(const struct device *dev)
{
	struct lis2ds12_data *data = dev->data;

	if (data->data_ready_handler != NULL) {
		data->data_ready_handler(dev, data->data_ready_trigger);
	}
}

static void lis2ds12_handle_int(const struct device *dev)
{
	const struct lis2ds12_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lis2ds12_all_sources_t sources;
	int ret;

	lis2ds12_all_sources_get(ctx, &sources);

	if (sources.status_dup.drdy) {
		lis2ds12_handle_drdy_int(dev);
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->gpio_int,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("%s: Not able to configure pin_int", dev->name);
	}
}

#ifdef CONFIG_LIS2DS12_TRIGGER_OWN_THREAD
static void lis2ds12_thread(struct lis2ds12_data *data)
{
	while (1) {
		k_sem_take(&data->trig_sem, K_FOREVER);
		lis2ds12_handle_int(data->dev);
	}
}
#endif

#ifdef CONFIG_LIS2DS12_TRIGGER_GLOBAL_THREAD
static void lis2ds12_work_cb(struct k_work *work)
{
	struct lis2ds12_data *data =
		CONTAINER_OF(work, struct lis2ds12_data, work);

	lis2ds12_handle_int(data->dev);
}
#endif

static int lis2ds12_init_interrupt(const struct device *dev)
{
	const struct lis2ds12_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lis2ds12_pin_int1_route_t route;
	int err;

	/* Enable pulsed mode */
	err = lis2ds12_int_notification_set(ctx, LIS2DS12_INT_PULSED);
	if (err < 0) {
		return err;
	}

	/* route data-ready interrupt on int1 */
	err = lis2ds12_pin_int1_route_get(ctx, &route);
	if (err < 0) {
		return err;
	}

	route.int1_drdy = 1;

	err = lis2ds12_pin_int1_route_set(ctx, route);
	if (err < 0) {
		return err;
	}

	return 0;
}

int lis2ds12_trigger_init(const struct device *dev)
{
	struct lis2ds12_data *data = dev->data;
	const struct lis2ds12_config *cfg = dev->config;
	int ret;

	/* setup data ready gpio interrupt (INT1 or INT2) */
	if (!device_is_ready(cfg->gpio_int.port)) {
		if (cfg->gpio_int.port) {
			LOG_ERR("%s: device %s is not ready", dev->name,
						cfg->gpio_int.port->name);
			return -ENODEV;
		}

		LOG_DBG("%s: gpio_int not defined in DT", dev->name);
		return 0;
	}

	data->dev = dev;

	ret = gpio_pin_configure_dt(&cfg->gpio_int, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure gpio");
		return ret;
	}

	LOG_INF("%s: int on %s.%02u", dev->name, cfg->gpio_int.port->name,
				      cfg->gpio_int.pin);

	gpio_init_callback(&data->gpio_cb,
			   lis2ds12_gpio_callback,
			   BIT(cfg->gpio_int.pin));

	ret = gpio_add_callback(cfg->gpio_int.port, &data->gpio_cb);
	if (ret < 0) {
		LOG_ERR("Could not set gpio callback");
		return ret;
	}

#if defined(CONFIG_LIS2DS12_TRIGGER_OWN_THREAD)
	k_sem_init(&data->trig_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&data->thread, data->thread_stack,
			CONFIG_LIS2DS12_THREAD_STACK_SIZE,
			(k_thread_entry_t)lis2ds12_thread,
			data, NULL, NULL,
			K_PRIO_COOP(CONFIG_LIS2DS12_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif defined(CONFIG_LIS2DS12_TRIGGER_GLOBAL_THREAD)
	data->work.handler = lis2ds12_work_cb;
#endif

	return gpio_pin_interrupt_configure_dt(&cfg->gpio_int,
					       GPIO_INT_EDGE_TO_ACTIVE);
}

int lis2ds12_trigger_set(const struct device *dev,
			 const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler)
{
	struct lis2ds12_data *data = dev->data;
	const struct lis2ds12_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	int16_t raw[3];
	int ret;

	__ASSERT_NO_MSG(trig->type == SENSOR_TRIG_DATA_READY);

	if (cfg->gpio_int.port == NULL) {
		LOG_ERR("trigger_set is not supported");
		return -ENOTSUP;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_DISABLE);
	if (ret < 0) {
		LOG_ERR("%s: Not able to configure pin_int", dev->name);
		return ret;
	}

	data->data_ready_handler = handler;
	if (handler == NULL) {
		LOG_WRN("lis2ds12: no handler");
		return 0;
	}

	/* re-trigger lost interrupt */
	lis2ds12_acceleration_raw_get(ctx, raw);

	data->data_ready_trigger = trig;

	lis2ds12_init_interrupt(dev);
	return gpio_pin_interrupt_configure_dt(&cfg->gpio_int,
					       GPIO_INT_EDGE_TO_ACTIVE);
}
