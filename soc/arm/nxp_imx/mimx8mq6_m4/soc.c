/*
 * Copyright (c) 2021, Kwon Tae-young <tykwon@m2i.co.kr>
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

	/*
	 * The M4 core is running at domain 1, enable the PLL clock sources
	 * to domain 1.
	 */
	CLOCK_ControlGate(kCLOCK_SysPll1Gate, kCLOCK_ClockNeededAll);
	CLOCK_ControlGate(kCLOCK_SysPll2Gate, kCLOCK_ClockNeededAll);
	CLOCK_ControlGate(kCLOCK_SysPll3Gate, kCLOCK_ClockNeededAll);
	CLOCK_ControlGate(kCLOCK_AudioPll1Gate, kCLOCK_ClockNeededAll);
	CLOCK_ControlGate(kCLOCK_AudioPll2Gate, kCLOCK_ClockNeededAll);
	CLOCK_ControlGate(kCLOCK_VideoPll1Gate, kCLOCK_ClockNeededAll);
	CLOCK_ControlGate(kCLOCK_VideoPll2Gate, kCLOCK_ClockNeededAll);
}

static void SOC_ClockInit(void)
{
	/*
	 * Switch AHB NOC root to 25M first in order to configure
	 * the SYSTEM PLL1
	 */
	CLOCK_SetRootMux(kCLOCK_RootAhb, kCLOCK_AhbRootmuxOsc25m);
	CLOCK_SetRootDivider(kCLOCK_RootAhb, 1U, 1U);
	/* Switch AHB to SYSTEM PLL1 DIV6 = 133MHZ */
	CLOCK_SetRootMux(kCLOCK_RootAhb, kCLOCK_AhbRootmuxSysPll1Div6);

	/*
	 * Switch AXI M4 root to 25M first in order to configure
	 * the SYSTEM PLL1
	 */
	CLOCK_SetRootMux(kCLOCK_RootM4, kCLOCK_M4RootmuxOsc25m);
	CLOCK_SetRootDivider(kCLOCK_RootM4, 1U, 1U);
	/* Switch cortex-m4 to SYSTEM PLL1 DIV3 */
	CLOCK_SetRootMux(kCLOCK_RootM4, kCLOCK_M4RootmuxSysPll1Div3);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart1), okay)
	/* Set UART source to SysPLL1 Div10 80MHZ */
	CLOCK_SetRootMux(kCLOCK_RootUart1, kCLOCK_UartRootmuxSysPll1Div10);
	/* Set root clock to 80MHZ/ 1= 80MHZ */
	CLOCK_SetRootDivider(kCLOCK_RootUart1, 1U, 1U);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart2), okay)
	/* Set UART source to SysPLL1 Div10 80MHZ */
	CLOCK_SetRootMux(kCLOCK_RootUart2, kCLOCK_UartRootmuxSysPll1Div10);
	/* Set root clock to 80MHZ/ 1= 80MHZ */
	CLOCK_SetRootDivider(kCLOCK_RootUart2, 1U, 1U);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart3), okay)
	/* Set UART source to SysPLL1 Div10 80MHZ */
	CLOCK_SetRootMux(kCLOCK_RootUart3, kCLOCK_UartRootmuxSysPll1Div10);
	/* Set root clock to 80MHZ/ 1= 80MHZ */
	CLOCK_SetRootDivider(kCLOCK_RootUart3, 1U, 1U);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart4), okay)
	/* Set UART source to SysPLL1 Div10 80MHZ */
	CLOCK_SetRootMux(kCLOCK_RootUart4, kCLOCK_UartRootmuxSysPll1Div10);
	/* Set root clock to 80MHZ/ 1= 80MHZ */
	CLOCK_SetRootDivider(kCLOCK_RootUart4, 1U, 1U);
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

static int nxp_mimx8mq6_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	/* SoC specific RDC settings */
	SOC_RdcInit();

	/* SoC specific Clock settings */
	SOC_ClockInit();

	return 0;
}

SYS_INIT(nxp_mimx8mq6_init, PRE_KERNEL_1, 0);
