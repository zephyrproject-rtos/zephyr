/*
 * Copyright (c) 2025 Michael Hope <michaelh@juju.nz>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT wch_00x_afio

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/pinctrl/ch32v00x-pinctrl.h>

#include <hal_ch32fun.h>

static GPIO_TypeDef *const wch_afio_pinctrl_regs[] = {
	(GPIO_TypeDef *)DT_REG_ADDR(DT_NODELABEL(gpioa)),
	(GPIO_TypeDef *)DT_REG_ADDR(DT_NODELABEL(gpiob)),
	(GPIO_TypeDef *)DT_REG_ADDR(DT_NODELABEL(gpioc)),
	(GPIO_TypeDef *)DT_REG_ADDR(DT_NODELABEL(gpiod)),
};

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	int i;

	for (i = 0; i < pin_cnt; i++, pins++) {
		uint8_t port = FIELD_GET(CH32V00X_PINCTRL_PORT_MASK, pins->config);
		uint8_t pin = FIELD_GET(CH32V00X_PINCTRL_PIN_MASK, pins->config);
		uint8_t bit0 = FIELD_GET(CH32V00X_PINCTRL_BASE_MASK, pins->config);
		uint8_t remap = FIELD_GET(CH32V00X_PINCTRL_RM_MASK, pins->config);
		GPIO_TypeDef *regs = wch_afio_pinctrl_regs[port];
		uint8_t cfg = 0;

		if (pins->output_high || pins->output_low) {
			cfg |= BIT(0);
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

		AFIO->PCFR1 |= remap << bit0;
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
