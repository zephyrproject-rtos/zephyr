/*
 * Copyright (c) 2018, Cypress
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/arch/arm/aarch32/cortex_m/cmsis.h>
#include <zephyr/irq.h>

#include "cy_syslib.h"
#include "cy_gpio.h"
#include "cy_scb_uart.h"
#include "cy_syslib.h"
#include "cy_syspm.h"
#include "cy_sysclk.h"

#define CY_CFG_SYSCLK_CLKFAST_ENABLED 1
#define CY_CFG_SYSCLK_FLL_ENABLED 1
#define CY_CFG_SYSCLK_CLKHF0_ENABLED 1
#define CY_CFG_SYSCLK_CLKHF0_FREQ_MHZ 100UL
#define CY_CFG_SYSCLK_ILO_ENABLED 1
#define CY_CFG_SYSCLK_IMO_ENABLED 1
#define CY_CFG_SYSCLK_CLKLF_ENABLED 1
#define CY_CFG_SYSCLK_CLKPATH0_ENABLED 1
#define CY_CFG_SYSCLK_CLKPATH1_ENABLED 1
#define CY_CFG_SYSCLK_CLKPATH2_ENABLED 1
#define CY_CFG_SYSCLK_CLKPATH3_ENABLED 1
#define CY_CFG_SYSCLK_CLKPATH4_ENABLED 1
#define CY_CFG_SYSCLK_CLKPERI_ENABLED 1
#define CY_CFG_SYSCLK_CLKSLOW_ENABLED 1
#define CY_CFG_PWR_ENABLED 1
#define CY_CFG_PWR_USING_LDO 1
#define CY_CFG_PWR_USING_PMIC 0
#define CY_CFG_PWR_LDO_VOLTAGE CY_SYSPM_LDO_VOLTAGE_1_1V
#define CY_CFG_PWR_VDDA_MV 3300
#define CY_CFG_PWR_USING_ULP 0

/* Dummy symbols, requres for cy_sysint.c module.
 * NOTE: in this PSoC 6 integration, PSoC 6 Zephyr drivers (uart, spi, gpio)
 * do not use cy_sysint.c implementation to handle interrupt routine.
 * Instead this they use IRQ_CONNECT to define ISR.
 */
uint32_t __ramVectors;
uint32_t __Vectors;

static const cy_stc_fll_manual_config_t srss_0__clock_0__fll_0__fllConfig = {
	.fllMult = 500u,
	.refDiv = 20u,
	.ccoRange = CY_SYSCLK_FLL_CCO_RANGE4,
	.enableOutputDiv = true,
	.lockTolerance = 10u,
	.igain = 9u,
	.pgain = 5u,
	.settlingCount = 8u,
	.outputMode = CY_SYSCLK_FLLPLL_OUTPUT_AUTO,
	.cco_Freq = 355u,
};

static inline void Cy_SysClk_ClkFastInit(void)
{
	Cy_SysClk_ClkFastSetDivider(0u);
}
static inline void Cy_SysClk_FllInit(void)
{
	Cy_SysClk_FllManualConfigure(&srss_0__clock_0__fll_0__fllConfig);
	Cy_SysClk_FllEnable(200000u);
}
static inline void Cy_SysClk_ClkHf0Init(void)
{
	Cy_SysClk_ClkHfSetSource(0u, CY_SYSCLK_CLKHF_IN_CLKPATH0);
	Cy_SysClk_ClkHfSetDivider(0u, CY_SYSCLK_CLKHF_NO_DIVIDE);
	Cy_SysClk_ClkHfEnable(0u);

}
static inline void Cy_SysClk_IloInit(void)
{
	Cy_SysClk_IloEnable();
	Cy_SysClk_IloHibernateOn(true);
}
static inline void Cy_SysClk_ClkLfInit(void)
{
	Cy_SysClk_ClkLfSetSource(CY_SYSCLK_CLKLF_IN_ILO);
}
static inline void Cy_SysClk_ClkPath0Init(void)
{
	Cy_SysClk_ClkPathSetSource(0u, CY_SYSCLK_CLKPATH_IN_IMO);
}
static inline void Cy_SysClk_ClkPath1Init(void)
{
	Cy_SysClk_ClkPathSetSource(1u, CY_SYSCLK_CLKPATH_IN_IMO);
}
static inline void Cy_SysClk_ClkPath2Init(void)
{
	Cy_SysClk_ClkPathSetSource(2u, CY_SYSCLK_CLKPATH_IN_IMO);
}
static inline void Cy_SysClk_ClkPath3Init(void)
{
	Cy_SysClk_ClkPathSetSource(3u, CY_SYSCLK_CLKPATH_IN_IMO);
}
static inline void Cy_SysClk_ClkPath4Init(void)
{
	Cy_SysClk_ClkPathSetSource(4u, CY_SYSCLK_CLKPATH_IN_IMO);
}
static inline void Cy_SysClk_ClkPeriInit(void)
{
	Cy_SysClk_ClkPeriSetDivider(1u);
}
static inline void Cy_SysClk_ClkSlowInit(void)
{
	Cy_SysClk_ClkSlowSetDivider(0u);
}


