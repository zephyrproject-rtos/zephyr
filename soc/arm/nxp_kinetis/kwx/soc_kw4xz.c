/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <drivers/uart.h>
#include <fsl_common.h>
#include <fsl_clock.h>
#include <arch/cpu.h>

#define LPUART0SRC_OSCERCLK	(1)
#define TPMSRC_MCGPLLCLK	(1)

#define CLKDIV1_DIVBY2		(1)

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
	.clkdiv1 = SIM_CLKDIV1_OUTDIV4(CLKDIV1_DIVBY2),
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

static int kwx_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	unsigned int oldLevel; /* old interrupt lock level */

	/* disable interrupts */
	oldLevel = irq_lock();

	/* Initialize system clock to 40 MHz */
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

void z_arm_watchdog_init(void)
{
	/* Disable the watchdog */
	SIM->COPC = 0;
}

SYS_INIT(kwx_init, PRE_KERNEL_1, 0);
