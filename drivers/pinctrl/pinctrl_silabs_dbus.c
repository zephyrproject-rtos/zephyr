/*
 * Copyright (c) 2024 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/arch/cpu.h>

#include <em_gpio.h>

#define DT_DRV_COMPAT silabs_dbus_pinctrl
#define PIN_MASK      0xF0000UL
#define ABUS_MASK(i)  GENMASK(((i) * 8) + 3, (i) * 8)

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		mem_addr_t enable_reg, route_reg;

		/* Configure ABUS */
		if (pins[i].en_bit == SILABS_PINCTRL_ANALOG) {
			enable_reg = DT_INST_REG_ADDR_BY_NAME(0, abus) +
				     (pins[i].base_offset * sizeof(mem_addr_t));
			sys_write32(FIELD_PREP(ABUS_MASK(pins[i].mode), pins[i].route_offset),
				    enable_reg);
			continue;
		}

		/* Configure GPIO */
		GPIO_PinModeSet(pins[i].port, pins[i].pin, pins[i].mode, pins[i].dout);

		/* Configure DBUS */
		enable_reg = DT_INST_REG_ADDR_BY_NAME(0, dbus) +
			     (pins[i].base_offset * sizeof(mem_addr_t));
		route_reg = enable_reg + (pins[i].route_offset * sizeof(mem_addr_t));

		if (pins[i].route_offset != SILABS_PINCTRL_UNUSED) {
			sys_write32(pins[i].port | FIELD_PREP(PIN_MASK, pins[i].pin), route_reg);
		}

		if (pins[i].en_bit != SILABS_PINCTRL_UNUSED) {
			if (pins[i].mode == gpioModeDisabled) {
				sys_clear_bit(enable_reg, pins[i].en_bit);
			} else {
				sys_set_bit(enable_reg, pins[i].en_bit);
			}
		}
	}

	return 0;
}