static void init_cycfg_platform(void)
{
	/* Set worst case memory wait states (! ultra low power, 150 MHz), will
	 * update at the end
	 */
	Cy_SysLib_SetWaitStates(false, 150);
#ifdef CY_CFG_PWR_ENABLED
	/* Configure power mode */
#if CY_CFG_PWR_USING_LDO
	Cy_SysPm_LdoSetVoltage(CY_CFG_PWR_LDO_VOLTAGE);
#else
	Cy_SysPm_BuckEnable(CY_CFG_PWR_SIMO_VOLTAGE);
#endif
	/* Configure PMIC */
	Cy_SysPm_UnlockPmic();
#if CY_CFG_PWR_USING_PMIC
	Cy_SysPm_PmicEnableOutput();
#else
	Cy_SysPm_PmicDisableOutput();
#endif
#endif
	/* Enable all source clocks */
#ifdef CY_CFG_SYSCLK_PILO_ENABLED
	Cy_SysClk_PiloInit();
#endif

#ifdef CY_CFG_SYSCLK_WCO_ENABLED
	Cy_SysClk_WcoInit();
#endif

#ifdef CY_CFG_SYSCLK_CLKLF_ENABLED
	Cy_SysClk_ClkLfInit();
#endif

#ifdef CY_CFG_SYSCLK_ALTHF_ENABLED
	Cy_SysClk_AltHfInit();
#endif

#ifdef CY_CFG_SYSCLK_ECO_ENABLED
	Cy_SysClk_EcoInit();
#endif

#ifdef CY_CFG_SYSCLK_EXTCLK_ENABLED
	Cy_SysClk_ExtClkInit();
#endif

	/* Configure CPU clock dividers */
#ifdef CY_CFG_SYSCLK_CLKFAST_ENABLED
	Cy_SysClk_ClkFastInit();
#endif

#ifdef CY_CFG_SYSCLK_CLKPERI_ENABLED
	Cy_SysClk_ClkPeriInit();
#endif

#ifdef CY_CFG_SYSCLK_CLKSLOW_ENABLED
	Cy_SysClk_ClkSlowInit();
#endif

	/* Configure HF clocks */
#ifdef CY_CFG_SYSCLK_CLKHF0_ENABLED
	Cy_SysClk_ClkHf0Init();
#endif
#ifdef CY_CFG_SYSCLK_CLKHF1_ENABLED
	Cy_SysClk_ClkHf1Init();
#endif
#ifdef CY_CFG_SYSCLK_CLKHF2_ENABLED
	Cy_SysClk_ClkHf2Init();
#endif
#ifdef CY_CFG_SYSCLK_CLKHF3_ENABLED
	Cy_SysClk_ClkHf3Init();
#endif
#ifdef CY_CFG_SYSCLK_CLKHF4_ENABLED
	Cy_SysClk_ClkHf4Init();
#endif
#ifdef CY_CFG_SYSCLK_CLKHF5_ENABLED
	Cy_SysClk_ClkHf5Init();
#endif
#ifdef CY_CFG_SYSCLK_CLKHF6_ENABLED
	Cy_SysClk_ClkHf6Init();
#endif
#ifdef CY_CFG_SYSCLK_CLKHF7_ENABLED
	Cy_SysClk_ClkHf7Init();
#endif
#ifdef CY_CFG_SYSCLK_CLKHF8_ENABLED
	Cy_SysClk_ClkHf8Init();
#endif
#ifdef CY_CFG_SYSCLK_CLKHF9_ENABLED
	Cy_SysClk_ClkHf9Init();
#endif
#ifdef CY_CFG_SYSCLK_CLKHF10_ENABLED
	Cy_SysClk_ClkHf10Init();
#endif
#ifdef CY_CFG_SYSCLK_CLKHF11_ENABLED
	Cy_SysClk_ClkHf11Init();
#endif
#ifdef CY_CFG_SYSCLK_CLKHF12_ENABLED
	Cy_SysClk_ClkHf12Init();
#endif
#ifdef CY_CFG_SYSCLK_CLKHF13_ENABLED
	Cy_SysClk_ClkHf13Init();
#endif
#ifdef CY_CFG_SYSCLK_CLKHF14_ENABLED
	Cy_SysClk_ClkHf14Init();
#endif
#ifdef CY_CFG_SYSCLK_CLKHF15_ENABLED
	Cy_SysClk_ClkHf15Init();
#endif

	/* Configure Path Clocks */
#ifdef CY_CFG_SYSCLK_CLKPATH0_ENABLED
	Cy_SysClk_ClkPath0Init();
#endif
#ifdef CY_CFG_SYSCLK_CLKPATH1_ENABLED
	Cy_SysClk_ClkPath1Init();
#endif
#ifdef CY_CFG_SYSCLK_CLKPATH2_ENABLED
	Cy_SysClk_ClkPath2Init();
#endif
#ifdef CY_CFG_SYSCLK_CLKPATH3_ENABLED
	Cy_SysClk_ClkPath3Init();
#endif
#ifdef CY_CFG_SYSCLK_CLKPATH4_ENABLED
	Cy_SysClk_ClkPath4Init();
#endif
#ifdef CY_CFG_SYSCLK_CLKPATH5_ENABLED
	Cy_SysClk_ClkPath5Init();
#endif
#ifdef CY_CFG_SYSCLK_CLKPATH6_ENABLED
	Cy_SysClk_ClkPath6Init();
#endif
#ifdef CY_CFG_SYSCLK_CLKPATH7_ENABLED
	Cy_SysClk_ClkPath7Init();
#endif
#ifdef CY_CFG_SYSCLK_CLKPATH8_ENABLED
	Cy_SysClk_ClkPath8Init();
#endif
#ifdef CY_CFG_SYSCLK_CLKPATH9_ENABLED
	Cy_SysClk_ClkPath9Init();
#endif
#ifdef CY_CFG_SYSCLK_CLKPATH10_ENABLED
	Cy_SysClk_ClkPath10Init();
#endif
#ifdef CY_CFG_SYSCLK_CLKPATH11_ENABLED
	Cy_SysClk_ClkPath11Init();
#endif
#ifdef CY_CFG_SYSCLK_CLKPATH12_ENABLED
	Cy_SysClk_ClkPath12Init();
#endif
#ifdef CY_CFG_SYSCLK_CLKPATH13_ENABLED
	Cy_SysClk_ClkPath13Init();
#endif
#ifdef CY_CFG_SYSCLK_CLKPATH14_ENABLED
	Cy_SysClk_ClkPath14Init();
#endif
#ifdef CY_CFG_SYSCLK_CLKPATH15_ENABLED
	Cy_SysClk_ClkPath15Init();
#endif

	/* Configure and enable FLL */
#ifdef CY_CFG_SYSCLK_FLL_ENABLED
	Cy_SysClk_FllInit();
#endif

	/* Configure and enable PLLs */
#ifdef CY_CFG_SYSCLK_PLL0_ENABLED
	Cy_SysClk_Pll0Init();
#endif
#ifdef CY_CFG_SYSCLK_PLL1_ENABLED
	Cy_SysClk_Pll1Init();
#endif
#ifdef CY_CFG_SYSCLK_PLL2_ENABLED
	Cy_SysClk_Pll2Init();
#endif
#ifdef CY_CFG_SYSCLK_PLL3_ENABLED
	Cy_SysClk_Pll3Init();
#endif
#ifdef CY_CFG_SYSCLK_PLL4_ENABLED
	Cy_SysClk_Pll4Init();
#endif
#ifdef CY_CFG_SYSCLK_PLL5_ENABLED
	Cy_SysClk_Pll5Init();
#endif
#ifdef CY_CFG_SYSCLK_PLL6_ENABLED
	Cy_SysClk_Pll6Init();
#endif
#ifdef CY_CFG_SYSCLK_PLL7_ENABLED
	Cy_SysClk_Pll7Init();
#endif
#ifdef CY_CFG_SYSCLK_PLL8_ENABLED
	Cy_SysClk_Pll8Init();
#endif
#ifdef CY_CFG_SYSCLK_PLL9_ENABLED
	Cy_SysClk_Pll9Init();
#endif
#ifdef CY_CFG_SYSCLK_PLL10_ENABLED
	Cy_SysClk_Pll10Init();
#endif
#ifdef CY_CFG_SYSCLK_PLL11_ENABLED
	Cy_SysClk_Pll11Init();
#endif
#ifdef CY_CFG_SYSCLK_PLL12_ENABLED
	Cy_SysClk_Pll12Init();
#endif
#ifdef CY_CFG_SYSCLK_PLL13_ENABLED
	Cy_SysClk_Pll13Init();
#endif
#ifdef CY_CFG_SYSCLK_PLL14_ENABLED
	Cy_SysClk_Pll14Init();
#endif

	/* Configure miscellaneous clocks */
#ifdef CY_CFG_SYSCLK_CLKTIMER_ENABLED
	Cy_SysClk_ClkTimerInit();
#endif

#ifdef CY_CFG_SYSCLK_CLKALTSYSTICK_ENABLED
	Cy_SysClk_ClkAltSysTickInit();
#endif

#ifdef CY_CFG_SYSCLK_CLKPUMP_ENABLED
	Cy_SysClk_ClkPumpInit();
#endif

#ifdef CY_CFG_SYSCLK_CLKBAK_ENABLED
	Cy_SysClk_ClkBakInit();
#endif

	/* Configure default enabled clocks */
#ifdef CY_CFG_SYSCLK_ILO_ENABLED
	Cy_SysClk_IloInit();
#else
	Cy_SysClk_IloDisable();
#endif

#ifndef CY_CFG_SYSCLK_IMO_ENABLED
#error the IMO must be enabled for proper chip operation
#endif

	/* Set accurate flash wait states */
#if (defined(CY_CFG_PWR_ENABLED) && defined(CY_CFG_SYSCLK_CLKHF0_ENABLED))
	Cy_SysLib_SetWaitStates(CY_CFG_PWR_USING_ULP != 0,
		CY_CFG_SYSCLK_CLKHF0_FREQ_MHZ);
#endif
}

/**
 * Function Name: Cy_SystemInit
 *
 * \brief This function is called by the start-up code for the selected device.
 * It performs all of the necessary device configuration based on the design
 * settings. This includes settings for the platform resources and peripheral
 * clock.
 *
 */
void Cy_SystemInit(void)
{
	/* Configure platform resources */
	init_cycfg_platform();

	/* Configure peripheral clocks */
	Cy_SysClk_PeriphSetDivider(CY_SYSCLK_DIV_8_BIT, 0u, 0u);
	Cy_SysClk_PeriphEnableDivider(CY_SYSCLK_DIV_8_BIT, 0u);

#if defined(CONFIG_SOC_PSOC6_M0_ENABLES_M4)
	Cy_SysEnableCM4(DT_REG_ADDR(DT_NODELABEL(flash1)));
#endif
}

static int init_cycfg_platform_wraper(const struct device *arg)
{
	ARG_UNUSED(arg);
	SystemInit();
	return 0;
}

SYS_INIT(init_cycfg_platform_wraper, PRE_KERNEL_1, 0);
