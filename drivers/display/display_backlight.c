/*
 * Copyright (c) 2024 Fabian Blatz <fabianblatz@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/display/display_backlight.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(display_backlight, CONFIG_DISPLAY_LOG_LEVEL);

int display_backlight_init(const struct device *dev)
{
	const struct display_backlight_common_config *config = dev->config;

#ifdef CONFIG_DISPLAY_BACKLIGHT_CONTROL_PWM
	if (!pwm_is_ready_dt(&config->backlight_pwms)) {
		LOG_ERR("PWM device %s is not ready", config->backlight_pwms.dev->name);
		return -ENODEV;
	}
#endif /* CONFIG_DISPLAY_BACKLIGHT_CONTROL_PWM */

#ifdef CONFIG_DISPLAY_BACKLIGHT_CONTROL_ENABLE_GPIO
	if (!gpio_is_ready_dt(&config->backlight_enable_gpios)) {
		LOG_ERR("GPIO device %s is not ready", config->backlight_enable_gpios.port->name);
		return -ENODEV;
	}

	if (gpio_pin_configure_dt(&config->backlight_enable_gpios, GPIO_OUTPUT) < 0) {
		LOG_ERR("Failed to configure GPIO device %s",
			config->backlight_enable_gpios.port->name);
		return -EIO;
	}
#endif /* CONFIG_DISPLAY_BACKLIGHT_CONTROL_ENABLE_GPIO */

	if (config->default_brightness) {
		return display_backlight_set_brightness(dev, config->default_brightness);
	}

	return 0;
}

int display_backlight_set_brightness(const struct device *dev, uint8_t brightness)
{
	const struct display_backlight_common_config *config = dev->config;
	int ret;

#ifdef CONFIG_DISPLAY_BACKLIGHT_CONTROL_PWM
	uint32_t duty_cycle = (brightness * config->backlight_pwms.period) / 255;

	ret = pwm_set_pulse_dt(&config->backlight_pwms, duty_cycle);
	if (ret < 0) {
		LOG_ERR("Failed to set PWM duty cycle");
		return ret;
	}
#endif /* CONFIG_DISPLAY_BACKLIGHT_CONTROL_PWM */

#ifdef CONFIG_DISPLAY_BACKLIGHT_CONTROL_ENABLE_GPIO
	ret = gpio_pin_set_dt(&config->backlight_enable_gpios, 1);
	if (ret < 0) {
		LOG_ERR("Failed to set GPIO pin");
		return ret;
	}
#endif /* CONFIG_DISPLAY_BACKLIGHT_CONTROL_ENABLE_GPIO */

	return 0;
}
