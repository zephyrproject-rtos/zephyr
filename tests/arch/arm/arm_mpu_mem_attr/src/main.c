/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <cmsis_core.h>

#include <zephyr/arch/arm/mpu/arm_mpu.h>
#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/memory-attr/memory-attr.h>
#include <zephyr/ztest.h>

#define TEST_REGION DT_NODELABEL(test_mpu_region)
#define TEST_REGION_ADDR DT_REG_ADDR(TEST_REGION)

#define TEST_VALID_ATTR \
	(DT_MEM_EXPLICIT_PERMS | DT_MEM_K_READABLE | DT_MEM_NON_CACHEABLE | DT_MEM_NON_VOLATILE)

static inline uint32_t read_mpu_info(void)
{
	uint32_t value;

	__get_CP(15, 0, value, 0, 0, 4);
	return value;
}

#if defined(CONFIG_ARMV7_R)
static inline void select_region(uint32_t index)
{
	__set_CP(15, 0, index, 6, 2, 0);
}

static inline uint32_t read_region_base(void)
{
	uint32_t value;

	__get_CP(15, 0, value, 6, 1, 0);
	return value;
}

static inline uint32_t read_region_attributes(void)
{
	uint32_t value;

	__get_CP(15, 0, value, 6, 1, 4);
	return value;
}

static const arm_mpu_region_attr_t ram_rw = REGION_RAM_XN_ATTR(REGION_4K);
static const arm_mpu_region_attr_t ram_ro = REGION_RAM_PRIV_RO_XN_ATTR(REGION_4K);
static const arm_mpu_region_attr_t ram_text = REGION_RAM_PRIV_RX_ATTR(REGION_4K);
static const arm_mpu_region_attr_t ram_rwx = REGION_RAM_PRIV_RWX_ATTR(REGION_4K);
static const arm_mpu_region_attr_t shared =
	REGION_RAM_NOCACHE_SHAREABLE_P_RW_U_RW_XN_ATTR(REGION_4K);

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
						     DT_MEM_EXPLICIT_PERMS |
						     DT_MEM_K_WRITABLE | DT_MEM_CACHEABLE),
		      -EINVAL, "unreadable policy was accepted");
	zassert_equal(z_arm_mpu_validate_dt_region(TEST_REGION_ADDR, 0x1000,
						     DT_MEM_EXPLICIT_PERMS |
						     DT_MEM_K_READABLE | DT_MEM_CACHEABLE |
						     DT_MEM_NON_CACHEABLE),
		      -EINVAL, "conflicting cache policy was accepted");
	zassert_equal(z_arm_mpu_validate_dt_region(TEST_REGION_ADDR + 1U, 0x1000,
						     TEST_VALID_ATTR),
		      -EINVAL, "misaligned region was accepted");
}
#elif defined(CONFIG_AARCH32_ARMV8_R)
static inline void select_region(uint32_t index)
{
	__set_CP(15, 0, index, 6, 2, 1);
}

static inline uint32_t read_region_base(void)
{
	uint32_t value;

	__get_CP(15, 0, value, 6, 3, 0);
	return value;
}

static inline uint32_t read_region_limit(void)
{
	uint32_t value;

	__get_CP(15, 0, value, 6, 3, 1);
	return value;
}

static const arm_mpu_region_attr_t ram_rw = REGION_RAM_ATTR(0x1000);
static const arm_mpu_region_attr_t ram_ro = REGION_RAM_PRIV_RO_XN_ATTR(0x1000);
static const arm_mpu_region_attr_t ram_text = REGION_RAM_PRIV_RX_ATTR(0x1000);
static const arm_mpu_region_attr_t ram_rwx = REGION_RAM_PRIV_RWX_ATTR(0x1000);
static const arm_mpu_region_attr_t shared =
	REGION_RAM_NOCACHE_OUTER_SHAREABLE_P_RW_U_RW_XN_ATTR(0x1000);

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
						     DT_MEM_EXPLICIT_PERMS |
						     DT_MEM_K_WRITABLE | DT_MEM_CACHEABLE),
		      -EINVAL, "unreadable policy was accepted");
	zassert_equal(z_arm_mpu_validate_dt_region(TEST_REGION_ADDR, 0x1000,
						     DT_MEM_EXPLICIT_PERMS |
						     DT_MEM_K_READABLE | DT_MEM_CACHEABLE |
						     DT_MEM_NON_CACHEABLE),
		      -EINVAL, "conflicting cache policy was accepted");
	zassert_equal(z_arm_mpu_validate_dt_region(TEST_REGION_ADDR + 1U, 0x1000,
						     TEST_VALID_ATTR),
		      -EINVAL, "misaligned region was accepted");
	zassert_equal(z_arm_mpu_validate_dt_region(TEST_REGION_ADDR, 0x1000,
						     TEST_VALID_ATTR), 0);
	zassert_equal(z_arm_mpu_validate_dt_region(TEST_REGION_ADDR, 0x1001,
						     TEST_VALID_ATTR), -EINVAL);
}
#else
#error "This test requires ARMv7-R or ARMv8-R"
#endif

