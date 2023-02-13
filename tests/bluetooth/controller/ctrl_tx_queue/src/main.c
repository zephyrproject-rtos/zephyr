/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/types.h>
#include <zephyr/ztest.h>

#include <stdio.h>
#include <stdlib.h>

#include "util/util.h"
#include "util/memq.h"
#include "util/dbuf.h"

#include "pdu_df.h"
#include "lll/pdu_vendor.h"
#include "pdu.h"
#include "lll.h"

#include "lll_df_types.h"
/* mock ccm which is used in lll_conn.h */
struct ccm {
};
#include "lll_conn.h"

#include "ull_tx_queue.h"

#define SIZE 10U

void test_init(void)
{
	struct ull_tx_q tx_q;
	struct node_tx *node;

	ull_tx_q_init(&tx_q);
	zassert_equal(0U, tx_q.pause_data, "pause_data must be zero on init");

	/* Tx Queue shall be empty */
	node = ull_tx_q_dequeue(&tx_q);
	zassert_equal_ptr(node, NULL, "");
}

/*
 * (1) Enqueue ctrl nodes.
 * Dequeue and verify order of the ctrl nodes from (1).
 * Verify Tx Queue is empty.
 */
void test_ctrl(void)
{
	struct ull_tx_q tx_q;
	struct node_tx *node;
	struct node_tx ctrl_nodes1[SIZE] = { 0 };

	ull_tx_q_init(&tx_q);

	/* Enqueue ctrl nodes */
	for (int i = 0U; i < SIZE; i++) {
		ull_tx_q_enqueue_ctrl(&tx_q, &ctrl_nodes1[i]);
	}

	/* Dequeue ctrl nodes */
	for (int i = 0U; i < SIZE; i++) {
		node = ull_tx_q_dequeue(&tx_q);
		zassert_equal_ptr(node, &ctrl_nodes1[i], NULL);
	}

	/* Tx Queue shall be empty */
	node = ull_tx_q_dequeue(&tx_q);
	zassert_equal_ptr(node, NULL, "");
}

/*
 * (1) Enqueue data nodes.
 * Dequeue and verify order of the data nodes from (1).
 * Verify Tx Queue is empty.
 */
void test_data(void)
{
	struct ull_tx_q tx_q;
	struct node_tx *node;
	struct node_tx nodes[SIZE] = { 0 };

	ull_tx_q_init(&tx_q);

	/* Enqueue data nodes */
	for (int i = 0U; i < SIZE; i++) {
		ull_tx_q_enqueue_data(&tx_q, &nodes[i]);
	}

	/* Dequeue data nodes */
	for (int i = 0U; i < SIZE; i++) {
		node = ull_tx_q_dequeue(&tx_q);
		zassert_equal_ptr(node, &nodes[i], NULL);
	}

	/* Tx Queue shall be empty */
	node = ull_tx_q_dequeue(&tx_q);
	zassert_equal_ptr(node, NULL, "");
}

/*
 * (1) Enqueue ctrl and data nodes interleaved.
 * Dequeue and verify order of the data and ctrl nodes from (1).
 * Verify Tx Queue is empty.
 */
void test_ctrl_and_data_1(void)
{
	struct ull_tx_q tx_q;
	struct node_tx *node;
	struct node_tx ctrl_nodes1[SIZE] = { 0 };
	struct node_tx data_nodes1[SIZE] = { 0 };

	ull_tx_q_init(&tx_q);

	/* Enqueue ctrl and data nodes */
	for (int i = 0U; i < SIZE; i++) {
		ull_tx_q_enqueue_ctrl(&tx_q, &ctrl_nodes1[i]);
		ull_tx_q_enqueue_data(&tx_q, &data_nodes1[i]);
	}

	/* Dequeue ctrl and data nodes */
	for (int i = 0U; i < SIZE; i++) {
		node = ull_tx_q_dequeue(&tx_q);
		zassert_equal_ptr(node, &ctrl_nodes1[i], NULL);

		node = ull_tx_q_dequeue(&tx_q);
		zassert_equal_ptr(node, &data_nodes1[i], NULL);
	}

	/* Tx Queue shall be empty */
	node = ull_tx_q_dequeue(&tx_q);
	zassert_equal_ptr(node, NULL, "");
}

