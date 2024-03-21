/*
 * Copyright (c) 2023 Enphase Energy
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "eth_ivshmem_priv.h"

#define SHMEM_SECTION_SIZE KB(4)

#define VRING_DESC_LEN 32
#define VRING_HEADER_SIZE 1792
#define VRING_DATA_MAX_LEN 2304

static struct eth_ivshmem_queue q1, q2;
static uint8_t shmem_buff[2][SHMEM_SECTION_SIZE] __aligned(KB(4));
static const void *rx_message;
static size_t rx_len;

static void test_init_queues(void)
{
	int res;

	res = eth_ivshmem_queue_init(
		&q1, (uintptr_t)shmem_buff[0],
		(uintptr_t)shmem_buff[1], SHMEM_SECTION_SIZE);
	zassert_ok(res);
	res = eth_ivshmem_queue_init(
		&q2, (uintptr_t)shmem_buff[1],
		(uintptr_t)shmem_buff[0], SHMEM_SECTION_SIZE);
	zassert_ok(res);
}

static inline int queue_tx(struct eth_ivshmem_queue *q, const void *data, size_t len)
{
	void *dest;
	int res = eth_ivshmem_queue_tx_get_buff(q, &dest, len);

	if (res == 0) {
		memcpy(dest, data, len);
		res = eth_ivshmem_queue_tx_commit_buff(q);
	}
	return res;
}

static void test_setup(void *fixture)
{
	ARG_UNUSED(fixture);
	rx_message = NULL;
	rx_len = 0;
	test_init_queues();
}


ZTEST(eth_ivshmem_queue_tests, test_init)
{
	zassert_equal(q1.desc_max_len, VRING_DESC_LEN);
	zassert_equal(q1.vring_header_size, VRING_HEADER_SIZE);
	zassert_equal(q1.vring_data_max_len, VRING_DATA_MAX_LEN);
	zassert_equal_ptr(q1.tx.shmem, shmem_buff[0]);
	zassert_equal_ptr(q1.rx.shmem, shmem_buff[1]);
	zassert_equal_ptr(q2.tx.shmem, shmem_buff[1]);
	zassert_equal_ptr(q2.rx.shmem, shmem_buff[0]);
}

ZTEST(eth_ivshmem_queue_tests, test_simple_send_receive)
{
	/* Send */
	int x = 42;

	zassert_ok(queue_tx(&q1, &x, sizeof(x)));

	/* Receive */
	zassert_ok(eth_ivshmem_queue_rx(&q2, &rx_message, &rx_len));
	zassert_equal(rx_len, sizeof(x));
	zassert_equal(*(int *)rx_message, x);
	zassert_ok(eth_ivshmem_queue_rx_complete(&q2));
}

ZTEST(eth_ivshmem_queue_tests, test_send_receive_both_directions)
{
	/* Send q1 */
	int q1_tx_data = 42;

	zassert_ok(queue_tx(&q1, &q1_tx_data, sizeof(q1_tx_data)));

	/* Send q2 */
	int q2_tx_data = 21;

	zassert_ok(queue_tx(&q2, &q2_tx_data, sizeof(q2_tx_data)));

	/* Receive q2 */
	zassert_ok(eth_ivshmem_queue_rx(&q2, &rx_message, &rx_len));
	zassert_equal(rx_len, sizeof(q1_tx_data));
	zassert_equal(*(int *)rx_message, q1_tx_data);
	zassert_ok(eth_ivshmem_queue_rx_complete(&q2));

	/* Receive q1 */
	zassert_ok(eth_ivshmem_queue_rx(&q1, &rx_message, &rx_len));
	zassert_equal(rx_len, sizeof(q2_tx_data));
	zassert_equal(*(int *)rx_message, q2_tx_data);
	zassert_ok(eth_ivshmem_queue_rx_complete(&q1));
}

ZTEST(eth_ivshmem_queue_tests, test_queue_empty)
{
	/* Read with empty queue */
	zassert_equal(eth_ivshmem_queue_rx(&q1, &rx_message, &rx_len), -EWOULDBLOCK);

	/* Complete with empty queue */
	zassert_equal(eth_ivshmem_queue_rx_complete(&q1), -EWOULDBLOCK);

	/* TX commit without getting buffer */
	zassert_equal(eth_ivshmem_queue_tx_commit_buff(&q1), -EINVAL);

	/* Getting a buffer (without committing) should not modify/overflow the queue */
	for (int i = 0; i < 100; i++) {
		void *data;

		zassert_ok(eth_ivshmem_queue_tx_get_buff(&q1, &data, KB(1)));
	}
}

ZTEST(eth_ivshmem_queue_tests, test_queue_descriptors_full)
{
	/* Fill queue descriptors */
	for (int i = 0; i < VRING_DESC_LEN; i++) {
		zassert_ok(queue_tx(&q1, &i, sizeof(i)));
	}

	/* Fail to add another */
	int x = 0;

	zassert_equal(queue_tx(&q1, &x, sizeof(x)), -ENOBUFS);

	/* Read 3 */
	for (int i = 0; i < 3; i++) {
		zassert_ok(eth_ivshmem_queue_rx(&q2, &rx_message, &rx_len));
		zassert_equal(rx_len, sizeof(i));
		zassert_equal(*(int *)rx_message, i);
		zassert_ok(eth_ivshmem_queue_rx_complete(&q2));
	}

	/* Can now add 3 more */
	for (int i = 0; i < 3; i++) {
		zassert_ok(queue_tx(&q1, &i, sizeof(i)));
	}

	/* Fail to add another */
	zassert_equal(queue_tx(&q1, &x, sizeof(x)), -ENOBUFS);
}

ZTEST(eth_ivshmem_queue_tests, test_queue_shmem_full)
{
	static uint8_t large_message[KB(1)];

	/* Fill queue shmem */
	for (int i = 0; i < VRING_DATA_MAX_LEN / sizeof(large_message); i++) {
		zassert_ok(queue_tx(&q1, large_message, sizeof(large_message)));
	}

	/* Fail to add another */
	zassert_equal(queue_tx(&q1, large_message, sizeof(large_message)), -ENOBUFS);

	/* Read 1 */
	zassert_ok(eth_ivshmem_queue_rx(&q2, &rx_message, &rx_len));
	zassert_equal(rx_len, sizeof(large_message));
	zassert_ok(eth_ivshmem_queue_rx_complete(&q2));

	/* Can now add 1 more */
	zassert_ok(queue_tx(&q1, large_message, sizeof(large_message)));

	/* Fail to add another */
	zassert_equal(queue_tx(&q1, large_message, sizeof(large_message)), -ENOBUFS);
}

ZTEST_SUITE(eth_ivshmem_queue_tests, NULL, NULL, test_setup, NULL, NULL);
