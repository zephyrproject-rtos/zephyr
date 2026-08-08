/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/arch/arm/mpu/arm_mpu.h>
#include <zephyr/ztest.h>

#if defined(CONFIG_ARMV7_R)
static const arm_mpu_region_attr_t ram_rw = REGION_RAM_PRIV_RW_ATTR(REGION_4K);
static const arm_mpu_region_attr_t ram_ro = REGION_RAM_PRIV_RO_ATTR(REGION_4K);
static const arm_mpu_region_attr_t ram_text = REGION_RAM_PRIV_TEXT_ATTR(REGION_4K);
static const arm_mpu_region_attr_t ram_rwx = REGION_RAM_PRIV_RWX_ATTR(REGION_4K);
static const arm_mpu_region_attr_t shared = REGION_SHARED_MEM_USERSPACE_ATTR(REGION_4K);

ZTEST(arm_mpu_mem_attr, test_region_attributes)
{
	zassert_equal(ram_rw.rasr & MPU_RASR_AP_Msk, P_RW_U_NA_Msk);
	zassert_true(ram_rw.rasr & NOT_EXEC);
	zassert_equal(ram_ro.rasr & MPU_RASR_AP_Msk, P_RO_U_NA_Msk);
	zassert_true(ram_ro.rasr & NOT_EXEC);
	zassert_equal(ram_text.rasr & MPU_RASR_AP_Msk, P_RO_U_NA_Msk);
	zassert_false(ram_text.rasr & NOT_EXEC);
	zassert_equal(ram_rwx.rasr & MPU_RASR_AP_Msk, P_RW_U_NA_Msk);
	zassert_false(ram_rwx.rasr & NOT_EXEC);
	zassert_equal(shared.rasr & MPU_RASR_AP_Msk, P_RW_U_RW_Msk);
	zassert_true(shared.rasr & NOT_EXEC);
	zassert_true(shared.rasr & MPU_RASR_S_Msk);
	zassert_equal(shared.rasr, NORMAL_OUTER_INNER_NON_CACHEABLE_SHAREABLE |
				    NOT_EXEC | REGION_4K | P_RW_U_RW_Msk);
}
#elif defined(CONFIG_AARCH32_ARMV8_R)
static const arm_mpu_region_attr_t ram_rw = REGION_RAM_PRIV_RW_ATTR(0x1000);
static const arm_mpu_region_attr_t ram_ro = REGION_RAM_PRIV_RO_ATTR(0x1000);
static const arm_mpu_region_attr_t ram_text = REGION_RAM_PRIV_TEXT_ATTR(0x1000);
static const arm_mpu_region_attr_t ram_rwx = REGION_RAM_PRIV_RWX_ATTR(0x1000);
static const arm_mpu_region_attr_t shared = REGION_SHARED_MEM_USERSPACE_ATTR(0x1000);

ZTEST(arm_mpu_mem_attr, test_region_attributes)
{
	zassert_equal(ram_rw.rbar & MPU_RBAR_AP_Msk, P_RW_U_NA_Msk);
	zassert_true(ram_rw.rbar & NOT_EXEC);
	zassert_equal(ram_ro.rbar & MPU_RBAR_AP_Msk, P_RO_U_NA_Msk);
	zassert_true(ram_ro.rbar & NOT_EXEC);
	zassert_equal(ram_text.rbar & MPU_RBAR_AP_Msk, P_RO_U_NA_Msk);
	zassert_false(ram_text.rbar & NOT_EXEC);
	zassert_equal(ram_rwx.rbar & MPU_RBAR_AP_Msk, P_RW_U_NA_Msk);
	zassert_false(ram_rwx.rbar & NOT_EXEC);
	zassert_equal(shared.rbar & MPU_RBAR_AP_Msk, P_RW_U_RW_Msk);
	zassert_true(shared.rbar & NOT_EXEC);
	zassert_equal(shared.rbar & MPU_RBAR_SH_Msk, OUTER_SHAREABLE_Msk);
	zassert_equal(shared.mair_idx, MPU_MAIR_INDEX_SRAM_NOCACHE);
}
#else
#error "This test requires ARMv7-R or ARMv8-R"
#endif

ZTEST_SUITE(arm_mpu_mem_attr, NULL, NULL, NULL, NULL, NULL);
