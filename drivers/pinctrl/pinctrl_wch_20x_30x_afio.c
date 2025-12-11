/*
 * Copyright (c) 2024 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT wch_20x_30x_afio

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/pinctrl/ch32v20x_30x-pinctrl.h>

#include <hal_ch32fun.h>

static GPIO_TypeDef *const wch_afio_pinctrl_regs[] = {
	(GPIO_TypeDef *)DT_REG_ADDR(DT_NODELABEL(gpioa)),
	(GPIO_TypeDef *)DT_REG_ADDR(DT_NODELABEL(gpiob)),
	(GPIO_TypeDef *)DT_REG_ADDR(DT_NODELABEL(gpioc)),
	(GPIO_TypeDef *)DT_REG_ADDR(DT_NODELABEL(gpiod)),
#if DT_NODE_EXISTS(DT_NODELABEL(gpioe))
	(GPIO_TypeDef *)DT_REG_ADDR(DT_NODELABEL(gpioe)),
#endif
};

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	int i;

	for (i = 0; i < pin_cnt; i++, pins++) {
		uint8_t port = FIELD_GET(CH32V20X_V30X_PINCTRL_PORT_MASK, pins->config);
		uint8_t pin = FIELD_GET(CH32V20X_V30X_PINCTRL_PIN_MASK, pins->config);
		uint8_t bit0 = FIELD_GET(CH32V20X_V30X_PINCTRL_RM_BASE_MASK, pins->config);
		uint8_t pcfr_id = FIELD_GET(CH32V20X_V30X_PINCTRL_PCFR_ID_MASK, pins->config);
		uint8_t remap = FIELD_GET(CH32V20X_V30X_PINCTRL_RM_MASK, pins->config);
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

		if (pin < 8) {
			regs->CFGLR = (regs->CFGLR & ~(0x0F << (pin * 4))) | (cfg << (pin * 4));
		} else {
			regs->CFGHR = (regs->CFGHR & ~(0x0F << ((pin - 8) * 4))) |
				      (cfg << ((pin - 8) * 4));
		}
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

		if (remap != 0) {
			RCC->APB2PCENR |= RCC_AFIOEN;

			if (pcfr_id == 0 && bit0 == CH32V20X_V30X_PINMUX_USART1_RM) {
				AFIO->PCFR1 |= ((uint32_t)((remap >> 0) & 1)
					<< (CH32V20X_V30X_PINMUX_USART1_RM & 0x1F));
				AFIO->PCFR2 |= ((uint32_t)((remap >> 1) & 1)
					<< (CH32V20X_V30X_PINMUX_USART1_RM1 & 0x1F));
			} else {
				if (pcfr_id == 0) {
					AFIO->PCFR1 |= (uint32_t)remap << bit0;
				} else {
					AFIO->PCFR2 |= (uint32_t)remap << bit0;
				}
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
