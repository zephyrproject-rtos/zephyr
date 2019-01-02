/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <misc/dlist.h>

static sys_dlist_t test_list;
static sys_dlist_t test_list2;

struct container_node {
	sys_dnode_t node;
	int unused;
};

static struct container_node test_node_1;
static struct container_node test_node_2;
static struct container_node test_node_3;
static struct container_node test_node_4;

static inline bool verify_emptyness(sys_dlist_t *list)
{
	sys_dnode_t *node;
	sys_dnode_t *s_node;
	struct container_node *cnode;
	struct container_node *s_cnode;
	int count;

	if (!sys_dlist_is_empty(list)) {
		return false;
	}

	if (sys_dlist_peek_head(list)) {
		return false;
	}

	if (sys_dlist_peek_tail(list)) {
		return false;
	}

	count = 0;
	SYS_DLIST_FOR_EACH_NODE(list, node) {
		count++;
	}

	if (count) {
		return false;
	}

	SYS_DLIST_FOR_EACH_NODE_SAFE(list, node, s_node) {
		count++;
	}

	if (count) {
		return false;
	}

	count = 0;
	SYS_DLIST_FOR_EACH_CONTAINER(list, cnode, node) {
		count++;
	}

	if (count) {
		return false;
	}

	count = 0;
	SYS_DLIST_FOR_EACH_CONTAINER_SAFE(list, cnode, s_cnode, node) {
		count++;
	}

	if (count) {
		return false;
	}

	return true;
}

static inline bool verify_content_amount(sys_dlist_t *list, int amount)
{
	sys_dnode_t *node;
	sys_dnode_t *s_node;
	struct container_node *cnode;
	struct container_node *s_cnode;
	int count;

	if (sys_dlist_is_empty(list)) {
		return false;
	}

	if (!sys_dlist_peek_head(list)) {
		return false;
	}

	if (!sys_dlist_peek_tail(list)) {
		return false;
	}

	count = 0;
	SYS_DLIST_FOR_EACH_NODE(list, node) {
		count++;
	}

	if (count != amount) {
		return false;
	}

	count = 0;
	SYS_DLIST_FOR_EACH_NODE_SAFE(list, node, s_node) {
		count++;
	}

	if (count != amount) {
		return false;
	}

	count = 0;
	SYS_DLIST_FOR_EACH_CONTAINER(list, cnode, node) {
		count++;
	}

	if (count != amount) {
		return false;
	}

	count = 0;
	SYS_DLIST_FOR_EACH_CONTAINER_SAFE(list, cnode, s_cnode, node) {
		count++;
	}

	if (count != amount) {
		return false;
	}

	return true;
}

static inline bool verify_tail_head(sys_dlist_t *list,
				    sys_dnode_t *head,
				    sys_dnode_t *tail,
				    bool same)
{
	if (sys_dlist_peek_head(list) != head) {
		return false;
	}

	if (sys_dlist_peek_tail(list) != tail) {
		return false;
	}

	if (same) {
		if (sys_dlist_peek_head(list) != sys_dlist_peek_tail(list)) {
			return false;
		}
	} else {
		if (sys_dlist_peek_head(list) == sys_dlist_peek_tail(list)) {
			return false;
		}
	}

	return true;
}
/**
 * @addtogroup kernel_common_tests
 * @{
 */

/**
 * @brief Verify doubly linked list funtionalities
 *
 * @see sys_dlist_append(), sys_dlist_remove(), sys_dlist_prepend(),
 * sys_dlist_remove(), sys_dlist_insert_after(), sys_dlist_peek_next()
 * SYS_DLIST_ITERATE_FROM_NODE()
 */
