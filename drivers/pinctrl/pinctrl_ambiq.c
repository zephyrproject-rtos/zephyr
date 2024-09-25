/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>

/* ambiq-sdk includes */
#include <am_mcu_apollo.h>

static void pinctrl_configure_pin(const pinctrl_soc_pin_t *pin)
{
	am_hal_gpio_pincfg_t pin_config = {0};

#if defined(CONFIG_SOC_SERIES_APOLLO3X)
	pin_config.uFuncSel = pin->alt_func;
	pin_config.eGPInput =
		pin->input_enable ? AM_HAL_GPIO_PIN_INPUT_ENABLE : AM_HAL_GPIO_PIN_INPUT_NONE;
	pin_config.eGPOutcfg = pin->push_pull    ? AM_HAL_GPIO_PIN_OUTCFG_PUSHPULL
			       : pin->open_drain ? AM_HAL_GPIO_PIN_OUTCFG_OPENDRAIN
			       : pin->tristate   ? AM_HAL_GPIO_PIN_OUTCFG_TRISTATE
						 : AM_HAL_GPIO_PIN_OUTCFG_DISABLE;
	pin_config.eDriveStrength = pin->drive_strength;
	pin_config.uNCE = pin->iom_nce;
#if defined(CONFIG_SOC_APOLLO3P_BLUE)
	pin_config.bIomMSPIn = pin->iom_mspi;
#endif
	pin_config.uIOMnum = pin->iom_num;

	if (pin->bias_pull_up) {
		pin_config.ePullup = pin->ambiq_pull_up_ohms + AM_HAL_GPIO_PIN_PULLUP_1_5K;
	} else if (pin->bias_pull_down) {
		pin_config.ePullup = AM_HAL_GPIO_PIN_PULLDOWN;
	}
#else
	pin_config.GP.cfg_b.uFuncSel = pin->alt_func;
	pin_config.GP.cfg_b.eGPInput =
		pin->input_enable ? AM_HAL_GPIO_PIN_INPUT_ENABLE : AM_HAL_GPIO_PIN_INPUT_NONE;
	pin_config.GP.cfg_b.eGPOutCfg = pin->push_pull    ? AM_HAL_GPIO_PIN_OUTCFG_PUSHPULL
					: pin->open_drain ? AM_HAL_GPIO_PIN_OUTCFG_OPENDRAIN
					: pin->tristate   ? AM_HAL_GPIO_PIN_OUTCFG_TRISTATE
							  : AM_HAL_GPIO_PIN_OUTCFG_DISABLE;
	pin_config.GP.cfg_b.eDriveStrength = pin->drive_strength;
	pin_config.GP.cfg_b.uSlewRate = pin->slew_rate;
	pin_config.GP.cfg_b.uNCE = pin->iom_nce;

	if (pin->bias_pull_up) {
		pin_config.GP.cfg_b.ePullup = pin->ambiq_pull_up_ohms + AM_HAL_GPIO_PIN_PULLUP_1_5K;
	} else if (pin->bias_pull_down) {
		pin_config.GP.cfg_b.ePullup = AM_HAL_GPIO_PIN_PULLDOWN_50K;
	}
#endif
	am_hal_gpio_pinconfig(pin->pin_num, pin_config);
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		pinctrl_configure_pin(pins++);
	}

	return 0;
}
