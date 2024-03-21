/*
 * Copyright (c) 2024 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/dt-bindings/pinctrl/pinctrl-zynqmp.h>
#include <zephyr/logging/log.h>
#include "pinctrl_soc.h"

LOG_MODULE_REGISTER(pinctrl_xlnx_zynqmp, CONFIG_PINCTRL_LOG_LEVEL);

#define DT_DRV_COMPAT xlnx_pinctrl_zynqmp

static mm_reg_t base = DT_INST_REG_ADDR(0);
static uint8_t mio_pin_offset = 0x04;

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	for (uint8_t i = 0U; i < pin_cnt; i++) {
		uint32_t sel = 0;

		switch (pins[i].func) {
			case UART_FUNCTION: {
				sel = UARTX_SEL;
				break;
			}

			default: {
				LOG_ERR("Unsupported function enum was selected");
				break;
			}
		}
		sys_write32(sel, base + mio_pin_offset * pins[i].pin);
	}

	return 0;
}
