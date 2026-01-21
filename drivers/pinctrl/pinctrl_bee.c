/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>

#include <rtl_pinmux.h>

static void pinctrl_configure_pin(const pinctrl_soc_pin_t *pin)
{
	uint16_t cfg_fun = pin[0].fun;
	uint16_t cfg_pin = pin[0].pin;
	uint8_t cfg_dir = pin[0].dir ? PAD_OUT_ENABLE : PAD_OUT_DISABLE;
	uint8_t cfg_drv = pin[0].drive ? PAD_OUT_HIGH : PAD_OUT_LOW;
	uint8_t cfg_pull;
	uint8_t cfg_pull_strength = pin[0].pull_strength;
	uint8_t current_level = pin[0].current_level;

	if (pin[0].pull_dis) {
		cfg_pull = PAD_PULL_NONE;
	} else if (pin[0].pull_dir) {
		cfg_pull = PAD_PULL_UP;
	} else {
		cfg_pull = PAD_PULL_DOWN;
	}

	/* set pull strength */
	Pad_SetPullStrength(cfg_pin, cfg_pull_strength);

	/* set current level */
	switch (current_level) {
	case 0:
		Pad_SetDrivingCurrent(cfg_pin, LEVEL0);
		break;

	case 1:
		Pad_SetDrivingCurrent(cfg_pin, LEVEL1);
		break;

	case 2:
		Pad_SetDrivingCurrent(cfg_pin, LEVEL2);
		break;

	case 3:
		Pad_SetDrivingCurrent(cfg_pin, LEVEL3);
		break;

	default:
		break;
	}

	/* set pad and pinmux */
	if (cfg_fun == BEE_PWR_OFF) {
		Pad_Config(cfg_pin, PAD_SW_MODE, PAD_NOT_PWRON, cfg_pull, cfg_dir, cfg_drv);
	} else if (cfg_fun == BEE_SW_MODE) {
		Pad_Config(cfg_pin, PAD_SW_MODE, PAD_IS_PWRON, cfg_pull, cfg_dir, cfg_drv);
	} else if (cfg_fun < BEE_PINMUX_MAX) {
		Pad_Config(cfg_pin, PAD_PINMUX_MODE, PAD_IS_PWRON, cfg_pull, cfg_dir, cfg_drv);
		Pinmux_Config(cfg_pin, cfg_fun);
	} else if (cfg_fun > BEE_PWR_OFF) {
		if (cfg_fun <= BEE_SDHC1_D7_P4_7) {
			Pad_Config(cfg_pin, PAD_PINMUX_MODE, PAD_IS_PWRON, cfg_pull, cfg_dir,
				   cfg_drv);
			Pad_Dedicated_Config(cfg_pin, ENABLE);
			Pinmux_HS_Config(SDHC_HS_MUX);
		} else {
			Pad_Config(cfg_pin, PAD_PINMUX_MODE, PAD_IS_PWRON, cfg_pull, cfg_dir,
				   cfg_drv);
			Pinmux_AON_Config(cfg_fun);
		}
	} else {
		__ASSERT(false, "Pinctrl configuration fail");
	}
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		pinctrl_configure_pin(&pins[i]);
	}

	return 0;
}
