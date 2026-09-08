/*
 * Copyright (c) 2025-2026 Conclusive Engineering Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc35xx_pinctrl

#include <zephyr/arch/cpu.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/pinctrl/ti-cc35xx-pinctrl.h>

#define MEM_GPIO_PAD_CONFIG_MSK 0xFFF
#define MEM_GPIO0_PCFG_OFFSET   0x0002D004

#define MEM_GPIO0_CFG_OFFSET    0
#define MEM_GPIO0_PCTL_OFFSET   4
#define MEM_GPIO0_CTL_OFFSET    8
#define MEM_GPIO_NEXT_OFFSET    0x1000
#define MEM_GPIO_ANASWOVREN_BIT 9
#define MEM_GPIO_ANASW_BIT      10
#define MEM_GPIO_IE_BIT         11
#define MEM_GPIO_OUTDIS_BIT     12
#define MEM_GPIO_ODISOVEN_BIT   13
#define MEM_GPIO_PCTRL_OVR_BIT  8

static void pinctrl_configure_pin(pinctrl_soc_pin_t pincfg)
{
	uint8_t pin;
	mem_addr_t reg;

	uint32_t tmp;

	pin = (pincfg >> TI_CC35XX_PIN_POS) & TI_CC35XX_PIN_MSK;

	/* Set pin function */
	reg = DT_INST_REG_ADDR(0) + MEM_GPIO0_PCFG_OFFSET + pin * sizeof(uint32_t);
	sys_write32(pincfg & TI_CC35XX_MUX_MSK, reg);

	/* Disable both input and output function of pin */
	reg = DT_INST_REG_ADDR(0) + pin * MEM_GPIO_NEXT_OFFSET + MEM_GPIO0_CFG_OFFSET;
	tmp = sys_read32(reg);
	tmp &= ~BIT(MEM_GPIO_IE_BIT);
	tmp |= BIT(MEM_GPIO_OUTDIS_BIT);
	tmp &= ~BIT(MEM_GPIO_ODISOVEN_BIT);
	if (pincfg & TI_CC35XX_PINCTRL_DIR_IN) {
		tmp |= BIT(MEM_GPIO_IE_BIT);
	}
	if (pincfg & TI_CC35XX_PINCTRL_DIR_OUT) {
		tmp &= ~BIT(MEM_GPIO_OUTDIS_BIT);
	}
	sys_write32(tmp, reg);

	if ((pincfg & TI_CC35XX_MUX_MSK) == TI_CC35XX_ANALOG_MUX) {
		/*
		 * Enable analog pin function for adc input.
		 * These must be a separate writes
		 * to prevent glitches in the hardware.
		 */
		tmp = sys_read32(reg);
		tmp &= ~BIT(MEM_GPIO_ANASW_BIT);
		sys_write32(tmp, reg);

		tmp = sys_read32(reg);
		tmp &= ~BIT(MEM_GPIO_ANASWOVREN_BIT);
		sys_write32(tmp, reg);
	}

	tmp = (pincfg >> TI_CC35XX_PU_PD_POS) & TI_CC35XX_PU_PD_MSK;
	reg = DT_INST_REG_ADDR(0) + pin * MEM_GPIO_NEXT_OFFSET + MEM_GPIO0_PCTL_OFFSET;
	sys_write32(tmp, reg);

	tmp = TI_CC35XX_PINCTRL_OVR_VAL(pincfg);
	reg = DT_INST_REG_ADDR(0) + pin * MEM_GPIO_NEXT_OFFSET + MEM_GPIO0_CTL_OFFSET;
	if (tmp != TI_CC35XX_PINCTRL_OVR_NO_CHANGE) {
		tmp = (tmp == TI_CC35XX_PINCTRL_OVR_DISABLE) ? 0 : tmp << MEM_GPIO_PCTRL_OVR_BIT;
		sys_write32(tmp, reg);
	}
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0; i < pin_cnt; i++) {
		pinctrl_configure_pin(pins[i]);
	}

	/*
	 * Small delay to ensure pin configuration is applied
	 * before any peripheral tries to use the pins
	 */
	for (int i = 0; i < 1000; i++) {
		__asm__ volatile("nop");
	}

	return 0;
}
