/*
 * Copyright 2024 NXP
 *
 * Authors:
 *  Hou Zhiqiang <Zhiqiang.Hou@nxp.com>
 *  Yangbo Lu <yangbo.lu@nxp.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/init.h>
#include <zephyr/drivers/pinctrl.h>
#include "scmi_pinctrl.h"
#include "scmi_common.h"

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	/* configure all pins */
	for (uint8_t i = 0U; i < pin_cnt; i++) {
		uint32_t mux_register = pins[i].pinmux.mux_register;
		uint32_t mux_mode = pins[i].pinmux.mux_mode;
		uint32_t input_register = pins[i].pinmux.input_register;
		uint32_t input_daisy = pins[i].pinmux.input_daisy;
		uint32_t config_register = pins[i].pinmux.config_register;
		uint32_t pin_ctrl_flags = pins[i].pin_ctrl_flags;

		uint32_t status = SCMI_ERR_SUCCESS;
		scmi_pin_config_t configs[3];
		uint32_t num = 0;
		uint32_t attributes;

		/* pinmux set */
		if (mux_register) {
			configs[num].type = SCMI_PINCTRL_TYPE_MUX;
			configs[num].value = IOMUXC_MUX_MODE(mux_mode) |
				IOMUXC_SION(MCUX_IMX_INPUT_ENABLE(pin_ctrl_flags));
			num++;

		}

		if (input_register) {
			configs[num].type = SCMI_PINCTRL_TYPE_DAISY_ID;
			configs[num].value = (input_register - IOMUXC_DAISYREG) / 4;
			num++;

			configs[num].type = SCMI_PINCTRL_TYPE_DAISY_CFG;
			configs[num].value = input_daisy;
			num++;
		}

		if (num) {
			attributes = SCMI_PINCTRL_SET_ATTR_SELECTOR(SCMI_PINCTRL_SEL_PIN) |
				SCMI_PINCTRL_SET_ATTR_NUM_CONFIGS(num);
			status = SCMI_PinctrlSettingsConfigure(SM_A2P,
					(mux_register - IOMUXC_MUXREG) / 4,
					0, attributes, configs);
			if (status != SCMI_ERR_SUCCESS) {
				return -EIO;
			}
		}

		/* pincfg set */
		if (config_register) {
			attributes = SCMI_PINCTRL_SET_ATTR_SELECTOR(SCMI_PINCTRL_SEL_PIN) |
				SCMI_PINCTRL_SET_ATTR_NUM_CONFIGS(1);
			configs[0].type = SCMI_PINCTRL_TYPE_CONFIG;
			configs[0].value = pin_ctrl_flags & (~(1 << MCUX_IMX_INPUT_ENABLE_SHIFT));
			status = SCMI_PinctrlSettingsConfigure(SM_A2P,
					(config_register - IOMUXC_CFGREG) / 4,
					0, attributes, configs);
			if (status != SCMI_ERR_SUCCESS) {
				return -EIO;
			}
		}
	}

	return 0;
}
