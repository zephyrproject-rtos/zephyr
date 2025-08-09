/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <soc.h>

/* Renesas FIT module for iodefine.h data structures */
#include "platform.h"
#include "r_gpio_rx_if.h"
#include "r_mpc_rx_if.h"

#define PORT_POS (8)

extern const uint8_t g_gpio_open_drain_n_support[];
extern const uint8_t g_gpio_pull_up_support[];
#ifndef CONFIG_SOC_SERIES_RX261
extern const uint8_t g_gpio_dscr_support[];
#endif

static bool gpio_pin_function_check(uint8_t const *check_array, uint8_t port_number,
				    uint8_t pin_number)
{
	if ((check_array[port_number] & (1 << pin_number)) != 0) {
		return true;
	} else {
		return false;
	}
}

static int pinctrl_configure_pullup(const pinctrl_soc_pin_t *pin, uint32_t value)
{
	gpio_port_pin_t port_pin;
	bool pin_check;
	int ret = 0;

	port_pin = (pin->port_num << PORT_POS) | pin->pin_num;
	pin_check = gpio_pin_function_check(g_gpio_pull_up_support, pin->port_num, pin->pin_num);

	if (pin_check) {
		ret = R_GPIO_PinControl(port_pin, (value ? GPIO_CMD_IN_PULL_UP_ENABLE
							 : GPIO_CMD_IN_PULL_UP_DISABLE));
	}

	return ret;
}

#ifndef CONFIG_SOC_SERIES_RX261
static int pinctrl_configure_dscr(const pinctrl_soc_pin_t *pin, uint32_t value)
{
	gpio_port_pin_t port_pin;
	bool pin_check;
	int ret = 0;

	port_pin = (pin->port_num << PORT_POS) | pin->pin_num;
	pin_check = gpio_pin_function_check(g_gpio_dscr_support, pin->port_num, pin->pin_num);

	if (pin_check) {
		ret = R_GPIO_PinControl(port_pin,
					(value ? GPIO_CMD_DSCR_ENABLE : GPIO_CMD_DSCR_DISABLE));
	}

	return ret;
}
#endif

static int pinctrl_configure_opendrain(const pinctrl_soc_pin_t *pin, uint32_t value)
{
	gpio_port_pin_t port_pin;
	bool pin_check;
	int ret = 0;

	port_pin = (pin->port_num << PORT_POS) | pin->pin_num;
	pin_check =
		gpio_pin_function_check(g_gpio_open_drain_n_support, pin->port_num, pin->pin_num);

	if (pin_check) {
		ret = R_GPIO_PinControl(
			port_pin, (value ? GPIO_CMD_OUT_OPEN_DRAIN_N_CHAN : GPIO_CMD_OUT_CMOS));
	}

	return ret;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	gpio_port_pin_t port_pin;
	mpc_config_t pconfig = {
		.pin_function = 0x0,
		.irq_enable = false,
		.analog_enable = false,
	};
	int ret;

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		const pinctrl_soc_pin_t *pin = &pins[i];

		port_pin = (pin->port_num << PORT_POS) | pin->pin_num;

		/* Set PMR register to 0 before setting pin control register */
		ret = R_GPIO_PinControl(port_pin, GPIO_CMD_ASSIGN_TO_GPIO);
		if (ret != 0) {
			return -EINVAL;
		}

		/* Set output high */
		if (pin->cfg.output_high) {
			R_GPIO_PinWrite(port_pin, GPIO_LEVEL_HIGH);
		}

		/* Set port direction */
		if (pin->cfg.output_enable) {
			/* set output high*/
			if (pin->cfg.output_high) {
				R_GPIO_PinWrite(port_pin, GPIO_LEVEL_HIGH);
			} else {
				R_GPIO_PinWrite(port_pin, GPIO_LEVEL_LOW);
			}

			R_GPIO_PinDirectionSet(port_pin, GPIO_DIRECTION_OUTPUT);
		} else {
			R_GPIO_PinDirectionSet(port_pin, GPIO_DIRECTION_INPUT);
		}

		/* Set pull-up */
		ret = pinctrl_configure_pullup(pin, pin->cfg.bias_pull_up);

		if (ret != 0) {
			return -EINVAL;
		}

		/* Set open-drain */
		ret = pinctrl_configure_opendrain(pin, pin->cfg.drive_open_drain);

		if (ret != 0) {
			return -EINVAL;
		}
#ifndef CONFIG_SOC_SERIES_RX261
		/* Set drive-strength */
		ret = pinctrl_configure_dscr(pin, pin->cfg.drive_strength);

		if (ret != 0) {
			return -EINVAL;
		}
#endif

		/* Set pin function */
		pconfig.analog_enable = pin->cfg.analog_enable;
		pconfig.pin_function = pin->cfg.psels;
		ret = R_MPC_Write(port_pin, &pconfig);
		if (ret != 0) {
			return -EINVAL;
		}

		/* Set MODE */
		if (pin->cfg.pin_mode) {
			ret = R_GPIO_PinControl(port_pin, GPIO_CMD_ASSIGN_TO_PERIPHERAL);
			if (ret != 0) {
				return -EINVAL;
			}
		}
	}

	return 0;
}
