/*
 * Copyright 2021 Google LLC
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
}
INSERT AFTER .data;
