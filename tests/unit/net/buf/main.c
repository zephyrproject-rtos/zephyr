/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

unsigned int irq_lock(void)
{
	return 0;
}

void irq_unlock(unsigned int key)
{
}

#include <net/buf.c>

void k_fifo_init(struct k_fifo *fifo) {}
void k_fifo_put_list(struct k_fifo *fifo, void *head, void *tail) {}

int k_is_in_isr(void)
{
	return 0;
}

void *k_fifo_get(struct k_fifo *fifo, int32_t timeout)
{
	return NULL;
}

void k_fifo_put(struct k_fifo *fifo, void *data)
{
}

void k_lifo_init(struct k_lifo *lifo) {}

void *k_lifo_get(struct k_lifo *lifo, int32_t timeout)
{
	return NULL;
}

void k_lifo_put(struct k_lifo *lifo, void *data)
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

	assert_equal(buf->ref, 1, "Invalid refcount");
	assert_equal(buf->len, 0, "Invalid length");
	assert_equal(buf->flags, 0, "Invalid flags");
	assert_equal_ptr(buf->frags, NULL, "Frags not NULL");
}

void test_main(void)
{
	ztest_test_suite(net_buf_test,
		ztest_unit_test(test_get_single_buffer)
	);

	ztest_run_test_suite(net_buf_test);
}
