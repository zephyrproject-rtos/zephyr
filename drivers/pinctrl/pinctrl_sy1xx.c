/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2024 sensry.io
 */

#define DT_DRV_COMPAT sensry_sy1xx_pinctrl

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include <zephyr/arch/common/sys_io.h>

#define SY1XX_INVALID_BASE_ADDR (0UL)
#define SY1XX_INVALID_BASE_SIZE (0UL)

#define SY1XX_PAD_BASE_ADDR(nodelabel)                                                             \
	COND_CODE_1(DT_NODE_EXISTS(DT_NODELABEL(nodelabel)),					   \
		(DT_REG_ADDR(DT_NODELABEL(nodelabel))),						   \
		(SY1XX_INVALID_BASE_ADDR))

#define SY1XX_PAD_BASE_SIZE(nodelabel)                                                             \
	COND_CODE_1(DT_NODE_EXISTS(DT_NODELABEL(nodelabel)),					   \
		(DT_REG_ADDR_BY_IDX(DT_NODELABEL(nodelabel), 1)),				   \
		(SY1XX_INVALID_BASE_SIZE))

static uint32_t pinctrl0_base_addr = SY1XX_PAD_BASE_ADDR(pinctrl0);
static uint32_t pinctrl0_base_mask = ~(SY1XX_PAD_BASE_SIZE(pinctrl0) - 1);

/**
 * @brief Configure a pin.
 *
 * @param pin The pin to configure.
 */
static int pinctrl_configure_pin(const pinctrl_soc_pin_t *pin)
{
	if ((pin->addr & pinctrl0_base_mask) != pinctrl0_base_addr) {
		/* invalid address range */
		return -EINVAL;
	}

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

	uint32_t reg = ~(0xFFUL << pin->iro) & sys_read32(pin->addr);

	reg |= FIELD_PREP((0xFFUL << pin->iro), pin->cfg);
	sys_write32(reg, pin->addr);

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
