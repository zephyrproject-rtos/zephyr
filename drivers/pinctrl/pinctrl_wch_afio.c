/*
 * Copyright (c) 2024 Michael Hope
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/pinctrl/ch32v003-pinctrl.h>

#include <ch32fun.h>

static GPIO_TypeDef *const wch_afio_pinctrl_regs[] = {
	(GPIO_TypeDef *)DT_REG_ADDR(DT_NODELABEL(gpioa)),
	(GPIO_TypeDef *)DT_REG_ADDR(DT_NODELABEL(gpioc)),
	(GPIO_TypeDef *)DT_REG_ADDR(DT_NODELABEL(gpiod)),
};

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	int i;

	for (i = 0; i < pin_cnt; i++, pins++) {
		uint8_t port = (pins->config >> CH32V003_PINCTRL_PORT_SHIFT) & 0x03;
		uint8_t pin = (pins->config >> CH32V003_PINCTRL_PIN_SHIFT) & 0x0F;
		uint8_t bit0 = (pins->config >> CH32V003_PINCTRL_RM_BASE_SHIFT) & 0x1F;
		uint8_t remap = (pins->config >> CH32V003_PINCTRL_RM_SHIFT) & 0x3;
		GPIO_TypeDef *regs = wch_afio_pinctrl_regs[port];
		uint32_t pcfr1 = AFIO->PCFR1;
		uint8_t cfg = 0;

		if (remap != 0) {
			RCC->APB2PCENR |= RCC_AFIOEN;
		}

		if (pins->output_high || pins->output_low) {
			cfg |= (pins->slew_rate + 1);
			if (pins->drive_open_drain) {
				cfg |= BIT(2);
			}
			/* Select the alternate function */
			cfg |= BIT(3);
		} else {
			if (pins->bias_pull_up || pins->bias_pull_down) {
				cfg |= BIT(3);
			}
		}
		regs->CFGLR = (regs->CFGLR & ~(0x0F << (pin * 4))) | (cfg << (pin * 4));

		if (pins->output_high) {
			regs->OUTDR |= BIT(pin);
			regs->BSHR |= BIT(pin);
		} else if (pins->output_low) {
			regs->OUTDR |= BIT(pin);
			/* Reset the pin. */
			regs->BSHR |= BIT(pin + 16);
		} else {
			regs->OUTDR &= ~(1 << pin);
			if (pins->bias_pull_up) {
				regs->BSHR = BIT(pin);
			}
			if (pins->bias_pull_down) {
				regs->BCR = BIT(pin);
			}
		}

		if (bit0 == CH32V003_PINMUX_I2C1_RM) {
			pcfr1 |= ((remap & 1) << CH32V003_PINMUX_I2C1_RM) |
				 (((remap >> 1) & 1) << CH32V003_PINMUX_I2C1_RM1);
		} else if (bit0 == CH32V003_PINMUX_USART1_RM) {
			pcfr1 |= ((remap & 1) << CH32V003_PINMUX_USART1_RM) |
				 (((remap >> 1) & 1) << CH32V003_PINMUX_USART1_RM1);
		} else {
			pcfr1 |= remap << bit0;
		}
		AFIO->PCFR1 = pcfr1;
	}

	return 0;
}
