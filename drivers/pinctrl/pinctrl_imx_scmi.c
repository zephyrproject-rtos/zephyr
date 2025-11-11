/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/firmware/scmi/pinctrl.h>

static int scmi_pinctrl_configure_pin(const pinctrl_soc_pin_t *pin)
{
	struct scmi_pinctrl_settings settings;
	int ret, config_num;

	config_num = 0;

	/* set mux value, and daisy */
	settings.id = (pin->pinmux.mux_register - IOMUXC_MUXREG) / 4;
	settings.config[0] = PIN_CONFIG_TYPE_MUX;
	settings.config[1] = IOMUXC_INPUT_ENABLE(pin->pin_ctrl_flags)
				     ? (pin->pinmux.mux_mode | IOMUXC_SION(1))
				     : pin->pinmux.mux_mode;
	config_num++;

	if (pin->pinmux.input_register) {
		settings.config[2] = PIN_CONFIG_TYPE_DAISY_ID;
		settings.config[3] = (pin->pinmux.input_register - IOMUXC_DAISYREG) / 4;
		config_num++;

		settings.config[4] = PIN_CONFIG_TYPE_DAISY_CFG;
		settings.config[5] = pin->pinmux.input_daisy;
		config_num++;
	}

	settings.attributes =
		SCMI_PINCTRL_CONFIG_ATTRIBUTES(0x0, config_num, SCMI_PINCTRL_SELECTOR_PIN);

	ret = scmi_pinctrl_settings_configure(&settings);
	if (ret < 0) {
		return ret;
	}

	/* set config value */
	settings.attributes = SCMI_PINCTRL_CONFIG_ATTRIBUTES(0x0, 0x1, SCMI_PINCTRL_SELECTOR_PIN);
	settings.id = (pin->pinmux.config_register - IOMUXC_CFGREG) / 4;
	settings.config[0] = PIN_CONFIG_TYPE_CONFIG;
	settings.config[1] = pin->pin_ctrl_flags & (~(1 << IOMUXC_INPUT_ENABLE_SHIFT));

	ret = scmi_pinctrl_settings_configure(&settings);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	int ret;
	/* configure all pins */
	for (uint8_t i = 0U; i < pin_cnt; i++) {
		ret = scmi_pinctrl_configure_pin(&pins[i]);
		if (ret < 0) {
			return ret;
		}
	}
	return 0;
}
