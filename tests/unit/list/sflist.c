/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/sflist.h>

static sys_sflist_t test_list;
static sys_sflist_t append_list;

struct container_node {
	sys_sfnode_t node;
	int unused;
};

static struct container_node test_node_1;
static struct container_node test_node_2;
static struct container_node test_node_3;
static struct container_node test_node_4;

static inline bool verify_emptyness(sys_sflist_t *list)
{
	sys_sfnode_t *node;
	sys_sfnode_t *s_node;
	struct container_node *cnode;
	struct container_node *s_cnode;
	int count;

	if (!sys_sflist_is_empty(list)) {
		return false;
	}

	if (sys_sflist_peek_head(list)) {
		return false;
	}

	if (sys_sflist_peek_tail(list)) {
		return false;
	}

	if (sys_sflist_len(list) != 0) {
		return false;
	}

	count = 0;
	SYS_SFLIST_FOR_EACH_NODE(list, node) {
		count++;
	}

	if (count) {
		return false;
	}

	SYS_SFLIST_FOR_EACH_NODE_SAFE(list, node, s_node) {
		count++;
	}

	if (count) {
		return false;
	}

	count = 0;
	SYS_SFLIST_FOR_EACH_CONTAINER(list, cnode, node) {
		count++;
	}

	if (count) {
		return false;
	}

	count = 0;
	SYS_SFLIST_FOR_EACH_CONTAINER_SAFE(list, cnode, s_cnode, node) {
		count++;
	}

	if (count) {
		return false;
	}

	return true;
}

static inline bool verify_content_amount(sys_sflist_t *list, int amount)
{
	sys_sfnode_t *node;
	sys_sfnode_t *s_node;
	struct container_node *cnode;
	struct container_node *s_cnode;
	int count;

	if (sys_sflist_is_empty(list)) {
		return false;
	}

	if (!sys_sflist_peek_head(list)) {
		return false;
	}

	if (!sys_sflist_peek_tail(list)) {
		return false;
	}

	if (sys_sflist_len(list) != amount) {
		return false;
	}

	count = 0;
	SYS_SFLIST_FOR_EACH_NODE(list, node) {
		count++;
	}

	if (count != amount) {
		return false;
	}

	count = 0;
	SYS_SFLIST_FOR_EACH_NODE_SAFE(list, node, s_node) {
		count++;
	}

	if (count != amount) {
		return false;
	}

	count = 0;
	SYS_SFLIST_FOR_EACH_CONTAINER(list, cnode, node) {
		count++;
	}

	if (count != amount) {
		return false;
	}

	count = 0;
	SYS_SFLIST_FOR_EACH_CONTAINER_SAFE(list, cnode, s_cnode, node) {
		count++;
	}

	if (count != amount) {
		return false;
	}

	return true;
}

