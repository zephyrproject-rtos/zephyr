/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/rc/rc.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/ztest.h>

ZTEST(rc_no_callback, test_empty_section)
{
	int count;

	STRUCT_SECTION_COUNT(rc_callback, &count);
	zassert_equal(count, 0);
}

ZTEST_SUITE(rc_no_callback, NULL, NULL, NULL, NULL, NULL);
