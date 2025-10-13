/* Copyright (c) 2025 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_internal.h>
#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

ZTEST(riscv_pmp_memattr_entries, test_dt_pmp_perm_conversion)
{
	uint8_t result;

	result = DT_MEM_RISCV_TO_PMP_PERM(0);
	zassert_equal(result, 0, "Expected 0, got 0x%x", result);

	result = DT_MEM_RISCV_TO_PMP_PERM(DT_MEM_RISCV_TYPE_IO_R);
	zassert_equal(result, PMP_R, "Expected PMP_R (0x%x), got 0x%x", PMP_R, result);

	result = DT_MEM_RISCV_TO_PMP_PERM(DT_MEM_RISCV_TYPE_IO_W);
	zassert_equal(result, PMP_W, "Expected PMP_W (0x%x), got 0x%x", PMP_W, result);

	result = DT_MEM_RISCV_TO_PMP_PERM(DT_MEM_RISCV_TYPE_IO_X);
	zassert_equal(result, PMP_X, "Expected PMP_X (0x%x), got 0x%x", PMP_X, result);

	result = DT_MEM_RISCV_TO_PMP_PERM(DT_MEM_RISCV_TYPE_IO_R | DT_MEM_RISCV_TYPE_IO_W);
	zassert_equal(result, PMP_R | PMP_W, "Expected R|W (0x%x), got 0x%x", PMP_R | PMP_W,
		      result);

	result = DT_MEM_RISCV_TO_PMP_PERM(DT_MEM_RISCV_TYPE_IO_R | DT_MEM_RISCV_TYPE_IO_W |
					  DT_MEM_RISCV_TYPE_IO_X);
	zassert_equal(result, PMP_R | PMP_W | PMP_X, "Expected R|W|X (0x%x), got 0x%x",
		      PMP_R | PMP_W | PMP_X, result);
}

ZTEST_SUITE(riscv_pmp_memattr_entries, NULL, NULL, NULL, NULL, NULL);
