/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/rb.h>

#define TREE_SIZE 512
/* zephyr can't do floating-point arithmetic,
 * so manual: dlog_N = 2log(TREE_SIZE) = 18
 */
const static uint32_t dlog_N = 18;

/* rbnode structure is embeddable in user structure */
struct container_node {
	struct rbnode node;
	int value;
};
static struct rbnode nodes[TREE_SIZE];
static struct rbtree test_rbtree;

/* Our "lessthan" is just the location of the struct */
bool node_lessthan(struct rbnode *a, struct rbnode *b)
{
	return a < b;
}

/**
 * @brief Test whether rbtree node struct is embedded
 * in any user struct
 *
 * @details
 * Test Objective:
 * - Define and initialize a rbtree, and test two features:
 * first, rbtree node struct can be embedded in any user struct.
 * last, rbtree can be walked though by some macro APIs.
 *
 * Testing techniques:
 * - Interface testing
 * - Dynamic analysis and testing
 * - Structural test coverage(entry points,statements,branches)
 *
 * Prerequisite Conditions:
 * - Design a predicate function node_lessthan
 * - Define a rbtree by using struct rbtree
 *
 * Input Specifications:
 * - N/A
 *
 * Test Procedure:
 * -# Define some arrays of rbtree nodes.And initialize
 * the rbtree.
 * -# Then inserting some nodes into the rbtree.
 * -# Check if the inserted nodes are contained in the
 * rbtree by using the iteration APIs.
 *
 * Expected Test Result:
 * - The inserted nodes are contained in the
 * rbtree by using the iteration APIs.
 *
 * Pass/Fail Criteria:
 * - Successful if check points in test procedure are all pass,
 * otherwise failure.
 *
 * Assumptions and Constraints:
 * - N/A
 *
 * @ingroup lib_rbtree_tests
 *
 * @see RB_FOR_EACH(),RB_FOR_EACH_CONTAINER()
 */
ZTEST(rbtree_perf, test_rbtree_container)
{
	int count = 0;
	struct rbtree test_tree_l;
	struct container_node *c_foreach_node;
	struct rbnode *foreach_node;
	struct container_node tree_node[10];

	(void)memset(&test_tree_l, 0, sizeof(test_tree_l));
	(void)memset(tree_node, 0, sizeof(tree_node));

	test_tree_l.lessthan_fn = node_lessthan;
	for (uint32_t i = 0; i < ARRAY_SIZE(tree_node); i++) {
		tree_node[i].value = i;
		rb_insert(&test_tree_l, &tree_node[i].node);
	}

	RB_FOR_EACH(&test_tree_l, foreach_node) {
		zassert_true(CONTAINER_OF(foreach_node, struct container_node,
		node)->value == count, "RB_FOR_EACH failed");
		count++;
	}

	count = 0;

	RB_FOR_EACH_CONTAINER(&test_tree_l, c_foreach_node, node) {
		zassert_true(c_foreach_node->value == count,
		"RB_FOR_EACH_CONTAINER failed");
		count++;
	}
}

/* initialize and insert a tree */
static void init_tree(struct rbtree *tree, int size)
{
	tree->lessthan_fn = node_lessthan;

	for (uint32_t i = 0; i < size; i++) {
		rb_insert(tree, &nodes[i]);
	}
}

static int search_height_recurse(struct rbnode *node, struct rbnode
			*final_node, uint32_t current_height)
{
	if (node == NULL) {
		return -1;
	}

	if (node == final_node) {
		return current_height;
	}

	current_height++;
	struct rbnode *ch = z_rb_child(node,
			!test_rbtree.lessthan_fn(final_node, node));

	return search_height_recurse(ch, final_node, current_height);
}

static void verify_rbtree_perf(struct rbnode *root, struct rbnode *test)
{
	uint32_t node_height = 0;

	node_height = search_height_recurse(root, test, node_height);
	zassert_true(node_height <= dlog_N);
}

/**
 * @brief Test some operations of rbtree are running in
 * logarithmic time
 *
 * @details
 * Test Objective:
 * - The inserted, removed, get minimum and get maximum operations
 * of rbtree are in logarithmic time by comparing a node's operation
 * height with the worst-case's height.
 *
 * Testing techniques:
 * - Interface testing
 * - Dynamic analysis and testing
 * - Structural test coverage(entry points,statements,branches)
 *
 * Prerequisite Conditions:
 * - Design a predicate function node_lessthan
 * - Define a rbtree by using struct rbtree
 *
 * Input Specifications:
 * - N/A
 *
 * Test Procedure:
 * -# Initialize the rbtree and insert some nodes on it.
 * -# Search the far left/right node by recursion and
 * record its height.
 * -# Check if the recorded heights are less than the worst
 * case height(log2(N)).
 * -# Select out a node at the rbtree arbitrarily as remove
 * and insert node. And record the height.
 * -# repeat the check point above.
 *
 * Expected Test Result:
 * - Some operations of rbtree are running in
 * logarithmic time
 *
 * Pass/Fail Criteria:
 * - Successful if check points in test procedure are all pass,
 * otherwise failure.
 *
 * Assumptions and Constraints:
 * - N/A
 *
 * @ingroup lib_rbtree_tests
 *
 * @see rb_get_min(), rb_get_max()
 */
ZTEST(rbtree_perf, test_rbtree_perf)
{
	init_tree(&test_rbtree, TREE_SIZE);
	struct rbnode *root = test_rbtree.root;
	struct rbnode *test = NULL;

	test = rb_get_min(&test_rbtree);
	verify_rbtree_perf(root, test);

	test = rb_get_max(&test_rbtree);
	verify_rbtree_perf(root, test);

	/* insert and remove a same node with same height.Assume that
	 * the node nodes[TREE_SIZE/2] will be removed and inserted,
	 * verify that searching times is less than 2logN
	 * using the height of this node.
	 */
	test = &nodes[TREE_SIZE/2];
	verify_rbtree_perf(root, test);
}

ZTEST_SUITE(rbtree_perf, NULL, NULL, NULL, NULL, NULL);
