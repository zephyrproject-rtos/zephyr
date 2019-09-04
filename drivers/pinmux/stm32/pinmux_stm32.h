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
#ifdef CONFIG_SOC_SERIES_STM32F1X
#include <dt-bindings/pinctrl/stm32-pinctrlf1.h>
#else
#include <dt-bindings/pinctrl/stm32-pinctrl.h>
#endif /* CONFIG_SOC_SERIES_STM32F1X */
#include "pinmux/pinmux.h"


/* pretend that array will cover pin functions */
typedef int stm32_pin_func_t;

/**
 * @brief pinmux config wrapper
 *
 * GPIO function is assumed to be always available, as such it's not listed
 * in @funcs array
 */
struct stm32_pinmux_conf {
	u32_t pin;                      /* pin ID */
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
int z_pinmux_stm32_set(u32_t pin, u32_t func,
		      struct device *clk);

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
 * @return array of pin assignments
 */
void stm32_setup_pins(const struct pin_config *pinconf,
		      size_t pins);

/* common pinmux device name for all STM32 chips */
#define STM32_PINMUX_NAME "stm32-pinmux"

#ifdef CONFIG_SOC_SERIES_STM32F0X
#include "pinmux_stm32f0.h"
#elif CONFIG_SOC_SERIES_STM32F1X
#include "pinmux_stm32f1.h"
#elif CONFIG_SOC_SERIES_STM32F2X
#include "pinmux_stm32f2.h"
#elif CONFIG_SOC_SERIES_STM32F3X
#include "pinmux_stm32f3.h"
#elif CONFIG_SOC_SERIES_STM32F4X
#include "pinmux_stm32f4.h"
#elif CONFIG_SOC_SERIES_STM32F7X
#include "pinmux_stm32f7.h"
#elif CONFIG_SOC_SERIES_STM32H7X
#include "pinmux_stm32h7.h"
#elif CONFIG_SOC_SERIES_STM32G0X
#include "pinmux_stm32g0.h"
#elif CONFIG_SOC_SERIES_STM32G4X
#include "pinmux_stm32g4x.h"
#elif CONFIG_SOC_SERIES_STM32L0X
#include "pinmux_stm32l0.h"
#elif CONFIG_SOC_SERIES_STM32L1X
#include "pinmux_stm32l1x.h"
#elif CONFIG_SOC_SERIES_STM32L4X
#include "pinmux_stm32l4x.h"
#elif CONFIG_SOC_SERIES_STM32MP1X
#include "pinmux_stm32mp1x.h"
#elif CONFIG_SOC_SERIES_STM32WBX
#include "pinmux_stm32wbx.h"
#endif

#endif  /* ZEPHYR_DRIVERS_PINMUX_STM32_PINMUX_STM32_H_ */
