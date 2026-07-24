/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pse84_boot.h"

/* Integer constants for preprocessor */
#define PM_MODE_DS     0
#define PM_MODE_DS_RAM 1
#define PM_MODE_DS_OFF 2
#define DEEPSLEEP_MODE PM_MODE_DS

#if (DEEPSLEEP_MODE == PM_MODE_DS)
#define DEEPSLEEP_MODE_ENUM CY_SYSPM_MODE_DEEPSLEEP
#elif (DEEPSLEEP_MODE == PM_MODE_DS_RAM)
#define DEEPSLEEP_MODE_ENUM CY_SYSPM_MODE_DEEPSLEEP_RAM
#elif (DEEPSLEEP_MODE == PM_MODE_DS_OFF)
#define DEEPSLEEP_MODE_ENUM CY_SYSPM_MODE_DEEPSLEEP_OFF
#endif

#if defined(CONFIG_SOC_PSE84_M55_ENABLE)
void ifx_pse84_cm55_startup(void)
{
	/* The IO-freeze bits are set by hardware on Hibernate/DS-OFF entry */
	Cy_SysPm_DeepSleepIoUnfreeze();
	Cy_SysPm_IoUnfreeze();

	/* Setup System Control Block */
	SysCtrlBlk_Setup();
	/* Setup NS NVIC interrupts */
	NVIC_NS_Setup();

#if defined(__FPU_USED) && (__FPU_USED == 1U) && defined(TZ_FPU_NS_USAGE) && (TZ_FPU_NS_USAGE == 1U)
	/* FPU initialization */
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

	/* Configure Memory Protection Controller for Non-Secure */
	cy_mpc_init();

	/* Reduce deepsleep wakeup time in hardware */
	cy_pd_pdcm_clear_dependency(CY_PD_PDCM_APPCPUSS, CY_PD_PDCM_SYSCPU);

	/* Clear SYSCPU and APPCPU power domain dependency set by boot code */
	cy_pd_pdcm_clear_dependency(CY_PD_PDCM_APPCPU, CY_PD_PDCM_SYSCPU);

#if (DEEPSLEEP_MODE != PM_MODE_DS)
	/* DS-OFF requires SOCMEM to have no dependency on SYSCPU so
	 * the power controller can shut down the SOCMEM domain.
	 */
	cy_pd_pdcm_clear_dependency(CY_PD_PDCM_SOCMEM, CY_PD_PDCM_SYSCPU);
#endif

	uint32_t cm55_start_address = DT_REG_ADDR(DT_NODELABEL(m55_xip));

	/* Enable CM55 */
	Cy_SysEnableCM55(MXCM55, cm55_start_address, CM55_BOOT_WAIT_TIME_USEC);

	/* System Domain Idle Power Mode Configuration */
	Cy_SysPm_SetDeepSleepMode(DEEPSLEEP_MODE_ENUM);
	Cy_SysPm_SetAppDeepSleepMode(DEEPSLEEP_MODE_ENUM);
	Cy_SysPm_SetSOCMEMDeepSleepMode(DEEPSLEEP_MODE_ENUM);

#if (DEEPSLEEP_MODE == PM_MODE_DS_OFF)
	/* TOKEN must be before PPC configuration */
	SRSS_PWR_HIBERNATE |= (uint32_t)CY_SYSLIB_DEEP_SLEEP_OFF_TOKEN;
#endif

	/* Configure Peripheral Protection Controller for Non-Secure */
	cy_ppc0_init();
	cy_ppc1_init();

	/* Manually set DeepSleep configuration to avoid SRF dependency */
	SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
	__DSB();

#if (DEEPSLEEP_MODE != PM_MODE_DS)
	/* FPU must be power-gated for DS-RAM/DS-OFF entry */
	SCS_CPPWR |= SCS_ENABLE_CPPWR_SU10_SU11;
	SCB->CPACR &= ~SCB_ENABLE_CPACR_CP10_CP11;
	__DSB();
#endif

	for (;;) {
		__WFI();
	}
}
#endif
