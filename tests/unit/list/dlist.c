/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/dlist.h>

static sys_dlist_t test_list;

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
 * @addtogroup unit_tests
 * @{
 */

/**
 * @brief Verify doubly linked list functionalities
 *
 * @see sys_dlist_append(), sys_dlist_remove(), sys_dlist_prepend(),
 * sys_dlist_remove(), sys_dlist_insert(), sys_dlist_peek_next()
 * SYS_DLIST_ITERATE_FROM_NODE()
 */
ZTEST(dlist_api, test_dlist)
{
	sys_dlist_init(&test_list);

	zassert_true((verify_emptyness(&test_list)),
			"test_list should be empty");

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
	sys_dlist_insert(test_node_2.node.next, &test_node_4.node);

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

int cond(sys_dnode_t *node, void *data)
{
	return (node == data) ? 1 : 0;
}
/**
 * @brief Verify doubly linked list functionalities
 *
 * @see sys_dlist_is_head(),sys_dlist_is_tail(),
 * sys_dlist_has_multiple_nodes(),sys_dlist_get()
 * sys_dlist_peek_head_not_empty(),sys_dlist_insert_at(),
 * sys_dlist_peek_prev(),
 */
ZTEST(dlist_api, test_dlist2)
{
	struct container_node test_node[6];
	struct container_node insert_node;
	struct container_node insert_node2;

	/* Initialize */
	memset(test_node, 0, sizeof(test_node));
	memset(&insert_node, 0, sizeof(insert_node));
	memset(&insert_node2, 0, sizeof(insert_node2));
	sys_dlist_init(&test_list);

	/* Check if the dlist is empty */
	zassert_true(sys_dlist_get(&test_list) == NULL,
			"Get a empty dilst, NULL will be returned");

	/* Check if a node can append as head if dlist is empty */
	sys_dlist_insert_at(&test_list, &insert_node.node,
				cond, &test_node[2].node);
	zassert_true(test_list.head == &insert_node.node, "");
	zassert_true(test_list.tail == &insert_node.node, "");

	/* Re-initialize and insert nodes */
	sys_dlist_init(&test_list);

	for (int i = 0; i < 5; i++) {
		sys_dlist_append(&test_list, &test_node[i].node);
	}

	zassert_true(sys_dlist_peek_head_not_empty(&test_list) != NULL,
				"dlist appended incorrectly");

	zassert_true(sys_dlist_is_head(&test_list,
						&test_node[0].node),
				"dlist appended incorrectly");

	zassert_true(sys_dlist_is_tail(&test_list,
					&test_node[4].node),
				"dlist appended incorrectly");

	zassert_true(sys_dlist_has_multiple_nodes(&test_list),
				"dlist appended incorrectly");

	zassert_true(sys_dlist_peek_prev(&test_list,
				&test_node[2].node) == &test_node[1].node,
				"dlist appended incorrectly");

	zassert_true(sys_dlist_peek_prev(&test_list,
				&test_node[0].node) == NULL,
				"dlist appended incorrectly");

	zassert_true(sys_dlist_peek_prev(&test_list,
				NULL) == NULL,
				"dlist appended incorrectly");

	zassert_true(sys_dlist_get(&test_list) ==
				&test_node[0].node,
				"Get a dilst, head will be returned");

	/* Check if a node can insert in front of known nodes */
	sys_dlist_insert_at(&test_list, &insert_node.node,
				cond, &test_node[2].node);
	zassert_true(sys_dlist_peek_next(&test_list,
				&test_node[1].node) == &insert_node.node, " ");

	/* Check if a node can append if the node is unknown */
	sys_dlist_insert_at(&test_list, &insert_node2.node,
				cond, &test_node[5].node);
	zassert_true(sys_dlist_peek_next(&test_list,
				&test_node[4].node) == &insert_node2.node, " ");
}

/**
 * @}
 */
