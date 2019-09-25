/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel.h>
#include <sys/printk.h>


/* This function will allow execute from sram region.
 * This is needed only for this sample because by default all soc will
 * disable the execute from SRAM.
 * An application that requires that the code be executed from SRAM will
 *  have to configure the region appropriately in arm_mpu_regions.c.
 */

#ifdef CONFIG_ARM_MPU
#include <arch/arm/cortex_m/cmsis.h>
void disable_mpu_rasr_xn(void)
{
	u32_t index;
	/* Kept the max index as 8(irrespective of soc) because the sram
	 * would most likely be set at index 2.
	 */
	for (index = 0U; index < 8; index++) {
		MPU->RNR = index;
		if (MPU->RASR & MPU_RASR_XN_Msk) {
			MPU->RASR ^= MPU_RASR_XN_Msk;
		}
	}

}
#endif	/* CONFIG_ARM_MPU */


extern void function_in_sram2(void);
extern void function_in_split_multiple(void);

void main(void)
{
#ifdef CONFIG_ARM_MPU
	disable_mpu_rasr_xn();
#endif	/* CONFIG_ARM_MPU */

	printk("Address of main function %p\n\n", &main);
	function_in_sram2();
	function_in_split_multiple();
}
