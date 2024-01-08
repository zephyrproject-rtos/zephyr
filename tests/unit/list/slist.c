/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/slist.h>

static sys_slist_t test_list;
static sys_slist_t append_list;

struct container_node {
	sys_snode_t node;
	int unused;
};

static struct container_node test_node_1;
static struct container_node test_node_2;
static struct container_node test_node_3;
static struct container_node test_node_4;

static inline bool verify_emptyness(sys_slist_t *list)
{
	sys_snode_t *node;
	sys_snode_t *s_node;
	struct container_node *cnode;
	struct container_node *s_cnode;
	int count;

	if (!sys_slist_is_empty(list)) {
		return false;
	}

	if (sys_slist_peek_head(list)) {
		return false;
	}

	if (sys_slist_peek_tail(list)) {
		return false;
	}

	if (sys_slist_len(list) != 0) {
		return false;
	}

	count = 0;
	SYS_SLIST_FOR_EACH_NODE(list, node) {
		count++;
	}

	if (count) {
		return false;
	}

	SYS_SLIST_FOR_EACH_NODE_SAFE(list, node, s_node) {
		count++;
	}

	if (count) {
		return false;
	}

	count = 0;
	SYS_SLIST_FOR_EACH_CONTAINER(list, cnode, node) {
		count++;
	}

	if (count) {
		return false;
	}

	count = 0;
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(list, cnode, s_cnode, node) {
		count++;
	}

	if (count) {
		return false;
	}

	return true;
}

static inline bool verify_content_amount(sys_slist_t *list, int amount)
{
	sys_snode_t *node;
	sys_snode_t *s_node;
	struct container_node *cnode;
	struct container_node *s_cnode;
	int count;

	if (sys_slist_is_empty(list)) {
		return false;
	}

	if (!sys_slist_peek_head(list)) {
		return false;
	}

	if (!sys_slist_peek_tail(list)) {
		return false;
	}

	if (sys_slist_len(list) != amount) {
		return false;
	}

	count = 0;
	SYS_SLIST_FOR_EACH_NODE(list, node) {
		count++;
	}

	if (count != amount) {
		return false;
	}

	count = 0;
	SYS_SLIST_FOR_EACH_NODE_SAFE(list, node, s_node) {
		count++;
	}

	if (count != amount) {
		return false;
	}

	count = 0;
	SYS_SLIST_FOR_EACH_CONTAINER(list, cnode, node) {
		count++;
	}

	if (count != amount) {
		return false;
	}

	count = 0;
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(list, cnode, s_cnode, node) {
		count++;
	}

	if (count != amount) {
		return false;
	}

	return true;
}

