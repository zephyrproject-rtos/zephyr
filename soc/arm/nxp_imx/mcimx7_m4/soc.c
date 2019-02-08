/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <soc.h>
#include <dt-bindings/rdc/imx_rdc.h>
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
			RDC_DOMAIN_PERM(M4_DOMAIN_ID, RDC_DOMAIN_PERM_RW),
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
	RDC_SetDomainID(RDC, rdcMdaM4, M4_DOMAIN_ID, false);
}

#ifdef CONFIG_GPIO_IMX
static void nxp_mcimx7_gpio_config(void)
{

#ifdef CONFIG_GPIO_IMX_PORT_1
	RDC_SetPdapAccess(RDC, rdcPdapGpio1, DT_NXP_IMX_GPIO_GPIO_1_RDC, false, false);
	/* Enable gpio clock gate */
	CCM_ControlGate(CCM, ccmCcgrGateGpio1, ccmClockNeededRunWait);
#endif /* CONFIG_GPIO_IMX_PORT_1 */


#ifdef CONFIG_GPIO_IMX_PORT_2
	RDC_SetPdapAccess(RDC, rdcPdapGpio2, DT_NXP_IMX_GPIO_GPIO_2_RDC, false, false);
	/* Enable gpio clock gate */
	CCM_ControlGate(CCM, ccmCcgrGateGpio2, ccmClockNeededRunWait);
#endif /* CONFIG_GPIO_IMX_PORT_2 */


#ifdef CONFIG_GPIO_IMX_PORT_7
	RDC_SetPdapAccess(RDC, rdcPdapGpio7, DT_NXP_IMX_GPIO_GPIO_7_RDC, false, false);
	/* Enable gpio clock gate */
	CCM_ControlGate(CCM, ccmCcgrGateGpio7, ccmClockNeededRunWait);
#endif /* CONFIG_GPIO_IMX_PORT_2 */

}
#endif /* CONFIG_GPIO_IMX */

#ifdef CONFIG_UART_IMX
static void nxp_mcimx7_uart_config(void)
{

#ifdef CONFIG_UART_IMX_UART_2
	/* We need to grasp board uart exclusively */
	RDC_SetPdapAccess(RDC, rdcPdapUart2, DT_NXP_IMX_UART_UART_2_RDC, false, false);
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

#ifdef CONFIG_UART_IMX_UART_6
	/* We need to grasp board uart exclusively */
	RDC_SetPdapAccess(RDC, rdcPdapUart6, DT_NXP_IMX_UART_UART_6_RDC, false, false);
	/* Select clock derived from OSC clock(24M) */
	CCM_UpdateRoot(CCM, ccmRootUart6, ccmRootmuxUartOsc24m, 0, 0);
	/* Enable uart clock */
	CCM_EnableRoot(CCM, ccmRootUart6);
	/*
	 * IC Limitation
	 * M4 stop will cause A7 UART lose functionality
	 * So we need UART clock all the time
	 */
	CCM_ControlGate(CCM, ccmCcgrGateUart6, ccmClockNeededAll);
#endif /* #ifdef CONFIG_UART_IMX_UART_6 */
}
#endif /* CONFIG_UART_IMX */


#ifdef CONFIG_I2C_IMX
static void nxp_mcimx7_i2c_config(void)
{

#ifdef CONFIG_I2C_1
	/* In this example, we need to grasp board I2C exclusively */
	RDC_SetPdapAccess(RDC, rdcPdapI2c1, DT_FSL_IMX7D_I2C_I2C_1_RDC, false, false);
	/* Select I2C clock derived from OSC clock(24M) */
	CCM_UpdateRoot(CCM, ccmRootI2c1, ccmRootmuxI2cOsc24m, 0, 0);
	/* Enable I2C clock */
	CCM_EnableRoot(CCM, ccmRootI2c1);
	CCM_ControlGate(CCM, ccmCcgrGateI2c1, ccmClockNeededRunWait);
#endif /* CONFIG_I2C_1 */

#ifdef CONFIG_I2C_2
	/* In this example, we need to grasp board I2C exclusively */
	RDC_SetPdapAccess(RDC, rdcPdapI2c2, DT_FSL_IMX7D_I2C_I2C_2_RDC, false, false);
	/* Select I2C clock derived from OSC clock(24M) */
	CCM_UpdateRoot(CCM, ccmRootI2c2, ccmRootmuxI2cOsc24m, 0, 0);
	/* Enable I2C clock */
	CCM_EnableRoot(CCM, ccmRootI2c2);
	CCM_ControlGate(CCM, ccmCcgrGateI2c2, ccmClockNeededRunWait);
#endif /* CONFIG_I2C_2 */

#ifdef CONFIG_I2C_3
	/* In this example, we need to grasp board I2C exclusively */
	RDC_SetPdapAccess(RDC, rdcPdapI2c3, DT_FSL_IMX7D_I2C_I2C_3_RDC, false, false);
	/* Select I2C clock derived from OSC clock(24M) */
	CCM_UpdateRoot(CCM, ccmRootI2c3, ccmRootmuxI2cOsc24m, 0, 0);
	/* Enable I2C clock */
	CCM_EnableRoot(CCM, ccmRootI2c3);
	CCM_ControlGate(CCM, ccmCcgrGateI2c3, ccmClockNeededRunWait);
#endif /* CONFIG_I2C_3 */

#ifdef CONFIG_I2C_4
	/* In this example, we need to grasp board I2C exclusively */
	RDC_SetPdapAccess(RDC, rdcPdapI2c4, DT_FSL_IMX7D_I2C_I2C_4_RDC, false, false);
	/* Select I2C clock derived from OSC clock(24M) */
	CCM_UpdateRoot(CCM, ccmRootI2c4, ccmRootmuxI2cOsc24m, 0, 0);
	/* Enable I2C clock */
	CCM_EnableRoot(CCM, ccmRootI2c4);
	CCM_ControlGate(CCM, ccmCcgrGateI2c4, ccmClockNeededRunWait);
#endif /* CONFIG_I2C_4 */

}
#endif /* CONFIG_I2C_IMX */

