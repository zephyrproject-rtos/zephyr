/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <soc.h>
#include "wdog_imx.h"

/* Initialize clock. */
void SOC_ClockInit(void)
{
	/* OSC/PLL is already initialized by Cortex-A7 (u-boot) */

	/*
	 * Disable WDOG3
	 *	Note : The WDOG clock Root is shared by all the 4 WDOGs,
	 *	so Zephyr code should avoid closing it
	 */
	CCM_UpdateRoot(CCM, ccmRootWdog, ccmRootmuxWdogOsc24m, 0, 0);
	CCM_EnableRoot(CCM, ccmRootWdog);
	CCM_ControlGate(CCM, ccmCcgrGateWdog3, ccmClockNeededRun);

	RDC_SetPdapAccess(RDC, rdcPdapWdog3,
			RDC_DOMAIN_PERM(CONFIG_DOMAIN_ID, RDC_DOMAIN_PERM_RW),
			false, false);

	WDOG_DisablePowerdown(WDOG3);

	CCM_ControlGate(CCM, ccmCcgrGateWdog3, ccmClockNotNeeded);

	/* We need system PLL Div2 to run M4 core */
	CCM_ControlGate(CCM, ccmPllGateSys, ccmClockNeededRun);
	CCM_ControlGate(CCM, ccmPllGateSysDiv2, ccmClockNeededRun);

	/* Enable clock gate for IP bridge and IO mux */
	CCM_ControlGate(CCM, ccmCcgrGateIpmux1, ccmClockNeededRun);
	CCM_ControlGate(CCM, ccmCcgrGateIpmux2, ccmClockNeededRun);
	CCM_ControlGate(CCM, ccmCcgrGateIpmux3, ccmClockNeededRun);
	CCM_ControlGate(CCM, ccmCcgrGateIomux, ccmClockNeededRun);
	CCM_ControlGate(CCM, ccmCcgrGateIomuxLpsr, ccmClockNeededRun);

	/* Enable clock gate for RDC */
	CCM_ControlGate(CCM, ccmCcgrGateRdc, ccmClockNeededRun);
}

void SOC_RdcInit(void)
{
	/* Move M4 core to specific RDC domain */
	RDC_SetDomainID(RDC, rdcMdaM4, CONFIG_DOMAIN_ID, false);
}

#ifdef CONFIG_GPIO_IMX
static void nxp_mcimx7_gpio_config(void)
{

#ifdef CONFIG_GPIO_IMX_PORT_1
	RDC_SetPdapAccess(RDC, rdcPdapGpio1,
			  RDC_DOMAIN_PERM(CONFIG_DOMAIN_ID, RDC_DOMAIN_PERM_RW),
			  false, false);
	/* Enable gpio clock gate */
	CCM_ControlGate(CCM, ccmCcgrGateGpio1, ccmClockNeededRunWait);
#endif /* CONFIG_GPIO_IMX_PORT_1 */


#ifdef CONFIG_GPIO_IMX_PORT_2
	RDC_SetPdapAccess(RDC, rdcPdapGpio2,
			  RDC_DOMAIN_PERM(CONFIG_DOMAIN_ID, RDC_DOMAIN_PERM_RW),
			  false, false);
	/* Enable gpio clock gate */
	CCM_ControlGate(CCM, ccmCcgrGateGpio2, ccmClockNeededRunWait);
#endif /* CONFIG_GPIO_IMX_PORT_2 */

}
#endif /* CONFIG_GPIO_IMX */

#ifdef CONFIG_UART_IMX
static void nxp_mcimx7_uart_config(void)
{

#ifdef CONFIG_UART_IMX_UART_2
	/* We need to grasp board uart exclusively */
	RDC_SetPdapAccess(RDC, rdcPdapUart2,
			RDC_DOMAIN_PERM(CONFIG_DOMAIN_ID, RDC_DOMAIN_PERM_RW),
			false, false);
	/* Select clock derived from OSC clock(24M) */
	CCM_UpdateRoot(CCM, ccmRootUart2, ccmRootmuxUartOsc24m, 0, 0);
	/* Enable uart clock */
	CCM_EnableRoot(CCM, ccmRootUart2);
	/*
	 * IC Limitation
	 * M4 stop will cause A7 UART lose functionality
	 * So we need UART clock all the time
	 */
	CCM_ControlGate(CCM, ccmCcgrGateUart2, ccmClockNeededAll);
#endif /* #ifdef CONFIG_UART_IMX_UART_2 */

}
#endif /* CONFIG_UART_IMX */

static int nxp_mcimx7_init(struct device *arg)
{
	ARG_UNUSED(arg);

	/* SoC specific RDC settings */
	SOC_RdcInit();

	/* BoC specific clock settings */
	SOC_ClockInit();

#ifdef CONFIG_GPIO_IMX
	nxp_mcimx7_gpio_config();
#endif /* CONFIG_GPIO_IMX */

#ifdef CONFIG_UART_IMX
	nxp_mcimx7_uart_config();
#endif /* CONFIG_UART_IMX */

	return 0;
}

SYS_INIT(nxp_mcimx7_init, PRE_KERNEL_1, 0);
