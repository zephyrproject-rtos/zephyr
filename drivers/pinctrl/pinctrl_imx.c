/*
 * Copyright (c) 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <soc.h>


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
#if defined(CONFIG_SOC_SERIES_IMX_RT10XX) || defined(CONFIG_SOC_SERIES_IMX_RT11XX)
		volatile uint32_t *gpr_register =
			(volatile uint32_t *)pins[i].pinmux.gpr_register;
		if (gpr_register) {
			/* Set or clear specified GPR bit for this mux */
			if (pins[i].pinmux.gpr_val) {
				*gpr_register |=
					(pins[i].pinmux.gpr_val << pins[i].pinmux.gpr_shift);
			} else {
				*gpr_register &= ~(0x1 << pins[i].pinmux.gpr_shift);
			}
		}
#endif
		*((volatile uint32_t *)mux_register) = IOMUXC_SW_MUX_CTL_PAD_MUX_MODE(mux_mode) |
			IOMUXC_SW_MUX_CTL_PAD_SION(MCUX_IMX_INPUT_ENABLE(pin_ctrl_flags));
		if (input_register) {
			*((volatile uint32_t *)input_register) =
				IOMUXC_SELECT_INPUT_DAISY(input_daisy);
		}
		if (config_register) {
			*((volatile uint32_t *)config_register) =
				pin_ctrl_flags & (~(0x1 << MCUX_IMX_INPUT_ENABLE_SHIFT));
		}


	}
	return 0;
}

static int imx_pinctrl_init(const struct device *dev)
{
	ARG_UNUSED(dev);
#ifdef CONFIG_SOC_SERIES_IMX_RT
	CLOCK_EnableClock(kCLOCK_Iomuxc);
#ifdef CONFIG_SOC_SERIES_IMX_RT10XX
	CLOCK_EnableClock(kCLOCK_IomuxcSnvs);
	CLOCK_EnableClock(kCLOCK_IomuxcGpr);
#elif defined(CONFIG_SOC_SERIES_IMX_RT11XX)
	CLOCK_EnableClock(kCLOCK_Iomuxc_Lpsr);
#endif /* CONFIG_SOC_SERIES_IMX_RT10XX */
#elif defined(CONFIG_SOC_MIMX8MQ6)
	CLOCK_EnableClock(kCLOCK_Iomux);
#endif /* CONFIG_SOC_SERIES_IMX_RT */

	return 0;
}

SYS_INIT(imx_pinctrl_init, PRE_KERNEL_1, 0);
