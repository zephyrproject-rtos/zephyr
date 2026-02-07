/*
 * Copyright (c) 2025 James Roy <rruuaanng@outlook.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/set.h>

struct sys_set_node *sys_set_find(struct sys_set_node *node)
{
	struct sys_set_node *parent;

	while (node != node->parent) {
		parent = node->parent;
		node->parent = parent->parent;
		node = parent;
	}

	return node;
}

void sys_set_union(struct sys_set_node *node1, struct sys_set_node *node2)
{
	struct sys_set_node *root1 = sys_set_find(node1);
	struct sys_set_node *root2 = sys_set_find(node2);

	if (root1 == root2) {
		return;
	}

	if (root1->rank < root2->rank) {
		root1->parent = root2;
	} else if (root1->rank > root2->rank) {
		root2->parent = root1;
	} else {
		root2->parent = root1;
		root1->rank++;
	}
}
