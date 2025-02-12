/* Copyright 2023 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Infineon CYW920829  soc.
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>

#include <cy_sysint.h>
#include <system_cat1b.h>
#include "cy_pdl.h"

/* Power profiles definition */
#define CY_CFG_PWR_MODE_POWER_PROFILE_0  0 /* LP MCU + Radio ON */
#define CY_CFG_PWR_MODE_POWER_PROFILE_1  1 /* ULP MCU + Radio ON */
#define CY_CFG_PWR_MODE_POWER_PROFILE_2A 2 /* LP MCU Only */
#define CY_CFG_PWR_MODE_POWER_PROFILE_2B 3 /* LP MCU Only */
#define CY_CFG_PWR_MODE_POWER_PROFILE_3  4 /* ULP MCU Only */

#define CY_CFG_PWR_SYS_ACTIVE_PROFILE                                                              \
	UTIL_CAT(CY_CFG_PWR_MODE, DT_STRING_UPPER_TOKEN_OR(DT_NODELABEL(srss_power),               \
							   power_profile, POWER_PROFILE_0))
#define CY_CFG_PWR_VBACKUP_USING_VDDD  1
#define CY_CFG_PWR_REGULATOR_MODE_MIN  0
#define CY_CFG_PWR_SYS_LP_PROFILE_MODE 0
#define CY_CFG_PWR_SDR0_MODE_BYPASS    true

#if (CY_CFG_PWR_SYS_ACTIVE_PROFILE == CY_CFG_PWR_MODE_POWER_PROFILE_0)
#define CY_CFG_PWR_CBUCK_MODE  CY_SYSPM_CORE_BUCK_MODE_HP
#define CY_CFG_PWR_CBUCK_VOLT  CY_SYSPM_CORE_BUCK_VOLTAGE_1_16V
#define CY_CFG_PWR_SDR0_VOLT   CY_SYSPM_SDR_VOLTAGE_1_100V
#define CY_CFG_PWR_SDR1_VOLT   CY_SYSPM_SDR_VOLTAGE_1_100V
#define CY_CFG_PWR_SDR1_ENABLE true

#elif (CY_CFG_PWR_SYS_ACTIVE_PROFILE == CY_CFG_PWR_MODE_POWER_PROFILE_1)
#define CY_CFG_PWR_CBUCK_MODE  CY_SYSPM_CORE_BUCK_MODE_HP
#define CY_CFG_PWR_CBUCK_VOLT  CY_SYSPM_CORE_BUCK_VOLTAGE_1_16V
#define CY_CFG_PWR_SDR0_VOLT   CY_SYSPM_SDR_VOLTAGE_1_000V
#define CY_CFG_PWR_SDR1_VOLT   CY_SYSPM_SDR_VOLTAGE_1_100V
#define CY_CFG_PWR_SDR1_ENABLE true

#elif (CY_CFG_PWR_SYS_ACTIVE_PROFILE == CY_CFG_PWR_MODE_POWER_PROFILE_2A)
#define CY_CFG_PWR_CBUCK_MODE  CY_SYSPM_CORE_BUCK_MODE_HP
#define CY_CFG_PWR_CBUCK_VOLT  CY_SYSPM_CORE_BUCK_VOLTAGE_1_16V
#define CY_CFG_PWR_SDR0_VOLT   CY_SYSPM_SDR_VOLTAGE_1_100V
#define CY_CFG_PWR_SDR1_VOLT   0
#define CY_CFG_PWR_SDR1_ENABLE false

#elif (CY_CFG_PWR_SYS_ACTIVE_PROFILE == CY_CFG_PWR_MODE_POWER_PROFILE_2B)
#define CY_CFG_PWR_CBUCK_MODE  CY_SYSPM_CORE_BUCK_MODE_LP
#define CY_CFG_PWR_CBUCK_VOLT  CY_SYSPM_CORE_BUCK_VOLTAGE_1_10V
#define CY_CFG_PWR_SDR0_VOLT   CY_SYSPM_SDR_VOLTAGE_1_100V
#define CY_CFG_PWR_SDR1_VOLT   0
#define CY_CFG_PWR_SDR1_ENABLE false

