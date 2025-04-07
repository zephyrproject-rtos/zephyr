/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if !defined(CONFIG_CPU_CORTEX_M)
#error project can only run on Cortex-M
#endif

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/ztest.h>

#ifdef SCB_VTOR_TBLBASE_Msk
#define VTOR_MASK (SCB_VTOR_TBLBASE_Msk | SCB_VTOR_TBLOFF_Msk)
#else
#define VTOR_MASK SCB_VTOR_TBLOFF_Msk
#endif

/* This function will allow execute from sram region.
 * This is needed only for this sample because by default all soc will
 * disable the execute from SRAM.
 * An application that requires that the code be executed from SRAM will
 *  have to configure the region appropriately in arm_mpu_regions.c.
 */

#if (defined(CONFIG_ARM_MPU) && !defined(CONFIG_CPU_HAS_NXP_SYSMPU))
#include <cmsis_core.h>
void disable_mpu_rasr_xn(void)
{
	uint32_t index;
	/* Kept the max index as 8(irrespective of soc) because the sram
	 * would most likely be set at index 2.
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

ZTEST(vector_table_relocation, test_vector_table_in_ram)
{
	/* Check that VTOR register effectively point to a RAM based location */
	volatile uint32_t vtor_address = SCB->VTOR & VTOR_MASK;

	printf("VTOR address: 0x%x\n", vtor_address);
	zassert_true(vtor_address >= CONFIG_SRAM_BASE_ADDRESS &&
			     vtor_address <= CONFIG_SRAM_BASE_ADDRESS + CONFIG_SRAM_SIZE,
		     "Vector table is not in RAM! Address: 0x%x", vtor_address);
}

void *relocate_code_setup(void)
{
#if (defined(CONFIG_ARM_MPU) && !defined(CONFIG_CPU_HAS_NXP_SYSMPU))
	disable_mpu_rasr_xn();
#endif /* CONFIG_ARM_MPU */
	return NULL;
}

ZTEST_SUITE(vector_table_relocation, NULL, relocate_code_setup, NULL, NULL, NULL);
