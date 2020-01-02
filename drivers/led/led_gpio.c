/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <drivers/gpio.h>
#include <drivers/led.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(led_gpio, CONFIG_LED_GPIO_LOG_LEVEL);

#define DT_DRV_COMPAT gpio_leds
#define LED_DATA(node_id)                      \
{                                              \
	{                                      \
		DT_GPIO_LABEL(node_id, gpios), \
		DT_GPIO_PIN(node_id, gpios),   \
		DT_GPIO_FLAGS(node_id, gpios)  \
	},                                     \
	DT_LABEL(node_id),                     \
	DT_PROP(node_id, id)                   \
},

struct gpio_cfg {
	const char *port;
	gpio_pin_t pin;
	gpio_dt_flags_t flags;
};

struct led_dts_cfg {
	struct gpio_cfg gpio;
	const char *label;
	uint32_t id;
};

struct led_gpio_data {
	struct device *gpio_dev;
	const struct led_dts_cfg *cfg;
	bool state;
};

struct led_gpio_cfg {
	const struct led_dts_cfg *led;
	uint32_t led_cnt;
};

static inline struct led_gpio_data *get_dev_data(struct device *dev)
{
	return dev->driver_data;
}

static inline const struct led_gpio_cfg *get_dev_config(struct device *dev)
{
	return dev->config_info;
}

static struct led_gpio_data *led_parameters_get(struct device *dev,
						uint32_t led)
{
	const struct led_gpio_cfg *cfg = get_dev_config(dev);
	struct led_gpio_data *data = get_dev_data(dev);

	for (size_t i = 0; i < cfg->led_cnt; i++) {
		if (data[i].cfg->id == led) {
			return &data[i];
		}
	}

	return NULL;
}

static int gpio_led_set_no_delay(struct led_gpio_data *data, bool state)
{
	int err;

	data->state = state;

	err = gpio_pin_set(data->gpio_dev, data->cfg->gpio.pin,
			   state);
	if (err) {
		LOG_DBG("%s turn %s error %d",
			data->cfg->label,
			state ? "on" : "off",
			err);
	} else {
		LOG_DBG("%s turn %s", data->cfg->label,
			state ? "on" : "off");
	}

	return err;
}

static int gpio_led_set(struct led_gpio_data *data, bool state)
{
	return gpio_led_set_no_delay(data, state);
}

static int gpio_led_on(struct device *dev, uint32_t led)
{
	struct led_gpio_data *data = led_parameters_get(dev, led);

	if (!data) {
		return -EINVAL;
	}

	return gpio_led_set(data, true);
}

static int gpio_led_off(struct device *dev, uint32_t led)
{
	struct led_gpio_data *data = led_parameters_get(dev, led);

	if (!data) {
		return -EINVAL;
	}

	return gpio_led_set(data, false);
}

static const struct led_driver_api led_api = {
	.on = gpio_led_on,
	.off = gpio_led_off,
};

static int gpio_led_init(struct device *dev)
{
	int err;
	const struct led_gpio_cfg *cfg = get_dev_config(dev);
	struct led_gpio_data *data = get_dev_data(dev);

	for (size_t i = 0; i < cfg->led_cnt; i++) {
		data[i].gpio_dev = device_get_binding(cfg->led[i].gpio.port);
		if (!data[i].gpio_dev) {
			LOG_DBG("Failed to get GPIO device for port %s pin %d",
				cfg->led[i].gpio.port, cfg->led[i].gpio.pin);
			return -EINVAL;
		}

		/* Set all leds pin as output */
		err = gpio_pin_configure(data[i].gpio_dev,
					 cfg->led[i].gpio.pin,
					 (GPIO_OUTPUT_INACTIVE |
					  cfg->led[i].gpio.flags));
		if (err) {
			LOG_DBG("Failed to set GPIO port %s pin %d as output",
				cfg->led[i].gpio.port, cfg->led[i].gpio.pin);
			return err;
		}


		data[i].cfg = &cfg->led[i];
	}

	return 0;
}

static const struct led_dts_cfg led_dts[] = {DT_INST_FOREACH_CHILD(0,
								   LED_DATA)};
static struct led_gpio_data led_data[ARRAY_SIZE(led_dts)];
static const struct led_gpio_cfg led_cfg = {
	.led_cnt = ARRAY_SIZE(led_dts),
	.led = led_dts
};

DEVICE_AND_API_INIT(gpio_led, DT_PROP(DT_DRV_INST(0), driver),
		    gpio_led_init, led_data,
		    &led_cfg, POST_KERNEL, CONFIG_LED_INIT_PRIORITY,
		    &led_api);
