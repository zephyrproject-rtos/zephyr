/*
 * Copyright (c) 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/init.h>
#include <zephyr/drivers/pinctrl.h>

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
#if defined(CONFIG_SOC_SERIES_IMXRT10XX) || defined(CONFIG_SOC_SERIES_IMXRT11XX)
		volatile uint32_t *gpr_register =
			(volatile uint32_t *)((uintptr_t)pins[i].pinmux.gpr_register);
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

#ifdef CONFIG_SOC_MIMX9352_A55
		sys_write32(IOMUXC1_SW_MUX_CTL_PAD_MUX_MODE(mux_mode) |
			IOMUXC1_SW_MUX_CTL_PAD_SION(MCUX_IMX_INPUT_ENABLE(pin_ctrl_flags)),
			(mem_addr_t)mux_register);
		if (input_register) {
			sys_write32(IOMUXC1_SELECT_INPUT_DAISY(input_daisy),
				    (mem_addr_t)input_register);
		}
		if (config_register) {
			sys_write32(pin_ctrl_flags & (~(0x1 << MCUX_IMX_INPUT_ENABLE_SHIFT)),
				    (mem_addr_t)config_register);
		}
#else
		sys_write32(
			IOMUXC_SW_MUX_CTL_PAD_MUX_MODE(mux_mode) |
				IOMUXC_SW_MUX_CTL_PAD_SION(MCUX_IMX_INPUT_ENABLE(pin_ctrl_flags)),
			(mem_addr_t)mux_register);
		if (input_register) {
			sys_write32(IOMUXC_SELECT_INPUT_DAISY(input_daisy),
				    (mem_addr_t)input_register);
		}
		if (config_register) {
			sys_write32(pin_ctrl_flags & (~(0x1 << MCUX_IMX_INPUT_ENABLE_SHIFT)),
				    config_register);
		}
#endif
	}
	return 0;
}

static int imx_pinctrl_init(void)
{
#if defined(CONFIG_SOC_SERIES_IMXRT10XX) || defined(CONFIG_SOC_SERIES_IMXRT11XX)
	CLOCK_EnableClock(kCLOCK_Iomuxc);
#ifdef CONFIG_SOC_SERIES_IMXRT10XX
	CLOCK_EnableClock(kCLOCK_IomuxcSnvs);
	CLOCK_EnableClock(kCLOCK_IomuxcGpr);
#elif defined(CONFIG_SOC_SERIES_IMXRT11XX)
	CLOCK_EnableClock(kCLOCK_Iomuxc_Lpsr);
#endif /* CONFIG_SOC_SERIES_IMXRT10XX */
#elif defined(CONFIG_SOC_MIMX8MQ6)
	CLOCK_EnableClock(kCLOCK_Iomux);
#endif /* CONFIG_SOC_SERIES_IMXRT10XX || CONFIG_SOC_SERIES_IMXRT11XX */

	return 0;
}

SYS_INIT(imx_pinctrl_init, PRE_KERNEL_1, 0);