ZTEST(arm_mpu_mem_attr, test_legacy_region_programmed)
{
	bool found = false;
#if defined(CONFIG_ARMV7_R)
	uint32_t count = (read_mpu_info() & MPU_TYPE_DREGION_Msk) >> MPU_TYPE_DREGION_Pos;
#else
	uint32_t count = (read_mpu_info() >> MPU_IR_REGION_Pos) & MPU_IR_REGION_Msk;
#endif

	for (uint32_t index = 0U; index < count; index++) {
		select_region(index);
#if defined(CONFIG_ARMV7_R)
		if ((read_region_base() & MPU_RBAR_ADDR_Msk) !=
		    DT_REG_ADDR(DT_NODELABEL(legacy_mpu_region))) {
			continue;
		}
		zassert_equal(read_region_attributes() & MPU_RASR_AP_Msk, P_RW_U_NA_Msk);
		zassert_true(read_region_attributes() & NOT_EXEC);
#else
		if ((read_region_base() & MPU_RBAR_BASE_Msk) !=
		    DT_REG_ADDR(DT_NODELABEL(legacy_mpu_region))) {
			continue;
		}
		zassert_equal(read_region_base() & MPU_RBAR_AP_Msk, P_RW_U_NA_Msk);
		zassert_true(read_region_base() & NOT_EXEC);
#endif
		found = true;
		break;
	}
	zassert_true(found, "Legacy DT region was not programmed");
}

#if defined(CONFIG_AARCH32_ARMV8_R)
/* Use two unused slots to exercise the same scan as boot initialization. */
ZTEST(arm_mpu_mem_attr, test_boot_region_overlaps)
{
	uint32_t count = (read_mpu_info() >> MPU_IR_REGION_Pos) & MPU_IR_REGION_Msk;
	uint32_t first = count - 2U;
	uint32_t second = count - 1U;
	uint32_t saved_index;
	uint32_t value;
	int adjacent_result;
	int pair_result;
	int dt_result;
	int static_result;
	unsigned int key = irq_lock();

	__get_CP(15, 0, saved_index, 6, 2, 1);
	select_region(first);
	if (read_region_limit() & MPU_RLAR_EN_Msk) {
		select_region(saved_index);
		irq_unlock(key);
		zassert_unreachable("Overlap test needs two unused MPU slots");
		return;
	}
	select_region(second);
	if (read_region_limit() & MPU_RLAR_EN_Msk) {
		select_region(saved_index);
		irq_unlock(key);
		zassert_unreachable("Overlap test needs two unused MPU slots");
		return;
	}

	select_region(first);
	value = TEST_REGION_ADDR + 0x2000U;
	__set_CP(15, 0, value, 6, 3, 0);
	value = (TEST_REGION_ADDR + 0x2fffU) & MPU_RLAR_LIMIT_Msk;
	value |= MPU_RLAR_EN_Msk;
	__set_CP(15, 0, value, 6, 3, 1);
	select_region(second);
	value = TEST_REGION_ADDR + 0x3000U;
	__set_CP(15, 0, value, 6, 3, 0);
	value = ((TEST_REGION_ADDR + 0x3fffU) & MPU_RLAR_LIMIT_Msk) | MPU_RLAR_EN_Msk;
	__set_CP(15, 0, value, 6, 3, 1);
	adjacent_result = z_arm_mpu_validate_boot_regions();

	select_region(second);
	value = TEST_REGION_ADDR + 0x2000U;
	__set_CP(15, 0, value, 6, 3, 0);
	pair_result = z_arm_mpu_validate_boot_regions();

	select_region(second);
	value = TEST_REGION_ADDR;
	__set_CP(15, 0, value, 6, 3, 0);
	dt_result = z_arm_mpu_validate_boot_regions();

	select_region(second);
	value = 0U;
	__set_CP(15, 0, value, 6, 3, 1);
	value = mpu_config.mpu_regions[0].base;
	__set_CP(15, 0, value, 6, 3, 0);
	value = (value & MPU_RLAR_LIMIT_Msk) | MPU_RLAR_EN_Msk;
	__set_CP(15, 0, value, 6, 3, 1);
	static_result = z_arm_mpu_validate_boot_regions();

	value = 0U;
	select_region(second);
	__set_CP(15, 0, value, 6, 3, 1);
	__set_CP(15, 0, value, 6, 3, 0);
	select_region(first);
	__set_CP(15, 0, value, 6, 3, 1);
	__set_CP(15, 0, value, 6, 3, 0);
	select_region(saved_index);
	irq_unlock(key);
	zassert_ok(adjacent_result, "Adjacent regions were rejected");
	zassert_equal(pair_result, -EINVAL, "Overlapping pair was accepted");
	zassert_equal(dt_result, -EINVAL, "Overlap with a boot DT region was accepted");
	zassert_equal(static_result, -EINVAL, "Overlap with a SoC region was accepted");
}
#endif

