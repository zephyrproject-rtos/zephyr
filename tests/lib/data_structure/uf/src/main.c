/*
 * Copyright (c) 2025 James Roy <rruuaanng@outlook.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/uf.h>
#include <zephyr/ztest.h>

struct user_data {
	int n;
	struct uf_node ufnode;
} data_list[5] = {{.n = 1}, {.n = 3}, {.n = 2}, {.n = 5}, {.n = 4}};

ZTEST(uf, test_find_union)
{
	uf_makeset(&data_list[0].ufnode, 5);
	uf_makeset(&data_list[1].ufnode, 4);
	uf_makeset(&data_list[2].ufnode, 3);
	uf_makeset(&data_list[3].ufnode, 2);
	uf_makeset(&data_list[4].ufnode, 1);

	uf_union(&data_list[0].ufnode, &data_list[1].ufnode);
	uf_union(&data_list[1].ufnode, &data_list[2].ufnode);
	uf_union(&data_list[2].ufnode, &data_list[3].ufnode);
	uf_union(&data_list[3].ufnode, &data_list[4].ufnode);

	struct uf_node *root_node = uf_find(&data_list[4].ufnode);
	struct user_data *root_data = CONTAINER_OF(root_node, struct user_data, ufnode);

	zassert_equal(root_data->n, data_list[0].n, "Root set data n are not equal");
	zassert_equal(root_data->ufnode.rank, root_node->rank, "Root set are not equal");
}

ZTEST_SUITE(uf, NULL, NULL, NULL, NULL, NULL);
