/*
 * Copyright (c) 2025 James Roy <rruuaanng@outlook.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @defgroup union_find_apis Disjoint-set
 * @ingroup datastructure_apis
 *
 * @brief Disjoint-set implementation
 *
 * This implements a disjoint-set (union-find) that guarantees
 * O(alpha(n)) runtime for all operations. The algorithms and
 * naming are conventional per existing academic and didactic
 * implementations, c.f.:
 *
 * https://en.wikipedia.org/wiki/Disjoint-set_data_structure
 *
 * @{
 */

#ifndef ZEPHYR_INCLUDE_SYS_UF_H_
#define ZEPHYR_INCLUDE_SYS_UF_H_

#include <stdint.h>

/**
 * @brief Disjoint-set node structure
 */
struct uf_node {
	/** @cond INTERNAL_HIDDEN */
	struct uf_node *parent;
	uint16_t rank;
	/** @endcond */
};

/**
 * @brief Initialize a disjoint-set.
 */
static inline void uf_makeset(struct uf_node *node)
{
	node->parent = node;
	node->rank = 0;
}

/**
 * @brief Find the root of the disjoint-set.
 */
struct uf_node *uf_find(struct uf_node *node);

/**
 * @brief Merge two nodes into the same disjoint-set.
 */
void uf_union(struct uf_node *node1, struct uf_node *node2);

/** @} */

#endif /* ZEPHYR_INCLUDE_SYS_UF_H_ */
