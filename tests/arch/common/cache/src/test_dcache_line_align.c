/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_DCACHE_LINE_SIZE)

#include <zephyr/ztest.h>
#include <zephyr/cache.h>
#include <zephyr/linker/sections.h>
#include <zephyr/linker/linker-defs.h>

#define DCACHE_LINE_SIZE CONFIG_DCACHE_LINE_SIZE

/**
 * @brief DCache line alignment attributes tests
 * @defgroup tests_dcache_line DCache line alignment attributes
 * @ingroup all_tests
 * @{
 */

BUILD_ASSERT((DCACHE_LINE_SIZE & (DCACHE_LINE_SIZE - 1)) == 0,
	     "CONFIG_DCACHE_LINE_SIZE must be a power of 2");

static uint8_t var_aligned1 __dcacheline_aligned;
static uint8_t var_aligned2[5] __dcacheline_aligned;
static uint8_t var_control1;

ZTEST(dcache_line_align, test_dcacheline_aligned)
{
	zassert_true(IS_ALIGNED(&var_aligned1, DCACHE_LINE_SIZE));
	zassert_true(IS_ALIGNED(var_aligned2, DCACHE_LINE_SIZE));
	zassert_false(IS_ALIGNED(&var_aligned1 + 1, DCACHE_LINE_SIZE));

	var_aligned1 = 4;
	var_aligned2[0] = 5;
	var_control1 = 6;

	zassert_equal(var_aligned1, 4);
	zassert_equal(var_aligned2[0], 5);
	zassert_equal(var_control1, 6);
}

struct exclusive_obj_desc {
	const void *addr;
	size_t size;
};

static uintptr_t cacheline_start(const void *ptr)
{
	return ROUND_DOWN((uintptr_t)ptr, DCACHE_LINE_SIZE);
}

static uintptr_t cacheline_end(const void *ptr, size_t size)
{
	return ROUND_UP((uintptr_t)ptr + size, DCACHE_LINE_SIZE);
}

static bool ranges_overlap(uintptr_t start_a, uintptr_t end_a,
			   uintptr_t start_b, uintptr_t end_b)
{
	return (start_a < end_b) && (start_b < end_a);
}

static void assert_obj_in_section(const void *ptr, uintptr_t section_start,
				  uintptr_t section_end)
{
	uintptr_t addr = (uintptr_t)ptr;

	zassert_true(addr >= section_start,
		     "object at 0x%lx is below section start 0x%lx",
		     (unsigned long)addr, (unsigned long)section_start);
	zassert_true(addr < section_end,
		     "object at 0x%lx is at/after section end 0x%lx",
		     (unsigned long)addr, (unsigned long)section_end);
}

static void assert_obj_not_in_section(const void *ptr, uintptr_t section_start,
				      uintptr_t section_end)
{
	uintptr_t addr = (uintptr_t)ptr;

	zassert_true((addr < section_start) || (addr >= section_end),
		     "control object at 0x%lx unexpectedly landed in [0x%lx, 0x%lx)",
		     (unsigned long)addr,
		     (unsigned long)section_start,
		     (unsigned long)section_end);
}

static void assert_exclusive_layout(const struct exclusive_obj_desc *objs,
				    size_t count)
{
	for (size_t i = 0; i < count; i++) {
		uintptr_t start_i = cacheline_start(objs[i].addr);
		uintptr_t end_i = cacheline_end(objs[i].addr, objs[i].size);

		zassert_true(IS_ALIGNED(objs[i].addr, DCACHE_LINE_SIZE),
			     "object %d is not cache-line aligned", (int)i);
		zassert_true(end_i > start_i,
			     "object %d has invalid cache-line coverage", (int)i);

		for (size_t j = i + 1; j < count; j++) {
			uintptr_t start_j = cacheline_start(objs[j].addr);
			uintptr_t end_j = cacheline_end(objs[j].addr, objs[j].size);

			zassert_false(ranges_overlap(start_i, end_i, start_j, end_j),
				      "exclusive objects %d and %d share cache-line range",
				      (int)i, (int)j);
		}
	}
}

/*
 * Boundary-sized exclusive noinit objects:
 * - 1 byte
 * - line_size - 1
 * - line_size
 * - line_size + 1
 * - 2 * line_size + 3
 */
static uint8_t var_exclusive_noinit1[1] __dcacheline_exclusive_noinit;
static uint8_t var_exclusive_noinit2[DCACHE_LINE_SIZE - 1] __dcacheline_exclusive_noinit;
static uint8_t var_exclusive_noinit3[DCACHE_LINE_SIZE] __dcacheline_exclusive_noinit;
static uint8_t var_exclusive_noinit4[DCACHE_LINE_SIZE + 1] __dcacheline_exclusive_noinit;
static uint8_t var_exclusive_noinit5[(2 * DCACHE_LINE_SIZE) + 3] __dcacheline_exclusive_noinit;

static uint8_t var_control_noinit;
static uint8_t var_control_data = 11;

