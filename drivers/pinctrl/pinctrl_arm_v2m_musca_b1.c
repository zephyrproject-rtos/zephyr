/*
 * Copyright 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Implementation of pinctrl for the Musca-B1 board.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/devicetree/gpio.h>
#include <zephyr/drivers/gpio/gpio_cmsdk_ahb.h>

#define IOMUX_MAIN_INSEL       (0x68 >> 2)
#define IOMUX_MAIN_OUTSEL      (0x70 >> 2)
#define IOMUX_MAIN_OENSEL      (0x78 >> 2)
#define IOMUX_MAIN_DEFAULT_IN  (0x80 >> 2)
#define IOMUX_ALTF1_INSEL      (0x88 >> 2)
#define IOMUX_ALTF1_OUTSEL     (0x90 >> 2)
#define IOMUX_ALTF1_OENSEL     (0x98 >> 2)
#define IOMUX_ALTF1_DEFAULT_IN (0xA0 >> 2)
#define IOMUX_ALTF2_INSEL      (0xA8 >> 2)
#define IOMUX_ALTF2_OUTSEL     (0xB0 >> 2)
#define IOMUX_ALTF2_OENSEL     (0xB8 >> 2)
#define IOMUX_ALTF2_DEFAULT_IN (0xC0 >> 2)

static int pinctrl_configure_pin(const pinctrl_soc_pin_t *pin)
{
	volatile uint32_t *scc = (uint32_t *)DT_REG_ADDR(DT_INST(0, arm_scc));

	if (pin->alt_func) {
		/*
		 * Enable ALTF1 for this pin.
		 */
		scc[IOMUX_MAIN_OUTSEL] &= ~BIT(pin->pin_num);
		scc[IOMUX_MAIN_OENSEL] &= ~BIT(pin->pin_num);
		scc[IOMUX_MAIN_INSEL] &= ~BIT(pin->pin_num);

		/*
		 * NB Bits in ALTF1_INSEL must be 0 to select ALTF1, while the OUTSEL and
		 * OENSEL registers must be 1.
		 */
		scc[IOMUX_ALTF1_INSEL] &= ~BIT(pin->pin_num);
		scc[IOMUX_ALTF1_OUTSEL] |= BIT(pin->pin_num);
		scc[IOMUX_ALTF1_OENSEL] |= BIT(pin->pin_num);
	} else {
		/*
		 * Pin is not explicitly configured, so should be configured as GPIO.
		 * This is also the correct configuration for the status LEDs, which
		 * are connected to GPIO 2, 3 and 4.
		 */
		scc[IOMUX_MAIN_OUTSEL] |= BIT(pin->pin_num);
		scc[IOMUX_MAIN_OENSEL] |= BIT(pin->pin_num);
		scc[IOMUX_MAIN_INSEL] |= BIT(pin->pin_num);
	}

	return 0;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);
	for (uint8_t i = 0U; i < pin_cnt; i++) {
		if (pinctrl_configure_pin(&pins[i]) == -ENOTSUP) {
			return -ENOTSUP;
		}
	}

	return 0;
}
