/*
 * Copyright 2024 NXP
 * Copyright (c) 2018 Phytec Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_fxls8974

#include "fxls8974.h"
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(FXLS8974, CONFIG_SENSOR_LOG_LEVEL);

static void fxls8974_gpio_callback(const struct device *dev,
				   struct gpio_callback *cb,
				   uint32_t pin_mask)
{
	struct fxls8974_data *data =
		CONTAINER_OF(cb, struct fxls8974_data, gpio_cb);
	const struct fxls8974_config *config = data->dev->config;

	if ((pin_mask & BIT(config->int_gpio.pin)) == 0U) {
		return;
	}

	gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_DISABLE);

#if defined(CONFIG_FXLS8974_TRIGGER_OWN_THREAD)
	k_sem_give(&data->trig_sem);
#elif defined(CONFIG_FXLS8974_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

static int fxls8974_handle_drdy_int(const struct device *dev)
{
	struct fxls8974_data *data = dev->data;

	if (data->drdy_handler) {
		data->drdy_handler(dev, data->drdy_trig);
	}

	return 0;
}

static void fxls8974_handle_int(const struct device *dev)
{
	const struct fxls8974_config *config = dev->config;

	fxls8974_handle_drdy_int(dev);

	gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_LEVEL_HIGH);
}

#ifdef CONFIG_FXLS8974_TRIGGER_OWN_THREAD
static void fxls8974_thread_main(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct fxls8974_data *data = p1;

	while (true) {
		k_sem_take(&data->trig_sem, K_FOREVER);
		fxls8974_handle_int(data->dev);
	}
}
#endif

#ifdef CONFIG_FXLS8974_TRIGGER_GLOBAL_THREAD
static void fxls8974_work_handler(struct k_work *work)
{
	struct fxls8974_data *data =
		CONTAINER_OF(work, struct fxls8974_data, work);

	fxls8974_handle_int(data->dev);
}
#endif

int fxls8974_trigger_set(const struct device *dev,
			 const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler)
{
	struct fxls8974_data *data = dev->data;
	int ret = 0;

	k_sem_take(&data->sem, K_FOREVER);

	/* Put the sensor in standby mode */
	if (fxls8974_set_active(dev, FXLS8974_ACTIVE_OFF)) {
		LOG_ERR("Could not set standby mode");
		ret = -EIO;
		goto exit;
	}

	if (trig->type == SENSOR_TRIG_DATA_READY) {
		data->drdy_handler = handler;
		data->drdy_trig = trig;
	} else {
		LOG_ERR("Unsupported sensor trigger");
		ret = -ENOTSUP;
		goto exit;
	}


	/* Restore the previous active mode */
	if (fxls8974_set_active(dev, FXLS8974_ACTIVE_ON)) {
		LOG_ERR("Could not restore active mode");
		ret = -EIO;
		goto exit;
	}

exit:
	k_sem_give(&data->sem);

	return ret;
}


int fxls8974_trigger_init(const struct device *dev)
{
	const struct fxls8974_config *config = dev->config;
	struct fxls8974_data *data = dev->data;
	int ret;

	data->dev = dev;

#if defined(CONFIG_FXLS8974_TRIGGER_OWN_THREAD)
	k_sem_init(&data->trig_sem, 0, K_SEM_MAX_LIMIT);
	k_thread_create(&data->thread, data->thread_stack,
			CONFIG_FXLS8974_THREAD_STACK_SIZE,
			fxls8974_thread_main,
			data, NULL, NULL,
			K_PRIO_COOP(CONFIG_FXLS8974_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif defined(CONFIG_FXLS8974_TRIGGER_GLOBAL_THREAD)
	data->work.handler = fxls8974_work_handler;
#endif

	if (config->ops->byte_write(dev, FXLS8974_INTREG_EN,
				    FXLS8974_DRDY_MASK)) {
		LOG_ERR("Could not enable interrupt");
		return -EIO;
	}

#if !(CONFIG_FXLS8974_DRDY_INT1)
	if (config->ops->byte_write(dev, FXLS8974_INT_PIN_SEL_REG,
				    FXLS8974_DRDY_MASK)) {
		LOG_ERR("Could not configure interrupt pin routing");
		return -EIO;
	}
#endif

	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_ERR("GPIO device not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
	if (ret < 0) {
		return ret;
	}

	gpio_init_callback(&data->gpio_cb, fxls8974_gpio_callback,
			   BIT(config->int_gpio.pin));

	ret = gpio_add_callback(config->int_gpio.port, &data->gpio_cb);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		return ret;
	}

	return 0;
}
