/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/* The reset vector programs the MPU from __xt_mpu_init_table when one is
 * linked in. The map it leaves is read at entry to xtensa_mpu_init(),
 * before the MPU driver replaces it.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>
#include <xtensa/config/core-isa.h>
#include <xtensa/hal.h>

/* Covers all memory, with split points and memory types the default map does not use */
const xthal_MPU_entry __xt_mpu_init_table[] = {
	XTHAL_MPU_ENTRY(0x00000000, 1, XTHAL_AR_RWXrwx, XTHAL_MEM_WRITEBACK),
	XTHAL_MPU_ENTRY(0x12345000, 1, XTHAL_AR_RWXrwx, XTHAL_MEM_WRITETHRU),
	XTHAL_MPU_ENTRY(0x4abcd000, 1, XTHAL_AR_RWXrwx, XTHAL_MEM_WRITEBACK_NOALLOC),
	XTHAL_MPU_ENTRY(0x50000000, 1, XTHAL_AR_RWXrwx, XTHAL_MEM_WRITEBACK),
	XTHAL_MPU_ENTRY(0x9abcd000, 1, XTHAL_AR_RWXrwx, XTHAL_MEM_DEVICE),
	XTHAL_MPU_ENTRY(0xdead0000, 1, XTHAL_AR_RWXrwx, XTHAL_MEM_WRITETHRU),
};
const unsigned int __xt_mpu_init_table_size = ARRAY_SIZE(__xt_mpu_init_table);

#define TABLE_SIZE  ARRAY_SIZE(__xt_mpu_init_table)
#define FIRST_ENTRY (XCHAL_MPU_ENTRIES - TABLE_SIZE)

/* Entry index bits of the "at" word, filled in by the hardware */
#define AT_INDEX_MASK 0x1fU

static uint32_t reset_mpuenb;
static xthal_MPU_entry reset_map[XCHAL_MPU_ENTRIES];

void __real_xtensa_mpu_init(void);

void __wrap_xtensa_mpu_init(void)
{
	__asm__ volatile("rsr.mpuenb %0" : "=r"(reset_mpuenb));
	for (uint32_t i = 0U; i < XCHAL_MPU_ENTRIES; i++) {
		__asm__ volatile("rptlb0 %0, %1" : "=r"(reset_map[i].as) : "r"(i));
		__asm__ volatile("rptlb1 %0, %1" : "=r"(reset_map[i].at) : "r"(i));
	}
	__real_xtensa_mpu_init();
}

/* The reset vector left exactly the table entries enabled, in the top MPU entries */
ZTEST(xtensa_mpu_reset_table, test_map_is_table)
{
	zassert_equal(reset_mpuenb, GENMASK(XCHAL_MPU_ENTRIES - 1U, FIRST_ENTRY), "MPUENB 0x%08x",
		      reset_mpuenb);

	for (uint32_t i = 0U; i < TABLE_SIZE; i++) {
		const xthal_MPU_entry *hw = &reset_map[FIRST_ENTRY + i];

		zassert_equal(hw->as, __xt_mpu_init_table[i].as, "entry %u as 0x%08x", i, hw->as);
		zassert_equal(hw->at & ~AT_INDEX_MASK, __xt_mpu_init_table[i].at,
			      "entry %u at 0x%08x", i, hw->at);
	}
}

ZTEST_SUITE(xtensa_mpu_reset_table, NULL, NULL, NULL, NULL, NULL);