/*
 * (1) Enqueue ctrl and data nodes interleaved.
 * Pause Tx Queue.
 * (2) Enqueue data nodes.
 * Dequeue and verify order of the data and ctrl nodes from (1).
 * Verify Tx Queue is empty.
 */
void test_ctrl_and_data_2(void)
{
	struct ull_tx_q tx_q;
	struct node_tx *node;
	struct node_tx ctrl_nodes1[SIZE] = { 0 };
	struct node_tx data_nodes1[SIZE] = { 0 };
	struct node_tx data_nodes2[SIZE] = { 0 };

	ull_tx_q_init(&tx_q);

	/* Enqueue ctrl and data nodes */
	for (int i = 0U; i < SIZE; i++) {
		ull_tx_q_enqueue_ctrl(&tx_q, &ctrl_nodes1[i]);
		ull_tx_q_enqueue_data(&tx_q, &data_nodes1[i]);
	}

	/* Pause Tx Queue */
	ull_tx_q_pause_data(&tx_q);

	/* Enqueue data nodes */
	for (int i = 0U; i < SIZE; i++) {
		ull_tx_q_enqueue_data(&tx_q, &data_nodes2[i]);
	}

	/* Dequeue ctrl and data nodes */
	for (int i = 0U; i < SIZE; i++) {
		node = ull_tx_q_dequeue(&tx_q);
		zassert_equal_ptr(node, &ctrl_nodes1[i], NULL);

		node = ull_tx_q_dequeue(&tx_q);
		zassert_equal_ptr(node, &data_nodes1[i], NULL);
	}

	/* Tx Queue shall be empty */
	node = ull_tx_q_dequeue(&tx_q);
	zassert_equal_ptr(node, NULL, "");
}

/*
 * (1) Enqueue ctrl and data nodes interleaved.
 * Pause Tx Queue.
 * (2) Enqueue ctrl and data nodes interleaved.
 * Dequeue and verify order of ctrl and data nodes from (1).
 * Dequeue and verify order of ctrl nodes from (2).
 * Verify Tx Queue is empty.
 */
void test_ctrl_and_data_3(void)
{
	struct ull_tx_q tx_q;
	struct node_tx *node;
	struct node_tx ctrl_nodes1[SIZE] = { 0 };
	struct node_tx ctrl_nodes2[SIZE] = { 0 };
	struct node_tx data_nodes1[SIZE] = { 0 };
	struct node_tx data_nodes2[SIZE] = { 0 };

	ull_tx_q_init(&tx_q);

	/* Enqueue ctrl and data nodes */
	for (int i = 0U; i < SIZE; i++) {
		ull_tx_q_enqueue_ctrl(&tx_q, &ctrl_nodes1[i]);
		ull_tx_q_enqueue_data(&tx_q, &data_nodes1[i]);
	}

	/* Pause Tx Queue */
	ull_tx_q_pause_data(&tx_q);

	/* Enqueue ctrl and data nodes */
	for (int i = 0U; i < SIZE; i++) {
		ull_tx_q_enqueue_ctrl(&tx_q, &ctrl_nodes2[i]);
		ull_tx_q_enqueue_data(&tx_q, &data_nodes2[i]);
	}

	/* Dequeue ctrl and data nodes */
	for (int i = 0U; i < SIZE; i++) {
		node = ull_tx_q_dequeue(&tx_q);
		zassert_equal_ptr(node, &ctrl_nodes1[i], NULL);

		node = ull_tx_q_dequeue(&tx_q);
		zassert_equal_ptr(node, &data_nodes1[i], NULL);
	}

	/* Dequeue ctrl nodes */
	for (int i = 0U; i < SIZE; i++) {
		node = ull_tx_q_dequeue(&tx_q);
		zassert_equal_ptr(node, &ctrl_nodes2[i], NULL);
	}

	/* Tx Queue shall be empty */
	node = ull_tx_q_dequeue(&tx_q);
	zassert_equal_ptr(node, NULL, "");
}

/*
 * (1) Enqueue ctrl and data nodes interleaved.
 * Pause Tx Queue.
 * (2) Enqueue ctrl and data nodes interleaved.
 * Resume Tx Queue.
 * Dequeue and verify order of ctrl and data nodes from (1).
 * Dequeue and verify order of ctrl nodes from (2).
 * Dequeue and verify order of data nodes from (2).
 * Verify Tx Queue is empty.
 */
