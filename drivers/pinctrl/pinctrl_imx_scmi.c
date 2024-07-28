/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/firmware/scmi/pinctrl.h>

static int scmi_pinctrl_configure_pin(const pinctrl_soc_pin_t *pin)
{
	struct scmi_pinctrl_settings settings[4];
	int ret, settings_num, i;
	uint32_t attributes;

	settings_num = 0;

	attributes = SCMI_PINCTRL_CONFIG_ATTRIBUTES(0x0, 0x1, SCMI_PINCTRL_SELECTOR_PIN);

	/* set mux value */
	settings[0].attributes = attributes;
	settings[0].id = (pin->pinmux.mux_register - IOMUXC_MUXREG) / 4;
	settings[0].config[0] = PIN_CONFIG_TYPE_MUX;
	settings[0].config[1] = IOMUXC_INPUT_ENABLE(pin->pin_ctrl_flags) ?
		(pin->pinmux.mux_mode | IOMUXC_SION(1)) :
		pin->pinmux.mux_mode;
	settings_num++;

	/* set config value */
	settings[1].attributes = attributes;
	settings[1].id = (pin->pinmux.config_register - IOMUXC_CFGREG) / 4;
	settings[1].config[0] = PIN_CONFIG_TYPE_CONFIG;
	settings[1].config[1] = pin->pin_ctrl_flags & ~IOMUXC_SION(1);
	settings_num++;

	/* set daisy - optional */
	if (pin->pinmux.input_register) {
		settings[2].attributes = attributes;
		settings[2].id = (pin->pinmux.input_register - IOMUXC_DAISYREG) / 4;
		settings[2].config[0] = PIN_CONFIG_TYPE_DAISY_ID;
		settings[2].config[1] = 0x0; /* not relevant */
		settings_num++;

		settings[3].attributes = attributes;
		settings[3].id = 0x0; /* not relevant */
		settings[3].config[0] = PIN_CONFIG_TYPE_DAISY_CFG;
		settings[3].config[1] = pin->pinmux.input_daisy;
		settings_num++;
	}

	for (i = 0; i < settings_num; i++) {
		ret = scmi_pinctrl_settings_configure(&settings[i]);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
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