void test_dlist(void)
{
	sys_dlist_init(&test_list);

	zassert_true((verify_emptyness(&test_list)), "test_list should be empty");

	/* Appending node 1 */
	sys_dlist_append(&test_list, &test_node_1.node);
	zassert_true((verify_content_amount(&test_list, 1)),
		     "test_list has wrong content");

	zassert_true((verify_tail_head(&test_list, &test_node_1.node,
				       &test_node_1.node, true)),
		     "test_list head/tail are wrong");

	/* Finding and removing node 1 */
	zassert_true(sys_dnode_is_linked(&test_node_1.node),
		     "node1 is not linked");
	sys_dlist_remove(&test_node_1.node);
	zassert_true((verify_emptyness(&test_list)),
		     "test_list should be empty");
	zassert_false(sys_dnode_is_linked(&test_node_1.node),
		      "node1 is still linked");

	/* Prepending node 1 */
	sys_dlist_prepend(&test_list, &test_node_1.node);
	zassert_true((verify_content_amount(&test_list, 1)),
		     "test_list has wrong content");

	zassert_true((verify_tail_head(&test_list, &test_node_1.node,
				       &test_node_1.node, true)),
		     "test_list head/tail are wrong");

	/* Removing node 1 */
	sys_dlist_remove(&test_node_1.node);
	zassert_true((verify_emptyness(&test_list)),
		     "test_list should be empty");

	/* Appending node 1 */
	sys_dlist_append(&test_list, &test_node_1.node);
	/* Prepending node 2 */
	sys_dlist_prepend(&test_list, &test_node_2.node);

	zassert_true((verify_content_amount(&test_list, 2)),
		     "test_list has wrong content");

	zassert_true((verify_tail_head(&test_list, &test_node_2.node,
				       &test_node_1.node, false)),
		     "test_list head/tail are wrong");

	/* Appending node 3 */
	sys_dlist_append(&test_list, &test_node_3.node);

	zassert_true((verify_content_amount(&test_list, 3)),
		     "test_list has wrong content");

	zassert_true((verify_tail_head(&test_list, &test_node_2.node,
				       &test_node_3.node, false)),
		     "test_list head/tail are wrong");

	zassert_true((sys_dlist_peek_next(&test_list, &test_node_2.node) ==
		      &test_node_1.node),
		     "test_list node links are wrong");

	/* Inserting node 4 after node 2 */
	sys_dlist_insert_after(&test_list, &test_node_2.node,
			       &test_node_4.node);

	zassert_true((verify_tail_head(&test_list, &test_node_2.node,
				       &test_node_3.node, false)),
		     "test_list head/tail are wrong");

	zassert_true((sys_dlist_peek_next(&test_list, &test_node_2.node) ==
		      &test_node_4.node),
		     "test_list node links are wrong");

	/* Finding and removing node 1 */
	sys_dlist_remove(&test_node_1.node);
	zassert_true((verify_content_amount(&test_list, 3)),
		     "test_list has wrong content");

	zassert_true((verify_tail_head(&test_list, &test_node_2.node,
				       &test_node_3.node, false)),
		     "test_list head/tail are wrong");

	/* Removing node 3 */
	sys_dlist_remove(&test_node_3.node);
	zassert_true((verify_content_amount(&test_list, 2)),
		     "test_list has wrong content");

	zassert_true((verify_tail_head(&test_list, &test_node_2.node,
				       &test_node_4.node, false)),
		     "test_list head/tail are wrong");

	/* Removing node 4 */
	sys_dlist_remove(&test_node_4.node);
	zassert_true((verify_content_amount(&test_list, 1)),
		     "test_list has wrong content");

	zassert_true((verify_tail_head(&test_list, &test_node_2.node,
				       &test_node_2.node, true)),
		     "test_list head/tail are wrong");

	/* Removing node 2 */
	sys_dlist_remove(&test_node_2.node);
	zassert_true((verify_emptyness(&test_list)),
		     "test_list should be empty");


	/* Catenate an empty list to a non-empty list */
	sys_dlist_append(&test_list, &test_node_1.node);
	sys_dlist_init(&test_list2);
	sys_dlist_join(&test_list, &test_list2);
	zassert_true(sys_dlist_is_empty(&test_list2),
		     "list2 not empty");
	zassert_true((verify_tail_head(&test_list, &test_node_1.node,
				       &test_node_1.node, true)),
		     "test_list head/tail are wrong");

	/* Catenate a non-empty list to an empty list moves elements. */
	sys_dlist_join(&test_list2, &test_list);
	zassert_true(sys_dlist_is_empty(&test_list),
		     "list not empty");
	zassert_true((verify_tail_head(&test_list2, &test_node_1.node,
				       &test_node_1.node, true)),
		     "test_list2 head/tail are wrong");

	/* Catenate a non-empty list to a non-empty list moves elements. */
	sys_dlist_append(&test_list, &test_node_2.node);
	sys_dlist_append(&test_list, &test_node_3.node);
	zassert_true((verify_tail_head(&test_list, &test_node_2.node,
				       &test_node_3.node, false)),
		     "test_list head/tail are wrong");
	sys_dlist_join(&test_list2, &test_list);
	zassert_true(sys_dlist_is_empty(&test_list),
		     "list not empty");
	zassert_true((verify_tail_head(&test_list2, &test_node_1.node,
				       &test_node_3.node, false)),
		     "test_list2 head/tail are wrong");
	zassert_equal(test_node_1.node.next, &test_node_2.node,
		      "node2 not after node1");
	zassert_equal(test_node_2.node.prev, &test_node_1.node,
		      "node1 not before node2");

	/* Split list at head does nothing */
	sys_dlist_split(&test_list, &test_list2, &test_node_1.node);
	zassert_true(sys_dlist_is_empty(&test_list),
		     "list not empty");

	/* Split list after head moves */
	sys_dlist_split(&test_list, &test_list2, &test_node_2.node);
	zassert_true((verify_tail_head(&test_list, &test_node_1.node,
				       &test_node_1.node, true)),
		     "test_list head/tail are wrong");
	zassert_true((verify_tail_head(&test_list2, &test_node_2.node,
				       &test_node_3.node, false)),
		     "test_list2 head/tail are wrong");

	/* Split list after head moves */
	sys_dlist_split(&test_list, &test_list2, &test_node_3.node);
	zassert_true((verify_tail_head(&test_list, &test_node_1.node,
				       &test_node_2.node, false)),
		     "test_list head/tail are wrong");
	zassert_true((verify_tail_head(&test_list2, &test_node_3.node,
				       &test_node_3.node, true)),
		     "test_list2 head/tail are wrong");

	sys_dlist_remove(&test_node_1.node);
	sys_dlist_remove(&test_node_2.node);
	zassert_true(sys_dlist_is_empty(&test_list),
		     "list not empty");

	sys_dlist_remove(&test_node_3.node);
	zassert_true(sys_dlist_is_empty(&test_list2),
		     "list2 not empty");

	/* test iterator from a node */
	struct data_node {
		sys_dnode_t node;
		int data;
	} data_node[6] = {
		{ .data = 0 },
		{ .data = 1 },
		{ .data = 2 },
		{ .data = 3 },
		{ .data = 4 },
		{ .data = 5 },
	};
	sys_dnode_t *node = NULL;
	int ii;

	sys_dlist_init(&test_list);

	for (ii = 0; ii < 6; ii++) {
		sys_dlist_append(&test_list, &data_node[ii].node);
	}

	ii = 0;
	SYS_DLIST_ITERATE_FROM_NODE(&test_list, node) {
		ii++;
		if (((struct data_node *)node)->data == 2) {
			break;
		}
	}
	zassert_equal(ii, 3, "");

	ii = 0;
	SYS_DLIST_ITERATE_FROM_NODE(&test_list, node) {
		ii++;
		if (((struct data_node *)node)->data == 3) {
			break;
		}
	}
	zassert_equal(ii, 1, "");

	ii = 0;
	SYS_DLIST_ITERATE_FROM_NODE(&test_list, node) {
		ii++;
	}
	zassert_equal(ii, 2, "");
}

/**
 * @}
 */
