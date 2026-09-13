/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/arch/arm/mpu/arm_mpu.h>
#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/memory-attr/memory-attr.h>
#include <zephyr/ztest.h>

#define TEST_REGION DT_NODELABEL(test_mpu_region)
#define TEST_REGION_ADDR DT_REG_ADDR(TEST_REGION)

#define TEST_VALID_ATTR \
	(DT_MEM_READABLE | DT_MEM_NON_CACHEABLE | DT_MEM_NON_VOLATILE)

static inline uint32_t read_mpu_info(void)
{
	uint32_t value;

	__asm__ volatile("mrc p15, 0, %0, c0, c0, 4" : "=r" (value) ::);
	return value;
}

#if defined(CONFIG_ARMV7_R)
static inline void select_region(uint32_t index)
{
	__asm__ volatile("mcr p15, 0, %0, c6, c2, 0" : : "r" (index) :);
}

static inline uint32_t read_region_base(void)
{
	uint32_t value;

	__asm__ volatile("mrc p15, 0, %0, c6, c1, 0" : "=r" (value) ::);
	return value;
}

static inline uint32_t read_region_attributes(void)
{
	uint32_t value;

	__asm__ volatile("mrc p15, 0, %0, c6, c1, 4" : "=r" (value) ::);
	return value;
}

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
ZTEST(arm_mpu_mem_attr, test_dt_region_programmed)
{
	bool found = false;

	uint32_t region_count = (read_mpu_info() & MPU_TYPE_DREGION_Msk) >> MPU_TYPE_DREGION_Pos;

	for (uint32_t index = 0U; index < region_count; index++) {
		select_region(index);
		if ((read_region_base() & MPU_RBAR_ADDR_Msk) == TEST_REGION_ADDR) {
			uint32_t attributes = read_region_attributes();

			zassert_equal(attributes & MPU_RASR_AP_Msk, P_RO_U_NA_Msk);
			zassert_true(attributes & NOT_EXEC);
			zassert_equal(attributes & (MPU_RASR_TEX_Msk | MPU_RASR_C_Msk |
				      MPU_RASR_B_Msk),
				      NORMAL_OUTER_INNER_NON_CACHEABLE_NON_SHAREABLE);
			found = true;
			break;
		}
	}
	zassert_true(found, "DT-defined MPU region was not programmed");
}

ZTEST(arm_mpu_mem_attr, test_invalid_dt_regions_rejected)
{
	zassert_equal(z_arm_mpu_validate_dt_region(TEST_REGION_ADDR, 0x1000,
						     DT_MEM_WRITABLE | DT_MEM_CACHEABLE),
		      -EINVAL, "unreadable policy was accepted");
	zassert_equal(z_arm_mpu_validate_dt_region(TEST_REGION_ADDR, 0x1000,
						     DT_MEM_READABLE | DT_MEM_CACHEABLE |
						     DT_MEM_NON_CACHEABLE),
		      -EINVAL, "conflicting cache policy was accepted");
	zassert_equal(z_arm_mpu_validate_dt_region(TEST_REGION_ADDR + 1U, 0x1000,
						     TEST_VALID_ATTR),
		      -EINVAL, "misaligned region was accepted");
}
#elif defined(CONFIG_AARCH32_ARMV8_R)
static inline void select_region(uint32_t index)
{
	__asm__ volatile("mcr p15, 0, %0, c6, c2, 1" : : "r" (index) :);
}

static inline uint32_t read_region_base(void)
{
	uint32_t value;

	__asm__ volatile("mrc p15, 0, %0, c6, c3, 0" : "=r" (value) ::);
	return value;
}

static inline uint32_t read_region_limit(void)
{
	uint32_t value;

	__asm__ volatile("mrc p15, 0, %0, c6, c3, 1" : "=r" (value) ::);
	return value;
}

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
ZTEST(arm_mpu_mem_attr, test_dt_region_programmed)
{
	bool found = false;

	uint32_t region_count = (read_mpu_info() >> MPU_IR_REGION_Pos) & MPU_IR_REGION_Msk;

	for (uint32_t index = 0U; index < region_count; index++) {
		select_region(index);
		if ((read_region_base() & MPU_RBAR_BASE_Msk) == TEST_REGION_ADDR) {
			uint32_t rbar = read_region_base();
			uint32_t rlar = read_region_limit();

			zassert_equal(rbar & MPU_RBAR_AP_Msk, P_RO_U_NA_Msk);
			zassert_true(rbar & NOT_EXEC);
			zassert_equal(rbar & MPU_RBAR_SH_Msk, NON_SHAREABLE_Msk);
			zassert_equal((rlar & MPU_RLAR_AttrIndx_Msk) >>
				      MPU_RLAR_AttrIndx_Pos,
				      MPU_MAIR_INDEX_SRAM_NOCACHE);
			found = true;
			break;
		}
	}
	zassert_true(found, "DT-defined MPU region was not programmed");
}

ZTEST(arm_mpu_mem_attr, test_invalid_dt_regions_rejected)
{
	zassert_equal(z_arm_mpu_validate_dt_region(TEST_REGION_ADDR, 0x1000,
						     DT_MEM_WRITABLE | DT_MEM_CACHEABLE),
		      -EINVAL, "unreadable policy was accepted");
	zassert_equal(z_arm_mpu_validate_dt_region(TEST_REGION_ADDR, 0x1000,
						     DT_MEM_READABLE | DT_MEM_CACHEABLE |
						     DT_MEM_NON_CACHEABLE),
		      -EINVAL, "conflicting cache policy was accepted");
	zassert_equal(z_arm_mpu_validate_dt_region(TEST_REGION_ADDR + 1U, 0x1000,
						     TEST_VALID_ATTR),
		      -EINVAL, "misaligned region was accepted");
	zassert_equal(z_arm_mpu_validate_dt_region(TEST_REGION_ADDR, 0x1000,
						     TEST_VALID_ATTR),
		      -EINVAL, "overlapping region was accepted");
}
#else
#error "This test requires ARMv7-R or ARMv8-R"
#endif

ZTEST_SUITE(arm_mpu_mem_attr, NULL, NULL, NULL, NULL, NULL);