static inline bool verify_tail_head(sys_sflist_t *list,
				    sys_sfnode_t *head,
				    sys_sfnode_t *tail,
				    bool same)
{
	if (sys_sflist_peek_head(list) != head) {
		return false;
	}

	if (sys_sflist_peek_tail(list) != tail) {
		return false;
	}

	if (same) {
		if (sys_sflist_peek_head(list) != sys_sflist_peek_tail(list)) {
			return false;
		}
	} else {
		if (sys_sflist_peek_head(list) == sys_sflist_peek_tail(list)) {
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
 * @see sys_sflist_init(), sys_sflist_append(),
 * sys_sflist_find_and_remove(), sys_sflist_prepend(),
 * sys_sflist_remove(), sys_sflist_get(), sys_sflist_get_not_empty(),
 * sys_sflist_append_list(), sys_sflist_merge_list()
 */
ZTEST(dlist_api, test_sflist)
{
	sys_sflist_init(&test_list);

	zassert_true((verify_emptyness(&test_list)),
		      "test_list should be empty");

	/* Appending node 1 */
	sys_sflist_append(&test_list, &test_node_1.node);
	zassert_true((verify_content_amount(&test_list, 1)),
		     "test_list has wrong content");

	zassert_true((verify_tail_head(&test_list, &test_node_1.node,
				       &test_node_1.node, true)),
		     "test_list head/tail are wrong");

	/* Finding and removing node 1 */
	sys_sflist_find_and_remove(&test_list, &test_node_1.node);
	zassert_true((verify_emptyness(&test_list)),
		     "test_list should be empty");

	/* Prepending node 1 */
	sys_sflist_prepend(&test_list, &test_node_1.node);
	zassert_true((verify_content_amount(&test_list, 1)),
		     "test_list has wrong content");

	zassert_true((verify_tail_head(&test_list, &test_node_1.node,
				       &test_node_1.node, true)),
		     "test_list head/tail are wrong");

	/* Removing node 1 */
	sys_sflist_remove(&test_list, NULL, &test_node_1.node);
	zassert_true((verify_emptyness(&test_list)),
		     "test_list should be empty");

	/* Appending node 1 */
	sys_sflist_append(&test_list, &test_node_1.node);
	/* Prepending node 2 */
	sys_sflist_prepend(&test_list, &test_node_2.node);

	zassert_true((verify_content_amount(&test_list, 2)),
		     "test_list has wrong content");

	zassert_true((verify_tail_head(&test_list, &test_node_2.node,
				       &test_node_1.node, false)),
		     "test_list head/tail are wrong");

	/* Appending node 3 */
	sys_sflist_append(&test_list, &test_node_3.node);

	zassert_true((verify_content_amount(&test_list, 3)),
		     "test_list has wrong content");

	zassert_true((verify_tail_head(&test_list, &test_node_2.node,
				       &test_node_3.node, false)),
		     "test_list head/tail are wrong");

	zassert_true((sys_sflist_peek_next(&test_node_2.node) ==
		      &test_node_1.node),
		     "test_list node links are wrong");

	/* Inserting node 4 after node 2, peek with nocheck variant */
	sys_sflist_insert(&test_list, &test_node_2.node, &test_node_4.node);

	zassert_true((verify_tail_head(&test_list, &test_node_2.node,
				       &test_node_3.node, false)),
		     "test_list head/tail are wrong");

	zassert_true((sys_sflist_peek_next_no_check(&test_node_2.node) ==
		      &test_node_4.node),
		     "test_list node links are wrong");

	/* Finding and removing node 1 */
	sys_sflist_find_and_remove(&test_list, &test_node_1.node);
	zassert_true((verify_content_amount(&test_list, 3)),
		     "test_list has wrong content");

	zassert_true((verify_tail_head(&test_list, &test_node_2.node,
				       &test_node_3.node, false)),
		     "test_list head/tail are wrong");

	/* Removing node 3 */
	sys_sflist_remove(&test_list, &test_node_4.node, &test_node_3.node);
	zassert_true((verify_content_amount(&test_list, 2)),
		     "test_list has wrong content");

	zassert_true((verify_tail_head(&test_list, &test_node_2.node,
				       &test_node_4.node, false)),
		     "test_list head/tail are wrong");

	/* Removing node 4 */
	sys_sflist_remove(&test_list, &test_node_2.node, &test_node_4.node);
	zassert_true((verify_content_amount(&test_list, 1)),
		     "test_list has wrong content");

	zassert_true((verify_tail_head(&test_list, &test_node_2.node,
				       &test_node_2.node, true)),
		     "test_list head/tail are wrong");

	/* Removing node 2 */
	sys_sflist_remove(&test_list, NULL, &test_node_2.node);
	zassert_true((verify_emptyness(&test_list)),
		     "test_list should be empty");

	/* test iterator from a node */
	struct data_node {
		sys_sfnode_t node;
		int data;
	} data_node[6] = {
		{ .data = 0 },
		{ .data = 1 },
		{ .data = 2 },
		{ .data = 3 },
		{ .data = 4 },
		{ .data = 5 },
	};
	sys_sfnode_t *node = NULL;
	int ii;

	sys_sflist_init(&test_list);

	for (ii = 0; ii < 6; ii++) {
		sys_sflist_append(&test_list, &data_node[ii].node);
	}

	ii = 0;
	SYS_SFLIST_ITERATE_FROM_NODE(&test_list, node) {
		ii++;
		if (((struct data_node *)node)->data == 2) {
			break;
		}
	}
	zassert_equal(ii, 3, "");

	ii = 0;
	SYS_SFLIST_ITERATE_FROM_NODE(&test_list, node) {
		ii++;
		if (((struct data_node *)node)->data == 3) {
			break;
		}
	}
	zassert_equal(ii, 1, "");

	ii = 0;
	SYS_SFLIST_ITERATE_FROM_NODE(&test_list, node) {
		ii++;
	}
	zassert_equal(ii, 2, "");

	/* test sys_sflist_get_not_empty() and sys_sflist_get() APIs */
	for (ii = 0; ii < 6; ii++) {
		node = sys_sflist_get_not_empty(&test_list);
		zassert_equal(((struct data_node *)node)->data, ii, "");
	}
	for (ii = 0; ii < 6; ii++) {
		/* regenerate test_list since we just emptied it */
		sys_sflist_append(&test_list, &data_node[ii].node);
	}
	for (ii = 0; ii < 6; ii++) {
		node = sys_sflist_get(&test_list);
		zassert_equal(((struct data_node *)node)->data, ii, "");
	}
	node = sys_sflist_get(&test_list);
	zassert_equal(node, NULL, "");

	/* test sys_sflist_append_list() */
	sys_sflist_init(&append_list);
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
		sys_sflist_append(&test_list, &data_node[ii].node);
		/* Build append_list so that the node pointers are correct */
		sys_sflist_append(&append_list, &data_node_append[ii].node);
	}
	sys_sflist_append_list(&test_list, &data_node_append[0].node,
			      &data_node_append[5].node);
	for (ii = 0; ii < 12; ii++) {
		node = sys_sflist_get(&test_list);
		zassert_equal(((struct data_node *)node)->data, ii,
			      "expected %d got %d", ii,
			      ((struct data_node *)node)->data);
	}

	/* test sys_sflist_merge_sflist */
	sys_sflist_init(&test_list);
	sys_sflist_init(&append_list);
	for (ii = 0; ii < 6; ii++) {
		/* regenerate both lists */
		sys_sflist_append(&test_list, &data_node[ii].node);
		sys_sflist_append(&append_list, &data_node_append[ii].node);
	}
	sys_sflist_merge_sflist(&test_list, &append_list);
	for (ii = 0; ii < 12; ii++) {
		node = sys_sflist_get(&test_list);
		zassert_equal(((struct data_node *)node)->data, ii,
			      "expected %d got %d", ii,
			      ((struct data_node *)node)->data);
	}
	zassert_true(sys_sflist_is_empty(&append_list),
		     "merged list is not empty");

	/* tests for sys_sfnode_flags_get(), sys_sfnode_flags_set()
	 * sys_sfnode_init()
	 */
	sys_sflist_init(&test_list);
	/* Only iterating 0..3 due to limited range of flag values */
	for (ii = 0; ii < 4; ii++) {
		sys_sfnode_init(&data_node[ii].node, ii);
		sys_sflist_append(&test_list, &data_node[ii].node);
	}
	for (ii = 0; ii < 4; ii++) {
		node = sys_sflist_get(&test_list);
		zassert_equal(sys_sfnode_flags_get(node), ii,
			      "wrong flags value");
		/* Place the nodes back on the list with the flags set
		 * in reverse order for the next test
		 */
		sys_sfnode_flags_set(node, 3 - ii);
		sys_sflist_append(&test_list, node);
	}
	for (ii = 3; ii >= 0; ii--) {
		node = sys_sflist_get(&test_list);
		zassert_equal(sys_sfnode_flags_get(node), ii,
			      "wrong flags value");
	}
}

/**
 * @}
 */
