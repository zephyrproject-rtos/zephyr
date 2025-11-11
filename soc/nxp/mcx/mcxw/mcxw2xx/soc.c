/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for nxp_mcxw23 platform
 *
 * This module provides routines to initialize and support board-level
 * hardware for the nxp_mcxw23 platform.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>
#include <cortex_m/exception.h>
#include <fsl_power.h>
#include <fsl_clock.h>
#include <fsl_common.h>
#include <fsl_device_registers.h>
#ifdef CONFIG_GPIO_MCUX_LPC
#include <fsl_pint.h>
#endif

/* System clock frequency */
extern uint32_t SystemCoreClock;
extern void nxp_nbu_init(void);

#define CTIMER_CLOCK_SOURCE(node_id) \
	TO_CTIMER_CLOCK_SOURCE(DT_CLOCKS_CELL(node_id, name), DT_PROP(node_id, clk_source))
#define TO_CTIMER_CLOCK_SOURCE(inst, val) TO_CLOCK_ATTACH_ID(inst, val)
#define TO_CLOCK_ATTACH_ID(inst, val) MUX_A(CM_CTIMERCLKSEL##inst, val)
#define CTIMER_CLOCK_SETUP(node_id) CLOCK_AttachClk(CTIMER_CLOCK_SOURCE(node_id));
/** 32KHz oscillator load in cap setting */
#define OSC_CAP_IN_SETTING 5
/** 32KHz oscillator load out cap setting */
#define OSC_CAP_OUT_SETTING 5

static void configure_32k_osc(void)
{
	/* Configure 32KHz xtal caps for use with RDM */
	POWER_XTAL32K_ConfigureCaps(OSC_CAP_IN_SETTING, OSC_CAP_OUT_SETTING);
	POWER_PeripheralPowerOn(kPOWERCFG_XTAL32K);
	CLOCK_Select32kOscClkSrc(kCLOCK_Osc32kClockSrc_XTAL);
}

/**
 *
 * @brief Initialize the system clock
 *
 */
__weak void clock_init(void)
{
	CLOCK_AttachClk(kFRO12M_to_MAIN_CLK);

	/*!< Set FLASH wait states for core */
	CLOCK_SetFLASHAccessCyclesForFreq(kFreq_32MHz);

	/*!< Set up high frequency FRO output to selected frequency */
	CLOCK_SetupFROClocking(kFreq_32MHz);

	/* Set SystemCoreClock variable. */
	SystemCoreClock = kFreq_32MHz;

	CLOCK_EnableClock(kCLOCK_Iocon);

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm0), nxp_lpc_i2c, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm0), nxp_lpc_spi, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm0), nxp_lpc_usart, okay)
	CLOCK_AttachClk(kFRO_HF_DIV_to_FLEXCOMM0);
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm1), nxp_lpc_i2c, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm1), nxp_lpc_spi, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm1), nxp_lpc_usart, okay)
	CLOCK_AttachClk(kFRO_HF_DIV_to_FLEXCOMM1);
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm2), nxp_lpc_i2c, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm2), nxp_lpc_spi, okay) || \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm2), nxp_lpc_usart, okay)
	CLOCK_AttachClk(kFRO_HF_DIV_to_FLEXCOMM2);
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(wwdt0), nxp_lpc_wwdt, okay)
	CLOCK_Enable1MFRO(true);
#endif

	DT_FOREACH_STATUS_OKAY(nxp_lpc_ctimer, CTIMER_CLOCK_SETUP)
	DT_FOREACH_STATUS_OKAY(nxp_ctimer_pwm, CTIMER_CLOCK_SETUP)

	configure_32k_osc();

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(os_timer), nxp_os_timer, okay)
	/*!< OS event timer select FRO 1 MHz clock */
	PMC->OSTIMERr &= ~PMC_OSTIMER_OSTIMERCLKSEL_MASK;
	PMC->OSTIMERr |= OSTIMERCLKSEL_FRO_1MHz << PMC_OSTIMER_OSTIMERCLKSEL_SHIFT;
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(iap), nxp_iap_fmc55, okay)
	/* kCLOCK_Sysctl must be enabled for FLASH_CacheClear,
	 * FLASH_CacheSpeculationControl and FLASH_CheckECC to have an effect.
	 */
	CLOCK_EnableClock(kCLOCK_Sysctl);
#endif
}

#ifdef CONFIG_SOC_RESET_HOOK

void soc_reset_hook(void)
{
	SystemInit();

#ifndef CONFIG_LOG_BACKEND_SWO
	/*
	 * SystemInit unconditionally enables the trace clock.
	 * Disable the trace clock unless SWO is used
	 */
	SYSCON->TRACECLKDIV = 0x4000000;
#endif
}

#endif /* CONFIG_SOC_RESET_HOOK */

void soc_early_init_hook(void)
{
	z_arm_clear_faults();

	/* Initialize FRO/system clock to 96 MHz */
	clock_init();

#ifdef CONFIG_GPIO_MCUX_LPC
	/* Turn on PINT device*/
	PINT_Init(PINT);
#endif

#ifdef CONFIG_BT
	nxp_nbu_init();
#endif
}
