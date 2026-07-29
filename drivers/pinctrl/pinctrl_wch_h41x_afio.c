/*
 * Copyright (c) 2026 Liu Changjie
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT wch_h41x_afio

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/pinctrl/ch32h41x-pinctrl.h>
#include <zephyr/sys/util.h>

#include <hal_ch32fun.h>

static GPIO_TypeDef *const wch_h41x_pinctrl_regs[] = {
	(GPIO_TypeDef *)DT_REG_ADDR(DT_NODELABEL(gpioa)),
#if DT_NODE_EXISTS(DT_NODELABEL(gpiob))
	(GPIO_TypeDef *)DT_REG_ADDR(DT_NODELABEL(gpiob)),
#endif
};

/*
 * The AFIO block holds a pair of 32-bit AF select registers (AFLR for pins
 * 0..7, AFHR for pins 8..15) per port, laid out consecutively starting at
 * GPIOA_AFLR. Each pin occupies a 4-bit field.
 */
static int wch_h41x_set_af(uint8_t port, uint8_t pin, uint8_t af)
{
	volatile uint32_t *af_base = &AFIO->GPIOA_AFLR;
	volatile uint32_t *reg = &af_base[port * 2 + (pin >> 3)];
	uint8_t shift = (pin & 0x7) * 4;

	*reg = (*reg & ~((uint32_t)0xF << shift)) | ((uint32_t)(af & 0xF) << shift);

	return 0;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	for (int i = 0; i < pin_cnt; i++, pins++) {
		uint8_t port = FIELD_GET(CH32H41X_PINMUX_PORT_MASK, pins->config);
		uint8_t pin = FIELD_GET(CH32H41X_PINMUX_PIN_MASK, pins->config);
		uint8_t af = FIELD_GET(CH32H41X_PINMUX_AF_MASK, pins->config);
		GPIO_TypeDef *regs;
		bool output = pins->output_high || pins->output_low;
		uint8_t cfg = 0;

		if (port >= ARRAY_SIZE(wch_h41x_pinctrl_regs)) {
			return -EINVAL;
		}
		regs = wch_h41x_pinctrl_regs[port];

		if (output) {
			uint32_t speed_shift = pin * 2U;

			cfg = pins->drive_open_drain ? GPIO_CNF_OUT_OD_AF : GPIO_CNF_OUT_PP_AF;
			regs->SPEED = (regs->SPEED & ~(0x3U << speed_shift)) |
				      ((uint32_t)pins->slew_rate << speed_shift);
		} else {
			/* Input: CNF = pull-up/down when biased, else floating. */
			cfg = (pins->bias_pull_up || pins->bias_pull_down) ? GPIO_CNF_IN_PUPD
									   : GPIO_CNF_IN_FLOATING;
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
			regs->BCR = BIT(pin);
		} else if (pins->bias_pull_up) {
			regs->BSHR = BIT(pin);
		} else if (pins->bias_pull_down) {
			regs->BCR = BIT(pin);
		}

		wch_h41x_set_af(port, pin, af);
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
