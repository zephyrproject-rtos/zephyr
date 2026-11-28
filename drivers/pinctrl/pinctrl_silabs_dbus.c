/*
 * Copyright (c) 2024 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/arch/cpu.h>

#include <sl_hal_gpio.h>

#define DT_DRV_COMPAT silabs_dbus_pinctrl
#define PIN_MASK      0xF0000UL
#define ABUS_MASK(i)  GENMASK(((i) * 8) + 3, (i) * 8)

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		uint8_t en_bit = FIELD_GET(SILABS_PINCTRL_DBUS_ENABLE_BIT_MASK, pins[i].enable);
		uint8_t type = FIELD_GET(SILABS_PINCTRL_DBUS_ENABLE_TYPE_MASK, pins[i].enable);
		mem_addr_t enable_reg, route_reg;
		sl_gpio_t gpio = {
			.port = pins[i].port,
			.pin = pins[i].pin,
		};

		/* Configure ABUS */
		if (type == SILABS_PINCTRL_DBUS_ANALOG) {
			enable_reg = DT_INST_REG_ADDR_BY_NAME(0, abus) +
				     (pins[i].base_offset * sizeof(mem_addr_t));
			/* Read-modify-write. */
			uint32_t reg_val = sys_read32(enable_reg);

			reg_val &= ~ABUS_MASK(pins[i].mode);
			reg_val |= FIELD_PREP(ABUS_MASK(pins[i].mode), pins[i].route_offset);
			sys_write32(reg_val, enable_reg);
			continue;
		}

		/* Configure GPIO */
		sl_hal_gpio_set_pin_mode(&gpio, pins[i].mode, pins[i].dout);

		/* Configure DBUS */
		enable_reg = DT_INST_REG_ADDR_BY_NAME(0, dbus) +
			     (pins[i].base_offset * sizeof(mem_addr_t));
		route_reg = enable_reg + (pins[i].route_offset * sizeof(mem_addr_t));

		if (pins[i].route_offset != SILABS_PINCTRL_DBUS_NO_ROUTE) {
			sys_write32(pins[i].port | FIELD_PREP(PIN_MASK, pins[i].pin), route_reg);
		}

		switch (type) {
		case SILABS_PINCTRL_DBUS_ROUTE_ENABLE:
			if (pins[i].mode == SL_GPIO_MODE_DISABLED) {
				sys_clear_bit(enable_reg, en_bit);
			} else {
				sys_set_bit(enable_reg, en_bit);
			}
			break;
		case SILABS_PINCTRL_DBUS_ROUTE_DISABLE:
			sys_clear_bit(enable_reg, en_bit);
			break;
		default:
			break;
		}
	}

	return 0;
}
