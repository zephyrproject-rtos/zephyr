/*
 * Copyright (c) 2026 Embracecactus
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/common/sys_io.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>

#define BK7258_GPIO_CFG_BASE       0x44000400U
#define BK7258_GPIO_FUNC_MODE_BASE 0x440100C0U

#define BK_GPIO_CFG_OFF(gpio)    ((gpio) * 4U)
#define BK_GPIO_BIT_2ND_FUNC     BIT(6)
#define BK_GPIO_BIT_PULL_MODE_EN BIT(5)
#define BK_GPIO_BIT_PULL_MODE    BIT(4)
#define BK_GPIO_BIT_OUTPUT_EN    BIT(3) /* Active low */
#define BK_GPIO_BIT_INPUT_EN     BIT(2)

#define BK_GPIO_CONTROLLED_BITS                                                                    \
	(BK_GPIO_BIT_2ND_FUNC | BK_GPIO_BIT_PULL_MODE_EN | BK_GPIO_BIT_PULL_MODE |                 \
	 BK_GPIO_BIT_OUTPUT_EN | BK_GPIO_BIT_INPUT_EN)

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0; i < pin_cnt; i++) {
		pinctrl_soc_pin_t p = pins[i];
		uint8_t gpio = p & BK7258_GPIO_PIN_MASK;
		uint8_t func = (p >> BK7258_FUNC_POS) & BK7258_FUNC_MASK;
		uint32_t val;

		val = sys_read32(BK7258_GPIO_CFG_BASE + BK_GPIO_CFG_OFF(gpio));
		val &= ~BK_GPIO_CONTROLLED_BITS;
		val |= BK_GPIO_BIT_OUTPUT_EN;

		if (p & BK7258_FLAG_SECOND_FUNC) {
			val |= BK_GPIO_BIT_2ND_FUNC;
		}
		if (p & BK7258_FLAG_OUTPUT) {
			val &= ~BK_GPIO_BIT_OUTPUT_EN;
		}
		if (p & BK7258_FLAG_INPUT) {
			val |= BK_GPIO_BIT_INPUT_EN;
		}
		if (p & BK7258_FLAG_PULL_UP) {
			val |= BK_GPIO_BIT_PULL_MODE_EN | BK_GPIO_BIT_PULL_MODE;
		}
		if (p & BK7258_FLAG_PULL_DOWN) {
			val |= BK_GPIO_BIT_PULL_MODE_EN;
			val &= ~BK_GPIO_BIT_PULL_MODE;
		}

		sys_write32(val, BK7258_GPIO_CFG_BASE + BK_GPIO_CFG_OFF(gpio));

		uint8_t group = gpio / 8U;
		uint8_t shift = (gpio % 8U) * 4U;
		uint32_t addr = BK7258_GPIO_FUNC_MODE_BASE + group * 4U;
		uint32_t mask = BK7258_FUNC_MASK << shift;
		uint32_t fval = sys_read32(addr);

		fval &= ~mask;
		fval |= (uint32_t)func << shift;
		sys_write32(fval, addr);
	}

	return 0;
}
