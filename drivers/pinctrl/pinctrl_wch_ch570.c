/*
 * SPDX-FileCopyrightText: 2026 SMILE (smile.eu)
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * NOTE: This pin controller driver only supports the WCH CH570 and CH572 SoCs.
 */

#define DT_DRV_COMPAT wch_ch570_pinctrl

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/pinctrl/ch570-pinctrl.h>

#include <hal_ch32fun.h>

static GPIO_TypeDef *const wch_gpioa_regs = (GPIO_TypeDef *)DT_REG_ADDR(DT_NODELABEL(gpioa));

static void pinctrl_wch_ch570_gpio_init(const pinctrl_soc_pin_t *pin)
{
	uint8_t gpio_pin = FIELD_GET(CH570_PINCTRL_PIN_MASK, pin->config);
	uint32_t gpio_pin_mask = BIT(gpio_pin);

	if (pin->drive_push_pull || pin->output_high || pin->output_low) {
		wch_gpioa_regs->PA_DIR |= gpio_pin_mask;

		if (pin->drive_push_pull && pin->push_pull_high_capacity) {
			wch_gpioa_regs->PA_PD_DRV |= gpio_pin_mask;
		} else {
			wch_gpioa_regs->PA_PD_DRV &= ~gpio_pin_mask;
		}

		if (pin->output_high) {
			wch_gpioa_regs->PA_SET = gpio_pin_mask;
		} else if (pin->output_low) {
			wch_gpioa_regs->PA_CLR = gpio_pin_mask;
		} else {
			/* No output level specified */
		}
	} else if (pin->input_disable) {
		R16_PIN_ALTERNATE |= gpio_pin_mask;
	} else {
		wch_gpioa_regs->PA_DIR &= ~gpio_pin_mask;

		if (pin->bias_pull_up) {
			wch_gpioa_regs->PA_PD_DRV &= ~gpio_pin_mask;
			wch_gpioa_regs->PA_PU |= gpio_pin_mask;
		} else if (pin->bias_pull_down) {
			wch_gpioa_regs->PA_PD_DRV |= gpio_pin_mask;
			wch_gpioa_regs->PA_PU &= ~gpio_pin_mask;
		} else {
			/* bias-high-impedance */
			wch_gpioa_regs->PA_PD_DRV &= ~gpio_pin_mask;
			wch_gpioa_regs->PA_PU &= ~gpio_pin_mask;
		}
	}
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	for (int i = 0; i < pin_cnt; i++, pins++) {
		uint8_t pin = FIELD_GET(CH570_PINCTRL_PIN_MASK, pins->config);
		uint8_t alt_offset = FIELD_GET(CH570_PINCTRL_ALT_FIELD_OFFSET_MASK, pins->config);
		uint8_t alt_width = FIELD_GET(CH570_PINCTRL_ALT_FIELD_WIDTH_MASK, pins->config);
		uint8_t remap = FIELD_GET(CH570_PINCTRL_ALT_FUNC_MASK, pins->config);

		if (pin == CH570_PINCTRL_PIN_NOT_DEFINED) {
			/* skip this entry */
			continue;
		}

		/* GPIO initialization */
		pinctrl_wch_ch570_gpio_init(pins);

		if (remap < 16 && alt_width != 0) { /* alternate function selection available */
			uint16_t alt_mask;

			/* Alternate function remapping:
			 * Set 'alt_width' bits of R16_PIN_ALTERNATE_H starting at 'alt_offset' to
			 * 'remap' value by first clearing the relevant bits with 'alt_mask' and
			 * then OR'ing the shifted 'remap' value.
			 */
			alt_mask = ((1UL << alt_width) - 1) << (alt_offset);
			R16_PIN_ALTERNATE_H =
				(R16_PIN_ALTERNATE_H & ~alt_mask) | (remap << alt_offset);
		}
	}

	return 0;
}
