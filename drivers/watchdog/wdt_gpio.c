/*
 * Copyright (c) 2018 qianfan Zhao
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <watchdog.h>
#include <gpio.h>

#define LOG_LEVEL CONFIG_WDT_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(wdt_gpio);

#define WDT_GPIO_NAME			WDT_GPIO0_LABEL
#define WDT_GPIO_PORT			WDT_GPIO0_GPIO_CONTROLLER
#define WDT_GPIO_PIN			WDT_GPIO0_GPIO_PIN
#define WDT_HW_MARGIN_MS		WDT_GPIO0_HW_MARGIN_MS

struct wdt_gpio_data {
	struct k_delayed_work		work;
	struct device			*gpio_dev;
	int				gpio_pin_value;

#ifndef CONFIG_WDT_GPIO_MINIMUM_SUPPORT
	struct k_mutex			lock;
	s64_t				last_feed_time;
	int				timeout;
	int				enable;
#endif
};

#define DEV_DATA(dev)						\
	((struct wdt_gpio_data * const)(dev)->driver_data)
#define WDT_GPIO_DATA_LOCK(data)	k_mutex_lock(&(data)->lock, K_FOREVER)
#define WDT_GPIO_DATA_UNLOCK(data)	k_mutex_unlock(&(data)->lock)

static int wdt_gpio_setup(struct device *dev, u8_t options)
{
#ifndef CONFIG_WDT_GPIO_MINIMUM_SUPPORT
	struct wdt_gpio_data *data = DEV_DATA(dev);

	WDT_GPIO_DATA_LOCK(data);
	data->enable = 1;
	WDT_GPIO_DATA_UNLOCK(data);
#endif

	return 0;
}

static int wdt_gpio_disable(struct device *dev)
{
#ifndef CONFIG_WDT_GPIO_MINIMUM_SUPPORT
	struct wdt_gpio_data *data = DEV_DATA(dev);

	WDT_GPIO_DATA_LOCK(data);
	data->enable = 0;
	WDT_GPIO_DATA_UNLOCK(data);

	return 0;
#else
	ARG_UNUSED(dev);
	return -EPERM;
#endif
}

static int wdt_gpio_install_timeout(struct device *dev,
				    const struct wdt_timeout_cfg *cfg)
{
#ifndef CONFIG_WDT_GPIO_MINIMUM_SUPPORT
	struct wdt_gpio_data *data = DEV_DATA(dev);

	if (cfg->flags != WDT_FLAG_RESET_SOC) {
		return -ENOTSUP;
	}

	if (cfg->window.max < WDT_HW_MARGIN_MS) {
		LOG_ERR("window.max less than hardware margin time %d",
			WDT_HW_MARGIN_MS);
		return -EINVAL;
	}

	WDT_GPIO_DATA_LOCK(data);
	data->timeout = cfg->window.max;
	WDT_GPIO_DATA_UNLOCK(data);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(cfg);
#endif

	return 0;
}

static void wdt_gpio_togger_pin(struct wdt_gpio_data *data)
{
	/* Togget gpio pin to feed watchdog */
	data->gpio_pin_value = !data->gpio_pin_value;
	gpio_pin_write(data->gpio_dev, WDT_GPIO_PIN, data->gpio_pin_value);
	LOG_DBG("togger");
}

#ifndef CONFIG_WDT_GPIO_MINIMUM_SUPPORT
static void wdt_gpio_delayed_work_timer(struct k_work *work)
{
	struct wdt_gpio_data *data =
			CONTAINER_OF(work, struct wdt_gpio_data, work);

	WDT_GPIO_DATA_LOCK(data);
	s64_t last_time = data->last_feed_time;
	WDT_GPIO_DATA_UNLOCK(data);

	if (!data->enable || k_uptime_delta(&last_time) < data->timeout) {
		wdt_gpio_togger_pin(data);
		k_delayed_work_submit(&data->work, WDT_HW_MARGIN_MS / 2);
	} else {
		LOG_ERR("Watchdog timeout, system will be restart");
	}
}
#endif

static int wdt_gpio_feed(struct device *dev, int channel_id)
{
	struct wdt_gpio_data *data = DEV_DATA(dev);

#ifndef CONFIG_WDT_GPIO_MINIMUM_SUPPORT
	WDT_GPIO_DATA_LOCK(data);
	data->last_feed_time = k_uptime_get();
	WDT_GPIO_DATA_UNLOCK(data);
#else
	wdt_gpio_togger_pin(data);
#endif

	return 0;
}

static const struct wdt_driver_api wdt_gpio_api = {
	.setup = wdt_gpio_setup,
	.disable = wdt_gpio_disable,
	.install_timeout = wdt_gpio_install_timeout,
	.feed = wdt_gpio_feed,
};

static int init_wdt(struct device *dev)
{
	struct wdt_gpio_data *data = DEV_DATA(dev);

	data->gpio_dev = device_get_binding(WDT_GPIO_PORT);
	if (!data->gpio_dev) {
		LOG_ERR("Can't get binding device, %s\n", WDT_GPIO_PORT);
		return -EINVAL;
	}

	gpio_pin_configure(data->gpio_dev, WDT_GPIO_PIN, GPIO_DIR_OUT);
	gpio_pin_write(data->gpio_dev, WDT_GPIO_PIN, 1); /* Togger */
	gpio_pin_write(data->gpio_dev, WDT_GPIO_PIN, 0);
	data->gpio_pin_value = 0;

#ifndef CONFIG_WDT_GPIO_MINIMUM_SUPPORT
	data->enable = 0; /* Default is disabled */
	k_mutex_init(&data->lock);
	k_delayed_work_init(&data->work, wdt_gpio_delayed_work_timer);
	k_delayed_work_submit(&data->work, WDT_HW_MARGIN_MS / 2);
#endif

	return 0;
}

static struct wdt_gpio_data private_data;
DEVICE_AND_API_INIT(wdt_gpio, WDT_GPIO_NAME, init_wdt,
		    &private_data, NULL, POST_KERNEL,
		    CONFIG_WDT_GPIO_INIT_PRIORITY, &wdt_gpio_api);
