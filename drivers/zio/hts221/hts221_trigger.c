/* ST Microelectronics HTS221 humidity sensor
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/hts221.pdf
 */

#include <kernel.h>
#include <sensor.h>
#include <gpio.h>
#include <logging/log.h>

#include <drivers/zio/hts221.h>

LOG_MODULE_DECLARE(HTS221, CONFIG_ZIO_LOG_LEVEL);

/**
 * hts221_handle_interrupt - handle the drdy event
 * read data and call handler if registered any
 */
static void hts221_handle_interrupt(void *arg)
{
	struct device *dev = arg;
	struct hts221_data *hts221 = dev->driver_data;
	const struct hts221_config *cfg = dev->config->config_info;

	zio_dev_trigger(dev);
	gpio_pin_enable_callback(hts221->gpio, cfg->drdy_pin);
}

static void hts221_gpio_callback(struct device *dev,
				  struct gpio_callback *cb, u32_t pins)
{
	const struct hts221_config *cfg = dev->config->config_info;
	struct hts221_data *hts221 =
		CONTAINER_OF(cb, struct hts221_data, gpio_cb);

	ARG_UNUSED(pins);

	gpio_pin_disable_callback(dev, cfg->drdy_pin);

#if defined(CONFIG_HTS221_TRIGGER_OWN_THREAD)
	k_sem_give(&hts221->gpio_sem);
#elif defined(CONFIG_HTS221_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&hts221->work);
#endif /* CONFIG_HTS221_TRIGGER_OWN_THREAD */
}

#ifdef CONFIG_HTS221_TRIGGER_OWN_THREAD
static void hts221_thread(int dev_ptr, int unused)
{
	struct device *dev = INT_TO_POINTER(dev_ptr);
	struct hts221_data *hts221 = dev->driver_data;

	ARG_UNUSED(unused);

	while (1) {
		k_sem_take(&hts221->gpio_sem, K_FOREVER);
		hts221_handle_interrupt(dev);
	}
}
#endif /* CONFIG_HTS221_TRIGGER_OWN_THREAD */

#ifdef CONFIG_HTS221_TRIGGER_GLOBAL_THREAD
static void hts221_work_cb(struct k_work *work)
{
	struct hts221_data *hts221 =
		CONTAINER_OF(work, struct hts221_data, work);

	hts221_handle_interrupt(hts221->dev);
}
#endif /* CONFIG_HTS221_TRIGGER_GLOBAL_THREAD */

int hts221_init_interrupt(struct device *dev)
{
	struct hts221_data *hts221 = dev->driver_data;
	const struct hts221_config *cfg = dev->config->config_info;
	int ret;

	/* setup data ready gpio interrupt */
	hts221->gpio = device_get_binding(cfg->drdy_port);
	if (hts221->gpio == NULL) {
		LOG_DBG("Cannot get pointer to %s device", cfg->drdy_port);
		return -EINVAL;
	}

#if defined(CONFIG_HTS221_TRIGGER_OWN_THREAD)
	k_sem_init(&hts221->gpio_sem, 0, UINT_MAX);

	k_thread_create(&hts221->thread, hts221->thread_stack,
		       CONFIG_HTS221_THREAD_STACK_SIZE,
		       (k_thread_entry_t)hts221_thread, dev,
		       0, NULL, K_PRIO_COOP(CONFIG_HTS221_THREAD_PRIORITY),
		       0, 0);
#elif defined(CONFIG_HTS221_TRIGGER_GLOBAL_THREAD)
	hts221->work.handler = hts221_work_cb;
	hts221->dev = dev;
#endif /* CONFIG_HTS221_TRIGGER_OWN_THREAD */

	ret = gpio_pin_configure(hts221->gpio, cfg->drdy_pin,
				 GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
				 GPIO_INT_ACTIVE_HIGH | GPIO_INT_DEBOUNCE);
	if (ret < 0) {
		LOG_DBG("Could not configure gpio");
		return ret;
	}

	gpio_init_callback(&hts221->gpio_cb, hts221_gpio_callback,
			   BIT(cfg->drdy_pin));

	if (gpio_add_callback(hts221->gpio, &hts221->gpio_cb) < 0) {
		LOG_DBG("Could not set gpio callback");
		return -EIO;
	}

	/* configure interrupt active high */
	if (hts221_int_polarity_set(hts221->ctx, HTS221_ACTIVE_HIGH) < 0) {
		return -EIO;
	}

	/* enable interrupt in pulse mode */
	if (hts221_drdy_on_int_set(hts221->ctx, 1) < 0) {
		return -EIO;
	}

	gpio_pin_enable_callback(hts221->gpio, cfg->drdy_pin);

	/* manually trigger the first time */
	zio_dev_trigger(dev);

	return 0;
}
