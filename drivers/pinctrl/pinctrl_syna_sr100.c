/*
 * Copyright (c) 2025 Synaptics, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/dt-bindings/pinctrl/syna-sr100-pinctrl.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/arch/common/sys_io.h>

#define SRXXX_GET_PORT_ADDR_OR_NONE(nodelabel)                                                     \
	(DT_NODE_EXISTS(DT_NODELABEL(nodelabel)) ? DT_REG_ADDR(DT_NODELABEL(nodelabel)) : 0)

static const uint32_t pinctrl_addr[] = {
	SRXXX_GET_PORT_ADDR_OR_NONE(pinctrl_global),
	SRXXX_GET_PORT_ADDR_OR_NONE(pinctrl_aon_main),
	SRXXX_GET_PORT_ADDR_OR_NONE(pinctrl_lps_gear1),
	SRXXX_GET_PORT_ADDR_OR_NONE(pinctrl_swire),
	SRXXX_GET_PORT_ADDR_OR_NONE(pinctrl_port_ctrl),
};

static void pinctrl_cfg(pinctrl_soc_pin_t soc_pin, uint32_t *value, uint32_t mask)
{
	if (soc_pin.flags & mask) {
		*value &= ~(mask);
		*value |= soc_pin.pincfg & mask;
	}
}

static void pinctrl_configure_pin(pinctrl_soc_pin_t soc_pin)
{
	uint32_t value, ctrl, reg, bit, mode, mask;

	ctrl = SRXXX_PINMUX_CTRL(soc_pin.pinmux);
	if (soc_pin.pinmux != 0) {
		reg = SRXXX_PINMUX_REG(soc_pin.pinmux);
		bit = SRXXX_PINMUX_BIT(soc_pin.pinmux);
		mode = SRXXX_PINMUX_MODE(soc_pin.pinmux);
		mask = SRXXX_PINMUX_MASK(soc_pin.pinmux);

		value = sys_read32(pinctrl_addr[ctrl] + reg);
		value = (value & ~(mask << bit)) | (mode << bit);
		sys_write32(value, pinctrl_addr[ctrl] + reg);
	}

	if (soc_pin.port) {
		reg = SRXXX_PINMUX_REG(soc_pin.port);
		bit = SRXXX_PINMUX_BIT(soc_pin.port);
		mode = SRXXX_PINMUX_MODE(soc_pin.port);
		mask = SRXXX_PINMUX_MASK(soc_pin.port);

		value = sys_read32(pinctrl_addr[4] + reg);
		value = (value & ~(mask << bit)) | (mode << bit);
		sys_write32(value, pinctrl_addr[4] + reg);
	}

	if (soc_pin.offset) {
		value = sys_read32(pinctrl_addr[ctrl] + soc_pin.offset);

		pinctrl_cfg(soc_pin, &value, SRXXX_DRV_STRENGTH_MASK);
		pinctrl_cfg(soc_pin, &value, SRXXX_SLEW_RATE_MASK);
		pinctrl_cfg(soc_pin, &value, SRXXX_INPUT_ENABLE_MASK);
		pinctrl_cfg(soc_pin, &value, SRXXX_PULL_ENABLE_MASK);

		sys_write32(value, pinctrl_addr[ctrl] + soc_pin.offset);
	}
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		pinctrl_configure_pin(*pins++);
	}

	return 0;
}
