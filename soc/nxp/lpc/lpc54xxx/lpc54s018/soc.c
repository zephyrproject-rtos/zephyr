/*
 * Private Porting
 * by David Hor - Xtooltech 2025
 * david.hor@xtooltech.com
 */

/*
 * Copyright (c) 2017, 2024 NXP
 * Copyright (c) 2024 VCI Development - LPC54S018J4MET180E single-core M4
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for LPC54S018J4M platform
 *
 * This module provides routines to initialize and support SoC-level
 * hardware for the LPC54S018J4M platform.
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
/* LPC54S018 is single-core Cortex-M4F, no secondary core support */

/**
 *
 * @brief Initialize the system clock
 *
 */
#define CPU_FREQ DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency)

__weak void clock_init(void)
{

#ifdef CONFIG_SOC_LPC54S018
	/* Set up the clock sources */

	/* Ensure FRO is on */
	POWER_DisablePD(kPDRUNCFG_PD_FRO_EN);

	/*
	 * Switch to FRO 12MHz first to ensure we can change voltage without
	 * accidentally being below the voltage for current speed.
	 */
	CLOCK_AttachClk(kFRO12M_to_MAIN_CLK);

	/* Set FLASH wait states for core */
	/* Note: FLASHCFG register not available in LPC54S018 SYSCON peripheral.
	 * Flash wait states are handled automatically by hardware for this device.
	 * CLOCK_SetFLASHAccessCyclesForFreq(CPU_FREQ); - Not available in all SDK versions */

	/* Set up System PLL to 180MHz for LPC54S018 */
	const pll_setup_t pllSetup = {
		.pllctrl = SYSCON_SYSPLLCTRL_SELI(32U) | SYSCON_SYSPLLCTRL_SELP(16U) |
			   SYSCON_SYSPLLCTRL_SELR(0U),
		.pllmdec = SYSCON_SYSPLLMDEC_MDEC(8191U),
		.pllndec = SYSCON_SYSPLLNDEC_NDEC(770U),
		.pllpdec = SYSCON_SYSPLLPDEC_PDEC(98U),
		.pllRate = 180000000U,
		.flags = PLL_SETUPFLAG_WAITLOCK
	};

	/* Power up System PLL */
	/* Note: kPDRUNCFG_PD_SYS_PLL might not be defined in all SDK versions */
	/* Using direct register access instead for compatibility */
	SYSCON->PDRUNCFGCLR[0] = SYSCON_PDRUNCFG_PDEN_SYS_PLL_MASK;

	/* Setup System PLL to 180MHz */
	CLOCK_AttachClk(kFRO12M_to_SYS_PLL);
	CLOCK_SetPLLFreq(&pllSetup);

	/* Set up dividers */
	/* Set AHBCLKDIV divider to value 1 */
	CLOCK_SetClkDiv(kCLOCK_DivAhbClk, 1U, false);

	/* Set up clock selectors - Attach clocks to the peripheries */
	/* Switch MAIN_CLK to System PLL for 180MHz operation */
	CLOCK_AttachClk(kSYS_PLL_to_MAIN_CLK);

	/* Attach 12 MHz clock to FLEXCOMM0 */
	CLOCK_AttachClk(kFRO12M_to_FLEXCOMM0);

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm4), nxp_lpc_i2c, okay)
	/* attach 12 MHz clock to FLEXCOMM4 */
	CLOCK_AttachClk(kFRO12M_to_FLEXCOMM4);
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm5), nxp_lpc_spi, okay)
	/* Attach 12 MHz clock to FLEXCOMM5 */
	CLOCK_AttachClk(kFRO_HF_to_FLEXCOMM5);
#endif