void test_ctrl_and_data_4(void)
{
	struct ull_tx_q tx_q;
	struct node_tx *node;
	struct node_tx ctrl_nodes1[SIZE] = { 0 };
	struct node_tx ctrl_nodes2[SIZE] = { 0 };
	struct node_tx data_nodes1[SIZE] = { 0 };
	struct node_tx data_nodes2[SIZE] = { 0 };

	ull_tx_q_init(&tx_q);

	/* Enqueue ctrl and data nodes */
	for (int i = 0U; i < SIZE; i++) {
		ull_tx_q_enqueue_ctrl(&tx_q, &ctrl_nodes1[i]);
		ull_tx_q_enqueue_data(&tx_q, &data_nodes1[i]);
	}

	/* Pause Tx Queue */
	ull_tx_q_pause_data(&tx_q);

	/* Enqueue ctrl and data nodes */
	for (int i = 0U; i < SIZE; i++) {
		ull_tx_q_enqueue_ctrl(&tx_q, &ctrl_nodes2[i]);
		ull_tx_q_enqueue_data(&tx_q, &data_nodes2[i]);
	}

	/* Resume Tx Queue */
	ull_tx_q_resume_data(&tx_q);

	/* Dequeue ctrl and data nodes */
	for (int i = 0U; i < SIZE; i++) {
		node = ull_tx_q_dequeue(&tx_q);
		zassert_equal_ptr(node, &ctrl_nodes1[i], NULL);

		node = ull_tx_q_dequeue(&tx_q);
		zassert_equal_ptr(node, &data_nodes1[i], NULL);
	}

	/* Dequeue ctrl nodes */
	for (int i = 0U; i < SIZE; i++) {
		node = ull_tx_q_dequeue(&tx_q);
		zassert_equal_ptr(node, &ctrl_nodes2[i], NULL);
	}

	/* Dequeue data nodes */
	for (int i = 0U; i < SIZE; i++) {
		node = ull_tx_q_dequeue(&tx_q);
		zassert_equal_ptr(node, &data_nodes2[i], NULL);
	}

	/* Tx Queue shall be empty */
	node = ull_tx_q_dequeue(&tx_q);
	zassert_equal_ptr(node, NULL, "");
}

/*
 * (1) Enqueue ctrl and data nodes interleaved.
 * Pause Tx Queue.
 * (2) Enqueue ctrl and data nodes interleaved.
 * Resume Tx Queue.
 * (3) Enqueue ctrl and data nodes interleaved.
 * Dequeue and verify order of ctrl and data nodes from (1).
 * Dequeue and verify order of ctrl nodes from (2).
 * Dequeue and verify order of data nodes from (2).
 * Dequeue and verify order of ctrl and data nodes from (3).
 * Verify Tx Queue is empty.
 */
void test_ctrl_and_data_5(void)
{
	struct ull_tx_q tx_q;
	struct node_tx *node;
	struct node_tx ctrl_nodes1[SIZE] = { 0 };
	struct node_tx ctrl_nodes2[SIZE] = { 0 };
	struct node_tx ctrl_nodes3[SIZE] = { 0 };
	struct node_tx data_nodes1[SIZE] = { 0 };
	struct node_tx data_nodes2[SIZE] = { 0 };
	struct node_tx data_nodes3[SIZE] = { 0 };

	ull_tx_q_init(&tx_q);

	/* Enqueue ctrl and data nodes */
	for (int i = 0U; i < SIZE; i++) {
		ull_tx_q_enqueue_ctrl(&tx_q, &ctrl_nodes1[i]);
		ull_tx_q_enqueue_data(&tx_q, &data_nodes1[i]);
	}

	/* Pause Tx Queue */
	ull_tx_q_pause_data(&tx_q);

	/* Enqueue ctrl and data nodes */
	for (int i = 0U; i < SIZE; i++) {
		ull_tx_q_enqueue_ctrl(&tx_q, &ctrl_nodes2[i]);
		ull_tx_q_enqueue_data(&tx_q, &data_nodes2[i]);
	}

	/* Resume Tx Queue */
	ull_tx_q_resume_data(&tx_q);

	/* Enqueue ctrl and data nodes */
	for (int i = 0U; i < SIZE; i++) {
		ull_tx_q_enqueue_ctrl(&tx_q, &ctrl_nodes3[i]);
		ull_tx_q_enqueue_data(&tx_q, &data_nodes3[i]);
	}

	/* Dequeue ctrl and data nodes */
	for (int i = 0U; i < SIZE; i++) {
		struct node_tx *node;

		node = ull_tx_q_dequeue(&tx_q);
		zassert_equal_ptr(node, &ctrl_nodes1[i], NULL);

		node = ull_tx_q_dequeue(&tx_q);
		zassert_equal_ptr(node, &data_nodes1[i], NULL);
	}

	/* Dequeue ctrl nodes */
	for (int i = 0U; i < SIZE; i++) {
		struct node_tx *node = ull_tx_q_dequeue(&tx_q);

		zassert_equal_ptr(node, &ctrl_nodes2[i], NULL);
	}

	/* Dequeue data nodes */
	for (int i = 0U; i < SIZE; i++) {
		struct node_tx *node = ull_tx_q_dequeue(&tx_q);

		zassert_equal_ptr(node, &data_nodes2[i], NULL);
	}

	/* Dequeue ctrl and data nodes */
	for (int i = 0U; i < SIZE; i++) {
		node = ull_tx_q_dequeue(&tx_q);
		zassert_equal_ptr(node, &ctrl_nodes3[i], NULL);

		node = ull_tx_q_dequeue(&tx_q);
		zassert_equal_ptr(node, &data_nodes3[i], NULL);
	}

	/* Tx Queue shall be empty */
	node = ull_tx_q_dequeue(&tx_q);
	zassert_equal_ptr(node, NULL, "");
}