ZTEST(dcache_line_align, test_dcacheline_exclusive_noinit)
{
	static const struct exclusive_obj_desc objs[] = {
		{var_exclusive_noinit1, sizeof(var_exclusive_noinit1)},
		{var_exclusive_noinit2, sizeof(var_exclusive_noinit2)},
		{var_exclusive_noinit3, sizeof(var_exclusive_noinit3)},
		{var_exclusive_noinit4, sizeof(var_exclusive_noinit4)},
		{var_exclusive_noinit5, sizeof(var_exclusive_noinit5)},
	};
	uintptr_t sec_start = (uintptr_t)__dcacheline_exclusive_noinit_start;
	uintptr_t sec_end = (uintptr_t)__dcacheline_exclusive_noinit_end;

	zassert_true(IS_ALIGNED(sec_start, DCACHE_LINE_SIZE));
	zassert_true(IS_ALIGNED(sec_end, DCACHE_LINE_SIZE));
	zassert_true(sec_end > sec_start);

	zassert_true(sec_start >= (uintptr_t)__noinit_start);
	zassert_true(sec_end <= (uintptr_t)__noinit_end);

	for (size_t i = 0; i < ARRAY_SIZE(objs); i++) {
		assert_obj_in_section(objs[i].addr, sec_start, sec_end);
	}

	assert_obj_not_in_section(&var_control_noinit, sec_start, sec_end);
	assert_obj_not_in_section(&var_control_data, sec_start, sec_end);

	assert_exclusive_layout(objs, ARRAY_SIZE(objs));

	var_exclusive_noinit1[0] = 1;
	var_exclusive_noinit2[0] = 2;
	var_exclusive_noinit3[DCACHE_LINE_SIZE - 1] = 3;
	var_exclusive_noinit4[DCACHE_LINE_SIZE] = 4;
	var_exclusive_noinit5[(2 * DCACHE_LINE_SIZE) + 2] = 5;
	var_control_noinit = 6;

	zassert_equal(var_exclusive_noinit1[0], 1);
	zassert_equal(var_exclusive_noinit2[0], 2);
	zassert_equal(var_exclusive_noinit3[DCACHE_LINE_SIZE - 1], 3);
	zassert_equal(var_exclusive_noinit4[DCACHE_LINE_SIZE], 4);
	zassert_equal(var_exclusive_noinit5[(2 * DCACHE_LINE_SIZE) + 2], 5);
	zassert_equal(var_control_noinit, 6);
}

/*
 * Boundary-sized exclusive data objects:
 * - 1 byte
 * - line_size - 1
 * - line_size
 * - line_size + 1
 * - 2 * line_size + 3
 */
static uint8_t var_exclusive_data1[1] __dcacheline_exclusive_data = { 9 };
static uint8_t var_exclusive_data2[DCACHE_LINE_SIZE - 1] __dcacheline_exclusive_data = { 4 };
static uint8_t var_exclusive_data3[DCACHE_LINE_SIZE] __dcacheline_exclusive_data = { 7 };
static uint8_t var_exclusive_data4[DCACHE_LINE_SIZE + 1] __dcacheline_exclusive_data = { 1 };
static uint8_t var_exclusive_data5[(2 * DCACHE_LINE_SIZE) + 3] __dcacheline_exclusive_data = { 2 };

ZTEST(dcache_line_align, test_dcacheline_exclusive_data)
{
	static const struct exclusive_obj_desc objs[] = {
		{ var_exclusive_data1, sizeof(var_exclusive_data1) },
		{ var_exclusive_data2, sizeof(var_exclusive_data2) },
		{ var_exclusive_data3, sizeof(var_exclusive_data3) },
		{ var_exclusive_data4, sizeof(var_exclusive_data4) },
		{ var_exclusive_data5, sizeof(var_exclusive_data5) },
	};
	uintptr_t sec_start = (uintptr_t)__dcacheline_exclusive_data_start;
	uintptr_t sec_end = (uintptr_t)__dcacheline_exclusive_data_end;

	zassert_true(IS_ALIGNED(sec_start, DCACHE_LINE_SIZE));
	zassert_true(IS_ALIGNED(sec_end, DCACHE_LINE_SIZE));
	zassert_true(sec_end > sec_start);

#ifdef CONFIG_XIP
	zassert_true(sec_start >= (uintptr_t)__data_region_start);
	zassert_true(sec_end <= (uintptr_t)__data_region_end);
#endif

	for (size_t i = 0; i < ARRAY_SIZE(objs); i++) {
		assert_obj_in_section(objs[i].addr, sec_start, sec_end);
	}

	assert_obj_not_in_section(&var_control_data, sec_start, sec_end);
	assert_obj_not_in_section(&var_control_noinit, sec_start, sec_end);

	assert_exclusive_layout(objs, ARRAY_SIZE(objs));

	zassert_equal(var_exclusive_data1[0], 9);
	zassert_equal(var_exclusive_data2[0], 4);
	zassert_equal(var_exclusive_data3[0], 7);
	zassert_equal(var_exclusive_data4[0], 1);
	zassert_equal(var_exclusive_data5[0], 2);
	zassert_equal(var_control_data, 11);
}

ZTEST_SUITE(dcache_line_align, NULL, NULL, NULL, NULL, NULL);

/**
 * @}
 */

#endif /* CONFIG_DCACHE_LINE_SIZE */