#elif (CY_CFG_PWR_SYS_ACTIVE_PROFILE == CY_CFG_PWR_MODE_POWER_PROFILE_3)
#define CY_CFG_PWR_CBUCK_MODE  CY_SYSPM_CORE_BUCK_MODE_LP
#define CY_CFG_PWR_CBUCK_VOLT  CY_SYSPM_CORE_BUCK_VOLTAGE_1_00V
#define CY_CFG_PWR_SDR0_VOLT   CY_SYSPM_SDR_VOLTAGE_1_000V
#define CY_CFG_PWR_SDR1_VOLT   0
#define CY_CFG_PWR_SDR1_ENABLE false
#else
#error CY_CFG_PWR_SYS_ACTIVE_PROFILE configured incorrectly
#endif

extern int ifx_pm_init(void);

cy_en_sysint_status_t Cy_SysInt_Init(const cy_stc_sysint_t *config, cy_israddress userIsr)
{
	CY_ASSERT_L3(CY_SYSINT_IS_PRIORITY_VALID(config->intrPriority));
	cy_en_sysint_status_t status = CY_SYSINT_SUCCESS;

	/* The interrupt vector will be relocated only if the vector table was
	 * moved to SRAM (CONFIG_DYNAMIC_INTERRUPTS and CONFIG_GEN_ISR_TABLES
	 * must be enabled). Otherwise it is ignored.
	 */

#if defined(CONFIG_DYNAMIC_INTERRUPTS) && defined(CONFIG_GEN_ISR_TABLES)
	if (config != NULL) {
		uint32_t priority;

		/* NOTE:
		 * PendSV IRQ (which is used in Cortex-M variants to implement thread
		 * context-switching) is assigned the lowest IRQ priority level.
		 * If priority is same as PendSV, we will catch assertion in
		 * z_arm_irq_priority_set function. To avoid this, change priority
		 * to IRQ_PRIO_LOWEST, if it > IRQ_PRIO_LOWEST. Macro IRQ_PRIO_LOWEST
		 * takes in to account PendSV specific.
		 */
		priority = (config->intrPriority > IRQ_PRIO_LOWEST) ?
			   IRQ_PRIO_LOWEST : config->intrPriority;

		/* Configure a dynamic interrupt */
		(void) irq_connect_dynamic(config->intrSrc, priority,
					   (void *) userIsr, NULL, 0);
	} else {
		status = CY_SYSINT_BAD_PARAM;
	}
#endif /* defined(CONFIG_DYNAMIC_INTERRUPTS) && defined(CONFIG_GEN_ISR_TABLES) */

	return(status);
}

/*
 * This function will allow execute from sram region.  This is needed only for
 * this sample because by default all soc will disable the execute from SRAM.
 * An application that requires that the code be executed from SRAM will have
 * to configure the region appropriately in arm_mpu_regions.c.
 */
#ifdef CONFIG_ARM_MPU
#include <cmsis_core.h>
void disable_mpu_rasr_xn(void)
{
	uint32_t index;

	/*
	 * Kept the max index as 8(irrespective of soc) because the sram would
	 * most likely be set at index 2.
	 */
	for (index = 0U; index < 8; index++) {
		MPU->RNR = index;
#if defined(CONFIG_ARMV8_M_BASELINE) || defined(CONFIG_ARMV8_M_MAINLINE)
		if (MPU->RBAR & MPU_RBAR_XN_Msk) {
			MPU->RBAR ^= MPU_RBAR_XN_Msk;
		}
#else
		if (MPU->RASR & MPU_RASR_XN_Msk) {
			MPU->RASR ^= MPU_RASR_XN_Msk;
		}
#endif /* CONFIG_ARMV8_M_BASELINE || CONFIG_ARMV8_M_MAINLINE */
	}
}
#endif /* CONFIG_ARM_MPU */

