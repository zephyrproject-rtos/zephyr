/*
 * Copyright 2025-2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <fsl_clock.h>
#include <soc.h>

/* Board xtal frequency in Hz */
#define BOARD_XTAL_CLK_HZ     32000U
/* Core clock frequency: 96MHz */
#define CLOCK_INIT_CORE_CLOCK 96000000U

#if DT_NODE_EXISTS(DT_PATH(cpus, cpu_0))
#define CPU_NODE DT_PATH(cpus, cpu_0)
#elif DT_NODE_EXISTS(DT_PATH(cpus, cpu_1))
#define CPU_NODE DT_PATH(cpus, cpu_1)
#else
#error "No CPU node found in devicetree"
#endif

/* Core clock frequency */
#define CPU_CLOCK_FREQ DT_PROP(CPU_NODE, clock_frequency)

/* System clock frequency. */
extern uint32_t SystemCoreClock;

void board_early_init_hook(void)
{
	/* Enable APB clock gate for access to AON. */
	CLOCK_EnableClock(kCLOCK_GateAonAPB);

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(porta))
	CLOCK_EnableClock(kCLOCK_GateAonPORT);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpuart0)) || \
	DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpspi0)) || \
	DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpi2c0))
	CLOCK_AttachClk(kFRO12M_to_PERIPH_GROUP0);
	CLOCK_SetClockDiv(kCLOCK_DivPeriphGroup0, 1u);
	CLOCK_EnableClock(kCLOCK_GatePERIPH_GROUP0);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpuart1)) || \
	DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpspi1)) || \
	DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpi2c1))
	CLOCK_AttachClk(kFRO12M_to_PERIPH_GROUP1);
	CLOCK_SetClockDiv(kCLOCK_DivPeriphGroup1, 1u);
	CLOCK_EnableClock(kCLOCK_GatePERIPH_GROUP1);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(aon_lpuart0)) || \
	DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(aon_lpi2c0))
	CLOCK_AttachClk(kFROdiv1_to_AON_COM);
	CLOCK_SetClockDiv(kCLOCK_DIVAonCMP, 1U);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(aon_lpuart0))
	CLOCK_EnableClock(kCLOCK_GateAonUART);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(aon_lpi2c0))
	CLOCK_EnableClock(kCLOCK_GateAonI2C);
	RESET_ReleasePeripheralReset(kAonI2C_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(aon_qtmr0)) || \
	DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(aon_qtmr1))
	CLOCK_AttachClk(kFROdiv4_to_AON_TMR);
	/* AON QTMR0 and AON QTMR1 share one reset line; deassert it here. */
	RESET_ReleasePeripheralReset(kAonQTMR0_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(aon_qtmr0))
	CLOCK_EnableClock(kCLOCK_GateAonQTMR0);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(aon_qtmr1))
	CLOCK_EnableClock(kCLOCK_GateAonQTMR1);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ostimer0))
	/* Select 1 MHz clock source for OSTIMER0. */
	CLOCK_AttachClk(kCLK_1M_to_OSTIMER0);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(rtc))
	if (!CLOCK_IsRoscInitialized()) {
		rosc_init_config_t rosc_init_config;

		CLOCK_GetDefaultInitRoscConfig(&rosc_init_config);
		rosc_init_config.detectionDelay = 50U;
		rosc_init_config.detectionTimeout = 0U;
		rosc_init_config.detectionDelaySwitchedMode = 50U;
		rosc_init_config.detectionTimeoutSwitchedMode = 50U;
		(void)CLOCK_InitRosc(&rosc_init_config);
	}
#endif

	/* Set SystemCoreClock variable. */
	SystemCoreClock = CPU_CLOCK_FREQ;
}
