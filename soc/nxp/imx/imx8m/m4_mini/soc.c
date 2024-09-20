/*
 * Copyright (c) 2020, Manivannan Sadhasivam <mani@kernel.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <fsl_clock.h>
#include <fsl_common.h>
#include <fsl_rdc.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <soc.h>

#include <zephyr/dt-bindings/rdc/imx_rdc.h>

/* OSC/PLL is already initialized by ROM and Cortex-A53 (u-boot) */
static void SOC_RdcInit(void)
{
	/* Move M4 core to specific RDC domain 1 */
	rdc_domain_assignment_t assignment = {0};

	assignment.domainId = M4_DOMAIN_ID;
	RDC_SetMasterDomainAssignment(RDC, kRDC_Master_M4, &assignment);

	/*
	 * The M4 core is running at domain 1, enable clock gate for
	 * Iomux to run at domain 1.
	 */
	CLOCK_EnableClock(kCLOCK_Iomux);
	CLOCK_EnableClock(kCLOCK_Ipmux1);
	CLOCK_EnableClock(kCLOCK_Ipmux2);
	CLOCK_EnableClock(kCLOCK_Ipmux3);
	CLOCK_EnableClock(kCLOCK_Ipmux4);

	CLOCK_EnableClock(kCLOCK_Qspi);

	/*
	 * The M4 core is running at domain 1, enable the PLL clock sources
	 * to domain 1.
	 */
	/* Enable SysPLL1 to Domain 1 */
	CLOCK_ControlGate(kCLOCK_SysPll1Gate, kCLOCK_ClockNeededAll);
	/* Enable SysPLL2 to Domain 1 */
	CLOCK_ControlGate(kCLOCK_SysPll2Gate, kCLOCK_ClockNeededAll);
	/* Enable SysPLL3 to Domain 1 */
	CLOCK_ControlGate(kCLOCK_SysPll3Gate, kCLOCK_ClockNeededAll);
	/* Enable AudioPLL1 to Domain 1 */
	CLOCK_ControlGate(kCLOCK_AudioPll1Gate, kCLOCK_ClockNeededAll);
	/* Enable AudioPLL2 to Domain 1 */
	CLOCK_ControlGate(kCLOCK_AudioPll2Gate, kCLOCK_ClockNeededAll);
	/* Enable VideoPLL1 to Domain 1 */
	CLOCK_ControlGate(kCLOCK_VideoPll1Gate, kCLOCK_ClockNeededAll);
}

/* AUDIO PLL1 configuration */
static const ccm_analog_frac_pll_config_t g_audioPll1Config = {
	.refSel  = kANALOG_PllRefOsc24M, /* PLL reference OSC24M */
	.mainDiv = 655U,
	.dsm     = 23593U,
	.preDiv  = 5U,
	.postDiv = 2U, /* AUDIO PLL1 frequency  = 786432000HZ */
};

/* AUDIO PLL2 configuration */
static const ccm_analog_frac_pll_config_t g_audioPll2Config = {
	.refSel  = kANALOG_PllRefOsc24M, /* PLL reference OSC24M */
	.mainDiv = 301U,
	.dsm     = 3670U,
	.preDiv  = 5U,
	.postDiv = 1U, /* AUDIO PLL2 frequency  = 722534399HZ */
};

static void SOC_ClockInit(void)
{
	/*
	 * Switch AHB NOC root to 24M first in order to configure
	 * the SYSTEM PLL1
	 */
	CLOCK_SetRootMux(kCLOCK_RootAhb, kCLOCK_AhbRootmuxOsc24M);

	/*
	 * Switch AXI M4 root to 24M first in order to configure
	 * the SYSTEM PLL2
	 */
	CLOCK_SetRootMux(kCLOCK_RootM4, kCLOCK_M4RootmuxOsc24M);

	/* Init AUDIO PLL1 to run at 786432000HZ */
	CLOCK_InitAudioPll1(&g_audioPll1Config);
	/* Init AUDIO PLL2 to run at 722534399HZ */
	CLOCK_InitAudioPll2(&g_audioPll2Config);

	CLOCK_SetRootDivider(kCLOCK_RootM4, 1U, 2U);
	/* Switch cortex-m4 to SYSTEM PLL1 */
	CLOCK_SetRootMux(kCLOCK_RootM4, kCLOCK_M4RootmuxSysPll1);

	CLOCK_SetRootDivider(kCLOCK_RootAhb, 1U, 1U);
	/* Switch AHB to SYSTEM PLL1 DIV6 = 133MHZ */
	CLOCK_SetRootMux(kCLOCK_RootAhb, kCLOCK_AhbRootmuxSysPll1Div6);

	/* Set root clock to 800MHZ/ 2= 400MHZ */
	CLOCK_SetRootDivider(kCLOCK_RootAudioAhb, 1U, 2U);
	/* switch AUDIO AHB to SYSTEM PLL1 */
	CLOCK_SetRootMux(kCLOCK_RootAudioAhb, kCLOCK_AudioAhbRootmuxSysPll1);

#if defined(CONFIG_UART_MCUX_IUART)
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(uart1))
	/* Set UART source to SysPLL1 Div10 80MHZ */
	CLOCK_SetRootMux(kCLOCK_RootUart1, kCLOCK_UartRootmuxSysPll1Div10);
	/* Set root clock to 80MHZ/ 1= 80MHZ */
	CLOCK_SetRootDivider(kCLOCK_RootUart1, 1U, 1U);
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(uart2))
	/* Set UART source to SysPLL1 Div10 80MHZ */
	CLOCK_SetRootMux(kCLOCK_RootUart2, kCLOCK_UartRootmuxSysPll1Div10);
	/* Set root clock to 80MHZ/ 1= 80MHZ */
	CLOCK_SetRootDivider(kCLOCK_RootUart2, 1U, 1U);
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(uart3))
	/* Set UART source to SysPLL1 Div10 80MHZ */
	CLOCK_SetRootMux(kCLOCK_RootUart3, kCLOCK_UartRootmuxSysPll1Div10);
	/* Set root clock to 80MHZ/ 1= 80MHZ */
	CLOCK_SetRootDivider(kCLOCK_RootUart3, 1U, 1U);
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(uart4))
	/* Set UART source to SysPLL1 Div10 80MHZ */
	CLOCK_SetRootMux(kCLOCK_RootUart4, kCLOCK_UartRootmuxSysPll1Div10);
	/* Set root clock to 80MHZ/ 1= 80MHZ */
	CLOCK_SetRootDivider(kCLOCK_RootUart4, 1U, 1U);
#endif
#endif

	/* Enable RDC clock */
	CLOCK_EnableClock(kCLOCK_Rdc);

	/*
	 * The purpose to enable the following modules clock is to make
	 * sure the M4 core could work normally when A53 core
	 * enters the low power state
	 */
	CLOCK_EnableClock(kCLOCK_Sim_display);
	CLOCK_EnableClock(kCLOCK_Sim_m);
	CLOCK_EnableClock(kCLOCK_Sim_main);
	CLOCK_EnableClock(kCLOCK_Sim_s);
	CLOCK_EnableClock(kCLOCK_Sim_wakeup);
	CLOCK_EnableClock(kCLOCK_Debug);
	CLOCK_EnableClock(kCLOCK_Dram);
	CLOCK_EnableClock(kCLOCK_Sec_Debug);
}

void soc_early_init_hook(void)
{

	/* SoC specific RDC settings */
	SOC_RdcInit();

	/* SoC specific Clock settings */
	SOC_ClockInit();
}
