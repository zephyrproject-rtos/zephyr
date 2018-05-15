/*
 * Copyright (c) 2017,2018, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <linker/sections.h>
#include <fsl_clock.h>
#include <arch/cpu.h>
#include <cortex_m/exc.h>

/* ARM PLL configuration for RUN mode */
const clock_arm_pll_config_t armPllConfig = {
	.loopDivider = 100U
};

/* SYS PLL configuration for RUN mode */
const clock_sys_pll_config_t sysPllConfig = {
	.loopDivider = 1U
};

/* USB1 PLL configuration for RUN mode */
const clock_usb_pll_config_t usb1PllConfig = {
	.loopDivider = 0U
};

/**
 *
 * @brief Initialize the system clock
 *
 * @return N/A
 *
 */
static ALWAYS_INLINE void clkInit(void)
{
	/* Boot ROM did initialize the XTAL, here we only sets external XTAL
	 * OSC freq
	 */
	CLOCK_SetXtalFreq(24000000U);
	CLOCK_SetRtcXtalFreq(32768U);

	/* Set PERIPH_CLK2 MUX to OSC */
	CLOCK_SetMux(kCLOCK_PeriphClk2Mux, 0x1);

	/* Set PERIPH_CLK MUX to PERIPH_CLK2 */
	CLOCK_SetMux(kCLOCK_PeriphMux, 0x1);

	/* Setting the VDD_SOC to 1.5V. It is necessary to config AHB to 600Mhz
	 */
	DCDC->REG3 = (DCDC->REG3 & (~DCDC_REG3_TRG_MASK)) | DCDC_REG3_TRG(0x12);

	CLOCK_InitArmPll(&armPllConfig); /* Configure ARM PLL to 1200M */
	CLOCK_InitSysPll(&sysPllConfig); /* Configure SYS PLL to 528M */
	CLOCK_InitUsb1Pll(&usb1PllConfig); /* Configure USB1 PLL to 480M */

	CLOCK_SetDiv(kCLOCK_ArmDiv, 0x1); /* Set ARM PODF to 0, divide by 2 */
	CLOCK_SetDiv(kCLOCK_AhbDiv, 0x0); /* Set AHB PODF to 0, divide by 1 */
	CLOCK_SetDiv(kCLOCK_IpgDiv, 0x3); /* Set IPG PODF to 3, divide by 4 */

	/* Set PRE_PERIPH_CLK to PLL1, 1200M */
	CLOCK_SetMux(kCLOCK_PrePeriphMux, 0x3);

	/* Set PERIPH_CLK MUX to PRE_PERIPH_CLK */
	CLOCK_SetMux(kCLOCK_PeriphMux, 0x0);

#ifdef CONFIG_UART_MCUX_LPUART
	/* Configure UART divider to default */
	CLOCK_SetMux(kCLOCK_UartMux, 0); /* Set UART source to PLL3 80M */
	CLOCK_SetDiv(kCLOCK_UartDiv, 0); /* Set UART divider to 1 */
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

static int imxrt_init(struct device *arg)
{
	ARG_UNUSED(arg);

	int oldLevel; /* old interrupt lock level */

	/* disable interrupts */
	oldLevel = irq_lock();

	/* Watchdog disable */
	if (WDOG1->WCR & WDOG_WCR_WDE_MASK) {
		WDOG1->WCR &= ~WDOG_WCR_WDE_MASK;
	}

	if (WDOG2->WCR & WDOG_WCR_WDE_MASK) {
		WDOG2->WCR &= ~WDOG_WCR_WDE_MASK;
	}

	RTWDOG->CNT = 0xD928C520U; /* 0xD928C520U is the update key */
	RTWDOG->TOVAL = 0xFFFF;
	RTWDOG->CS = (uint32_t) ((RTWDOG->CS) & ~RTWDOG_CS_EN_MASK)
		| RTWDOG_CS_UPDATE_MASK;

	/* Disable Systick which might be enabled by bootrom */
	if (SysTick->CTRL & SysTick_CTRL_ENABLE_Msk) {
		SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
	}

	SCB_EnableICache();
	SCB_EnableDCache();

	_ClearFaults();

	/* Initialize system clock */
	clkInit();

	/*
	 * install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

	/* restore interrupt state */
	irq_unlock(oldLevel);
	return 0;
}

SYS_INIT(imxrt_init, PRE_KERNEL_1, 0);
