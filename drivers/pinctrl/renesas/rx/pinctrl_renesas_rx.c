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

		/* set PMR register to 0 before setting pin control register*/
		ret = R_GPIO_PinControl(port_pin, GPIO_CMD_ASSIGN_TO_GPIO);
		if (ret != 0) {
			return -EINVAL;
		}

		/* set output high*/
		if (pin->cfg.output_high) {
			R_GPIO_PinWrite(port_pin, GPIO_LEVEL_HIGH);
		}

		/* set port direction*/
		if (pin->cfg.output_enable) {
			R_GPIO_PinDirectionSet(port_pin, GPIO_DIRECTION_OUTPUT);
		}

		/* set pull-up*/
		if (pin->cfg.bias_pull_up) {
			ret = R_GPIO_PinControl(port_pin, GPIO_CMD_IN_PULL_UP_ENABLE);
			if (ret != 0) {
				return -EINVAL;
			}
		}

		/* set open-drain*/
		if (pin->cfg.drive_open_drain) {
			ret = R_GPIO_PinControl(port_pin, GPIO_CMD_OUT_OPEN_DRAIN_N_CHAN);
			if (ret != 0) {
				return -EINVAL;
			}
		}

		/* set driver-strength*/
		if (pin->cfg.drive_strength) {
			ret = R_GPIO_PinControl(port_pin, GPIO_CMD_DSCR_ENABLE);
			if (ret != 0) {
				return -EINVAL;
			}
		}

		/* set pin function*/
		pconfig.analog_enable = pin->cfg.analog_enable;
		pconfig.pin_function = pin->cfg.psels;
		ret = R_MPC_Write(port_pin, &pconfig);
		if (ret != 0) {
			return -EINVAL;
		}

		/* set MODE*/
		if (pin->cfg.pin_mode) {
			ret = R_GPIO_PinControl(port_pin, GPIO_CMD_ASSIGN_TO_PERIPHERAL);
			if (ret != 0) {
				return -EINVAL;
			}
		}
	}

	return 0;
}
