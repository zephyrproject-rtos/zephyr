/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Infineon PSOC EDGE84 soc.
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <cy_sysint.h>
#include <system_edge.h>
#include "cy_pdl.h"
#include "soc.h"

#define CY_IPC_MAX_ENDPOINTS (8UL)

/**
 * Config_noncacheable_region() copied from
 * hal_infineon\...\
 * COMPONENT_CM55\COMPONENT_NON_SECURE_DEVICE\ns_start_pse84.c
 */
#define MPU_SRAM1_SHARED_MEM_REG_ID   0x6
#define MPU_SRAM1_SHARED_MEM_ATTR_IDX 0x6

void config_noncacheable_region(void)
{
	ARM_MPU_Disable();

	/* Program MAIR0 and MAIR1 */
	ARM_MPU_SetMemAttr(MPU_SRAM1_SHARED_MEM_ATTR_IDX,
			   ARM_MPU_ATTR(ARM_MPU_ATTR_NON_CACHEABLE, ARM_MPU_ATTR_NON_CACHEABLE));

	ARM_MPU_SetRegion(MPU_SRAM1_SHARED_MEM_REG_ID,
			  ARM_MPU_RBAR(SRAM1_NS_SAHB_SHARED_START, ARM_MPU_SH_INNER, 0UL, 1UL, 1UL),
			  ARM_MPU_RLAR((SRAM1_NS_SAHB_SHARED_START + SRAM1_SHARED_SIZE - 1UL),
				       MPU_SRAM1_SHARED_MEM_ATTR_IDX));

	ARM_MPU_Enable(4);
}

cy_en_sysint_status_t Cy_SysInt_Init(const cy_stc_sysint_t *config, cy_israddress userIsr)
{
	CY_ASSERT_L3(CY_SYSINT_IS_PRIORITY_VALID(config->intrPriority));
	cy_en_sysint_status_t status = CY_SYSINT_SUCCESS;

	/* The interrupt vector will be relocated only if the vector table was
	 * moved to SRAM (CONFIG_DYNAMIC_INTERRUPTS and CONFIG_GEN_ISR_TABLES
	 * must be enabled). Otherwise it is ignored.
	 */

#if (CY_CPU_CORTEX_M0P)
#error Cy_SysInt_Init does not support CM0p core.
#endif /* (CY_CPU_CORTEX_M0P) */

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
		priority = (config->intrPriority > IRQ_PRIO_LOWEST) ? IRQ_PRIO_LOWEST
								    : config->intrPriority;

		/* Configure a dynamic interrupt */
		(void)irq_connect_dynamic(config->intrSrc, priority, (void *)userIsr, NULL, 0);
	} else {
		status = CY_SYSINT_BAD_PARAM;
	}
#endif /* defined(CONFIG_DYNAMIC_INTERRUPTS) && defined(CONFIG_GEN_ISR_TABLES) */

	return (status);
}

/*
 * This function will allow execute from sram region.  This is needed only for
 * this sample because by default all soc will disable the execute from SRAM.
 * An application that requires that the code be executed from SRAM will have
 * to configure the region appropriately in arm_mpu_regions.c.
 */
#ifdef CONFIG_ARM_MPU
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

void soc_early_init_hook(void)
{
	/* Config non-cacheable region */
	config_noncacheable_region();

#ifdef CONFIG_ARM_MPU
	disable_mpu_rasr_xn();
#endif /* CONFIG_ARM_MPU */

	/* Enable Loop and branch info cache */
	__DMB();
	__ISB();
	SCB_EnableICache();
	SCB_EnableDCache();

	/* Initializes the system */
	SystemInit();

	static cy_stc_ipc_pipe_ep_t systemIpcPipeEpArray[CY_IPC_MAX_ENDPOINTS];

	Cy_IPC_Pipe_Config(systemIpcPipeEpArray);
}

cy_israddress Cy_SysInt_SetVector(IRQn_Type IRQn, cy_israddress userIsr)
{
	return 0;
}
