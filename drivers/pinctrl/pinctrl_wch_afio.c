/*
 * Copyright (c) 2024 Michael Hope
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/pinctrl/ch32v003-pinctrl.h>

#include <ch32v003fun.h>

static GPIO_TypeDef *const wch_afio_pinctrl_regs[] = {
	GPIOA,
	NULL,
	GPIOC,
	GPIOD,
};

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	int i;

	for (i = 0; i < pin_cnt; ++i, ++pins) {
		uint8_t pin = (pins->config >> CH32V003_PINCTRL_PIN_SHIFT) & 0x0F;
		uint8_t port = (pins->config >> CH32V003_PINCTRL_PORT_SHIFT) & 0x03;
		bool is_output = ((pins->config >> CH32V003_PINCTRL_OUTPUT_BIT) & 0x01) != 0;
		uint8_t bit0 = (pins->config >> CH32V003_PINCTRL_AFIO0_SHIFT) & 0x1F;
		uint8_t bit1 = (pins->config >> CH32V003_PINCTRL_AFIO1_SHIFT) & 0x1F;
		GPIO_TypeDef *regs = wch_afio_pinctrl_regs[port];
		uint32_t pcfr1 = AFIO->PCFR1;
		uint8_t cfg = 0;

		if (is_output) {
			cfg |= (pins->slew_rate + 1);
			if (pins->drive_open_drain) {
				cfg |= BIT(2);
			}
			if (bit0 != CH32V003_PINCTRL_AFIO_UNSET) {
				cfg |= BIT(3);
			}
		} else {
			if (pins->bias_pull_up || pins->bias_pull_down) {
				cfg |= BIT(3);
			}
		}
		regs->CFGLR = (regs->CFGLR & ~(0x0F << (pin * 4))) | (cfg << (pin * 4));

		if (is_output) {
			regs->OUTDR |= BIT(pin);
		} else {
			regs->OUTDR &= ~(1 << pin);
			if (pins->bias_pull_up) {
				regs->BSHR = BIT(pin);
			}
			if (pins->bias_pull_down) {
				regs->BCR = BIT(pin);
			}
		}

		if (bit0 != CH32V003_PINCTRL_AFIO_UNSET) {
			pcfr1 |= BIT(bit0);
			if (bit1 != CH32V003_PINCTRL_AFIO_UNSET) {
				pcfr1 |= BIT(bit1);
			}
		}
	}

	return 0;
}
