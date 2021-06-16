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
#include <stddef.h>
#include <drivers/clock_control.h>
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_pinctrl)
#include <dt-bindings/pinctrl/stm32f1-pinctrl.h>
#else
#include <dt-bindings/pinctrl/stm32-pinctrl.h>
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_pinctrl) */

#ifdef __cplusplus
extern "C" {
#endif

struct pin_config {
	uint8_t pin_num;
	uint32_t mode;
};

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
	(((__pin) >> 12) & 0xf)

/**
 * @brief helper to extract IO pin number from STM32_PINMUX() encoded
 * value
 */
#define STM32_DT_PINMUX_LINE(__pin) \
	(((__pin) >> 8) & 0xf)

/**
 * @brief helper to extract IO pin func from STM32_PINMUX() encoded
 * value
 */
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_pinctrl)
#define STM32_DT_PINMUX_FUNC(__pin) \
	(((__pin) >> 6) & 0x3)
#else
#define STM32_DT_PINMUX_FUNC(__pin) \
	((__pin) & 0xff)
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_pinctrl)
/**
 * @brief helper to extract IO pin remap from STM32_PINMUX() encoded
 * value
 */
#define STM32_DT_PINMUX_REMAP(__pin) \
	((__pin) & 0x1f)
#endif

/* pretend that array will cover pin functions */
typedef int stm32_pin_func_t;

/**
 * @brief pinmux config wrapper
 *
 * GPIO function is assumed to be always available, as such it's not listed
 * in @funcs array
 */
struct stm32_pinmux_conf {
	uint32_t pin;                      /* pin ID */
	const stm32_pin_func_t *funcs;  /* functions array, indexed with
					 * (stm32_pin_alt_func - 1)
					 */
	const size_t nfuncs;            /* number of alternate functions, not
					 * counting GPIO
					 */
};

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
 * @brief helper for mapping IO port to its clock subsystem
 *
 * @param port  IO port
 *
 * Map given IO @port to corresponding clock subsystem. The returned
 * clock subsystem ID must suitable for passing as parameter to
 * clock_control_on(). Implement this function at the SoC level.
 *
 * @return clock subsystem ID
 */
clock_control_subsys_t stm32_get_port_clock(int port);

/**
 * @brief helper for configuration of IO pin
 *
 * @param pin IO pin, STM32PIN() encoded
 * @param func IO function encoded
 * @param clk clock control device, for enabling/disabling clock gate
 * for the port
 */
int z_pinmux_stm32_set(uint32_t pin, uint32_t func);

/**
 * @brief helper for obtaining pin configuration for the board
 *
 * @param[out] pins  set to the number of pins in the array
 *
 * Obtain pin assignment/configuration for current board. This call
 * needs to be implemented at the board integration level. After
 * restart all pins are already configured as GPIO and can be skipped
 * in the configuration array. Pin numbers in @pin_num field are
 * STM32PIN() encoded.
 *
 */
void stm32_setup_pins(const struct pin_config *pinconf,
		      size_t pins);

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
 * @param base device base register value
 *
 * @return 0 value on success, -EINVAL otherwise
 */
int stm32_dt_pinctrl_remap(const struct soc_gpio_pinctrl *pinctrl,
			   size_t list_size, uint32_t base);
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_pinctrl) */

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_DRIVERS_PINMUX_STM32_PINMUX_STM32_H_ */
