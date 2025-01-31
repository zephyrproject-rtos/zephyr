/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2024 sensry.io
 */

#define DT_DRV_COMPAT sensry_sy1xx_pinctrl

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include <zephyr/arch/common/sys_io.h>

static uint32_t pinctrl0_base_addr = DT_REG_ADDR(DT_NODELABEL(pinctrl));
static uint32_t pinctrl0_base_mask = DT_REG_SIZE(DT_NODELABEL(pinctrl)) - 1;

/**
 * @brief Configure a pin.
 *
 * @param pin The pin to configure.
 */
static int pinctrl_configure_pin(const pinctrl_soc_pin_t *pin)
{
	uint32_t addr = (pin->addr & pinctrl0_base_mask) | pinctrl0_base_addr;

	switch (pin->iro) {
	case 0:
	case 8:
	case 16:
	case 24:
		/* fall through */
		break;
	default:
		/* invalid inter address offset */
		return -EINVAL;
	}

	uint32_t reg = ~(0xFFUL << pin->iro) & sys_read32(addr);

	reg |= FIELD_PREP((0xFFUL << pin->iro), pin->cfg);
	sys_write32(reg, addr);

	return 0;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		int ret = pinctrl_configure_pin(pins++);

		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}
