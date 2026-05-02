/*
 * Copyright (c) 2022, Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_gpio_emul_sdl

#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "gpio_emul_sdl_bottom.h"

LOG_MODULE_REGISTER(gpio_emul_sdl, CONFIG_GPIO_LOG_LEVEL);

struct gpio_sdl_config {
	const struct device *emul;

	const int *codes;
	uint8_t num_codes;
	struct gpio_sdl_data *data;
};

static int sdl_filter_top(struct gpio_sdl_data *bottom_data)
{
	const struct device *port = bottom_data->dev;
	const struct gpio_sdl_config *config = port->config;
	int ret;

	gpio_pin_t pin = 0;

	/* Search for the corresponding scancode */
	while (pin < config->num_codes) {
		if (config->codes[pin] == bottom_data->event_scan_code) {
			break;
		}
		pin++;
	}

	if (pin == config->num_codes) {
		/* Not tracked */
		return 1;
	}

	/* Lock the scheduler so we can't be preempted,
	 * as the gpio_emul driver keeps a mutex locked
	 * for as long as there are pending interrupts
	 */
	k_sched_lock();

	/* Update the pin state */
	ret = gpio_emul_input_set(config->emul, pin, bottom_data->key_down);

	k_sched_unlock();
	if (ret < 0) {
		LOG_WRN("Failed to emulate input (%d)", ret);
	}

	return 0;
}

static int gpio_sdl_init(const struct device *dev)
{
	const struct gpio_sdl_config *config = dev->config;

	for (uint8_t pin = 0; pin < config->num_codes; ++pin) {
		if (config->codes[pin] != GPIOEMULSDL_SCANCODE_UNKNOWN) {
			LOG_INF("GPIO %s:%u = %u", dev->name, pin, config->codes[pin]);
		}
	}

	config->data->dev = (void *)dev;
	config->data->callback = sdl_filter_top;
	gpio_sdl_init_bottom(config->data);
	return 0;
}

#define GPIO_SDL_DEFINE(inst)								\
	BUILD_ASSERT(DT_NODE_HAS_COMPAT_STATUS(DT_INST_PARENT(inst),			\
					       zephyr_gpio_emul, okay),			\
		     "Enabled parent zephyr,gpio-emul node is required");		\
											\
	static const int gpio_sdl_##inst##_codes[]					\
		= DT_INST_PROP(inst, scancodes);					\
											\
	static struct gpio_sdl_data data_##inst;				\
											\
	static const struct gpio_sdl_config gpio_sdl_##inst##_config = {		\
		.emul = DEVICE_DT_GET(DT_INST_PARENT(inst)),				\
		.codes = gpio_sdl_##inst##_codes,					\
		.num_codes = DT_INST_PROP_LEN(inst, scancodes),				\
		.data = &data_##inst,					\
	};										\
												\
	DEVICE_DT_INST_DEFINE(inst, gpio_sdl_init, NULL, NULL,				\
			      &gpio_sdl_##inst##_config, POST_KERNEL,			\
			      CONFIG_GPIO_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(GPIO_SDL_DEFINE)
