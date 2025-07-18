/*
 * Copyright (c) 2025 James Roy <rruuaanng@outlook.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/uf.h>

struct uf_node *uf_find(struct uf_node *node)
{
	while (node != node->parent) {
		node->parent = node->parent->parent;
		node = node->parent;
	}

	return node;
}

void uf_union(struct uf_node *node1, struct uf_node *node2)
{
	struct uf_node *root1 = uf_find(node1);
	struct uf_node *root2 = uf_find(node2);

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
