/*
 * Copyright (c) 2019, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <fsl_common.h>
#include <fsl_clock.h>
#include <fsl_sim.h>
#include <arch/cpu.h>

/*******************************************************************************
 * Variables
 ******************************************************************************/

static ALWAYS_INLINE void clock_init(void)
{
	/*
	 * Core clock: 48MHz
	 * Bus clock: 24MHz
	 */
	const mcglite_config_t mcgliteConfig = {
		.outSrc = kMCGLITE_ClkSrcHirc,  /* MCGOUTCLK source is HIRC */
		.irclkEnableMode = kMCGLITE_IrclkEnable, /* MCGIRCLK enabled, MCGIRCLK disabled in STOP mode */
		.ircs = kMCGLITE_Lirc8M,	    /* Slow internal reference (LIRC) 8 MHz clock selected */
		.fcrdiv = kMCGLITE_LircDivBy1,  /* Low-frequency Internal Reference Clock Divider: divided by 1 */
		.lircDiv2 = kMCGLITE_LircDivBy1, /* Second Low-frequency Internal Reference Clock Divider: divided by 1 */
		.hircEnableInNotHircMode = true, /* HIRC source is enabled */
	};

	const sim_clock_config_t simConfig = {
		.er32kSrc = 0, /* OSC32KSEL select: OSC32KCLK clock */
		.clkdiv1  = 0x10000U, /* SIM_CLKDIV1 - OUTDIV1: /1, OUTDIV4: /2 */
	};

	CLOCK_SetSimSafeDivs();
	CLOCK_SetMcgliteConfig(&mcgliteConfig);

	CLOCK_SetSimConfig(&simConfig);

#ifdef CONFIG_UART_MCUX_LPUART_0
	CLOCK_SetLpuart0Clock(1);
#endif
#if CONFIG_USB_KINETIS
	CLOCK_EnableUsbfs0Clock(kCLOCK_UsbSrcIrc48M,
				DT_ARM_CORTEX_M0PLUS_0_CLOCK_FREQUENCY);
#endif
}

static int k32l_init(struct device *arg)
{
	ARG_UNUSED(arg);

	unsigned int oldLevel; /* old interrupt lock level */

	/* disable interrupts */
	oldLevel = irq_lock();

	/* Disable the watchdog */
	SIM->COPC = 0;

	/* Initialize system clock to 48 MHz */
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

SYS_INIT(k32l_init, PRE_KERNEL_1, 0);
