/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 * Copyright (c) 2025 Linumiz GmbH

 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>

/* ambiq-sdk includes */
#include <soc.h>

#if defined(CONFIG_SOC_SERIES_APOLLO2X)
static void pinctrl_configure_pin(const pinctrl_soc_pin_t *cfg)
{
	uint32_t config = 0;

	if (cfg->alt_func) {
		config |= AM_HAL_GPIO_FUNC(cfg->alt_func);
	}

	if (cfg->input_enable) {
		config |= AM_HAL_GPIO_INPEN;
	}

	switch (cfg->drive_strength) {
	case 2:
		config |= AM_HAL_GPIO_DRIVE_2MA;
		break;
	case 4:
		config |= AM_HAL_GPIO_DRIVE_4MA;
		break;
	case 8:
		config |= AM_HAL_GPIO_DRIVE_8MA;
		break;
	case 12:
		config |= AM_HAL_GPIO_DRIVE_12MA;
		break;
	}

	if (cfg->bias_pull_up) {

		switch (cfg->ambiq_pull_up_ohms) {
		case 1500:
			config |= AM_HAL_GPIO_PULL1_5K;
			break;
		case 6000:
			config |= AM_HAL_GPIO_PULL6K;
			break;
		case 12000:
			config |= AM_HAL_GPIO_PULL12K;
			break;
		case 24000:
			config |= AM_HAL_GPIO_PULL24K;
			break;
		}
	}

	if (cfg->open_drain) {
		config |= AM_HAL_GPIO_OUT_OPENDRAIN;
	}

	am_hal_gpio_pin_config(cfg->pin_num, config);
}
#else
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
	pin_config.uNCE = pin->nce;
	pin_config.eCEpol = pin->nce_pol;
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
#if defined(CONFIG_SOC_SERIES_APOLLO4X)
	pin_config.GP.cfg_b.uSlewRate = pin->slew_rate;
	switch (pin->sdif_cdwp) {
	case 1:
		am_hal_gpio_cd_pin_config(pin->pin_num);
		break;
	case 2:
		am_hal_gpio_wp_pin_config(pin->pin_num);
		break;
	default:
		/* not a sdif pin */
		break;
	}
#else
	switch (pin->sdif_cdwp) {
	case 1:
		am_hal_gpio_cd0_pin_config(pin->pin_num);
		break;
	case 2:
		am_hal_gpio_wp0_pin_config(pin->pin_num);
		break;
	case 3:
		am_hal_gpio_cd1_pin_config(pin->pin_num);
		break;
	case 4:
		am_hal_gpio_wp1_pin_config(pin->pin_num);
		break;
	default:
		/* not a sdif pin */
		break;
	}
#endif
	pin_config.GP.cfg_b.uNCE = pin->nce;
	pin_config.GP.cfg_b.eCEpol = pin->nce_pol;

	if (pin->bias_pull_up) {
		pin_config.GP.cfg_b.ePullup = pin->ambiq_pull_up_ohms + AM_HAL_GPIO_PIN_PULLUP_1_5K;
	} else if (pin->bias_pull_down) {
		pin_config.GP.cfg_b.ePullup = AM_HAL_GPIO_PIN_PULLDOWN_50K;
	}
#endif
	am_hal_gpio_pinconfig(pin->pin_num, pin_config);
}
#endif

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		pinctrl_configure_pin(pins++);
	}
	return 0;
}
