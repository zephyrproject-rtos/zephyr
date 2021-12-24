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
	/* configure all pins */
	for (uint8_t i = 0U; i < pin_cnt; i++) {
		uint32_t mux_register = pins[i].pinmux.mux_register;
		uint32_t mux_mode = pins[i].pinmux.mux_mode;
		uint32_t input_register = pins[i].pinmux.input_register;
		uint32_t input_daisy = pins[i].pinmux.input_daisy;
		uint32_t pin_ctrl_flags = pins[i].pin_ctrl_flags;
		volatile uint32_t *config_register = (uint32_t *)pins[i].pinmux.config_register;
		uint32_t config_val = *config_register;



		IOMUXC_SetPinMux(mux_register, mux_mode, input_register,
			input_daisy, (uint32_t)config_register,
			MCUX_RT_INPUT_ENABLE(pin_ctrl_flags));

		if (MCUX_RT_INPUT_SCHMITT_ENABLE(pin_ctrl_flags)) {
			config_val |= IOMUXC_SW_PAD_CTL_PAD_HYS(1);
		}
		if (MCUX_RT_DRIVE_OPEN_DRAIN(pin_ctrl_flags)) {
			config_val |= IOMUXC_SW_PAD_CTL_PAD_ODE(1);
		}
		if (MCUX_RT_BIAS_BUS_HOLD(pin_ctrl_flags)) {
			/* Set pull keeper select to keeper, and pull keeper enable to 1 */
			config_val |= IOMUXC_SW_PAD_CTL_PAD_PKE(1);
			config_val &= ~IOMUXC_SW_PAD_CTL_PAD_PUE_MASK;
		}
		if (MCUX_RT_BIAS_PULL_DOWN(pin_ctrl_flags)) {
			/* Set pull keeper select to pull, and pull keeper enable to 1 */
			config_val |= IOMUXC_SW_PAD_CTL_PAD_PKE(1) | IOMUXC_SW_PAD_CTL_PAD_PUE(1);
			/* Set PUS to 0b00 to select pulldown resistor */
			config_val &= ~IOMUXC_SW_PAD_CTL_PAD_PUS_MASK;
		}
		if (MCUX_RT_BIAS_PULL_UP(pin_ctrl_flags)) {
			/* Set pull keeper select to pull, and pull keeper enable to 1 */
			config_val |= IOMUXC_SW_PAD_CTL_PAD_PKE(1) | IOMUXC_SW_PAD_CTL_PAD_PUE(1);
			/* Set PUS field to selected value */
			config_val = ((config_val & ~IOMUXC_SW_PAD_CTL_PAD_PUS_MASK) |
				IOMUXC_SW_PAD_CTL_PAD_PUS(MCUX_RT_BIAS_PULL_UP(pin_ctrl_flags)));
		}
		/* Set drive strength field */
		config_val = ((config_val & ~IOMUXC_SW_PAD_CTL_PAD_DSE_MASK) |
			IOMUXC_SW_PAD_CTL_PAD_DSE(MCUX_RT_DRIVE_STRENGTH(pin_ctrl_flags)));
		/* Set speed field */
		config_val = ((config_val & ~IOMUXC_SW_PAD_CTL_PAD_SPEED_MASK) |
			IOMUXC_SW_PAD_CTL_PAD_SPEED(MCUX_RT_SPEED(pin_ctrl_flags)));
		/* Set slew rate field */
		config_val = ((config_val & ~IOMUXC_SW_PAD_CTL_PAD_SRE_MASK) |
			IOMUXC_SW_PAD_CTL_PAD_SRE(MCUX_RT_SLEW_RATE(pin_ctrl_flags)));
		/* Write out config value */
		*config_register = config_val;
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
