/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * NOTE: This pin controller driver only supports the WCH CH582M SoC.
 */

#define DT_DRV_COMPAT wch_ch582m_pinctrl

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/pinctrl/ch582m-pinctrl.h>

#include <hal_ch32fun.h>

static GPIO_TypeDef *const wch_gpio_regs = (GPIO_TypeDef *)DT_REG_ADDR(DT_NODELABEL(gpios));

static void pinctrl_wch_ch582m_gpio_init(const pinctrl_soc_pin_t *pin)
{
	uint8_t gpio_pin = FIELD_GET(CH582M_PINCTRL_PIN_MASK, pin->config);
	uint8_t port = FIELD_GET(CH582M_PINCTRL_PORT_MASK, pin->config);
	uint32_t gpio_pin_mask = BIT(gpio_pin);

	if (port == CH582M_PINCTRL_PORT_B) {
		if (pin->drive_push_pull || pin->output_high || pin->output_low) {
			wch_gpio_regs->PB_DIR |= gpio_pin_mask;

			if (pin->drive_push_pull && pin->push_pull_high_capacity) {
				wch_gpio_regs->PB_PD_DRV |= gpio_pin_mask;
			} else {
				wch_gpio_regs->PB_PD_DRV &= ~gpio_pin_mask;
			}

			if (pin->output_high) {
				wch_gpio_regs->PB_SET = gpio_pin_mask;
			} else if (pin->output_low) {
				wch_gpio_regs->PB_CLR = gpio_pin_mask;
			} else {
				/* No output level specified */
			}
		} else {
			wch_gpio_regs->PB_DIR &= ~gpio_pin_mask;
			if (pin->bias_pull_up) {
				wch_gpio_regs->PB_PD_DRV &= ~gpio_pin_mask;
				wch_gpio_regs->PB_PU |= gpio_pin_mask;
			} else if (pin->bias_pull_down) {
				wch_gpio_regs->PB_PD_DRV |= gpio_pin_mask;
				wch_gpio_regs->PB_PU &= ~gpio_pin_mask;
			} else {
				/* bias-high-impedance */
				wch_gpio_regs->PB_PD_DRV &= ~gpio_pin_mask;
				wch_gpio_regs->PB_PU &= ~gpio_pin_mask;
			}
		}
	} else {
		if (pin->drive_push_pull || pin->output_high || pin->output_low) {
			wch_gpio_regs->PA_DIR |= gpio_pin_mask;

			if (pin->drive_push_pull && pin->push_pull_high_capacity) {
				wch_gpio_regs->PA_PD_DRV |= gpio_pin_mask;
			} else {
				wch_gpio_regs->PA_PD_DRV &= ~gpio_pin_mask;
			}

			if (pin->output_high) {
				wch_gpio_regs->PA_SET = gpio_pin_mask;
			} else if (pin->output_low) {
				wch_gpio_regs->PA_CLR = gpio_pin_mask;
			} else {
				/* No output level specified */
			}
		} else {
			wch_gpio_regs->PA_DIR &= ~gpio_pin_mask;

			if (pin->bias_pull_up) {
				wch_gpio_regs->PA_PD_DRV &= ~gpio_pin_mask;
				wch_gpio_regs->PA_PU |= gpio_pin_mask;
			} else if (pin->bias_pull_down) {
				wch_gpio_regs->PA_PD_DRV |= gpio_pin_mask;
				wch_gpio_regs->PA_PU &= ~gpio_pin_mask;
			} else {
				/* bias-high-impedance */
				wch_gpio_regs->PA_PD_DRV &= ~gpio_pin_mask;
				wch_gpio_regs->PA_PU &= ~gpio_pin_mask;
			}
		}
	}
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	for (int i = 0; i < pin_cnt; i++, pins++) {
		/* GPIO initialization. CH582M select each peripheral function via a
		 * single enable bit in R32_PIN_CONFIG rather than a remap field, so no
		 * additional alternate function mux step is required here.
		 */
		pinctrl_wch_ch582m_gpio_init(pins);
	}

	return 0;
}
