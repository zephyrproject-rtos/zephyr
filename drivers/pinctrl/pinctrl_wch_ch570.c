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

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	for (int i = 0; i < pin_cnt; i++, pins++) {
		uint8_t pin = FIELD_GET(CH570_PINCTRL_PIN_MASK, pins->config);
		uint8_t alt_offset = FIELD_GET(CH570_PINCTRL_ALT_FIELD_OFFSET_MASK, pins->config);
		uint8_t alt_width = FIELD_GET(CH570_PINCTRL_ALT_FIELD_WIDTH_MASK, pins->config);
		uint8_t remap = FIELD_GET(CH570_PINCTRL_ALT_FUNC_MASK, pins->config);
		uint32_t pin_mask = BIT(pin);

		/* GPIO initialization */
		if (pins->drive_push_pull || pins->output_high || pins->output_low) {
			wch_gpioa_regs->PA_DIR |= pin_mask;
			if (pins->drive_push_pull && pins->push_pull_high_capacity) {
				wch_gpioa_regs->PA_PD_DRV |= pin_mask;
			} else {
				wch_gpioa_regs->PA_PD_DRV &= ~pin_mask;
			}

			if (pins->output_high) {
				wch_gpioa_regs->PA_SET = pin_mask;
			} else if (pins->output_low) {
				wch_gpioa_regs->PA_CLR = pin_mask;
			} else {
				/* No output level specified */
			}
		} else if (pins->input_disable) {
			R16_PIN_ALTERNATE |= pin_mask;
		} else {
			wch_gpioa_regs->PA_DIR &= ~pin_mask;
			if (pins->bias_pull_up) {
				wch_gpioa_regs->PA_PD_DRV &= ~pin_mask;
				wch_gpioa_regs->PA_PU |= pin_mask;
			} else if (pins->bias_pull_down) {
				wch_gpioa_regs->PA_PD_DRV |= pin_mask;
				wch_gpioa_regs->PA_PU &= ~pin_mask;
			} else {
				/* bias-high-impedance */
				wch_gpioa_regs->PA_PD_DRV &= ~pin_mask;
				wch_gpioa_regs->PA_PU &= ~pin_mask;
			}
		}

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
