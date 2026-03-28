/* ST Microelectronics LPS2XDF pressure and temperature sensor
 *
 * Copyright (c) 2023 STMicroelectronics
 * Copyright (c) 2023 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lps22df.pdf
 * https://www.st.com/resource/en/datasheet/lps28dfw.pdf
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "lps2xdf.h"

#if DT_HAS_COMPAT_STATUS_OKAY(st_lps22df)
#include "lps22df.h"
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(st_lps28dfw)
#include "lps28dfw.h"
#endif

LOG_MODULE_DECLARE(LPS2XDF, CONFIG_SENSOR_LOG_LEVEL);

int lps2xdf_config_int(const struct device *dev)
{
	const struct lps2xdf_config *const cfg = dev->config;
	const struct lps2xdf_chip_api *chip_api = cfg->chip_api;

	return chip_api->config_interrupt(dev);
}

int lps2xdf_trigger_set(const struct device *dev,
			  const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler)
{
	const struct lps2xdf_config *const cfg = dev->config;
	const struct lps2xdf_chip_api *chip_api = cfg->chip_api;

	return chip_api->trigger_set(dev, trig, handler);
}

static void lps2xdf_intr_callback(struct lps2xdf_data *lps2xdf)
{
#if defined(CONFIG_LPS2XDF_TRIGGER_OWN_THREAD)
	k_sem_give(&lps2xdf->intr_sem);
#elif defined(CONFIG_LPS2XDF_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&lps2xdf->work);
#endif /* CONFIG_LPS2XDF_TRIGGER_OWN_THREAD */
}

static void lps2xdf_gpio_callback(const struct device *dev,
				  struct gpio_callback *cb, uint32_t pins)
{
	struct lps2xdf_data *lps2xdf =
		CONTAINER_OF(cb, struct lps2xdf_data, gpio_cb);

	ARG_UNUSED(pins);
	const struct lps2xdf_config *cfg = lps2xdf->dev->config;
	int ret;

	ret = gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_DISABLE);
	if (ret < 0) {
		LOG_ERR("%s: Not able to configure pin_int", dev->name);
	}

	lps2xdf_intr_callback(lps2xdf);
}

#ifdef CONFIG_LPS2XDF_TRIGGER_OWN_THREAD
static void lps2xdf_thread(struct lps2xdf_data *lps2xdf)
{
	const struct device *dev = lps2xdf->dev;
	const struct lps2xdf_config *const cfg = dev->config;
	const struct lps2xdf_chip_api *chip_api = cfg->chip_api;

	while (1) {
		k_sem_take(&lps2xdf->intr_sem, K_FOREVER);
		chip_api->handle_interrupt(dev);
	}
}
#endif /* CONFIG_LPS2XDF_TRIGGER_OWN_THREAD */

#ifdef CONFIG_LPS2XDF_TRIGGER_GLOBAL_THREAD
static void lps2xdf_work_cb(struct k_work *work)
{
	struct lps2xdf_data *lps2xdf =
		CONTAINER_OF(work, struct lps2xdf_data, work);
	const struct device *dev = lps2xdf->dev;
	const struct lps2xdf_config *const cfg = dev->config;
	const struct lps2xdf_chip_api *chip_api = cfg->chip_api;

	chip_api->handle_interrupt(dev);
}
#endif /* CONFIG_LPS2XDF_TRIGGER_GLOBAL_THREAD */

#if (DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(st_lps22df, i3c) ||\
	DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(st_lps28dfw, i3c))
static int lps2xdf_ibi_cb(struct i3c_device_desc *target,
			  struct i3c_ibi_payload *payload)
{
	const struct device *dev = target->dev;
	struct lps2xdf_data *lps2xdf = dev->data;

	ARG_UNUSED(payload);

	lps2xdf_intr_callback(lps2xdf);

	return 0;
}
#endif

int lps2xdf_init_interrupt(const struct device *dev, enum sensor_variant variant)
{
	struct lps2xdf_data *lps2xdf = dev->data;
	const struct lps2xdf_config *cfg = dev->config;
	int ret;

	/* setup data ready gpio interrupt */
	if (!gpio_is_ready_dt(&cfg->gpio_int) && !ON_I3C_BUS(cfg)) {
		if (cfg->gpio_int.port) {
			LOG_ERR("%s: device %s is not ready", dev->name,
						cfg->gpio_int.port->name);
			return -ENODEV;
		}

		LOG_DBG("%s: gpio_int not defined in DT", dev->name);
		return 0;
	}

	lps2xdf->dev = dev;

#if defined(CONFIG_LPS2XDF_TRIGGER_OWN_THREAD)
	k_sem_init(&lps2xdf->intr_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&lps2xdf->thread, lps2xdf->thread_stack,
		       CONFIG_LPS2XDF_THREAD_STACK_SIZE,
		       (k_thread_entry_t)lps2xdf_thread, lps2xdf,
		       NULL, NULL, K_PRIO_COOP(CONFIG_LPS2XDF_THREAD_PRIORITY),
		       0, K_NO_WAIT);
#elif defined(CONFIG_LPS2XDF_TRIGGER_GLOBAL_THREAD)
	lps2xdf->work.handler = lps2xdf_work_cb;
#endif /* CONFIG_LPS2XDF_TRIGGER_OWN_THREAD */

	if (!ON_I3C_BUS(cfg)) {
		ret = gpio_pin_configure_dt(&cfg->gpio_int, GPIO_INPUT);
		if (ret < 0) {
			LOG_ERR("Could not configure gpio");
			return ret;
		}

		LOG_INF("%s: int on %s.%02u", dev->name, cfg->gpio_int.port->name,
					      cfg->gpio_int.pin);

		gpio_init_callback(&lps2xdf->gpio_cb,
				   lps2xdf_gpio_callback,
				   BIT(cfg->gpio_int.pin));

		ret = gpio_add_callback(cfg->gpio_int.port, &lps2xdf->gpio_cb);
		if (ret < 0) {
			LOG_ERR("Could not set gpio callback");
			return ret;
		}
	}

	LOG_DBG("drdy_pulsed is %d", (int)cfg->drdy_pulsed);

	/* enable drdy in pulsed/latched mode */
	ret = lps2xdf_config_int(dev);
	if (ret < 0) {
		LOG_ERR("Could not configure interrupt mode");
		return ret;
	}

#if (DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(st_lps22df, i3c) ||\
	DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(st_lps28dfw, i3c))
	if (cfg->i3c.bus != NULL) {
		/* I3C IBI does not utilize GPIO interrupt. */
		lps2xdf->i3c_dev->ibi_cb = lps2xdf_ibi_cb;

		if (i3c_ibi_enable(lps2xdf->i3c_dev) != 0) {
			LOG_DBG("Could not enable I3C IBI");
			return -EIO;
		}

		return 0;
	}
#endif

	return gpio_pin_interrupt_configure_dt(&cfg->gpio_int,
					       GPIO_INT_EDGE_TO_ACTIVE);
}
