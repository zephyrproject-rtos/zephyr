/*
 * Copyright (c) 2018, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <soc.h>
#include <zephyr/dt-bindings/rdc/imx_rdc.h>
#include <zephyr/arch/arm/aarch32/cortex_m/cmsis.h>
#include <zephyr/arch/arm/aarch32/nmi.h>
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

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart1), okay)
	/* Set access to UART_1 for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapUart1, RDC_DT_VAL(uart1), false, false);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart2), okay)
	/* Set access to UART_2 for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapUart2, RDC_DT_VAL(uart2), false, false);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart3), okay)
	/* Set access to UART_3 for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapUart3, RDC_DT_VAL(uart3), false, false);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart4), okay)
	/* Set access to UART_4 for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapUart4, RDC_DT_VAL(uart4), false, false);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart5), okay)
	/* Set access to UART_5 for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapUart5, RDC_DT_VAL(uart5), false, false);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart6), okay)
	/* Set access to UART_6 for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapUart6, RDC_DT_VAL(uart6), false, false);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio1), okay)
	/* Set access to GPIO_1 for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapGpio1, RDC_DT_VAL(gpio1), false, false);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio2), okay)
	/* Set access to GPIO_2 for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapGpio2, RDC_DT_VAL(gpio2), false, false);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio3), okay)
	/* Set access to GPIO_3 for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapGpio3, RDC_DT_VAL(gpio3), false, false);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio4), okay)
	/* Set access to GPIO_4 for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapGpio4, RDC_DT_VAL(gpio4), false, false);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio5), okay)
	/* Set access to GPIO_5 for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapGpio5, RDC_DT_VAL(gpio5), false, false);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio6), okay)
	/* Set access to GPIO_6 for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapGpio6, RDC_DT_VAL(gpio6), false, false);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio7), okay)
	/* Set access to GPIO_7 for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapGpio7, RDC_DT_VAL(gpio7), false, false);
#endif

#ifdef CONFIG_IPM_IMX
	/* Set access to MU B for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapMuB, RDC_DT_VAL(mub), false, false);
#endif /* CONFIG_IPM_IMX */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(epit1), okay)
	/* Set access to EPIT_1 for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapEpit1, RDC_DT_VAL(epit1), false, false);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(epit2), okay)
	/* Set access to EPIT_2 for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapEpit2, RDC_DT_VAL(epit2), false, false);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c1), okay)
	/* Set access to I2C-1 for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapI2c1, RDC_DT_VAL(i2c1), false, false);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c2), okay)
	/* Set access to I2C-2 for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapI2c2, RDC_DT_VAL(i2c2), false, false);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c3), okay)
	/* Set access to I2C-3 for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapI2c3, RDC_DT_VAL(i2c3), false, false);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c4), okay)
	/* Set access to I2C-4 for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapI2c4, RDC_DT_VAL(i2c4), false, false);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(pwm1), okay)
	/* Set access to PWM-1 for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapPwm1, RDC_DT_VAL(pwm1), false, false);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pwm2), okay)
	/* Set access to PWM-2 for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapPwm2, RDC_DT_VAL(pwm2), false, false);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pwm3), okay)
	/* Set access to PWM-3 for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapPwm3, RDC_DT_VAL(pwm3), false, false);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pwm4), okay)
	/* Set access to PWM-4 for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapPwm4, RDC_DT_VAL(pwm4), false, false);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pwm5), okay)
	/* Set access to PWM-5 for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapPwm5, RDC_DT_VAL(pwm5), false, false);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pwm6), okay)
	/* Set access to PWM-6 for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapPwm6, RDC_DT_VAL(pwm6), false, false);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pwm7), okay)
	/* Set access to PWM-7 for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapPwm7, RDC_DT_VAL(pwm7), false, false);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pwm8), okay)
	/* Set access to PWM-8 for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapPwm8, RDC_DT_VAL(pwm8), false, false);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(adc1), okay)
	/* Set access to ADC-1 for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapAdc1, RDC_DT_VAL(adc1), false, false);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(adc2), okay)
	/* Set access to ADC-2 for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapAdc2, RDC_DT_VAL(adc2), false, false);
#endif
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

#ifdef CONFIG_COUNTER_IMX_EPIT
	/* Select EPIT clock is derived from OSC (24M) */
	CCM_SetRootMux(CCM, ccmRootPerclkClkSel, ccmRootmuxPerclkClkOsc24m);

	/* Configure EPIT divider */
	CCM_SetRootDivider(CCM, ccmRootPerclkPodf, 0);

	/* Enable EPIT clocks */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(epit1), okay)
	CCM_ControlGate(CCM, ccmCcgrGateEpit1Clk, ccmClockNeededAll);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(epit2), okay)
	CCM_ControlGate(CCM, ccmCcgrGateEpit2Clk, ccmClockNeededAll);
