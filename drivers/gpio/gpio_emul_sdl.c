/*
 * Copyright (c) 2022, Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_gpio_emul_sdl

#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <SDL.h>

LOG_MODULE_REGISTER(gpio_emul_sdl, CONFIG_GPIO_LOG_LEVEL);

struct gpio_sdl_config {
	const struct device *emul;

	const SDL_Scancode *codes;
	uint8_t num_codes;
};

static int sdl_filter(void *arg, SDL_Event *event)
{
	const struct device *port = arg;
	const struct gpio_sdl_config *config = port->config;
	int ret;

	gpio_pin_t pin = 0;

	/* Only handle keyboard events */
	switch (event->type) {
	case SDL_KEYDOWN:
	case SDL_KEYUP:
		break;
	default:
		return 1;
	}

	/* Search for the corresponding scancode */
	while (pin < config->num_codes) {
		if (config->codes[pin] == event->key.keysym.scancode) {
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
	ret = gpio_emul_input_set(config->emul, pin, event->type == SDL_KEYDOWN ? 1 : 0);

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
		if (config->codes[pin] != SDL_SCANCODE_UNKNOWN) {
			LOG_INF("GPIO %s:%u = %u", dev->name, pin, config->codes[pin]);
		}
	}

	SDL_AddEventWatch(sdl_filter, (void *)dev);

	return 0;
}

#define GPIO_SDL_DEFINE(inst)								\
	BUILD_ASSERT(DT_NODE_HAS_COMPAT_STATUS(DT_INST_PARENT(inst),			\
					       zephyr_gpio_emul, okay),			\
		     "Enabled parent zephyr,gpio-emul node is required");		\
											\
	static const SDL_Scancode gpio_sdl_##inst##_codes[]				\
		= DT_INST_PROP(inst, scancodes);					\
											\
	static const struct gpio_sdl_config gpio_sdl_##inst##_config = {		\
		.emul = DEVICE_DT_GET(DT_INST_PARENT(inst)),				\
		.codes = gpio_sdl_##inst##_codes,					\
		.num_codes = DT_INST_PROP_LEN(inst, scancodes),				\
	};										\
											\
	DEVICE_DT_INST_DEFINE(inst, gpio_sdl_init, NULL, NULL,				\
			      &gpio_sdl_##inst##_config, POST_KERNEL,			\
			      CONFIG_GPIO_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(GPIO_SDL_DEFINE)
