/*
 * Copyright (c) 2024 Michael Hope
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT wch_afio

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/pinctrl/ch32v003-pinctrl.h>

#include <hal_ch32fun.h>

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
		uint8_t cfg = 0;

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

		if (remap != 0) {
			RCC->APB2PCENR |= RCC_AFIOEN;

			if (bit0 == CH32V003_PINMUX_I2C1_RM) {
				AFIO->PCFR1 |= ((uint32_t)((remap >> 0) & 1)
							<< CH32V003_PINMUX_I2C1_RM) |
					       ((uint32_t)((remap >> 1) & 1)
							<< CH32V003_PINMUX_I2C1_RM1);
			} else if (bit0 == CH32V003_PINMUX_USART1_RM) {
				AFIO->PCFR1 |= ((uint32_t)((remap >> 0) & 1)
							<< CH32V003_PINMUX_USART1_RM) |
					       ((uint32_t)((remap >> 1) & 1)
							<< CH32V003_PINMUX_USART1_RM1);
			} else {
				AFIO->PCFR1 |= (uint32_t)remap << bit0;
			}
		}
	}

	return 0;
}

static int pinctrl_clock_init(void)
{
	const struct device *clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0));
	uint8_t clock_id = DT_INST_CLOCKS_CELL(0, id);

	return clock_control_on(clock_dev, (clock_control_subsys_t *)(uintptr_t)clock_id);
}

SYS_INIT(pinctrl_clock_init, PRE_KERNEL_1, 0);