/*
 * (1) Enqueue data nodes.
 * Pause Tx Queue TWICE.
 * (2) Enqueue data nodes.
 * Dequeue and verify order of data nodes from (1).
 * Verify Tx Queue is empty.
 * Resume Tx Queue.
 * Verify Tx Queue is empty.
 * Resume Tx Queue.
 * Dequeue and verify order of data nodes from (2).
 */
void test_multiple_pause_resume(void)
{
	struct ull_tx_q tx_q;
	struct node_tx *node;
	struct node_tx data_nodes1[SIZE] = { 0 };
	struct node_tx data_nodes2[SIZE] = { 0 };

	ull_tx_q_init(&tx_q);

	/* Enqueue data 1 nodes */
	for (int i = 0U; i < SIZE; i++) {
		ull_tx_q_enqueue_data(&tx_q, &data_nodes1[i]);
	}

	/* Pause Tx Queue Twice */
	ull_tx_q_pause_data(&tx_q);
	ull_tx_q_pause_data(&tx_q);

	/* Enqueue data 2 nodes */
	for (int i = 0U; i < SIZE; i++) {
		ull_tx_q_enqueue_data(&tx_q, &data_nodes2[i]);
	}

	/* Dequeue data 1 nodes */
	for (int i = 0U; i < SIZE; i++) {
		node = ull_tx_q_dequeue(&tx_q);
		zassert_equal_ptr(node, &data_nodes1[i], NULL);
	}

	/* Tx Queue shall be empty */
	node = ull_tx_q_dequeue(&tx_q);
	zassert_equal_ptr(node, NULL, "");

	/* Resume Tx Queue */
	ull_tx_q_resume_data(&tx_q);

	/* Tx Queue shall be empty */
	node = ull_tx_q_dequeue(&tx_q);
	zassert_equal_ptr(node, NULL, "");

	/* Resume Tx Queue */
	ull_tx_q_resume_data(&tx_q);

	/* Dequeue data 2 nodes */
	for (int i = 0U; i < SIZE; i++) {
		node = ull_tx_q_dequeue(&tx_q);
		zassert_equal_ptr(node, &data_nodes2[i], NULL);
	}

	/* Tx Queue shall be empty */
	node = ull_tx_q_dequeue(&tx_q);
	zassert_equal_ptr(node, NULL, "");
}

void test_main(void)
{
	ztest_test_suite(test, ztest_unit_test(test_init), ztest_unit_test(test_ctrl),
			 ztest_unit_test(test_data), ztest_unit_test(test_ctrl_and_data_1),
			 ztest_unit_test(test_ctrl_and_data_2),
			 ztest_unit_test(test_ctrl_and_data_3),
			 ztest_unit_test(test_ctrl_and_data_4),
			 ztest_unit_test(test_ctrl_and_data_5),
			 ztest_unit_test(test_multiple_pause_resume));
	ztest_run_test_suite(test);
}