#endif
#endif /* CONFIG_COUNTER_IMX_EPIT */

#ifdef CONFIG_I2C_IMX
	/* Select I2C clock is derived from OSC (24M) */
	CCM_SetRootMux(CCM, ccmRootPerclkClkSel, ccmRootmuxPerclkClkOsc24m);

	/* Set relevant divider = 1. */
	CCM_SetRootDivider(CCM, ccmRootPerclkPodf, 0);

	/* Enable I2C clock */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c1), okay)
	CCM_ControlGate(CCM, ccmCcgrGateI2c1Serialclk, ccmClockNeededAll);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c2), okay)
	CCM_ControlGate(CCM, ccmCcgrGateI2c2Serialclk, ccmClockNeededAll);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c3), okay)
	CCM_ControlGate(CCM, ccmCcgrGateI2c3Serialclk, ccmClockNeededAll);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c4), okay)
	CCM_ControlGate(CCM, ccmCcgrGateI2c4Serialclk, ccmClockNeededAll);
#endif
#endif /* CONFIG_I2C_IMX */

#ifdef CONFIG_PWM_IMX
	/* Select PWM clock is derived from OSC (24M) */
	CCM_SetRootMux(CCM, ccmRootPerclkClkSel, ccmRootmuxPerclkClkOsc24m);

	/* Set relevant divider = 1. */
	CCM_SetRootDivider(CCM, ccmRootPerclkPodf, 0);

	/* Enable PWM clock */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pwm1), okay)
	CCM_ControlGate(CCM, ccmCcgrGatePwm1Clk, ccmClockNeededAll);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pwm2), okay)
	CCM_ControlGate(CCM, ccmCcgrGatePwm2Clk, ccmClockNeededAll);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pwm3), okay)
	CCM_ControlGate(CCM, ccmCcgrGatePwm3Clk, ccmClockNeededAll);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pwm4), okay)
	CCM_ControlGate(CCM, ccmCcgrGatePwm4Clk, ccmClockNeededAll);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pwm5), okay)
	CCM_ControlGate(CCM, ccmCcgrGatePwm5Clk, ccmClockNeededAll);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pwm6), okay)
	CCM_ControlGate(CCM, ccmCcgrGatePwm6Clk, ccmClockNeededAll);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pwm7), okay)
	CCM_ControlGate(CCM, ccmCcgrGatePwm7Clk, ccmClockNeededAll);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pwm8), okay)
	CCM_ControlGate(CCM, ccmCcgrGatePwm8Clk, ccmClockNeededAll);
#endif
#endif /* CONFIG_PWM_IMX */
}

/**
 *
 * @brief Perform basic hardware initialization
 *
 * Initialize the interrupt controller device drivers.
 * Also initialize the counter device driver, if required.
 *
 * @return 0
 */
static int mcimx6x_m4_init(void)
{

	unsigned int oldLevel; /* Old interrupt lock level */

	/* Disable interrupts */
	oldLevel = irq_lock();

	/* Configure RDC */
	SOC_RdcInit();

	/* Disable WDOG3 powerdown */
	WDOG_DisablePowerdown(WDOG3);

	/* Initialize Cache */
	SOC_CacheInit();

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
