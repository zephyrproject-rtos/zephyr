/*
 * Copyright (c) 2024 Realtek Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/pinctrl/rts5817_pinctrl.h>
#include "pinctrl_rts5817.h"

#define PAD_CFG_SIZE 0x40

static void pinctrl_configure_pin(pinctrl_soc_pin_t pincfg)
{
	uint32_t base;
	uint32_t value;
	uint8_t h3l1 = pincfg.power_source & BIT(0);
	uint8_t iev18 = (pincfg.power_source & BIT(1)) >> 1;

	if (pincfg.pin >= RTS_FP_PIN_AL0 && pincfg.pin <= RTS_FP_PIN_AL2) {
		base = DT_REG_ADDR_BY_IDX(DT_NODELABEL(pinctrl), 1);

		value = sys_read32(base);

		value &= ~((0x1 << (pincfg.pin - RTS_FP_PIN_AL0 + AL_GPIO_PU_CTRL_OFFSET)) |
			   (0x1 << (pincfg.pin - RTS_FP_PIN_AL0 + AL_GPIO_PD_CTRL_OFFSET)) |
			   (0x1 << (pincfg.pin - RTS_FP_PIN_AL0 + AL_GPIO_SEL_OFFSET)));

		if (pincfg.pin == RTS_FP_PIN_AL0) {
			value &= ~(1 << CS1_BRIDGE_EN_OFFSET);
		}

		if (pincfg.pin == RTS_FP_PIN_AL0 && pincfg.func == RTS_FP_PIN_FUNC2) {
			value |= (1 << CS1_BRIDGE_EN_OFFSET);
		} else {
			value |=
				(pincfg.func << (pincfg.pin - RTS_FP_PIN_AL0 + AL_GPIO_SEL_OFFSET));
		}

		value |= (pincfg.pulldown
			  << (pincfg.pin - RTS_FP_PIN_AL0 + AL_GPIO_PD_CTRL_OFFSET)) |
			 (pincfg.pullup << (pincfg.pin - RTS_FP_PIN_AL0 + AL_GPIO_PU_CTRL_OFFSET));

		sys_write32(value, base);
	} else if (pincfg.pin >= RTS_FP_PIN_SNR_RST && pincfg.pin <= RTS_FP_PIN_SNR_CS) {
		base = DT_REG_ADDR_BY_IDX(DT_NODELABEL(pinctrl), 2) +
		       (pincfg.pin - RTS_FP_PIN_SNR_RST) * 0x4;

		value = sys_read32(base);

		value &= ~(SENSOR_SCS_N_SEL_MASK | SENSOR_SCS_N_PDE_MASK | SENSOR_SCS_N_PUE_MASK |
			   SENSOR_SCS_N_H3L1_MASK | SENSOR_SCS_N_IEV18_MASK);

		value |= (pincfg.func << SENSOR_SCS_N_SEL_OFFSET) |
			 (pincfg.pulldown << SENSOR_SCS_N_PDE_OFFSET) |
			 (pincfg.pullup << SENSOR_SCS_N_PUE_OFFSET) |
			 (h3l1 << SENSOR_SCS_N_H3L1_OFFSET) | (iev18 << SENSOR_SCS_N_IEV18_OFFSET);

		sys_write32(value, base);
	} else if (pincfg.pin == RTS_FP_PIN_SNR_GPIO) {
		base = DT_REG_ADDR_BY_IDX(DT_NODELABEL(pinctrl), 2) + 0xC;

		value = sys_read32(base);

		value &= ~(GPIO_SVIO_PULLCTL_MASK | GPIO_SVIO_IEV18_MASK | GPIO_SVIO_H3L1_MASK);

		value |= (pincfg.pulldown << GPIO_SVIO_PULLCTL_OFFSET) |
			 (pincfg.pullup << (GPIO_SVIO_PULLCTL_OFFSET + 1)) |
			 (iev18 << GPIO_SVIO_IEV18_OFFSET) | (h3l1 << GPIO_SVIO_H3L1_OFFSET);

		sys_write32(value, base);
	} else {
		base = DT_REG_ADDR(DT_NODELABEL(pinctrl)) + pincfg.pin * PAD_CFG_SIZE;

		value = sys_read32(base + PAD_CFG);
		value &= ~(GPIO_FUNCTION_SEL_MASK | IEV18_MASK | H3L1_MASK | PU_MASK | PD_MASK);
		value |= (((1 << pincfg.func) << GPIO_FUNCTION_SEL_OFFSET) |
			  (pincfg.pulldown << PD_OFFSET) | (pincfg.pullup << PU_OFFSET) |
			  (iev18 << IEV18_OFFSET) | (h3l1 << H3L1_OFFSET));

		if (pincfg.pin == RTS_FP_PIN_CACHE_CS2) {
			sys_write32(0x1, base + PAD_GPIO_INC);
		}

		sys_write32(value, base);
	}
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (int i = 0; i < pin_cnt; i++) {
		pinctrl_configure_pin(pins[i]);
	}

	return 0;
}
