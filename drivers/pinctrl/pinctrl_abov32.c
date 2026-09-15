/*
 * Copyright (c) 2026 ABOV Semiconductor Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>

/* Platform HLL headers */
#include <hll_pcu.h>
#include <type/pcu_type.h>

/* Static assertion checks to ensure pinctrl definitions match HLL definitions */
BUILD_ASSERT((ABOV_PUPD_NONE == PCU_PUPD_DISABLED) &&
	     (ABOV_PUPD_PULLUP == PCU_PUPD_UP) &&
	     (ABOV_PUPD_PULLDN == PCU_PUPD_DOWN),
	     "pinctrl pull-up/down definitions != HLL definitions");

/**
 * @brief Configure a single pin according to pinctrl_soc_pin_t settings using HLL API.
 *
 * @param pin Pin configuration encoded in pinctrl_soc_pin_t format.
 */
static void pinctrl_configure_pin(pinctrl_soc_pin_t pin)
{
	PCU_ID_e port = ABOV_PORT_GET(pin);
	uint32_t pin_num = (uint32_t)ABOV_PIN_GET(pin);
	uint8_t alt = (uint8_t)ABOV_ALT_GET(pin);
	uint8_t pupd = (uint8_t)ABOV_PUPD_GET(pin);
	uint32_t otype = ABOV_OTYPE_GET(pin);

	/* 1. Enable PCU protected register write access */
	HLL_PCU_SetWriteEnable();

	/*
	 * 2. Configure alternate function (pin multiplexing) via HLL. Once a
	 *    pin's ALT type is selected, the peripheral's own open-drain vs
	 *    push-pull behavior applies automatically -- there is no separate
	 *    software output-type selection for alt-function pins on this
	 *    chip (confirmed against the datasheet), so `otype` only matters
	 *    for plain-GPIO pins, not here.
	 */
	if (pin_num < 8U) {
		HLL_PCU_SetAlt1Type(port, pin_num, alt);
	} else {
		HLL_PCU_SetAlt2Type(port, pin_num, alt);
	}

	/* 3. Configure pull-up / pull-down mode via HLL */
	HLL_PCU_SetPullUpDown(port, pin_num, pupd);

	/* 4. Disable PCU protected register write access */
	HLL_PCU_SetWriteDisable();

	ARG_UNUSED(otype);
}

/**
 * @brief Configure a set of pins for a peripheral state.
 *
 * This API is invoked by the Zephyr pinctrl subsystem to configure all pins
 * assigned to a specific peripheral state (e.g., pinctrl-0 / default state).
 *
 * @param pins Array of pin configurations.
 * @param pin_cnt Number of pins in the array.
 * @param reg Peripheral base address (unused for ABOV PCU).
 *
 * @return 0 on success, or negative error code on failure.
 */
int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		pinctrl_configure_pin(pins[i]);
	}

	return 0;
}
