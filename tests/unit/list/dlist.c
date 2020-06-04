/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <sys/dlist.h>
#include "time.h"

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

static enum dlist_perf_stats {PEEK_HEAD_TAIL, INSERT_REMOVE} operation;
clock_t check_dlist_perf(sys_dlist_t *test_dlist, size_t size)
{
	struct container_node node_ii[30] = {0};
	struct container_node test_node;

	sys_dlist_init(test_dlist);
	zassert_true(sys_dlist_is_empty(test_dlist), NULL);

	/*avoid that array index is overflow*/
	size = size < 30 ? size : 30;

	for (int i = 0; i < size; i++) {
		sys_dlist_append(test_dlist, &node_ii[i].node);
	}

	clock_t start = 0, finish = 0;

	start = clock();
	for (int i = 0; i < 10000; i++) {
		switch (operation) {
		case PEEK_HEAD_TAIL:
			sys_dlist_peek_head(test_dlist);
			sys_dlist_peek_tail(test_dlist);
			break;
		case INSERT_REMOVE:
			sys_dlist_insert(&node_ii[size/2].node,
					&test_node.node);
			sys_dlist_remove(&test_node.node);
			break;
		default:
			/*Just running without no operations*/
			break;
		}
	}
	finish = clock();

	return finish - start;
}

void check_some_operations(enum dlist_perf_stats op)
{
	clock_t const_time[3] = {0};

	operation = op;
	/*test list size is 10 nodes*/
	const_time[0] = check_dlist_perf(&test_list, 10);
	/*test list size is 20 nodes*/
	const_time[1] = check_dlist_perf(&test_list, 20);
	/*test list size is 30 nodes*/
	const_time[2] = check_dlist_perf(&test_list, 30);

	zassert_within(const_time[0], const_time[1], 20, NULL);
	zassert_within(const_time[1], const_time[2], 20, NULL);
}
/**
 * @addtogroup unit_tests
 * @{
 */

/**
 * @brief test dlist some operations running in constant time.
 *
 * @details
 * Define a double list, and record the time of running some
 * operations by API clock() in usr/libc of native posix platform
 * Verify some operations running in constant time.
 *
 * @ingroup lib_list_tests
 *
 * @see sys_dlist_peek_head(), sys_dlist_peek_tail(),
 * sys_dlist_insert(), sys_dlist_remove().
 */
void test_check_dlist_perf(void)
{
	/**TESTPOINT: test peek head and tail in constant time*/
	check_some_operations(PEEK_HEAD_TAIL);
	/**TESTPOINT: test insert and remove in constant time*/
	check_some_operations(INSERT_REMOVE);
}


/**
 * @brief Verify doubly linked list functionalities
 *
 * @see sys_dlist_append(), sys_dlist_remove(), sys_dlist_prepend(),
 * sys_dlist_remove(), sys_dlist_insert(), sys_dlist_peek_next()
 * SYS_DLIST_ITERATE_FROM_NODE()
 */
void test_dlist(void)
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

	/* test iterator of "for each" style from a node */
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
