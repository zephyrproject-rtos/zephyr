/*
 * Copyright 2025 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/dt-bindings/clock/mcux_lpc_syscon_clock.h>
#include <fsl_clock.h>
#include <fsl_pmu.h>
#include <soc.h>

/* Board xtal frequency in Hz */
#define BOARD_XTAL_CLK_HZ                         32000U
/* Core clock frequency: 96MHz */
#define CLOCK_INIT_CORE_CLOCK                     96000000U
/* System clock frequency. */
extern uint32_t SystemCoreClock;

void board_early_init_hook(void)
{
	/* Enable APB clock gate for access to AON. */
	CLOCK_EnableClock(kCLOCK_GateAonAPB);
	/* Switch MAIN_CLK to SIRC to allow FIRC reconfiguration. */
	CLOCK_AttachClk(kSIRC_to_MAIN_CLK);
	/* Set flash wait states for SIRC frequency */
	CLOCK_SetFlashWaitStateBasedOnFreq(12000000U);

	/* Unlock FIRCCSR register and disable FIRC */
	SCG0->FIRCCSR &= ~SCG_FIRCCSR_LK_MASK;
	SCG0->FIRCCSR &= ~SCG_FIRCCSR_FIRCEN_MASK;

	/* Set VDD_CORE_MAIN to 1.1V (typical value)*/
	PMU_UpdateVDDCore1P1InActiveMode(AON__PMU, 0x8FU);

	/* Set FROHF to 96MHz */
	CLOCK_SetupFROHFClocking(96000000U, 0U);
	/* FIRC is disabled in Deep Sleep mode */
	SCG0->FIRCCSR &= ~SCG_FIRCCSR_FIRCSTEN_MASK;
	/* Lock FIRCCSR register */
	SCG0->FIRCCSR |= SCG_FIRCCSR_LK_MASK;

	/* Set FROHFDIV divider to value 1 */
	CLOCK_SetClockDiv(kCLOCK_DivFRO_HF_DIV, 1U);
	/* Set AHBCLKDIV divider to value 1 */
	CLOCK_SetClockDiv(kCLOCK_DivAHBCLK, 1U);
	/* Set SLOWCLKDIV divider to value 4 */
	CLOCK_SetClockDiv(kCLOCK_DivAHBAIPSCLK, 4U);

	/* Switch MAIN_CLK to FIRC */
	CLOCK_SetFlashWaitStateBasedOnFreq(96000000U);
	CLOCK_AttachClk(kFIRC_to_MAIN_CLK);

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(gpio1))
	RESET_ReleasePeripheralReset(kGPIO1_RST_SHIFT_RSTn);
	CLOCK_EnableClock(kCLOCK_GateGPIO1);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(gpio2))
	RESET_ReleasePeripheralReset(kGPIO2_RST_SHIFT_RSTn);
	CLOCK_EnableClock(kCLOCK_GateGPIO2);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(gpio3))
	RESET_ReleasePeripheralReset(kGPIO3_RST_SHIFT_RSTn);
	CLOCK_EnableClock(kCLOCK_GateGPIO3);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(porta))
	CLOCK_EnableClock(kCLOCK_GateAonPORT);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(portb))
	RESET_ReleasePeripheralReset(kPORT1_RST_SHIFT_RSTn);
	CLOCK_EnableClock(kCLOCK_GatePORT1);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(portc))
	RESET_ReleasePeripheralReset(kPORT2_RST_SHIFT_RSTn);
	CLOCK_EnableClock(kCLOCK_GatePORT2);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(portd))
	RESET_ReleasePeripheralReset(kPORT3_RST_SHIFT_RSTn);
	CLOCK_EnableClock(kCLOCK_GatePORT3);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpuart0))
	/* Switch PERIPH_GROUP0 to FRO12M */
	CLOCK_AttachClk(kFRO12M_to_PERIPH_GROUP0);
	/* Set PGRP0CLKDIV divider to value 1 */
	CLOCK_SetClockDiv(kCLOCK_DivLPUART0, 1u);
	CLOCK_EnableClock(kCLOCK_GatePERIPH_GROUP0);
	RESET_ReleasePeripheralReset(kLPUART0_RST_SHIFT_RSTn);
	CLOCK_EnableClock(kCLOCK_GateLPUART0);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpuart1))
	/* Switch PERIPH_GROUP1 to FRO12M */
	CLOCK_AttachClk(kFRO12M_to_PERIPH_GROUP1);
	/* Set PGRP1CLKDIV divider to value 1 */
	CLOCK_SetClockDiv(kCLOCK_DivLPUART1, 1u);
	CLOCK_EnableClock(kCLOCK_GatePERIPH_GROUP1);
	RESET_ReleasePeripheralReset(kLPUART1_RST_SHIFT_RSTn);
	CLOCK_EnableClock(kCLOCK_GateLPUART1);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(aon_lpuart0))
	CLOCK_AttachClk(kFROdiv1_to_AON_COM);
	CLOCK_SetClockDiv(kCLOCK_DIVAonCMP, 1U);
	CLOCK_EnableClock(kCLOCK_GateAonUART);
#endif

	/* Set SystemCoreClock variable. */
	SystemCoreClock = CLOCK_INIT_CORE_CLOCK;
}
