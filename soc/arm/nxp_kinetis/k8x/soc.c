/*
 * Copyright (c) 2019 SEAL AG
 *
 * Based on NXP K6x soc.c, which is:
 * Copyright (c) 2014-2015 Wind River Systems, Inc.
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <fsl_common.h>
#include <fsl_clock.h>

#define PERIPH_CLK_PLLFLLSEL	(1)
#define PERIPH_CLK_OSCERCLK	(2)
#define PERIPH_CLK_MCGIRCLK	(3)

#define RUNM_RUN		(0)
#define RUNM_VLPR		(2)
#define RUNM_HSRUN		(3)

static const osc_config_t osc_config = {
	.freq = CONFIG_OSC_XTAL0_FREQ,
	.capLoad = 0,

#if defined(CONFIG_OSC_EXTERNAL)
	.workMode = kOSC_ModeExt,
#elif defined(CONFIG_OSC_LOW_POWER)
	.workMode = kOSC_ModeOscLowPower,
#elif defined(CONFIG_OSC_HIGH_GAIN)
	.workMode = kOSC_ModeOscHighGain,
#else
#error "An oscillator mode must be defined"
#endif

	.oscerConfig = {
		.enableMode = kOSC_ErClkEnable,
#if (defined(FSL_FEATURE_OSC_HAS_EXT_REF_CLOCK_DIVIDER) && \
	FSL_FEATURE_OSC_HAS_EXT_REF_CLOCK_DIVIDER)
		.erclkDiv = 0U,
#endif
	},
};

static const mcg_pll_config_t pll0_config = {
	.enableMode = 0U,
	.prdiv = CONFIG_MCG_PRDIV0,
	.vdiv = CONFIG_MCG_VDIV0,
};

static const sim_clock_config_t sim_config = {
	.pllFllSel = DT_PROP(DT_INST(0, nxp_kinetis_sim), pllfll_select),
	.er32kSrc = DT_PROP(DT_INST(0, nxp_kinetis_sim), er32k_select),
	.clkdiv1 = SIM_CLKDIV1_OUTDIV1(CONFIG_K8X_CORE_CLOCK_DIVIDER - 1) |
		   SIM_CLKDIV1_OUTDIV2(CONFIG_K8X_BUS_CLOCK_DIVIDER - 1) |
		   SIM_CLKDIV1_OUTDIV3(CONFIG_K8X_FLEXBUS_CLOCK_DIVIDER - 1) |
		   SIM_CLKDIV1_OUTDIV4(CONFIG_K8X_FLASH_CLOCK_DIVIDER - 1),
	/* Divide PLL output frequency by 2 for peripherals */
	.pllFllDiv = (1),
	.pllFllFrac = (0),
};

static ALWAYS_INLINE void clk_init(void)
{
	CLOCK_SetSimSafeDivs();

	CLOCK_InitOsc0(&osc_config);
	CLOCK_SetXtal0Freq(CONFIG_OSC_XTAL0_FREQ);

	CLOCK_BootToPeeMode(kMCG_OscselOsc, kMCG_PllClkSelPll0, &pll0_config);

	CLOCK_SetInternalRefClkConfig(kMCG_IrclkEnable, kMCG_IrcSlow,
				      CONFIG_MCG_FCRDIV);

	CLOCK_SetSimConfig(&sim_config);

#if CONFIG_UART_MCUX_LPUART
	CLOCK_SetLpuartClock(PERIPH_CLK_PLLFLLSEL);
#endif

#if CONFIG_USB_KINETIS
	CLOCK_EnableUsbfs0Clock(kCLOCK_UsbSrcPll0, 120000000UL);
#endif
}

static int k8x_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	unsigned int old_level; /* old interrupt lock level */
#if !defined(CONFIG_ARM_MPU)
	uint32_t temp_reg;
#endif /* !CONFIG_ARM_MPU */

	/* Disable interrupts */
	old_level = irq_lock();

	/* release I/O power hold to allow normal run state */
	PMC->REGSC |= PMC_REGSC_ACKISO_MASK;

#if !defined(CONFIG_ARM_MPU)
	/*
	 * Disable memory protection and clear slave port errors.
	 * Note that the K8x does not implement the optional ARMv7-M memory
	 * protection unit (MPU), specified by the architecture (PMSAv7), in the
	 * Cortex-M4 core.  Instead, the processor includes its own MPU module.
	 */
	temp_reg = SYSMPU->CESR;
	temp_reg &= ~SYSMPU_CESR_VLD_MASK;
	temp_reg |= SYSMPU_CESR_SPERR_MASK;
	SYSMPU->CESR = temp_reg;
#endif /* !CONFIG_ARM_MPU */

	/* Initialize system clocks and PLL */
	clk_init();

	/*
	 * Install default handler that simply resets the CPU if
	 * configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

	/* Restore interrupt state */
	irq_unlock(old_level);

	return 0;
}

SYS_INIT(k8x_init, PRE_KERNEL_1, 0);
