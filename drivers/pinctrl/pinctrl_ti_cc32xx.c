/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc32xx_pinctrl

#include <zephyr/arch/cpu.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/pinctrl/ti-cc32xx-pinctrl.h>

#define MEM_GPIO_PAD_CONFIG_MSK 0xFFFU

/* pin to pad mapping (255 indicates invalid pin) */
static const uint8_t pin2pad[] = {
	10U,  11U,  12U,  13U,  14U,  15U,  16U,  17U,  255U, 255U, 18U,  19U,  20U,
	21U,  22U,  23U,  24U,  40U,  28U,  29U,  25U,  255U, 255U, 255U, 255U, 255U,
	255U, 255U, 26U,  27U,  255U, 255U, 255U, 255U, 255U, 255U, 255U, 255U, 255U,
	255U, 255U, 255U, 255U, 255U, 31U,  255U, 255U, 255U, 255U, 0U,   255U, 32U,
	30U,  255U, 1U,   255U, 2U,   3U,   4U,   5U,   6U,   7U,   8U,   9U,
};

static int pinctrl_configure_pin(pinctrl_soc_pin_t pincfg)
{
	uint8_t pin;

	pin = (pincfg >> TI_CC32XX_PIN_POS) & TI_CC32XX_PIN_MSK;
	if ((pin >= ARRAY_SIZE(pin2pad)) || (pin2pad[pin] == 255U)) {
		return -EINVAL;
	}

	sys_write32(pincfg & MEM_GPIO_PAD_CONFIG_MSK, DT_INST_REG_ADDR(0) + (pin2pad[pin] << 2U));

	return 0;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		int ret;

		ret = pinctrl_configure_pin(pins[i]);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}
