/*
 * Copyright (c) 2021, Laird Connectivity
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
	/* Move M7 core to specific RDC domain 1 */
	rdc_domain_assignment_t assignment = {0};
	uint8_t domainId                   = 0U;

	domainId = RDC_GetCurrentMasterDomainId(RDC);
	/* Only configure the RDC if RDC peripheral write access allowed. */
	if ((0x1U & RDC_GetPeriphAccessPolicy(RDC, kRDC_Periph_RDC, domainId)) != 0U) {
		assignment.domainId = M7_DOMAIN_ID;
		RDC_SetMasterDomainAssignment(RDC, kRDC_Master_M7, &assignment);
	}

	/*
	 * The M7 core is running at domain 1, now enable the clock gate of the following IP/BUS/PLL
	 * in domain 1 in the CCM. In this way, to ensure the clock of the peripherals used by M
	 * core not be affected by A core which is running at domain 0.
	 */
	CLOCK_EnableClock(kCLOCK_Iomux);

	CLOCK_EnableClock(kCLOCK_Ipmux1);
	CLOCK_EnableClock(kCLOCK_Ipmux2);
	CLOCK_EnableClock(kCLOCK_Ipmux3);

#if defined(FLASH_TARGET)
	CLOCK_EnableClock(kCLOCK_Qspi);
#endif

	/* Enable the CCGR gate for SysPLL1 in Domain 1 */
	CLOCK_ControlGate(kCLOCK_SysPll1Gate, kCLOCK_ClockNeededAll);
	/* Enable the CCGR gate for SysPLL2 in Domain 1 */
	CLOCK_ControlGate(kCLOCK_SysPll2Gate, kCLOCK_ClockNeededAll);
	/* Enable the CCGR gate for SysPLL3 in Domain 1 */
	CLOCK_ControlGate(kCLOCK_SysPll3Gate, kCLOCK_ClockNeededAll);
#ifdef CONFIG_INIT_VIDEO_PLL
	/* Enable the CCGR gate for VideoPLL1 in Domain 1 */
	CLOCK_ControlGate(kCLOCK_VideoPll1Gate, kCLOCK_ClockNeededAll);
#endif
}

/* Integer PLLs: Fout = (mainDiv * refSel) / (preDiv * 2^ postDiv) */
/* SYSTEM PLL1 configuration */
const ccm_analog_integer_pll_config_t g_sysPll1Config = {
	.refSel  = kANALOG_PllRefOsc24M, /*!< PLL reference OSC24M */
	.mainDiv = 400U,
	.preDiv  = 3U,
	.postDiv = 2U, /*!< SYSTEM PLL1 frequency  = 800MHZ */
};

/* SYSTEM PLL2 configuration */
const ccm_analog_integer_pll_config_t g_sysPll2Config = {
	.refSel  = kANALOG_PllRefOsc24M, /*!< PLL reference OSC24M */
	.mainDiv = 250U,
	.preDiv  = 3U,
	.postDiv = 1U, /*!< SYSTEM PLL2 frequency  = 1000MHZ */
};

/* SYSTEM PLL3 configuration */
const ccm_analog_integer_pll_config_t g_sysPll3Config = {
	.refSel  = kANALOG_PllRefOsc24M, /*!< PLL reference OSC24M */
	.mainDiv = 300,
	.preDiv  = 3U,
	.postDiv = 2U, /*!< SYSTEM PLL3 frequency  = 600MHZ */
};

static void SOC_ClockInit(void)
{
	/*
	 * The following steps just show how to configure the PLL clock sources using the clock
	 * driver on M7 core side . Please note that the ROM has already configured the SYSTEM PLL1
	 * to 800Mhz when power up the SOC, meanwhile A core would enable SYSTEM PLL1, SYSTEM PLL2
	 * and SYSTEM PLL3 by U-Boot. Therefore, there is no need to configure the system PLL again
	 * on M7 side, otherwise it would have a risk to make the SOC hang.
	 */

	/* switch AHB NOC root to 24M first in order to configure the SYSTEM PLL1. */
	CLOCK_SetRootMux(kCLOCK_RootAhb, kCLOCK_AhbRootmuxOsc24M);

	/* switch AXI M7 root to 24M first in order to configure the SYSTEM PLL2. */
	CLOCK_SetRootMux(kCLOCK_RootM7, kCLOCK_M7RootmuxOsc24M);

	/* Set root clock to 800M */
	CLOCK_SetRootDivider(kCLOCK_RootM7, 1U, 1U);
	/* switch cortex-m7 to SYSTEM PLL1 */
	CLOCK_SetRootMux(kCLOCK_RootM7, kCLOCK_M7RootmuxSysPll1);

	/* Set root clock freq to 133M / 1= 133MHZ */
	CLOCK_SetRootDivider(kCLOCK_RootAhb, 1U, 1U);
	/* switch AHB to SYSTEM PLL1 DIV6 */
	CLOCK_SetRootMux(kCLOCK_RootAhb, kCLOCK_AhbRootmuxSysPll1Div6);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart4), okay) && CONFIG_UART_MCUX_IUART
	/* Set UART source to SysPLL1 Div10 80MHZ */
	CLOCK_SetRootMux(kCLOCK_RootUart4, kCLOCK_UartRootmuxSysPll1Div10);
	/* Set root clock to 80MHZ/ 1= 80MHZ */
	CLOCK_SetRootDivider(kCLOCK_RootUart4, 1U, 1U);
#endif

	CLOCK_EnableClock(kCLOCK_Rdc);   /* Enable RDC clock */
	CLOCK_EnableClock(kCLOCK_Ocram); /* Enable Ocram clock */

	/* The purpose to enable the following modules clock is to make sure the M7 core could work
	 * normally when A53 core enters the low power status.
	 */
	CLOCK_EnableClock(kCLOCK_Sim_m);
	CLOCK_EnableClock(kCLOCK_Sim_main);
	CLOCK_EnableClock(kCLOCK_Sim_s);
	CLOCK_EnableClock(kCLOCK_Sim_wakeup);
	CLOCK_EnableClock(kCLOCK_Debug);
	CLOCK_EnableClock(kCLOCK_Dram);
	CLOCK_EnableClock(kCLOCK_Sec_Debug);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart4), okay) && CONFIG_UART_MCUX_IUART
	CLOCK_EnableClock(kCLOCK_Uart4);
#endif
}

static int nxp_mimx8ml8_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	/* SoC specific RDC settings */
	SOC_RdcInit();

	/* SoC specific Clock settings */
	SOC_ClockInit();

	return 0;
}

SYS_INIT(nxp_mimx8ml8_init, PRE_KERNEL_1, 0);
