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

#include <ztest.h>
#include <sys/dlist.h>

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

void test_dlist_container(void)
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

void test_dlist_for_each(void)
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

/* ztest main entry */
void test_main(void)
{
	ztest_test_suite(test_dlist,
			ztest_unit_test(test_dlist_container),
			ztest_unit_test(test_dlist_for_each)
			);
	ztest_run_test_suite(test_dlist);
}
