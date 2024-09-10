/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/drivers/uart.h>
#include <fsl_common.h>
#include <fsl_clock.h>

#define LPUART0SRC_OSCERCLK	(1)
#define TPMSRC_MCGPLLCLK	(1)

#define CLOCK_NODEID(clk) \
	DT_CHILD(DT_INST(0, nxp_kinetis_sim), clk)

#define CLOCK_DIVIDER(clk) \
	DT_PROP_OR(CLOCK_NODEID(clk), clock_div, 1) - 1

static const osc_config_t oscConfig = {
	.freq = CONFIG_OSC_XTAL0_FREQ,

#if defined(CONFIG_OSC_EXTERNAL)
	.workMode = kOSC_ModeExt,
#elif defined(CONFIG_OSC_LOW_POWER)
	.workMode = kOSC_ModeOscLowPower,
#elif defined(CONFIG_OSC_HIGH_GAIN)
	.workMode = kOSC_ModeOscHighGain,
#else
#error "An oscillator mode must be defined"
#endif
};

static const sim_clock_config_t simConfig = {
	.er32kSrc = DT_PROP(DT_INST(0, nxp_kinetis_sim), er32k_select),
	.clkdiv1 = SIM_CLKDIV1_OUTDIV1(CLOCK_DIVIDER(core_clk)) |
		   SIM_CLKDIV1_OUTDIV4(CLOCK_DIVIDER(flash_clk)),
};

/* This function comes from the MCUX SDK:
 * modules/hal/nxp/mcux/devices/MKW41Z4/clock_config.c
 */
static void CLOCK_SYS_FllStableDelay(void)
{
	uint32_t i = 30000U;
	while (i--) {
		__NOP();
	}
}

static ALWAYS_INLINE void clock_init(void)
{
	CLOCK_SetSimSafeDivs();

	CLOCK_InitOsc0(&oscConfig);
	CLOCK_SetXtal0Freq(CONFIG_OSC_XTAL0_FREQ);

	CLOCK_BootToFeeMode(kMCG_OscselOsc, CONFIG_MCG_FRDIV, kMCG_Dmx32Default,
			    kMCG_DrsMid, CLOCK_SYS_FllStableDelay);

	CLOCK_SetInternalRefClkConfig(kMCG_IrclkEnable, kMCG_IrcSlow,
				      CONFIG_MCG_FCRDIV);

	CLOCK_SetSimConfig(&simConfig);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(lpuart0), okay)
	CLOCK_SetLpuartClock(LPUART0SRC_OSCERCLK);
#endif

#if defined(CONFIG_PWM) && \
	(DT_NODE_HAS_STATUS(DT_NODELABEL(pwm0), okay) || \
	 DT_NODE_HAS_STATUS(DT_NODELABEL(pwm1), okay) || \
	 DT_NODE_HAS_STATUS(DT_NODELABEL(pwm2), okay))
	CLOCK_SetTpmClock(TPMSRC_MCGPLLCLK);
#endif
}

void soc_early_init_hook(void)
{
	/* Initialize system clock to 40 MHz */
	clock_init();
}

#ifdef CONFIG_SOC_RESET_HOOK

void soc_reset_hook(void)
{
	SystemInit();
}

#endif /* CONFIG_SOC_RESET_HOOK */
