/*
 * Copyright (c) 2026 Chen Ren
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT wch_x03x_afio

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/pinctrl/ch32x035-pinctrl.h>

#include <hal_ch32fun.h>

static GPIO_TypeDef *const wch_afio_pinctrl_regs[] = {
	(GPIO_TypeDef *)DT_REG_ADDR(DT_NODELABEL(gpioa)),
	(GPIO_TypeDef *)DT_REG_ADDR(DT_NODELABEL(gpiob)),
	(GPIO_TypeDef *)DT_REG_ADDR(DT_NODELABEL(gpioc)),
};

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	int i;

	for (i = 0; i < pin_cnt; i++, pins++) {
		uint8_t port = FIELD_GET(CH32X035_PINCTRL_PORT_MASK, pins->config);
		uint8_t pin = FIELD_GET(CH32X035_PINCTRL_PIN_MASK, pins->config);
		uint8_t bit0 = FIELD_GET(CH32X035_PINCTRL_BASE_MASK, pins->config);
		uint8_t remap = FIELD_GET(CH32X035_PINCTRL_RM_MASK, pins->config);
		GPIO_TypeDef *regs = wch_afio_pinctrl_regs[port];
		uint8_t cfg = 0;

		/*
		 * MODE/CNF encoding: MODE 01 selects output mode (the CH32X035
		 * has no output speed selection), CNF 1x selects alternate
		 * function, CNF x1 selects open drain, and in input mode CNF 10
		 * selects pull-up/pull-down.
		 */
		if (pins->output_high || pins->output_low) {
			cfg |= BIT(0);
			if (pins->drive_open_drain) {
				cfg |= BIT(2);
			}
			/* Select the alternate function */
			cfg |= BIT(3);
		} else {
			if (pins->bias_pull_up || pins->bias_pull_down) {
				/* CNF 10 selects pull-up/pull-down. */
				cfg |= BIT(3);
			} else {
				/* CNF 01 selects floating input. */
				cfg |= BIT(2);
			}
		}

		if (pin < 16) {
			volatile uint32_t *cfgreg = (pin < 8) ? &regs->CFGLR : &regs->CFGHR;
			uint32_t shift = ((pin < 8) ? pin : pin - 8) * 4;

			*cfgreg = (*cfgreg & ~(0x0F << shift)) | (cfg << shift);

			if (pins->output_high) {
				regs->BSHR = BIT(pin);
			} else if (pins->output_low) {
				/* Reset the pin. */
				regs->BCR = BIT(pin);
			} else {
				if (pins->bias_pull_up) {
					regs->BSHR = BIT(pin);
				}
				if (pins->bias_pull_down) {
					regs->BCR = BIT(pin);
				}
			}
		} else {
			/* Pins 16-23 are configured and set/reset through the
			 * extended CFGXR/BSXR registers.
			 */
			uint32_t shift = (pin - 16) * 4;

			regs->CFGXR = (regs->CFGXR & ~(0x0F << shift)) | (cfg << shift);

			if (pins->output_high) {
				regs->BSXR = BIT(pin - 16);
			} else if (pins->output_low) {
				regs->BSXR = BIT(pin - 16) << 16;
			} else {
				if (pins->bias_pull_up) {
					regs->BSXR = BIT(pin - 16);
				}
				if (pins->bias_pull_down) {
					regs->BSXR = BIT(pin - 16) << 16;
				}
			}
		}

		if (remap != 0) {
			RCC->APB2PCENR |= RCC_AFIOEN;
			AFIO->PCFR1 |= (uint32_t)remap << bit0;
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
