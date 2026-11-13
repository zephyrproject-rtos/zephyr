/*
 * Copyright (c) 2017, 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for nxp_lpc546xx platform
 *
 * This module provides routines to initialize and support board-level
 * hardware for the nxp_lpc546xx platform.
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

/**
 *
 * @brief Initialize the system clock
 *
 */
#define CPU_FREQ DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency)

__weak void clock_init(void)
{
	/* 220 MHz system PLL setup for a 12 MHz input (BOARD_BootClockPLL220M). */
	const pll_setup_t pll_setup = {
		.pllctrl = SYSCON_SYSPLLCTRL_SELI(34U) | SYSCON_SYSPLLCTRL_SELP(31U) |
			   SYSCON_SYSPLLCTRL_SELR(0U),
		.pllmdec = SYSCON_SYSPLLMDEC_MDEC(13243U),
		.pllndec = SYSCON_SYSPLLNDEC_NDEC(1U),
		.pllpdec = SYSCON_SYSPLLPDEC_PDEC(98U),
		.pllRate = 220000000U,
		.flags = PLL_SETUPFLAG_WAITLOCK | PLL_SETUPFLAG_POWERUP,
	};

	/* Ensure FRO is on */
	POWER_DisablePD(kPDRUNCFG_PD_FRO_EN);

	/*
	 * Switch to FRO 12MHz first to ensure we can change voltage without
	 * accidentally being below the voltage for current speed.
	 */
	CLOCK_AttachClk(kFRO12M_to_MAIN_CLK);

	/* Power up the 12 MHz crystal oscillator (SYSOSC) on XTAL1/XTAL2 */
	POWER_DisablePD(kPDRUNCFG_PD_SYS_OSC);
	SYSCON->SYSOSCCTRL = (SYSCON->SYSOSCCTRL & ~SYSCON_SYSOSCCTRL_FREQRANGE_MASK) |
			     SYSCON_SYSOSCCTRL_FREQRANGE(0U); /* 1-20 MHz range */

	/*
	 * Raise VDDCORE to the level required for the target core frequency
	 * before increasing the clock, and set the matching flash wait states.
	 */
	POWER_SetVoltageForFreq(CPU_FREQ);
	CLOCK_SetFLASHAccessCyclesForFreq(CPU_FREQ);

	/* Configure the system PLL from the external crystal and wait for lock */
	CLOCK_AttachClk(kEXT_CLK_to_SYS_PLL);
	CLOCK_SetPLLFreq(&pll_setup);

	/* Set AHBCLKDIV divider to value 1 */
	CLOCK_SetClkDiv(kCLOCK_DivAhbClk, 1U, false);

	/* Switch MAIN_CLK to the system PLL */
	CLOCK_AttachClk(kSYS_PLL_to_MAIN_CLK);

	/* Attach 12 MHz clock to FLEXCOMM0 */
	CLOCK_AttachClk(kFRO12M_to_FLEXCOMM0);

#if defined(CONFIG_UDC_NXP_IP3511) || defined(CONFIG_USB_DC_NXP_LPCIP3511)
#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(usbhs), nxp_lpcip3511, okay)
	/* Turn on the USB1 high-speed PHY. */
	POWER_DisablePD(kPDRUNCFG_PD_USB1_PHY);

	/* enable usb1 host clock */
	CLOCK_EnableClock(kCLOCK_Usbh1);
	/*
	 * According to the reference manual, device mode has to be selected by
	 * accessing the USB1 host controller's PORTMODE register.
	 */
	USBHSH->PORTMODE |= USBHSH_PORTMODE_DEV_ENABLE_MASK;
	/* disable usb1 host clock */
	CLOCK_DisableClock(kCLOCK_Usbh1);

	/* enable USB1 device clock (sets up the USB PLL from the 12 MHz xtal) */
	CLOCK_EnableUsbhs0DeviceClock(kCLOCK_UsbSrcUsbPll, 0U);
#if defined(FSL_FEATURE_USBHSD_USB_RAM) && (FSL_FEATURE_USBHSD_USB_RAM)
	memset((uint8_t *)FSL_FEATURE_USBHSD_USB_RAM_BASE_ADDRESS, 0,
	       FSL_FEATURE_USBHSD_USB_RAM);
#endif
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(usbfs), nxp_lpcip3511, okay)
	/* Turn on the USB0 full-speed PHY. */
	POWER_DisablePD(kPDRUNCFG_PD_USB0_PHY);

	/* USB0 is clocked from FRO_HF, which is not set up by the main clock */
	CLOCK_SetupFROClocking(96000000U);
	CLOCK_SetClkDiv(kCLOCK_DivUsb0Clk, 1, false);
	CLOCK_AttachClk(kFRO_HF_to_USB0_CLK);

	/* enable usb0 host clock */
	CLOCK_EnableClock(kCLOCK_Usbhsl0);
	/*
	 * According to the reference manual, device mode has to be selected by
	 * accessing the USB0 host controller's PORTMODE register.
	 */
	USBFSH->PORTMODE |= USBFSH_PORTMODE_DEV_ENABLE_MASK;
	/* disable usb0 host clock */
	CLOCK_DisableClock(kCLOCK_Usbhsl0);

	/* enable USB0 device clock */
	CLOCK_EnableUsbfs0DeviceClock(kCLOCK_UsbSrcFro, CLOCK_GetFroHfFreq());
