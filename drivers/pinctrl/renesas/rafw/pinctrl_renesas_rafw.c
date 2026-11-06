/*
 * SPDX-FileCopyrightText: 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <soc.h>

#define PORT_POS (BSP_IO_PORT_OFFSET)

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	bsp_io_port_pin_t port_pin;

	R_BSP_PinAccessEnable();

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		const pinctrl_soc_pin_t *pin = &pins[i];
		uint32_t cfg = pin->func << GPIO_P0_00_MODE_REG_PID_Pos;

		if (pin->bias_pull_up) {
			cfg |= 0x01 << GPIO_P0_00_MODE_REG_PUPD_Pos;
		} else if (pin->bias_pull_down) {
			cfg |= 0x02 << GPIO_P0_00_MODE_REG_PUPD_Pos;
		} else if (pin->output_enable) {
			cfg |= 0x03 << GPIO_P0_00_MODE_REG_PUPD_Pos;
		}

		port_pin = (pin->port << PORT_POS) | pin->pin;
		R_BSP_PinCfg(port_pin, cfg);
		if (pin->isolation_enable) {
			bsp_io_pad_isolation_enable(port_pin);
		} else {
			bsp_io_pad_isolation_disable(port_pin);
		}
		if (pin->port == BSP_IO_PORT_01) {
			if (pin->low_power_enable) {
				GPIO->P1_PADPWR_CTRL_REG |= (1 << pin->pin);
			} else {
				GPIO->P1_PADPWR_CTRL_REG &= ~(1 << pin->pin);
			}
		}
	}

	R_BSP_PinAccessDisable();

	return 0;
}
