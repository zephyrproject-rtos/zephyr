/*
 * Copyright (c) 2018 Intel Corporation.
 * Copyright (c) 2022, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/ztest.h>

/* This function will allow execute from sram region.
 * This is needed only for this sample because by default all soc will
 * disable the execute from SRAM.
 * An application that requires that the code be executed from SRAM will
 *  have to configure the region appropriately in arm_mpu_regions.c.
 */

#if (defined(CONFIG_ARM_MPU) && !defined(CONFIG_CPU_HAS_NXP_MPU))
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
#endif	/* CONFIG_ARM_MPU */

/* override the default memcpy as zephyr will call this before relocation happens */
__boot_func
void  z_early_memcpy(void *dst, const void *src, size_t n)
{
	/* attempt word-sized copying only if buffers have identical alignment */
	unsigned char *d_byte = (unsigned char *)dst;
	const unsigned char *s_byte = (const unsigned char *)src;
	/* do byte-sized copying until finished */

	while (n > 0) {
		*(d_byte++) = *(s_byte++);
		n--;
	}

	return (void)dst;
}

__boot_func
void z_early_memset(void *dst, int c, size_t n)
{
	/* do byte-sized initialization until word-aligned or finished */

	unsigned char *d_byte = (unsigned char *)dst;
	unsigned char c_byte = (unsigned char)c;

	while (n > 0) {
		*(d_byte++) = c_byte;
		n--;
	}
	return (void)dst;
}

void *relocate_code_setup(void)
{
#if (defined(CONFIG_ARM_MPU) && !defined(CONFIG_CPU_HAS_NXP_MPU))
	disable_mpu_rasr_xn();
#endif	/* CONFIG_ARM_MPU */
	return NULL;
}

ZTEST_SUITE(code_relocation, NULL, relocate_code_setup, NULL, NULL, NULL);
