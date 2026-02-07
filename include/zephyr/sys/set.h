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

#ifndef ZEPHYR_INCLUDE_SYS_SET_H_
#define ZEPHYR_INCLUDE_SYS_SET_H_

#include <stdint.h>

/**
 * @brief Disjoint-set node structure
 */
struct sys_set_node {
	/** @cond INTERNAL_HIDDEN */
	struct sys_set_node *parent;
	uint16_t rank;
	/** @endcond */
};

/**
 * @brief Initialize a disjoint-set.
 *
 * @param node Pointer to a sys_set_node structure.
 * @param rank Initial rank (usually set to 0).
 */
static inline void sys_set_makeset(struct sys_set_node *node, uint16_t rank)
{
	node->parent = node;
	node->rank = rank;
}

/**
 * @brief Find the root of the disjoint-set.
 *
 * @param node Pointer to a sys_set_node structure.
 * @return Pointer to the root sys_set_node of the set containing the @p node.
 */
struct sys_set_node *sys_set_find(struct sys_set_node *node);

/**
 * @brief Merge two nodes into the same disjoint-set.
 *
 * This function attaches the node with the smaller rank to the one with the
 * larger rank. That say, if @p node1 has a smaller rank than @p node2, it will
 * be linked to @p node2.
 *
 * @param node1 Pointer to a sys_set_node structure.
 * @param node2 Pointer to a sys_set_node structure.
 */
void sys_set_union(struct sys_set_node *node1, struct sys_set_node *node2);

/** @} */

#endif /* ZEPHYR_INCLUDE_SYS_SET_H_ */