static inline bool verify_tail_head(sys_slist_t *list,
				    sys_snode_t *head,
				    sys_snode_t *tail,
				    bool same)
{
	if (sys_slist_peek_head(list) != head) {
		return false;
	}

	if (sys_slist_peek_tail(list) != tail) {
		return false;
	}

	if (same) {
		if (sys_slist_peek_head(list) != sys_slist_peek_tail(list)) {
			return false;
		}
	} else {
		if (sys_slist_peek_head(list) == sys_slist_peek_tail(list)) {
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
 * @brief Test singly linked list functionalities
 *
 * @details Test list initialization, append item to the list,
 * find and remove item, prepend, append, remove list
 *
 * @see sys_slist_init(), sys_slist_append(),
 * sys_slist_find_and_remove(), sys_slist_prepend(),
 * sys_slist_remove(), sys_slist_get(), sys_slist_get_not_empty(),
 * sys_slist_append_list(), sys_slist_merge_list(), sys_slist_find()
 */
ZTEST(dlist_api, test_slist)
{
	sys_slist_init(&test_list);

	zassert_true((verify_emptyness(&test_list)),
		      "test_list should be empty");

	/* Appending node 1 */
	sys_slist_append(&test_list, &test_node_1.node);
	zassert_true((verify_content_amount(&test_list, 1)),
		     "test_list has wrong content");

	zassert_true((verify_tail_head(&test_list, &test_node_1.node,
				       &test_node_1.node, true)),
		     "test_list head/tail are wrong");

	/* Find the node 1, previous node should be null */
	sys_snode_t *test_node_1_prev = &test_node_1.node;

	zassert_true(sys_slist_find(&test_list, &test_node_1.node, &test_node_1_prev),
		     "test_list did not find node ");
	zassert_is_null(test_node_1_prev, "test_list previous node not null ");

	/* Finding and removing node 1 */
	sys_slist_find_and_remove(&test_list, &test_node_1.node);
	zassert_true((verify_emptyness(&test_list)),
		     "test_list should be empty");

	/* Prepending node 1 */
	sys_slist_prepend(&test_list, &test_node_1.node);
	zassert_true((verify_content_amount(&test_list, 1)),
		     "test_list has wrong content");

	zassert_true((verify_tail_head(&test_list, &test_node_1.node,
				       &test_node_1.node, true)),
		     "test_list head/tail are wrong");

	/* Removing node 1 */
	sys_slist_remove(&test_list, NULL, &test_node_1.node);
	zassert_true((verify_emptyness(&test_list)),
		     "test_list should be empty");

	/* Appending node 1 */
	sys_slist_append(&test_list, &test_node_1.node);
	/* Prepending node 2 */
	sys_slist_prepend(&test_list, &test_node_2.node);

	zassert_true((verify_content_amount(&test_list, 2)),
		     "test_list has wrong content");

	zassert_true((verify_tail_head(&test_list, &test_node_2.node,
				       &test_node_1.node, false)),
		     "test_list head/tail are wrong");

	/* Appending node 3 */
	sys_slist_append(&test_list, &test_node_3.node);

	zassert_true((verify_content_amount(&test_list, 3)),
		     "test_list has wrong content");

	zassert_true((verify_tail_head(&test_list, &test_node_2.node,
				       &test_node_3.node, false)),
		     "test_list head/tail are wrong");

	zassert_true((sys_slist_peek_next(&test_node_2.node) ==
		      &test_node_1.node),
		     "test_list node links are wrong");

	/* Inserting node 4 after node 2, peek with nocheck variant */
	sys_slist_insert(&test_list, &test_node_2.node, &test_node_4.node);

	zassert_true((verify_tail_head(&test_list, &test_node_2.node,
				       &test_node_3.node, false)),
		     "test_list head/tail are wrong");

	zassert_true((sys_slist_peek_next_no_check(&test_node_2.node) ==
		      &test_node_4.node),
		     "test_list node links are wrong");

    /* Find the node 4 and get the previous node*/
	sys_snode_t *test_node_4_prev = NULL;

	zassert_true(sys_slist_find(&test_list, &test_node_4.node, &test_node_4_prev),
		     "test_list did not find node");
	zassert_equal(&test_node_2.node, test_node_4_prev,
		     "test_list previous node wrong ");

	/* Finding and removing node 1 */
	sys_slist_find_and_remove(&test_list, &test_node_1.node);
	zassert_true((verify_content_amount(&test_list, 3)),
		     "test_list has wrong content");

	zassert_true((verify_tail_head(&test_list, &test_node_2.node,
				       &test_node_3.node, false)),
		     "test_list head/tail are wrong");

	/* Removing node 3 */
	sys_slist_remove(&test_list, &test_node_4.node, &test_node_3.node);
	zassert_true((verify_content_amount(&test_list, 2)),
		     "test_list has wrong content");

	zassert_true((verify_tail_head(&test_list, &test_node_2.node,
				       &test_node_4.node, false)),
		     "test_list head/tail are wrong");

	/* Removing node 4 */
	sys_slist_remove(&test_list, &test_node_2.node, &test_node_4.node);
	zassert_true((verify_content_amount(&test_list, 1)),
		     "test_list has wrong content");

	zassert_true((verify_tail_head(&test_list, &test_node_2.node,
				       &test_node_2.node, true)),
		     "test_list head/tail are wrong");

	/* Removing node 2 */
	sys_slist_remove(&test_list, NULL, &test_node_2.node);
	zassert_true((verify_emptyness(&test_list)),
		     "test_list should be empty");

	/* test iterator from a node */
	struct data_node {
		sys_snode_t node;
		int data;
	} data_node[6] = {
		{ .data = 0 },
		{ .data = 1 },
		{ .data = 2 },
		{ .data = 3 },
		{ .data = 4 },
		{ .data = 5 },
	};
	sys_snode_t *node = NULL;
	int ii;

	sys_slist_init(&test_list);

	for (ii = 0; ii < 6; ii++) {
		sys_slist_append(&test_list, &data_node[ii].node);
	}

	ii = 0;
	SYS_SLIST_ITERATE_FROM_NODE(&test_list, node) {
		ii++;
		if (((struct data_node *)node)->data == 2) {
			break;
		}
	}
	zassert_equal(ii, 3, "");

	ii = 0;
	SYS_SLIST_ITERATE_FROM_NODE(&test_list, node) {
		ii++;
		if (((struct data_node *)node)->data == 3) {
			break;
		}
	}
	zassert_equal(ii, 1, "");

	ii = 0;
	SYS_SLIST_ITERATE_FROM_NODE(&test_list, node) {
		ii++;
	}
	zassert_equal(ii, 2, "");

	/* test sys_slist_get_not_empty() and sys_slist_get() APIs */
	for (ii = 0; ii < 6; ii++) {
		node = sys_slist_get_not_empty(&test_list);
		zassert_equal(((struct data_node *)node)->data, ii, "");
	}
	for (ii = 0; ii < 6; ii++) {
		/* regenerate test_list since we just emptied it */
		sys_slist_append(&test_list, &data_node[ii].node);
	}
	for (ii = 0; ii < 6; ii++) {
		node = sys_slist_get(&test_list);
		zassert_equal(((struct data_node *)node)->data, ii, "");
	}
	node = sys_slist_get(&test_list);
	zassert_equal(node, NULL, "");

	/* test sys_slist_append_list() */
	sys_slist_init(&append_list);
	struct data_node data_node_append[6] = {
		{ .data = 6 },
		{ .data = 7 },
		{ .data = 8 },
		{ .data = 9 },
		{ .data = 10 },
		{ .data = 11 },
	};
	for (ii = 0; ii < 6; ii++) {
		/* regenerate test_list, which we just emptied */
		sys_slist_append(&test_list, &data_node[ii].node);
		/* Build append_list so that the node pointers are correct */
		sys_slist_append(&append_list, &data_node_append[ii].node);
	}
	sys_slist_append_list(&test_list, &data_node_append[0].node,
			      &data_node_append[5].node);
	for (ii = 0; ii < 12; ii++) {
		node = sys_slist_get(&test_list);
		zassert_equal(((struct data_node *)node)->data, ii,
			      "expected %d got %d", ii,
			      ((struct data_node *)node)->data);
	}

	/* test sys_slist_append_list with emtpy list */
	sys_slist_init(&test_list);
	sys_slist_init(&append_list);
	for (ii = 0; ii < 6; ii++) {
		/* regenerate test_list only */
		sys_slist_append(&test_list, &data_node[ii].node);
	}
	sys_slist_append_list(&test_list, append_list.head, append_list.tail);
	node = sys_slist_peek_tail(&test_list);
	zassert_equal(((struct data_node *)node)->data, data_node[5].data, "expected %d got %d",
		      data_node[5].data, ((struct data_node *)node)->data);

	/* test sys_slist_merge_slist */
	sys_slist_init(&test_list);
	sys_slist_init(&append_list);
	for (ii = 0; ii < 6; ii++) {
		/* regenerate both lists */
		sys_slist_append(&test_list, &data_node[ii].node);
		sys_slist_append(&append_list, &data_node_append[ii].node);
	}
	sys_slist_merge_slist(&test_list, &append_list);
	for (ii = 0; ii < 12; ii++) {
		node = sys_slist_get(&test_list);
		zassert_equal(((struct data_node *)node)->data, ii,
			      "expected %d got %d", ii,
			      ((struct data_node *)node)->data);
	}
	zassert_true(sys_slist_is_empty(&append_list),
		     "merged list is not empty");

	/* test sys_slist_merge_slist with emtpy list */
	sys_slist_init(&test_list);
	sys_slist_init(&append_list);
	for (ii = 0; ii < 6; ii++) {
		/* regenerate test_list only */
		sys_slist_append(&test_list, &data_node[ii].node);
	}

	sys_slist_merge_slist(&test_list, &append_list);
	node = sys_slist_peek_tail(&test_list);
	zassert_equal(((struct data_node *)node)->data, data_node[5].data, "expected %d got %d",
		      data_node[5].data, ((struct data_node *)node)->data);
}

/**
 * @}
 */
