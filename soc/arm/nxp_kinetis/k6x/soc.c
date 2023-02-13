/*
 * Copyright (c) 2014-2015 Wind River Systems, Inc.
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for fsl_frdm_k64f platform
 *
 * This module provides routines to initialize and support board-level
 * hardware for the fsl_frdm_k64f platform.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/drivers/uart.h>
#include <fsl_common.h>
#include <fsl_clock.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/arch/arm/aarch32/cortex_m/cmsis.h>

#define LPUART0SRC_OSCERCLK     (1)

#define TIMESRC_OSCERCLK        (2)

#define RUNM_HSRUN              (3)

#define CLOCK_NODEID(clk) \
	DT_CHILD(DT_INST(0, nxp_kinetis_sim), clk)

#define CLOCK_DIVIDER(clk) \
	DT_PROP_OR(CLOCK_NODEID(clk), clock_div, 1) - 1

static const osc_config_t oscConfig = {
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

static const mcg_pll_config_t pll0Config = {
	.enableMode = 0U,
	.prdiv = CONFIG_MCG_PRDIV0,
	.vdiv = CONFIG_MCG_VDIV0,
};

static const sim_clock_config_t simConfig = {
	.pllFllSel = DT_PROP(DT_INST(0, nxp_kinetis_sim), pllfll_select),
	.er32kSrc = DT_PROP(DT_INST(0, nxp_kinetis_sim), er32k_select),
	.clkdiv1 = SIM_CLKDIV1_OUTDIV1(CLOCK_DIVIDER(core_clk)) |
		   SIM_CLKDIV1_OUTDIV2(CLOCK_DIVIDER(bus_clk)) |
		   SIM_CLKDIV1_OUTDIV3(CLOCK_DIVIDER(flexbus_clk)) |
		   SIM_CLKDIV1_OUTDIV4(CLOCK_DIVIDER(flash_clk)),
};

/**
 *
 * @brief Initialize the system clock
 *
 * This routine will configure the multipurpose clock generator (MCG) to
 * set up the system clock.
 * The MCG has nine possible modes, including Stop mode.  This routine assumes
 * that the current MCG mode is FLL Engaged Internal (FEI), as from reset.
 * It transitions through the FLL Bypassed External (FBE) and
 * PLL Bypassed External (PBE) modes to get to the desired
 * PLL Engaged External (PEE) mode and generate the maximum 120 MHz system
 * clock.
 *
 */
static ALWAYS_INLINE void clock_init(void)
{
	CLOCK_SetSimSafeDivs();

	CLOCK_InitOsc0(&oscConfig);
	CLOCK_SetXtal0Freq(CONFIG_OSC_XTAL0_FREQ);

	CLOCK_BootToPeeMode(kMCG_OscselOsc, kMCG_PllClkSelPll0, &pll0Config);

	CLOCK_SetInternalRefClkConfig(kMCG_IrclkEnable, kMCG_IrcSlow,
				      CONFIG_MCG_FCRDIV);

	CLOCK_SetSimConfig(&simConfig);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(lpuart0), okay)
	CLOCK_SetLpuartClock(LPUART0SRC_OSCERCLK);
#endif

#if CONFIG_ETH_MCUX
	CLOCK_SetEnetTime0Clock(TIMESRC_OSCERCLK);
#endif
#if CONFIG_ETH_MCUX_RMII_EXT_CLK
	CLOCK_SetRmii0Clock(1);
#endif
#if CONFIG_USB_KINETIS || CONFIG_UDC_KINETIS
	CLOCK_EnableUsbfs0Clock(kCLOCK_UsbSrcPll0,
				DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency));
#endif
}

/**
 *
 * @brief Perform basic hardware initialization
 *
 * Initialize the interrupt controller device drivers.
 * Also initialize the timer device driver, if required.
 *
 * @return 0
 */

static int k6x_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	unsigned int oldLevel; /* old interrupt lock level */
#if !defined(CONFIG_ARM_MPU)
	uint32_t temp_reg;
#endif /* !CONFIG_ARM_MPU */

	/* disable interrupts */
	oldLevel = irq_lock();

	/* release I/O power hold to allow normal run state */
	PMC->REGSC |= PMC_REGSC_ACKISO_MASK;

#ifdef CONFIG_TEMP_KINETIS
	/* enable bandgap buffer */
	PMC->REGSC |= PMC_REGSC_BGBE_MASK;
#endif /* CONFIG_TEMP_KINETIS */

#if !defined(CONFIG_ARM_MPU)
	/*
	 * Disable memory protection and clear slave port errors.
	 * Note that the K64F does not implement the optional ARMv7-M memory
	 * protection unit (MPU), specified by the architecture (PMSAv7), in the
	 * Cortex-M4 core.  Instead, the processor includes its own MPU module.
	 */
	temp_reg = SYSMPU->CESR;
	temp_reg &= ~SYSMPU_CESR_VLD_MASK;
	temp_reg |= SYSMPU_CESR_SPERR_MASK;
	SYSMPU->CESR = temp_reg;
#endif /* !CONFIG_ARM_MPU */

#ifdef CONFIG_K6X_HSRUN
	/* Switch to HSRUN mode */
	SMC->PMPROT |= SMC_PMPROT_AHSRUN_MASK;
	SMC->PMCTRL = (SMC->PMCTRL & ~SMC_PMCTRL_RUNM_MASK) |
		SMC_PMCTRL_RUNM(RUNM_HSRUN);
#endif
	/* Initialize PLL/system clock up to 180 MHz */
	clock_init();

	/*
	 * install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

	/* restore interrupt state */
	irq_unlock(oldLevel);
	return 0;
}

SYS_INIT(k6x_init, PRE_KERNEL_1, 0);
