/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <net/buf.h>

struct net_buf_pool _net_buf_pool_list[1];

unsigned int irq_lock(void)
{
	return 0;
}

void irq_unlock(unsigned int key)
{
}

#include <net/buf.c>

void k_queue_init(struct k_queue *fifo) {}
void k_queue_append_list(struct k_queue *fifo, void *head, void *tail) {}

int k_is_in_isr(void)
{
	return 0;
}

void *k_queue_get(struct k_queue *fifo, s32_t timeout)
{
	return NULL;
}

void k_queue_append(struct k_queue *fifo, void *data)
{
}

void k_queue_prepend(struct k_queue *fifo, void *data)
{
}

#define TEST_BUF_COUNT 1
#define TEST_BUF_SIZE 74

NET_BUF_POOL_DEFINE(bufs_pool, TEST_BUF_COUNT, TEST_BUF_SIZE,
		    sizeof(int), NULL);

static void test_get_single_buffer(void)
{
	struct net_buf *buf;

	buf = net_buf_alloc(&bufs_pool, K_NO_WAIT);

	zassert_equal(buf->ref, 1, "Invalid refcount");
	zassert_equal(buf->len, 0, "Invalid length");
	zassert_equal(buf->flags, 0, "Invalid flags");
	zassert_equal_ptr(buf->frags, NULL, "Frags not NULL");
}

void test_main(void)
{
	ztest_test_suite(net_buf_test,
		ztest_unit_test(test_get_single_buffer)
	);

	ztest_run_test_suite(net_buf_test);
}
