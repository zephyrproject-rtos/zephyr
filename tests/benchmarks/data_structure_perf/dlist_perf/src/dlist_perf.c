/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief doubly-linked list tests
 *
 * @defgroup lib_dlist_tests Dlist
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/dlist.h>

#define NODE_SIZE 5

static sys_dlist_t test_list;

struct container_node {
	sys_dnode_t node;
	int value;
};

/**
 * @brief Test whether dlist node struct is embeddedable
 * in any user structure
 *
 * @details Initialize an user defined structure contains
 * dlist node.Appending nodes into the doubly-linked list
 * and enumerate the doubly-linked list.
 *
 * Verify that the value enumerated is equal to the
 * value initialized.If the verification pass,the user
 * defined structure works.
 *
 * @ingroup lib_dlist_tests
 *
 * @see sys_dlist_append()
 */

ZTEST(dlist_perf, test_dlist_container)
{
	int i, count;
	struct container_node *cnode, *s_cnode;

	/* Initialize an user-defiend structure of contains dlist node */
	struct container_node data_node[NODE_SIZE] = {
		{ .value = 0 },
		{ .value = 1 },
		{ .value = 2 },
		{ .value = 3 },
		{ .value = 4 }
	};

	sys_dlist_init(&test_list);
	zassert_true(sys_dlist_is_empty(&test_list),
			"sys_dlist_init failed");

	/* Add into a doubly-linked list */
	for (i = 0; i < NODE_SIZE; i++) {
		sys_dlist_append(&test_list, &data_node[i].node);
	}

	/* Enumerate the doubly-linked list */
	count = 0;
	SYS_DLIST_FOR_EACH_CONTAINER(&test_list, cnode, node) {
		zassert_true(cnode->value == count,
				"SYS_DLIST_FOR_EACH_CONTAINER failed");
		count++;
	}
	zassert_true(count == NODE_SIZE,
			"SYS_DLIST_FOR_EACH_CONTAINER failed "
			"expected %d get %d", NODE_SIZE, count);

	/* Enumerate the doubly-linked list */
	count = 0;
	SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&test_list, cnode, s_cnode, node) {
		zassert_true(cnode->value == count,
				"SYS_DLIST_FOR_EACH_CONTAINER_SAFE failed");
		count++;
	}
	zassert_true(count == NODE_SIZE,
			"SYS_DLIST_FOR_EACH_CONTAINER_SAFE failed");
}

/**
 * @brief Test dlist for each function
 *
 * @details Initialize a double-linked list.
 * Appending nodes into the doubly-linked list
 * and enumerate the doubly-linked list.
 *
 * Verify that the value enumerated is equal to the
 * value initialized.If the verification pass,the "for each"
 * style API work.
 *
 * @ingroup lib_dlist_tests
 *
 * @see SYS_DLIST_FOR_EACH_NODE(),SYS_DLIST_FOR_EACH_NODE_SAFE()
 * SYS_DLIST_ITERATE_FROM_NODE()
 */

ZTEST(dlist_perf, test_dlist_for_each)
{
	int i, count, val;
	sys_dnode_t *node, *s_node;

	/* Initialize a doubly-linked list */
	struct container_node data_node[NODE_SIZE] = {
		{ .value = 0 },
		{ .value = 1 },
		{ .value = 2 },
		{ .value = 3 },
		{ .value = 4 }
	};

	sys_dlist_init(&test_list);
	zassert_true(sys_dlist_is_empty(&test_list),
			"sys_dlist_init failed");

	for (i = 0; i < NODE_SIZE; i++) {
		sys_dlist_append(&test_list, &data_node[i].node);
	}

	/* Enumerate the doubly-linked list */
	count = 0;
	SYS_DLIST_FOR_EACH_NODE(&test_list, node) {
		val = CONTAINER_OF(node, struct container_node, node)->value;
		zassert_true(val == count, "SYS_DLIST_FOR_EACH_NODE failed"
				" expected %d get %d", count, val);
		count++;
	}
	zassert_true(count == NODE_SIZE,
			"SYS_DLIST_FOR_EACH_NODE failed");

	/* Enumerate the doubly-linked list */
	count = 0;
	SYS_DLIST_FOR_EACH_NODE_SAFE(&test_list, node, s_node) {
		val = CONTAINER_OF(node, struct container_node, node)->value;
		zassert_true(val == count, "SYS_DLIST_FOR_EACH_NODE_SAFE failed"
				" expected %d get %d", count, val);
		count++;
	}
	zassert_true(count == NODE_SIZE,
			"SYS_DLIST_FOR_EACH_NODE_SAFE failed");

	/* Enumerate the doubly-linked list */
	count = 0;
	SYS_DLIST_ITERATE_FROM_NODE(&test_list, node) {
		count++;
		val = ((struct container_node *)node)->value;
		if (val == 1) {
			break;
		}
	}
	zassert_true(count == 2, "SYS_DLIST_ITERATE_FROM_NODE failed");

	count = 0;
	SYS_DLIST_ITERATE_FROM_NODE(&test_list, node) {
		count++;
		val = ((struct container_node *)node)->value;
		if (val == 2) {
			break;
		}
	}
	zassert_true(count == 1, "SYS_DLIST_ITERATE_FROM_NODE failed");

	count = 0;
	SYS_DLIST_ITERATE_FROM_NODE(&test_list, node) {
		count++;
	}
	zassert_true(count == 2, "SYS_DLIST_ITERATE_FROM_NODE failed");
}

