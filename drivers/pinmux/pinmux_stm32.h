/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file header for STM32 pin multiplexing
 */

#ifndef ZEPHYR_DRIVERS_PINMUX_STM32_PINMUX_STM32_H_
#define ZEPHYR_DRIVERS_PINMUX_STM32_PINMUX_STM32_H_

#include <zephyr/types.h>
#include <zephyr/drivers/clock_control.h>
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_pinctrl)
#include <zephyr/dt-bindings/pinctrl/stm32f1-pinctrl.h>
#else
#include <zephyr/dt-bindings/pinctrl/stm32-pinctrl.h>
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_pinctrl) */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief structure to convey pinctrl information for stm32 soc
 * value
 */
struct soc_gpio_pinctrl {
	uint32_t pinmux;
	uint32_t pincfg;
};

/**
 * @brief helper to extract IO port number from STM32_PINMUX() encoded
 * value
 */
#define STM32_DT_PINMUX_PORT(__pin) \
	(((__pin) >> STM32_PORT_SHIFT) & STM32_PORT_MASK)

/**
 * @brief helper to extract IO pin number from STM32_PINMUX() encoded
 * value
 */
#define STM32_DT_PINMUX_LINE(__pin) \
	(((__pin) >> STM32_LINE_SHIFT) & STM32_LINE_MASK)

/**
 * @brief helper to extract IO pin func from STM32_PINMUX() encoded
 * value
 */
#define STM32_DT_PINMUX_FUNC(__pin) \
	(((__pin) >> STM32_MODE_SHIFT) & STM32_MODE_MASK)

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_pinctrl)
/**
 * @brief helper to extract IO pin remap from STM32_PINMUX() encoded
 * value
 */
#define STM32_DT_PINMUX_REMAP(__pin) \
	(((__pin) >> STM32_REMAP_SHIFT) & STM32_REMAP_MASK)
#endif

/**
 * @brief helper to extract IO port number from STM32PIN() encoded
 * value
 */
#define STM32_PORT(__pin) \
	((__pin) >> 4)

/**
 * @brief helper to extract IO pin number from STM32PIN() encoded
 * value
 */
#define STM32_PIN(__pin) \
	((__pin) & 0xf)

/**
 * @brief helper for converting dt stm32 pinctrl format to existing pin config
 *        format
 *
 * @param *pinctrl pointer to soc_gpio_pinctrl list
 * @param list_size list size
 * @param base device base register value
 *
 * @return 0 on success, -EINVAL otherwise
 */
int stm32_dt_pinctrl_configure(const struct soc_gpio_pinctrl *pinctrl,
			       size_t list_size, uint32_t base);

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_pinctrl)
/**
 * @brief Helper function to check and apply provided pinctrl remap
 *        configuration
 *
 * Check operation verifies that pin remapping configuration is the same on all
 * pins. If configuration is valid AFIO clock is enabled and remap is applied
 *
 * @param *pinctrl pointer to soc_gpio_pinctrl list
 * @param list_size list size
 *
 * @return 0 value on success, -EINVAL otherwise
 */
int stm32_dt_pinctrl_remap(const struct soc_gpio_pinctrl *pinctrl,
			   size_t list_size);
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_pinctrl) */

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_DRIVERS_PINMUX_STM32_PINMUX_STM32_H_ */
