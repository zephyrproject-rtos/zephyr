/*
 * Copyright (c) 2022 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

SECTION_PROLOGUE(ztest,,)
{
	Z_LINK_ITERABLE(ztest_expected_result_entry);

	Z_LINK_ITERABLE(ztest_suite_node);

	Z_LINK_ITERABLE(ztest_unit_test);

	Z_LINK_ITERABLE(ztest_test_rule);
} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
