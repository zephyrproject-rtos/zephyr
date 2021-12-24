/*
 * Copyright (c) 2021 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/pinctrl.h>
#include <soc.h>
#include <fsl_iomuxc.h>
#include <fsl_gpio.h>

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	uint8_t i;

	/* configure all pins */
	for (i = 0U; i < pin_cnt; i++) {
		uint32_t mux_register = pins[i].pinmux.mux_register;
		uint32_t mux_mode = pins[i].pinmux.mux_mode;
		uint32_t input_register = pins[i].pinmux.input_register;
		uint32_t input_daisy = pins[i].pinmux.input_daisy;
		uint32_t config_register = pins[i].pinmux.config_register;
		uint32_t input_on_field = pins[i].pinmux.input_on_field;
		uint32_t pin_ctrl_flags = pins[i].pin_ctrl_flags;

		IOMUXC_SetPinMux(mux_register, mux_mode, input_register,
			input_daisy, config_register, input_on_field);
		IOMUXC_SetPinConfig(mux_register, mux_mode, input_register,
			input_daisy, config_register, pin_ctrl_flags);
	}

	return 0;
}

static int mcux_pinctrl_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	CLOCK_EnableClock(kCLOCK_Iomuxc);
	CLOCK_EnableClock(kCLOCK_IomuxcSnvs);

	return 0;
}

SYS_INIT(mcux_pinctrl_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