#ifdef CONFIG_FLASH_LPC_SPIFI_NOR
	/* Configure AUDIO PLL for SPIFI at 96MHz */
	const pll_setup_t audio_pllSetup = {
		.pllctrl = SYSCON_AUDPLLCTRL_SELI(36U) | SYSCON_AUDPLLCTRL_SELP(17U) | SYSCON_AUDPLLCTRL_SELR(0U),
		.pllmdec = SYSCON_AUDPLLMDEC_MDEC(16383U),
		.pllndec = SYSCON_AUDPLLNDEC_NDEC(770U),
		.pllpdec = SYSCON_AUDPLLPDEC_PDEC(66U),
		.pllRate = 96000000U,
		.flags = PLL_SETUPFLAG_WAITLOCK | PLL_SETUPFLAG_POWERUP
	};
	
	/* Power up AUDIO PLL */
	SYSCON->PDRUNCFGCLR[1] = SYSCON_PDRUNCFG_PDEN_AUD_PLL_MASK;
	
	/* Setup AUDIO PLL - use FRO12M as source */
	CLOCK_AttachClk(kFRO12M_to_AUDIO_PLL);
	CLOCK_SetAudioPLLFreq(&audio_pllSetup);
	
	/* Attach SPIFI to AUDIO PLL for 96MHz operation */
	CLOCK_AttachClk(kAUDIO_PLL_to_SPIFI_CLK);
#endif

#if defined(CONFIG_USB_DEVICE_DRIVER) || defined(CONFIG_USB_DEVICE_STACK)
	/* Configure USB PLL for 96MHz (48MHz USB clock output) */
	const pll_setup_t usb_pllSetup = {
		.pllctrl = SYSCON_USBPLLCTRL_SELI(36U) | SYSCON_USBPLLCTRL_SELP(17U) | SYSCON_USBPLLCTRL_SELR(0U),
		.pllmdec = SYSCON_USBPLLMDEC_MDEC(0x1FFF),
		.pllndec = SYSCON_USBPLLNDEC_NDEC(0x302),
		.pllpdec = SYSCON_USBPLLPDEC_PDEC(66U),
		.pllRate = 96000000U,
		.flags = PLL_SETUPFLAG_WAITLOCK | PLL_SETUPFLAG_POWERUP
	};
	
	/* Power up USB PLL */
	SYSCON->PDRUNCFGCLR[0] = SYSCON_PDRUNCFG_PDEN_USB_PLL_MASK;
	
	/* Setup USB PLL - use FRO12M as source */
	CLOCK_AttachClk(kFRO12M_to_USB_PLL);
	CLOCK_SetUsbPLLFreq(&usb_pllSetup);
	
	/* Enable USB0 PHY power */
	SYSCON->PDRUNCFGCLR[0] = SYSCON_PDRUNCFG_PDEN_USB0_PHY_MASK;
	
	/* Enable USB1 PHY power */
	SYSCON->PDRUNCFGCLR[0] = SYSCON_PDRUNCFG_PDEN_USB1_PHY_MASK;
	
	/* Set USB0 clock source to USB PLL (48MHz) */
	CLOCK_AttachClk(kUSB_PLL_to_USB0_CLK);
	CLOCK_SetClkDiv(kCLOCK_DivUsb0Clk, 1U, false);
	
	/* Set USB1 clock source to USB PLL (48MHz) */
	CLOCK_AttachClk(kUSB_PLL_to_USB1_CLK);
	CLOCK_SetClkDiv(kCLOCK_DivUsb1Clk, 1U, false);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(enet), okay)
	/* Enable Ethernet clock */
	CLOCK_EnableClock(kCLOCK_Eth);
	/* ENET uses system clock directly, no separate clock attach needed */
	/* The ENET peripheral runs from the system clock (96MHz) */
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(emc), okay)
	/* Enable EMC clock */
	CLOCK_EnableClock(kCLOCK_Emc);
	/* EMC clock = CPU clock / 2 for stable operation */
	CLOCK_SetClkDiv(kCLOCK_DivEmcClk, 0U, true);  /* Reset divider */
	CLOCK_SetClkDiv(kCLOCK_DivEmcClk, 1U, false); /* Divide by 2 */
#endif

#endif /* CONFIG_SOC_LPC54S018 */
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

static int nxp_lpc54s018_init(void)
{
	/* Initialize FRO/system clock to 48 MHz */
	clock_init();

#ifdef CONFIG_GPIO_MCUX_LPC
	/* Turn on PINT device*/
	PINT_Init(PINT);
#endif

	return 0;
}

SYS_INIT(nxp_lpc54s018_init, PRE_KERNEL_1, 0);

#if defined(CONFIG_SOC_RESET_HOOK)

void soc_reset_hook(void)
{
	SystemInit();
}

#endif /* CONFIG_SOC_RESET_HOOK */
