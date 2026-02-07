/*
 * Copyright (c) 2025 James Roy <rruuaanng@outlook.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/set.h>
#include <zephyr/ztest.h>

struct user_data {
	int n;
	struct sys_set_node node;
} data_list[5] = {{.n = 1}, {.n = 3}, {.n = 2}, {.n = 5}, {.n = 4}};

ZTEST(set, test_find_and_union)
{
	sys_set_makeset(&data_list[0].node, 5);
	sys_set_makeset(&data_list[1].node, 4);
	sys_set_makeset(&data_list[2].node, 3);
	sys_set_makeset(&data_list[3].node, 2);
	sys_set_makeset(&data_list[4].node, 1);

	/* Check all parent nodes */
	for (int i = 0, j = 5; i < 5; i++, j--) {
		zassert_equal(data_list[i].node.rank, j);
	}

	sys_set_union(&data_list[0].node, &data_list[1].node);
	sys_set_union(&data_list[1].node, &data_list[2].node);
	sys_set_union(&data_list[2].node, &data_list[3].node);
	sys_set_union(&data_list[3].node, &data_list[4].node);

	struct sys_set_node *root_node = sys_set_find(&data_list[4].node);
	struct user_data *root_data = CONTAINER_OF(root_node, struct user_data, node);

	zassert_equal(root_data->n, data_list[0].n, "Root set data n are not equal");
	zassert_equal(root_data->node.rank, root_node->rank, "Root set are not equal");
}

ZTEST_SUITE(set, NULL, NULL, NULL, NULL, NULL);