#ifdef CONFIG_PWM_IMX
static void nxp_mcimx7_pwm_config(void)
{

#ifdef CONFIG_PWM_1
	/* We need to grasp board pwm exclusively */
	RDC_SetPdapAccess(RDC, rdcPdapPwm1, DT_FSL_IMX7D_PWM_PWM_1_RDC, false, false);
	/* Select clock derived from OSC clock(24M) */
	CCM_UpdateRoot(CCM, ccmRootPwm1, ccmRootmuxPwmOsc24m, 0, 0);
	/* Enable pwm clock */
	CCM_EnableRoot(CCM, ccmRootPwm1);
	CCM_ControlGate(CCM, ccmCcgrGatePwm1, ccmClockNeededAll);
#endif /* #ifdef CONFIG_PWM_1 */

#ifdef CONFIG_PWM_2
	/* We need to grasp board pwm exclusively */
	RDC_SetPdapAccess(RDC, rdcPdapPwm2, DT_FSL_IMX7D_PWM_PWM_2_RDC, false, false);
	/* Select clock derived from OSC clock(24M) */
	CCM_UpdateRoot(CCM, ccmRootPwm2, ccmRootmuxPwmOsc24m, 0, 0);
	/* Enable pwm clock */
	CCM_EnableRoot(CCM, ccmRootPwm2);
	CCM_ControlGate(CCM, ccmCcgrGatePwm2, ccmClockNeededAll);
#endif /* #ifdef CONFIG_PWM_2 */

#ifdef CONFIG_PWM_3
	/* We need to grasp board pwm exclusively */
	RDC_SetPdapAccess(RDC, rdcPdapPwm3, DT_FSL_IMX7D_PWM_PWM_3_RDC, false, false);
	/* Select clock derived from OSC clock(24M) */
	CCM_UpdateRoot(CCM, ccmRootPwm3, ccmRootmuxPwmOsc24m, 0, 0);
	/* Enable pwm clock */
	CCM_EnableRoot(CCM, ccmRootPwm3);
	CCM_ControlGate(CCM, ccmCcgrGatePwm3, ccmClockNeededAll);
#endif /* #ifdef CONFIG_PWM_3 */

#ifdef CONFIG_PWM_4
	/* We need to grasp board pwm exclusively */
	RDC_SetPdapAccess(RDC, rdcPdapPwm4, DT_FSL_IMX7D_PWM_PWM_4_RDC, false, false);
	/* Select clock derived from OSC clock(24M) */
	CCM_UpdateRoot(CCM, ccmRootPwm4, ccmRootmuxPwmOsc24m, 0, 0);
	/* Enable pwm clock */
	CCM_EnableRoot(CCM, ccmRootPwm4);
	CCM_ControlGate(CCM, ccmCcgrGatePwm4, ccmClockNeededAll);
#endif /* #ifdef CONFIG_PWM_4 */

}
#endif /* CONFIG_PWM_IMX */

#ifdef CONFIG_IPM_IMX
static void nxp_mcimx7_mu_config(void)
{
	/* Set access to MU B for M4 core */
	RDC_SetPdapAccess(RDC, rdcPdapMuB, DT_NXP_IMX_MU_MU_B_RDC, false, false);

	/* Enable clock gate for MU*/
	CCM_ControlGate(CCM, ccmCcgrGateMu, ccmClockNeededRun);
}
#endif /* CONFIG_IPM_IMX */

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

#ifdef CONFIG_I2C_IMX
	nxp_mcimx7_i2c_config();
#endif /* CONFIG_I2C_IMX */

#ifdef CONFIG_PWM_IMX
	nxp_mcimx7_pwm_config();
#endif /* CONFIG_PWM_IMX */

#ifdef CONFIG_IPM_IMX
	nxp_mcimx7_mu_config();
#endif /* CONFIG_IPM_IMX */

	return 0;
}

SYS_INIT(nxp_mcimx7_init, PRE_KERNEL_1, 0);
