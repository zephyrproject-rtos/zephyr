/*
 * Copyright (c) 2024 Fabian Blatz <fabianblatz@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DISPLAY_BACKLIGHT_H_
#define ZEPHYR_INCLUDE_DISPLAY_BACKLIGHT_H_

/**
 * @brief Display Backlight API
 * @defgroup display_backlight Display Backlight API
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>

/**
 * @brief Common display backlight config.
 */
struct display_backlight_common_config {
#ifdef CONFIG_DISPLAY_BACKLIGHT_CONTROL_ENABLE_GPIO
	/* GPIO pin to enable the backlight */
	struct gpio_dt_spec backlight_enable_gpios;
#endif /* CONFIG_DISPLAY_BACKLIGHT_CONTROL_ENABLE_GPIO */
#ifdef CONFIG_DISPLAY_BACKLIGHT_CONTROL_PWM
	/* PWM device to control the backlight */
	struct pwm_dt_spec backlight_pwms;
#endif /* CONFIG_DISPLAY_BACKLIGHT_CONTROL_PWM */
	/* Default brightness level */
	uint8_t default_brightness;
};

/**
 * @brief Initialize common backlight config from devicetree.
 *
 * @param node_id The devicetree node identifier.
 * @param api Pointer to a @ref display_backlight_api structure.
 */
#define DISPLAY_BACKLIGHT_DT_COMMON_CONFIG_INIT(node_id)                                           \
	{IS_ENABLED(CONFIG_DISPLAY_BACKLIGHT_CONTROL_ENABLE_GPIO)                                  \
	 ?.backlight_enable_gpios = GPIO_DT_SPEC_GET_OR(node_id, backlight_enable_gpios, {0}),     \
	 IS_ENABLED(CONFIG_DISPLAY_BACKLIGHT_CONTROL_PWM)                                          \
	 ?.backlight_pwms = PWM_DT_SPEC_GET_OR(node_id, backlight_pwms, {0}),                      \
	 .default_brightness = DT_PROP_OR(node_id, default_brightness, 0), }

/**
 * @brief Initialize common backlight config from devicetree instance.
 *
 * @param inst Instance.
 */
#define DISPLAY_BACKLIGHT_DT_INST_COMMON_CONFIG_INIT(inst)                                         \
	DISPLAY_BACKLIGHT_DT_COMMON_CONFIG_INIT(DT_DRV_INST(inst))

/**
 * @brief Validate the offset of the common data structures.
 *
 * @param config Name of the config structure.
 */
#define DISPLAY_BACKLIGHT_STRUCT_CHECK(config)                                                     \
	BUILD_ASSERT(offsetof(config, common_backlight) == 0,                                      \
		     "struct display_backlight_common_config must be placed first");

/**
 * @brief Common function to initialize a display backlight device at init time.
 *
 * This function must be called at the end of the device init function.
 *
 * @param dev Display backlight device instance.
 *
 * @retval 0 If initialized successfully.
 * @retval -errno Negative errno in case of failure.
 */
int display_backlight_init(const struct device *dev);

/**
 * @brief Set the brightness of the backlight.
 *
 * @param dev Pointer to device structure for driver instance.
 * @param brightness Brightness level to set.
 *
 * @retval 0 If initialized successfully.
 * @retval -errno Negative errno in case of failure.
 */
int display_backlight_set_brightness(const struct device *dev, uint8_t brightness);

/** @} */

#endif /* ZEPHYR_INCLUDE_DISPLAY_BACKLIGHT_H_ */