static cy_stc_syspm_core_buck_params_t coreBuckConfigParam = {
	.voltageSel = CY_CFG_PWR_CBUCK_VOLT,
	.mode = CY_CFG_PWR_CBUCK_MODE,
	.override = false,
	.copySettings = false,
	.useSettings = false,
	.inRushLimitSel = 0,
};

static cy_stc_syspm_sdr_params_t sdr0ConfigParam = {
	.coreBuckVoltSel = CY_CFG_PWR_CBUCK_VOLT,
	.coreBuckMode = CY_CFG_PWR_CBUCK_MODE,
	.coreBuckDpSlpVoltSel = CY_SYSPM_CORE_BUCK_VOLTAGE_0_90V,
	.coreBuckDpSlpMode = CY_SYSPM_CORE_BUCK_MODE_LP,
	.sdr0DpSlpVoltSel = CY_SYSPM_SDR_VOLTAGE_0_900V,
	.sdrVoltSel = CY_CFG_PWR_SDR0_VOLT,
	.sdr0Allowbypass = CY_CFG_PWR_SDR0_MODE_BYPASS,
};

static cy_stc_syspm_sdr_params_t sdr1ConfigParam = {
	.coreBuckVoltSel = CY_CFG_PWR_CBUCK_VOLT,
	.coreBuckMode = CY_CFG_PWR_CBUCK_MODE,
	.sdrVoltSel = CY_CFG_PWR_SDR1_VOLT,
	.sdr1HwControl = true,
	.sdr1Enable = true,
};

static inline void init_power(void)
{
	CY_UNUSED_PARAMETER(
		sdr1ConfigParam); /* Suppress a compiler warning about unused variables */

	Cy_SysPm_Init();
/* **Reset the Backup domain on POR, XRES, BOD only if Backup domain is supplied by VDDD** */
#if (CY_CFG_PWR_VBACKUP_USING_VDDD)
#ifdef CY_CFG_SYSCLK_ILO_ENABLED
	if (0u == Cy_SysLib_GetResetReason() /* POR, XRES, or BOD */) {
		Cy_SysLib_ResetBackupDomain();
		Cy_SysClk_IloDisable();
		Cy_SysClk_IloInit();
	}
#endif /* CY_CFG_SYSCLK_ILO_ENABLED */
#endif /* CY_CFG_PWR_VBACKUP_USING_VDDD */

	/* **System Active Power Mode Profile Configuration** */
	/* Core Buck Regulator Configuration */
	Cy_SysPm_CoreBuckConfig(&coreBuckConfigParam);

	/* SDR0 Regulator Configuration */
	Cy_SysPm_SdrConfigure(CY_SYSPM_SDR_0, &sdr0ConfigParam);

/* SDR1 Regulator Configuration */
#if (CY_CFG_PWR_SDR1_ENABLE)
	Cy_SysPm_SdrConfigure(CY_SYSPM_SDR_1, &sdr1ConfigParam);
#endif /* CY_CFG_PWR_SDR1_VOLT */

/* **System Active Low Power Profile(LPACTIVE/LPSLEEP) Configuration** */
#if (CY_CFG_PWR_SYS_LP_PROFILE_MODE)
	Cy_SysPm_SystemLpActiveEnter();
#endif /* CY_CFG_PWR_SYS_LP_PROFILE_MODE */

/* **System Regulators Low Current Configuration** */
#if (CY_CFG_PWR_REGULATOR_MODE_MIN)
	Cy_SysPm_SystemSetMinRegulatorCurrent();
#endif /* CY_CFG_PWR_REGULATOR_MODE_MIN */
}

void soc_early_init_hook(void)
{
#ifdef CONFIG_ARM_MPU
	disable_mpu_rasr_xn();
#endif /* CONFIG_ARM_MPU */

	/* Initializes the system */
	SystemInit();

	init_power();
#ifdef CONFIG_PM
	ifx_pm_init();
#endif
}