/* Exercise the translator, rather than only checking predefined macros. */
ZTEST(arm_mpu_mem_attr, test_explicit_permissions)
{
	static const struct {
		uint32_t permissions;
		uint32_t access;
		bool executable;
	} policies[] = {
		{DT_MEM_K_READABLE, P_RO_U_NA_Msk, false},
		{DT_MEM_K_READABLE | DT_MEM_K_WRITABLE, P_RW_U_NA_Msk, false},
		{DT_MEM_K_READABLE | DT_MEM_U_READABLE, P_RO_U_RO_Msk, false},
		{DT_MEM_K_READABLE | DT_MEM_K_WRITABLE | DT_MEM_U_READABLE |
		 DT_MEM_U_WRITABLE, P_RW_U_RW_Msk, false},
		{DT_MEM_K_READABLE | DT_MEM_K_EXECUTABLE, P_RO_U_NA_Msk, true},
		{DT_MEM_K_READABLE | DT_MEM_K_EXECUTABLE | DT_MEM_U_READABLE |
		 DT_MEM_U_EXECUTABLE, P_RO_U_RO_Msk, true},
#if defined(CONFIG_ARMV7_R)
		{0U, P_NA_U_NA_Msk, false},
		{DT_MEM_K_READABLE | DT_MEM_K_WRITABLE | DT_MEM_U_READABLE,
		 P_RW_U_RO_Msk, false},
#endif
	};

	for (size_t i = 0U; i < ARRAY_SIZE(policies); i++) {
		arm_mpu_region_attr_t attr;
		uint32_t encoded;
		uint32_t access_mask;

		zassert_ok(z_arm_mpu_dt_region_attr(TEST_REGION_ADDR, 0x1000,
			DT_MEM_EXPLICIT_PERMS | DT_MEM_NON_CACHEABLE | DT_MEM_SHAREABLE |
			policies[i].permissions, &attr));
#if defined(CONFIG_ARMV7_R)
		encoded = attr.rasr;
		access_mask = MPU_RASR_AP_Msk;
		zassert_true(encoded & MPU_RASR_S_Msk);
#else
		encoded = attr.rbar;
		access_mask = MPU_RBAR_AP_Msk;
		zassert_equal(encoded & MPU_RBAR_SH_Msk, OUTER_SHAREABLE_Msk);
#endif
		zassert_equal(encoded & access_mask, policies[i].access);
		zassert_equal((encoded & NOT_EXEC) == 0U, policies[i].executable);
	}
}

ZTEST(arm_mpu_mem_attr, test_unrepresentable_permissions)
{
	static const uint32_t invalid[] = {
		DT_MEM_U_READABLE,
		DT_MEM_K_READABLE | DT_MEM_U_WRITABLE,
		DT_MEM_K_READABLE | DT_MEM_U_EXECUTABLE,
		DT_MEM_K_READABLE | DT_MEM_K_EXECUTABLE | DT_MEM_U_READABLE,
#if defined(CONFIG_AARCH32_ARMV8_R)
		0U,
		DT_MEM_K_READABLE | DT_MEM_K_WRITABLE | DT_MEM_U_READABLE,
#endif
	};

	for (size_t i = 0U; i < ARRAY_SIZE(invalid); i++) {
		zassert_equal(z_arm_mpu_validate_dt_region(TEST_REGION_ADDR, 0x1000,
			DT_MEM_EXPLICIT_PERMS | DT_MEM_CACHEABLE | invalid[i]), -EINVAL);
	}
	zassert_equal(z_arm_mpu_validate_dt_region(TEST_REGION_ADDR, 0x1000,
		DT_MEM_EXPLICIT_PERMS | DT_MEM_K_READABLE), -EINVAL);
	zassert_equal(z_arm_mpu_validate_dt_region(0xfffff000U, 0x2000,
		TEST_VALID_ATTR), -EINVAL);
	zassert_equal(z_arm_mpu_validate_dt_region(TEST_REGION_ADDR, 0U,
		TEST_VALID_ATTR), -EINVAL);
#if defined(CONFIG_ARMV7_R)
	zassert_equal(z_arm_mpu_validate_dt_region(TEST_REGION_ADDR, 0x1800,
		TEST_VALID_ATTR), -EINVAL);
#endif
}

ZTEST_SUITE(arm_mpu_mem_attr, NULL, NULL, NULL, NULL, NULL);
