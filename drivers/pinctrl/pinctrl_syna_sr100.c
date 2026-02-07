/*
 * Copyright (c) 2025 Synaptics, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/dt-bindings/pinctrl/syna-sr100-pinctrl.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/arch/common/sys_io.h>

struct pinctrl_syna_controller {
	uint32_t mux;
	uint32_t cfg;
	uint32_t port;
};

#define SRXXX_GET_ADDR_OR_NONE(nodelabel, field)                        \
	(DT_NODE_EXISTS(DT_NODELABEL(nodelabel)) ?                      \
	 DT_REG_ADDR_BY_NAME_OR(DT_NODELABEL(nodelabel), field, 0) : 0)

#define SRXXX_GET_PINCTRL(nodelabel) {                                  \
	SRXXX_GET_ADDR_OR_NONE(nodelabel, pinmux),                      \
	SRXXX_GET_ADDR_OR_NONE(nodelabel, pincfg),                      \
	SRXXX_GET_ADDR_OR_NONE(nodelabel, port) }

static const struct pinctrl_syna_controller pinctrl_syna_ctrl[] = {
	SRXXX_GET_PINCTRL(pinctrl_global),
	SRXXX_GET_PINCTRL(pinctrl_aon_main),
	SRXXX_GET_PINCTRL(pinctrl_lps_gear1),
	SRXXX_GET_PINCTRL(pinctrl_swire),
};

static void pinctrl_cfg(pinctrl_soc_pin_t soc_pin, uint32_t *value,
			uint32_t mask)
{
	if (soc_pin.flags & mask) {
		*value &= ~(mask);
		*value |= soc_pin.pincfg & mask;
	}
}

static void pinctrl_configure_pin(pinctrl_soc_pin_t soc_pin)
{
	uint32_t value;
	uint32_t reg;
	uint32_t bit;
	uint32_t mode;
	uint32_t mask;
	const struct pinctrl_syna_controller *ctrl;

	ctrl = &pinctrl_syna_ctrl[SRXXX_PINMUX_CTRL(soc_pin.pinmux)];
	mask = SRXXX_PINMUX_MASK(soc_pin.pinmux);
	if (mask != 0) {
		reg = SRXXX_PINMUX_REG(soc_pin.pinmux);
		bit = SRXXX_PINMUX_BIT(soc_pin.pinmux);
		mode = SRXXX_PINMUX_MODE(soc_pin.pinmux);

		value = sys_read32(ctrl->mux + reg);
		value = (value & ~(mask << bit)) | (mode << bit);
		sys_write32(value, ctrl->mux + reg);
	}

	if (soc_pin.port != 0) {
		reg = SRXXX_PINMUX_REG(soc_pin.port);
		bit = SRXXX_PINMUX_BIT(soc_pin.port);
		mode = SRXXX_PINMUX_MODE(soc_pin.port);
		mask = SRXXX_PINMUX_MASK(soc_pin.port);

		value = sys_read32(ctrl->port + reg);
		value = (value & ~(mask << bit)) | (mode << bit);
		sys_write32(value, ctrl->port + reg);
	}

	if (soc_pin.flags != 0) {
		uint32_t cfg = SRXXX_PINMUX_CFG(soc_pin.pinmux);

		value = sys_read32(ctrl->cfg + cfg);

		pinctrl_cfg(soc_pin, &value, SRXXX_DRV_STRENGTH_MASK);
		pinctrl_cfg(soc_pin, &value, SRXXX_HOLD_ENABLE_MASK);
		pinctrl_cfg(soc_pin, &value, SRXXX_INPUT_ENABLE_MASK);
		pinctrl_cfg(soc_pin, &value, SRXXX_PULL_ENABLE_MASK);
		pinctrl_cfg(soc_pin, &value, SRXXX_SLEW_RATE_MASK);
		pinctrl_cfg(soc_pin, &value, SRXXX_SCHMITT_TRIG_MASK);

		sys_write32(value, ctrl->cfg + cfg);
	}
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		pinctrl_configure_pin(*pins++);
	}

	return 0;
}
