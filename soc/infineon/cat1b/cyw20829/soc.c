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

static int init_cycfg_platform_wrapper(void)
{
#ifdef CONFIG_ARM_MPU
	disable_mpu_rasr_xn();
#endif	/* CONFIG_ARM_MPU */

	/* Initializes the system */
	SystemInit();
	return 0;
}

SYS_INIT(init_cycfg_platform_wrapper, PRE_KERNEL_1, 0);