/**
 * @brief Test that access the 'head' and 'tail' in constant time
 *
 * @details Defined a double list and append several nodes.
 * defined two pointers---'head','tail'.No matter how many nodes
 * dlist have, get head and tail from the dlist directly.the time
 * complexity of accessing head and tail is O(1).
 *
 * @ingroup lib_dlist_tests
 */
ZTEST(dlist_perf, test_dlist_peak_head_tail)
{
	sys_dlist_t list;
	sys_dnode_t node[10];
	sys_dnode_t *head, *tail;

	sys_dlist_init(&list);

	for (int i = 0; i < ARRAY_SIZE(node); i++) {
		sys_dlist_append(&list, &node[i]);
	}

	/* Get 'head' node directly, the time complexity is O(1) */
	head = list.head;
	zassert_true(head == &node[0],
			"dlist can't access 'head' in constant time");

	/* Get 'tail' node directly, the time complexity is O(1) */
	tail = list.tail;
	zassert_true(tail == &node[ARRAY_SIZE(node) - 1],
			"dlist can't access 'tail' in constant time");
}

/**
 * @brief Test that insert or remove operates in constant time
 *
 * @details Defined a double list and append several nodes that
 * a node has two pointers 'pre' and 'next' in pointer area.
 * And define a node to be ready for inserting or removing,
 * 'insert' and 'remove' are the operations with fixed steps and
 * no matter the size of a dlist.
 * Verify that the operations are running in constant time by
 * proving the time complexity is O(1).
 *
 * @ingroup lib_dlist_tests
 */
ZTEST(dlist_perf, test_dlist_insert_and_remove)
{
	sys_dlist_t list;
	sys_dlist_t node[10];

	sys_dlist_init(&list);

	for (int i = 0; i < ARRAY_SIZE(node); i++) {
		sys_dlist_append(&list, &node[i]);
	}

	sys_dnode_t node_ins, *insert_at = &node[ARRAY_SIZE(node)/2];
	sys_dnode_t *insert_node = &node_ins;

	/* Insert a node and steps are fixed,the time complexity is O(1) */
	insert_node->prev = insert_at->prev;
	insert_node->next = insert_at;
	insert_at->prev->next = insert_node;
	insert_at->prev = insert_node;
	/*Check if the node is inserted successfully*/
	zassert_true(insert_node == sys_dlist_peek_prev(&list,
		&node[ARRAY_SIZE(node)/2]), "dlist can't insert a node in constant time");
	zassert_true(insert_node == sys_dlist_peek_next(&list,
		&node[ARRAY_SIZE(node)/2 - 1]), "dlist can't insert a node in constant time");

	/* Remove a node and steps are fixed,the time complexity is O(1) */
	insert_node->prev->next = insert_node->next;
	insert_node->next->prev = insert_node->prev;
	/*Check if the node is removed successfully*/
	zassert_true(insert_node != sys_dlist_peek_prev(&list,
		&node[ARRAY_SIZE(node)/2]), "dlist can't insert a node in constant time");
	zassert_true(insert_node != sys_dlist_peek_next(&list,
		&node[ARRAY_SIZE(node)/2 - 1]), "dlist can't remove a node in constant time");
}

ZTEST_SUITE(dlist_perf, NULL, NULL, NULL, NULL, NULL);
