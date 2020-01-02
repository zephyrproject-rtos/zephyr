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
#define LED_SW_BLINK BIT(0)
#define LED_BLINK_DISABLE BIT(1)

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
#if defined(CONFIG_LED_SOFTWARE_BLINK)
	struct k_timer timer;
	atomic_t flags;
	uint32_t blink_delay_on;
	uint32_t blink_delay_off;
	bool next_state;
#endif
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
#if defined(CONFIG_LED_SOFTWARE_BLINK)
	if (atomic_test_bit(&data->flags, LED_SW_BLINK)) {
		data->next_state = state;

		atomic_set_bit(&data->flags, LED_BLINK_DISABLE);
		k_timer_start(&data->timer, K_NO_WAIT, K_NO_WAIT);

		return 0;

	}
#endif /* defined(CONFIG_LED_SOFTWARE_BLINK) */

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

#if defined(CONFIG_LED_SOFTWARE_BLINK)
static void stop_sw_blink(struct led_gpio_data *data)
{
	k_timer_stop(&data->timer);
	data->blink_delay_on = 0;
	data->blink_delay_off = 0;
	atomic_clear_bit(&data->flags, LED_SW_BLINK);
}

static void led_timer_handler(struct k_timer *timer)
{
	struct led_gpio_data *data;
	uint32_t delay;
	bool state;
	int err;

	data = CONTAINER_OF(timer, struct led_gpio_data, timer);

	if (atomic_test_and_clear_bit(&data->flags, LED_BLINK_DISABLE)) {
		stop_sw_blink(data);
		gpio_led_set_no_delay(data, data->next_state);

		return;
	}

	if (data->state) {
		delay = data->blink_delay_off;
		state = 0;
	} else {
		delay = data->blink_delay_on;
		state = 1;
	}

	err = gpio_led_set_no_delay(data, state);
	if (err) {
		LOG_DBG("%s blink error: %d", data->cfg->label, err);
	}

	k_timer_start(&data->timer, K_MSEC(delay), K_NO_WAIT);
}

static int gpio_led_blink(struct device *dev, uint32_t led,
			  uint32_t delay_on, uint32_t delay_off)
{
	struct led_gpio_data *data = led_parameters_get(dev, led);

	if (!delay_on) {
		return gpio_led_off(dev, led);
	}

	if (!delay_off) {
		return gpio_led_on(dev, led);
	}

	data->blink_delay_on = delay_on;
	data->blink_delay_off = delay_off;

	atomic_set_bit(&data->flags, LED_SW_BLINK);

	LOG_DBG("%s blink start, delay on: %d, delay off: %d",
		data->cfg->label, delay_on, delay_off);

	k_timer_start(&data->timer, K_NO_WAIT, K_NO_WAIT);

	return 0;
}
#endif /* defined(CONFIG_LED_SOFTWARE_BLINK) */

static const struct led_driver_api led_api = {
#if defined(CONFIG_LED_SOFTWARE_BLINK)
	.blink = gpio_led_blink,
#endif
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

#if defined(CONFIG_LED_SOFTWARE_BLINK)
		k_timer_init(&data[i].timer, led_timer_handler, NULL);
#endif
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
