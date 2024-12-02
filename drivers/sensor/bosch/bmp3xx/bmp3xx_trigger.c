/* Bosch BMP388, BMP390 pressure sensor
 *
 * Copyright (c) 2020 Facebook, Inc. and its affiliates
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp388-ds001.pdf
 */

#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/logging/log.h>

#include "bmp3xx.h"

LOG_MODULE_DECLARE(BMP3XX, CONFIG_SENSOR_LOG_LEVEL);

static void bmp3xx_handle_interrupts(const void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct bmp3xx_data *data = dev->data;

	if (data->handler_drdy) {
		data->handler_drdy(dev, data->trig_drdy);
	}
}

#ifdef CONFIG_BMP3XX_TRIGGER_OWN_THREAD
static K_THREAD_STACK_DEFINE(bmp3xx_thread_stack, CONFIG_BMP3XX_THREAD_STACK_SIZE);
static struct k_thread bmp3xx_thread;

static void bmp3xx_thread_main(void *arg1, void *unused1, void *unused2)
{
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	const struct device *dev = (const struct device *)arg1;
	struct bmp3xx_data *data = dev->data;

	while (1) {
		k_sem_take(&data->sem, K_FOREVER);
		bmp3xx_handle_interrupts(dev);
	}
}
#endif

#ifdef CONFIG_BMP3XX_TRIGGER_GLOBAL_THREAD
static void bmp3xx_work_handler(struct k_work *work)
{
	struct bmp3xx_data *data = CONTAINER_OF(work, struct bmp3xx_data, work);

	bmp3xx_handle_interrupts(data->dev);
}
#endif

static void bmp3xx_gpio_callback(const struct device *port, struct gpio_callback *cb, uint32_t pin)
{
	struct bmp3xx_data *data = CONTAINER_OF(cb, struct bmp3xx_data, gpio_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pin);

#if defined(CONFIG_BMP3XX_TRIGGER_OWN_THREAD)
	k_sem_give(&data->sem);
#elif defined(CONFIG_BMP3XX_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#elif defined(CONFIG_BMP3XX_TRIGGER_DIRECT)
	bmp3xx_handle_interrupts(data->dev);
#endif
}

int bmp3xx_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
	struct bmp3xx_data *data = dev->data;

#ifdef CONFIG_PM_DEVICE
	enum pm_device_state state;

	(void)pm_device_state_get(dev, &state);
	if (state != PM_DEVICE_STATE_ACTIVE) {
		return -EBUSY;
	}
#endif

	if (trig->type != SENSOR_TRIG_DATA_READY) {
		return -ENOTSUP;
	}

	if (bmp3xx_reg_field_update(dev, BMP3XX_REG_INT_CTRL, BMP3XX_INT_CTRL_DRDY_EN_MASK,
				    (handler != NULL) << BMP3XX_INT_CTRL_DRDY_EN_POS) < 0) {
		LOG_ERR("Failed to enable DRDY interrupt");
		return -EIO;
	}

	data->handler_drdy = handler;
	data->trig_drdy = trig;

	return 0;
}

int bmp3xx_trigger_mode_init(const struct device *dev)
{
	struct bmp3xx_data *data = dev->data;
	const struct bmp3xx_config *cfg = dev->config;
	int ret;

	if (!gpio_is_ready_dt(&cfg->gpio_int)) {
		LOG_ERR("INT device is not ready");
		return -ENODEV;
	}

#if defined(CONFIG_BMP3XX_TRIGGER_OWN_THREAD)
	k_sem_init(&data->sem, 0, 1);
	k_thread_create(&bmp3xx_thread, bmp3xx_thread_stack, CONFIG_BMP3XX_THREAD_STACK_SIZE,
			bmp3xx_thread_main, (void *)dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_BMP3XX_THREAD_PRIORITY), 0, K_NO_WAIT);
#elif defined(CONFIG_BMP3XX_TRIGGER_GLOBAL_THREAD)
	data->work.handler = bmp3xx_work_handler;
#endif

#if defined(CONFIG_BMP3XX_TRIGGER_GLOBAL_THREAD) || defined(CONFIG_BMP3XX_TRIGGER_DIRECT)
	data->dev = dev;
#endif

	ret = gpio_pin_configure(cfg->gpio_int.port, cfg->gpio_int.pin,
				 GPIO_INPUT | cfg->gpio_int.dt_flags);
	if (ret < 0) {
		return ret;
	}

	gpio_init_callback(&data->gpio_cb, bmp3xx_gpio_callback, BIT(cfg->gpio_int.pin));

	ret = gpio_add_callback(cfg->gpio_int.port, &data->gpio_cb);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure(cfg->gpio_int.port, cfg->gpio_int.pin,
					   GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		return ret;
	}

	return 0;
}
