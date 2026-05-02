/*
 * Copyright (c) 2025 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_internal.h>
#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

ZTEST(riscv_pmp_decode_region, test_pmp_tor_index_0)
{
	unsigned long pmp_addr[] = {
		0x10000000U >> 2,
		0,
	};
	uint8_t cfg = PMP_TOR;
	unsigned int index = 0;
	unsigned long start_addr, end_addr;

	unsigned long expected_end = 0x10000000U - 1;

	pmp_decode_region(cfg, pmp_addr, index, &start_addr, &end_addr);

	zassert_equal(0, start_addr, "TOR index 0 start address mismatch");
	zassert_equal(expected_end, end_addr, "TOR index 0 end address mismatch");
}

ZTEST(riscv_pmp_decode_region, test_pmp_tor_index_n)
{
	unsigned long pmp_addr[] = {
		0x10000000U >> 2,
		0x20000000U >> 2,
	};
	uint8_t cfg = PMP_TOR;
	unsigned int index = 1;
	unsigned long start_addr, end_addr;

	unsigned long expected_start = 0x10000000U;
	unsigned long expected_end = 0x20000000U - 1;

	pmp_decode_region(cfg, pmp_addr, index, &start_addr, &end_addr);

	zassert_equal(expected_start, start_addr, "TOR index n start address mismatch");
	zassert_equal(expected_end, end_addr, "TOR index n end address mismatch");
}

ZTEST(riscv_pmp_decode_region, test_pmp_na4)
{
	unsigned long pmp_addr[] = {
		0xADBEEF00U >> 2,
	};
	uint8_t cfg = PMP_NA4;
	unsigned int index = 0;
	unsigned long start_addr, end_addr;

	unsigned long expected_start = 0xADBEEF00U;
	unsigned long expected_end = 0xADBEEF00U + 3;

	pmp_decode_region(cfg, pmp_addr, index, &start_addr, &end_addr);

	zassert_equal(expected_start, start_addr, "NA4 start address mismatch");
	zassert_equal(expected_end, end_addr, "NA4 end address mismatch");
}

ZTEST(riscv_pmp_decode_region, test_pmp_napot)
{
	unsigned long pmp_addr[] = {0x20000000U >> 2};
	uint8_t cfg = PMP_NAPOT;
	unsigned int index = 0;
	unsigned long start_addr, end_addr;

	unsigned long expected_start = 0x20000000U;
	unsigned long expected_end = 0x20000007U;

	pmp_decode_region(cfg, pmp_addr, index, &start_addr, &end_addr);
	zassert_equal(expected_start, start_addr, "NAPOT 8-byte start address mismatch");
	zassert_equal(expected_end, end_addr, "NAPOT 8-byte end address mismatch");
}

ZTEST(riscv_pmp_decode_region, test_pmp_default_disabled)
{
	unsigned long pmp_addr[] = {0x12345678U >> 2};
	uint8_t cfg = 0x00;
	unsigned int index = 0;
	unsigned long start_addr, end_addr;

	pmp_decode_region(cfg, pmp_addr, index, &start_addr, &end_addr);
	zassert_equal(0, start_addr, "Default start address mismatch");
	zassert_equal(0, end_addr, "Default end address mismatch");
}

ZTEST_SUITE(riscv_pmp_decode_region, NULL, NULL, NULL, NULL, NULL);
