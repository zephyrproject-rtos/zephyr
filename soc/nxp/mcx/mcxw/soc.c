/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/sections.h>

#include <soc.h>

#include <fsl_ccm32k.h>
#include <fsl_common.h>
#include <fsl_clock.h>

extern uint32_t SystemCoreClock;

static ALWAYS_INLINE void clock_init(void)
{
	/* Unlock Reference Clock Status Registers to allow writes */
	CLOCK_UnlockFircControlStatusReg();
	CLOCK_UnlockSircControlStatusReg();
	CLOCK_UnlockRoscControlStatusReg();
	CLOCK_UnlockSysOscControlStatusReg();

	/*
	 * Configuration for the 32 kHz Oscillator module
	 * Internal capatitor bank is required in order to use the more stable OSC32K source
	 */
	ccm32k_osc_config_t ccm32k_osc_config = {
		.coarseAdjustment = kCCM32K_OscCoarseAdjustmentRange0, /* ESR_Range0 */
		.enableInternalCapBank = true,      /* Internal capacitance bank is enabled */
		.xtalCap = kCCM32K_OscXtal8pFCap,   /* 8 pF */
		.extalCap = kCCM32K_OscExtal8pFCap, /* 8 pF */
	};
	/* Enable OSC32K */
	CCM32K_Set32kOscConfig(CCM32K, kCCM32K_Enable32kHzCrystalOsc, &ccm32k_osc_config);
	/* Disable ROSC Monitor, because switching the source would generate an expected error */
	CLOCK_SetRoscMonitorMode(kSCG_RoscMonitorDisable);
	/* Select the Real Time Clock (RTC) source as OSC32K */
	CCM32K_SelectClockSource(CCM32K, kCCM32K_ClockSourceSelectOsc32k);
	/* Wait for RTC Oscillator to be Valid */
	while (!CLOCK_IsRoscValid())
		;
	/* Re-enable monitor */
	CLOCK_SetRoscMonitorMode(kSCG_RoscMonitorInt);
	/* Disable the FRO32K to save power */
	CCM32K_Enable32kFro(CCM32K, false);

	/* Configuration to set FIRC to maximum frequency */
	scg_firc_config_t scg_firc_config = {
		.enableMode = kSCG_FircEnable, /* Fast IRC is enabled */
		.range = kSCG_FircRange96M,    /* 96 Mhz FIRC clock selected */
		.trimConfig = NULL,
	};

	scg_sys_clk_config_t sys_clk_safe_config_source = {
		.divSlow = (uint32_t)kSCG_SysClkDivBy4,
		.divCore = (uint32_t)kSCG_SysClkDivBy1,
		.src = (uint32_t)kSCG_SysClkSrcSirc,
	};

	CLOCK_SetRunModeSysClkConfig(&sys_clk_safe_config_source);

	scg_sys_clk_config_t cur_config;

	do {
		CLOCK_GetCurSysClkConfig(&cur_config);
	} while (cur_config.src != sys_clk_safe_config_source.src);

	(void)CLOCK_InitFirc(&scg_firc_config);

	scg_sys_clk_config_t sys_clk_config = {
		.divSlow = (uint32_t)kSCG_SysClkDivBy4, /* Slow Clock Divider: divided by 4 */
		.divBus = (uint32_t)kSCG_SysClkDivBy1,  /* Bus Clock Divider: divided by 1 */
		.divCore = (uint32_t)kSCG_SysClkDivBy1, /* Core Clock Divider: divided by 1 */
		.src = (uint32_t)kSCG_SysClkSrcFirc,    /* Select Fast IRC as System Clock */
	};
	CLOCK_SetRunModeSysClkConfig(&sys_clk_config);

	/* Wait for clock source switch to finish */
	do {
		CLOCK_GetCurSysClkConfig(&cur_config);
	} while (cur_config.src != sys_clk_config.src);

	SystemCoreClock = 96000000U;

	/* OSC-RF / System Oscillator Configuration */
	scg_sosc_config_t sosc_config = {
		.freq = 32000U,
		.monitorMode = kSCG_SysOscMonitorDisable,
		.enableMode = kSCG_SoscEnable,
	};

	/* Init OSC-RF / SOSC */
	(void)CLOCK_InitSysOsc(&sosc_config);
	CLOCK_SetXtal0Freq(sosc_config.freq);

	/* Slow internal reference clock (SIRC) configuration */
	scg_sirc_config_t sirc_config = {
		.enableMode = kSCG_SircDisableInSleep,
	};

	/* Init SIRC */
	(void)CLOCK_InitSirc(&sirc_config);

	/* Attach Clocks */
	CLOCK_SetIpSrc(kCLOCK_Lpuart0, kCLOCK_IpSrcFro192M);
	CLOCK_SetIpSrc(kCLOCK_Lpuart1, kCLOCK_IpSrcFro192M);
	CLOCK_SetIpSrc(kCLOCK_Lpspi0, kCLOCK_IpSrcFro192M);
	CLOCK_SetIpSrc(kCLOCK_Lpspi1, kCLOCK_IpSrcFro192M);
	CLOCK_SetIpSrc(kCLOCK_Can0, kCLOCK_IpSrcFro192M);
	CLOCK_SetIpSrc(kCLOCK_Tpm0, kCLOCK_IpSrcFro192M);
	CLOCK_SetIpSrc(kCLOCK_Tpm1, kCLOCK_IpSrcFro192M);
	CLOCK_SetIpSrc(kCLOCK_Lpi2c0, kCLOCK_IpSrcFro192M);
	CLOCK_SetIpSrcDiv(kCLOCK_Lpi2c0, kSCG_SysClkDivBy16);
	CLOCK_SetIpSrc(kCLOCK_Lpi2c1, kCLOCK_IpSrcFro192M);
	CLOCK_SetIpSrcDiv(kCLOCK_Lpi2c1, kSCG_SysClkDivBy16);

	/* Ungate clocks if the peripheral is enabled in devicetree */
	if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(lpuart0), nxp_lpc_lpuart, okay)) {
		CLOCK_EnableClock(kCLOCK_Lpuart0);
	}

	if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(lpuart1), nxp_lpc_lpuart, okay)) {
		CLOCK_EnableClock(kCLOCK_Lpuart1);
	}

	if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(tpm0), nxp_kinetis_tpm, okay)) {
		CLOCK_EnableClock(kCLOCK_Tpm0);
	}

	if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(tpm1), nxp_kinetis_tpm, okay)) {
		CLOCK_EnableClock(kCLOCK_Tpm1);
	}
}

static void vbat_init(void)
{
	VBAT_Type *base = (VBAT_Type *)DT_REG_ADDR(DT_NODELABEL(vbat));

	/* Write 1 to Clear POR detect status bit.
	 *
	 * Clearing this bit is acknowledement
	 * that software has recognized a power on reset.
	 *
	 * This avoids also niche issues with NVIC read/write
	 * when searching for available interrupt lines.
	 */
	base->STATUSA |= VBAT_STATUSA_POR_DET_MASK;
};

void soc_early_init_hook(void)
{
	unsigned int oldLevel; /* old interrupt lock level */

	/* disable interrupts */
	oldLevel = irq_lock();

	/* Initialize system clock to 96 MHz */
	clock_init();

	/* Smart power switch initialization */
	vbat_init();

	/* restore interrupt state */
	irq_unlock(oldLevel);
}
