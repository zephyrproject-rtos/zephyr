/*
 * Copyright 2022 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

SECTIONS
{
	.data.ztest_suite_node_area : ALIGN(4)
	{
		_ztest_suite_node_list_start = .;
		KEEP(*(SORT_BY_NAME(._ztest_suite_node.static.*)))
		_ztest_suite_node_list_end = .;
	}
	.data.ztest_unit_test_area : ALIGN(4)
	{
		_ztest_unit_test_list_start = .;
		KEEP(*(SORT_BY_NAME(._ztest_unit_test.static.*)))
		_ztest_unit_test_list_end = .;
	}
	.data.ztest_test_rule_area : ALIGN(4)
	{
		_ztest_test_rule_list_start = .;
		KEEP(*(SORT_BY_NAME(._ztest_test_rule.static.*)))
		_ztest_test_rule_list_end = .;
	}
}
INSERT AFTER .data;