#if defined(FSL_FEATURE_USB_USB_RAM) && (FSL_FEATURE_USB_USB_RAM)
	memset((uint8_t *)FSL_FEATURE_USB_USB_RAM_BASE_ADDRESS, 0,
	       FSL_FEATURE_USB_USB_RAM);
#endif
#endif
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm2), nxp_lpc_i2c, okay)
	/* Attach 12 MHz clock to FLEXCOMM2 */
	CLOCK_AttachClk(kFRO12M_to_FLEXCOMM2);
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(sdif), nxp_lpc_sdif, okay)
	/*
	 * SD/MMC host source clock: the main clock, divided to stay within the
	 * controller's maximum source frequency.
	 */
	CLOCK_AttachClk(kMAIN_CLK_to_SDIO_CLK);
	CLOCK_SetClkDiv(kCLOCK_DivSdioClk,
			(CPU_FREQ / FSL_FEATURE_SDIF_MAX_SOURCE_CLOCK) + 1U, true);
#endif

	/*
	 * The M_CAN functional clock is the core clock divided by CANnCLKDIV,
	 * which is halted out of reset. Divide by 11 -> 20 MHz at a 220 MHz
	 * core: the M_CAN core requires <= 80 MHz, and 20 MHz divides the
	 * common CAN bit rates cleanly.
	 */
#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(can0), nxp_lpc_mcan, okay)
	CLOCK_SetClkDiv(kCLOCK_DivCan0Clk, 11U, false);
#endif
#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(can1), nxp_lpc_mcan, okay)
	CLOCK_SetClkDiv(kCLOCK_DivCan1Clk, 11U, false);
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(wwdt0), nxp_lpc_wwdt, okay)
	/*
	 * The watchdog oscillator is powered down out of reset and its FREQSEL
	 * field selects no frequency, so the watchdog would see a rate of 0.
	 * FREQSEL 14 with DIVSEL 0 is 1 MHz, a 2 ms to 67 s timeout range.
	 */
	POWER_DisablePD(kPDRUNCFG_PD_WDT_OSC);
	SYSCON->WDTOSCCTRL = SYSCON_WDTOSCCTRL_FREQSEL(14U) | SYSCON_WDTOSCCTRL_DIVSEL(0U);
#endif

	if (IS_ENABLED(CONFIG_NXP_GINT)) {
		CLOCK_EnableClock(kCLOCK_Gint);
	}
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

static int nxp_lpc546xx_init(void)
{
	/* Initialize the system clock */
	clock_init();

#ifdef CONFIG_GPIO_MCUX_LPC
	/* Turn on PINT device*/
	PINT_Init(PINT);
#endif

	return 0;
}

SYS_INIT(nxp_lpc546xx_init, PRE_KERNEL_1, 0);

#ifdef CONFIG_SOC_RESET_HOOK

#ifdef CONFIG_SOC_SERIES_LPC54XXX_SRAM_CLOCKS

/*
 * SRAM2 and SRAM3 are not clocked out of reset; SystemInit() turns them on.
 * A board that describes the banks as one region - lpcxpresso54628 gives sram0
 * all 160K of SRAM0..SRAM3 - lets the linker place z_main_stack past the 96K
 * that SRAM0 and SRAM1 hold, so the boot stack can land in memory that is not
 * clocked yet. SystemInit() cannot fix that from there: its own prologue
 * pushes to that stack before it reaches the write.
 *
 * Ungate the banks first, then tail-call SystemInit, so neither step touches
 * the stack. A naked function is needed because a prologue would otherwise run
 * before the store. The MCUXpresso SDK does the same in the first instructions
 * of its reset handler, which Zephyr does not use. A build that turns the banks
 * off through SOC_SERIES_LPC54XXX_SRAM_CLOCKS keeps the plain call, and has to
 * keep its stack out of them.
 */
__attribute__((naked)) void soc_reset_hook(void)
{
	__asm__ volatile("ldr r0, =%[set]\n"
			 "movs r1, %[banks]\n"
			 "str r1, [r0]\n"
			 "b %[init]\n"
			 :
			 : [set] "i"(&SYSCON->AHBCLKCTRLSET[0]),
			   [banks] "i"(SYSCON_AHBCLKCTRL_SRAM1_MASK |
				       SYSCON_AHBCLKCTRL_SRAM2_MASK |
				       SYSCON_AHBCLKCTRL_SRAM3_MASK),
			   [init] "i"(SystemInit));
}

#else

void soc_reset_hook(void)
{
	SystemInit();
}

#endif /* CONFIG_SOC_SERIES_LPC54XXX_SRAM_CLOCKS */

#endif /* CONFIG_SOC_RESET_HOOK */
