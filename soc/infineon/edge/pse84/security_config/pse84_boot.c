/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pse84_boot.h"

#define CYMEM_CM33_0_m33_nvm_START 0x60300000u

#define CM33_NS_APP_BOOT_ADDR (CYMEM_CM33_0_m33_nvm_START)

#if defined(CONFIG_SOC_PSE84_M33_NS_ENABLE)
void ifx_pse84_cm33_ns_startup(void)
{
	uint32_t ns_stack;
	cy_cmse_funcptr nonsecure_reset_handler;

	/* SAU Init */
	cy_sau_init();

	/* Setup System Control Block */
	SysCtrlBlk_Setup();
	/* Setup NS NVIC interrupts */
	NVIC_NS_Setup();

#if defined(__FPU_USED) && (__FPU_USED == 1U) && defined(TZ_FPU_NS_USAGE) && (TZ_FPU_NS_USAGE == 1U)
	/*FPU initialization*/
	initFPU();
#endif

	/* Enable global interrupts */
	__enable_irq();

	/* Enables PD1 power domain */
	Cy_System_EnablePD1();

	/* Enables APP_MMIO_TCM memory for CM55 core */
	Cy_SysClk_PeriGroupSlaveInit(CY_MMIO_CM55_TCM_512K_PERI_NR, CY_MMIO_CM55_TCM_512K_GROUP_NR,
				     CY_MMIO_CM55_TCM_512K_SLAVE_NR,
				     CY_MMIO_CM55_TCM_512K_CLK_HF_NR);

	Cy_SysClk_PeriGroupSlaveInit(CY_MMIO_SMIF0_PERI_NR, CY_MMIO_SMIF0_GROUP_NR,
				     CY_MMIO_SMIF0_SLAVE_NR, CY_MMIO_SMIF0_CLK_HF_NR);

	Cy_SysClk_PeriGroupSlaveInit(CY_MMIO_SMIF01_PERI_NR, CY_MMIO_SMIF01_GROUP_NR,
				     CY_MMIO_SMIF01_SLAVE_NR, CY_MMIO_SMIF01_CLK_HF_NR);

	/*Enable SOCMEM*/
	Cy_SysEnableSOCMEM(true);

	/*configure MPC for NS*/
	cy_mpc_Init();

	ns_stack = (uint32_t)(*((uint32_t *)CM33_NS_APP_BOOT_ADDR));
	__TZ_set_MSP_NS(ns_stack);

	nonsecure_reset_handler = (cy_cmse_funcptr)(*((uint32_t *)(CM33_NS_APP_BOOT_ADDR + 4)));

	/* Reduce deepsleep wakeup time in hardware */
	cy_pd_pdcm_clear_dependency(CY_PD_PDCM_APPCPUSS, CY_PD_PDCM_SYSCPU);

	/* Clear SYSCPU and APPCPU power domain dependency set by boot code */
	cy_pd_pdcm_clear_dependency(CY_PD_PDCM_APPCPU, CY_PD_PDCM_SYSCPU);

	/*configure PPC for NS*/
	cy_ppc0_Init();
	cy_ppc1_Init();

	sys_clock_disable();

	/* Start non-secure application */
	nonsecure_reset_handler();

	for (;;) {
	}
}
#endif

#if defined(CONFIG_SOC_PSE84_M55_ENABLE)
void ifx_pse84_cm55_startup(void)
{
	/* Enable CM55 */
	Cy_SysEnableCM55(MXCM55, DT_REG_ADDR(DT_NODELABEL(m55_xip)), CM55_BOOT_WAIT_TIME_USEC);

	/* System Domain Idle Power Mode Configuration */
	Cy_SysPm_SetDeepSleepMode(CY_SYSPM_MODE_DEEPSLEEP);

	/* SoCMEM Idle Power Mode Configuration */
	Cy_SysPm_SetSOCMEMDeepSleepMode(CY_SYSPM_MODE_DEEPSLEEP);

	for (;;) {
	}
}
#endif
