/*
 * Copyright (c) 2018, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <soc.h>
#include <dt-bindings/rdc/imx_rdc.h>
#include <cortex_m/exc.h>
#include "wdog_imx.h"

/* Initialize Resource Domain Controller. */
static void SOC_RdcInit(void)
{
	/* Move M4 core to the configured RDC domain */
	RDC_SetDomainID(RDC, rdcMdaM4, M4_DOMAIN_ID, false);

	/* Set access to WDOG3 for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapWdog3,
			RDC_DOMAIN_PERM(M4_DOMAIN_ID, RDC_DOMAIN_PERM_RW),
			false, false);

#ifdef CONFIG_UART_IMX_UART_1
	/* Set access to UART_1 for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapUart1, UART_1_RDC, false, false);
#endif /* CONFIG_UART_IMX_UART_1 */
#ifdef CONFIG_UART_IMX_UART_2
	/* Set access to UART_2 for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapUart2, UART_2_RDC, false, false);
#endif /* CONFIG_UART_IMX_UART_2 */
#ifdef CONFIG_UART_IMX_UART_3
	/* Set access to UART_3 for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapUart3, UART_3_RDC, false, false);
#endif /* CONFIG_UART_IMX_UART_3 */
#ifdef CONFIG_UART_IMX_UART_4
	/* Set access to UART_4 for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapUart4, UART_4_RDC, false, false);
#endif /* CONFIG_UART_IMX_UART_4 */
#ifdef CONFIG_UART_IMX_UART_5
	/* Set access to UART_5 for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapUart5, UART_5_RDC, false, false);
#endif /* CONFIG_UART_IMX_UART_5 */
#ifdef CONFIG_UART_IMX_UART_6
	/* Set access to UART_6 for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapUart6, UART_6_RDC, false, false);
#endif /* CONFIG_UART_IMX_UART_6 */
#ifdef CONFIG_GPIO_IMX_PORT_1
	/* Set access to GPIO_1 for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapGpio1, GPIO_1_RDC, false, false);
#endif /* CONFIG_GPIO_IMX_PORT_1 */
#ifdef CONFIG_GPIO_IMX_PORT_2
	/* Set access to GPIO_2 for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapGpio2, GPIO_2_RDC, false, false);
#endif /* CONFIG_GPIO_IMX_PORT_2 */
#ifdef CONFIG_GPIO_IMX_PORT_3
	/* Set access to GPIO_3 for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapGpio3, GPIO_3_RDC, false, false);
#endif /* CONFIG_GPIO_IMX_PORT_3 */
#ifdef CONFIG_GPIO_IMX_PORT_4
	/* Set access to GPIO_4 for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapGpio4, GPIO_4_RDC, false, false);
#endif /* CONFIG_GPIO_IMX_PORT_4 */
#ifdef CONFIG_GPIO_IMX_PORT_5
	/* Set access to GPIO_5 for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapGpio5, GPIO_5_RDC, false, false);
#endif /* CONFIG_GPIO_IMX_PORT_5 */
#ifdef CONFIG_GPIO_IMX_PORT_6
	/* Set access to GPIO_6 for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapGpio6, GPIO_6_RDC, false, false);
#endif /* CONFIG_GPIO_IMX_PORT_6 */
#ifdef CONFIG_GPIO_IMX_PORT_7
	/* Set access to GPIO_7 for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapGpio7, GPIO_7_RDC, false, false);
#endif /* CONFIG_GPIO_IMX_PORT_7 */
}

/* Initialize cache. */
static void SOC_CacheInit(void)
{
	/* Enable System Bus Cache */
	/* set command to invalidate all ways and write GO bit
	 * to initiate command
	 */
	LMEM_PSCCR = LMEM_PSCCR_INVW1_MASK | LMEM_PSCCR_INVW0_MASK;
	LMEM_PSCCR |= LMEM_PSCCR_GO_MASK;
	/* Wait until the command completes */
	while (LMEM_PSCCR & LMEM_PSCCR_GO_MASK)
		;
	/* Enable system bus cache, enable write buffer */
	LMEM_PSCCR = (LMEM_PSCCR_ENWRBUF_MASK | LMEM_PSCCR_ENCACHE_MASK);
	__ISB();

	/* Enable Code Bus Cache */
	/* set command to invalidate all ways and write GO bit
	 * to initiate command
	 */
	LMEM_PCCCR = LMEM_PCCCR_INVW1_MASK | LMEM_PCCCR_INVW0_MASK;
	LMEM_PCCCR |= LMEM_PCCCR_GO_MASK;
	/* Wait until the command completes */
	while (LMEM_PCCCR & LMEM_PCCCR_GO_MASK)
		;
	/* Enable code bus cache, enable write buffer */
	LMEM_PCCCR = (LMEM_PCCCR_ENWRBUF_MASK | LMEM_PCCCR_ENCACHE_MASK);
	__ISB();
	__DSB();
}

/* Initialize clock. */
static void SOC_ClockInit(void)
{
	/* OSC/PLL is already initialized by Cortex-A9 core */

	/* Enable IP bridge and IO mux clock */
	CCM_ControlGate(CCM, ccmCcgrGateIomuxIptClkIo, ccmClockNeededAll);
	CCM_ControlGate(CCM, ccmCcgrGateIpmux1Clk, ccmClockNeededAll);
	CCM_ControlGate(CCM, ccmCcgrGateIpmux2Clk, ccmClockNeededAll);
	CCM_ControlGate(CCM, ccmCcgrGateIpmux3Clk, ccmClockNeededAll);

#ifdef CONFIG_UART_IMX
	/* Set UART clock is derived from OSC clock (24M) */
	CCM_SetRootMux(CCM, ccmRootUartClkSel, ccmRootmuxUartClkOsc24m);

	/* Configure UART divider */
	CCM_SetRootDivider(CCM, ccmRootUartClkPodf, 0);

	/* Enable UART clock */
	CCM_ControlGate(CCM, ccmCcgrGateUartClk, ccmClockNeededAll);
	CCM_ControlGate(CCM, ccmCcgrGateUartSerialClk, ccmClockNeededAll);
#endif /* CONFIG_UART_IMX */
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
static int mcimx6x_m4_init(struct device *arg)
{
	ARG_UNUSED(arg);

	unsigned int oldLevel; /* Old interrupt lock level */

	/* Disable interrupts */
	oldLevel = irq_lock();

	/* Configure RDC */
	SOC_RdcInit();

	/* Disable WDOG3 powerdown */
	WDOG_DisablePowerdown(WDOG3);

	/* Initialize Cache */
	SOC_CacheInit();

	_ClearFaults();

	/* Initialize clock */
	SOC_ClockInit();

	/*
	 * Install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

	/* Restore interrupt state */
	irq_unlock(oldLevel);

	return 0;
}

SYS_INIT(mcimx6x_m4_init, PRE_KERNEL_1, 0);
