/* main.c - Application main entry point */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <misc/printk.h>

#include <net/buf.h>

#include <ztest.h>

#define TEST_TIMEOUT SECONDS(1)

struct bt_data {
	void *hci_sync;

	union {
		u16_t hci_opcode;
		u16_t acl_handle;
	};

	u8_t type;
};

struct in6_addr {
	union {
		u8_t		u6_addr8[16];
		u16_t	u6_addr16[8]; /* In big endian */
		u32_t	u6_addr32[4]; /* In big endian */
	} in6_u;
#define s6_addr			in6_u.u6_addr8
#define s6_addr16		in6_u.u6_addr16
#define s6_addr32		in6_u.u6_addr32
};

struct ipv6_hdr {
	u8_t vtc;
	u8_t tcflow;
	u16_t flow;
	u8_t len[2];
	u8_t nexthdr;
	u8_t hop_limit;
	struct in6_addr src;
	struct in6_addr dst;
} __attribute__((__packed__));

struct udp_hdr {
	u16_t src_port;
	u16_t dst_port;
	u16_t len;
	u16_t chksum;
} __attribute__((__packed__));

static int destroy_called;
static int frag_destroy_called;

static void buf_destroy(struct net_buf *buf);
static void frag_destroy(struct net_buf *buf);
static void frag_destroy_big(struct net_buf *buf);

NET_BUF_POOL_DEFINE(bufs_pool, 22, 74, sizeof(struct bt_data), buf_destroy);
NET_BUF_POOL_DEFINE(no_data_pool, 1, 0, sizeof(struct bt_data), NULL);
NET_BUF_POOL_DEFINE(frags_pool, 13, 128, 0, frag_destroy);
NET_BUF_POOL_DEFINE(big_frags_pool, 1, 1280, 0, frag_destroy_big);

static void buf_destroy(struct net_buf *buf)
{
	struct net_buf_pool *pool = net_buf_pool_get(buf->pool_id);

	destroy_called++;
	zassert_equal(pool, &bufs_pool, "Invalid free pointer in buffer");
	net_buf_destroy(buf);
}

static void frag_destroy(struct net_buf *buf)
{
	struct net_buf_pool *pool = net_buf_pool_get(buf->pool_id);

	frag_destroy_called++;
	zassert_equal(pool, &frags_pool,
		     "Invalid free frag pointer in buffer");
	net_buf_destroy(buf);
}

static void frag_destroy_big(struct net_buf *buf)
{
	struct net_buf_pool *pool = net_buf_pool_get(buf->pool_id);

	frag_destroy_called++;
	zassert_equal(pool, &big_frags_pool,
		     "Invalid free big frag pointer in buffer");
	net_buf_destroy(buf);
}

static const char example_data[] = "0123456789"
				   "abcdefghijklmnopqrstuvxyz"
				   "!#¤%&/()=?";

static void net_buf_test_1(void)
{
	struct net_buf *bufs[bufs_pool.buf_count];
	struct net_buf *buf;
	int i;

	for (i = 0; i < bufs_pool.buf_count; i++) {
		buf = net_buf_alloc(&bufs_pool, K_NO_WAIT);
		zassert_not_null(buf, "Failed to get buffer");
		bufs[i] = buf;
	}

	for (i = 0; i < ARRAY_SIZE(bufs); i++) {
		net_buf_unref(bufs[i]);
	}

	zassert_equal(destroy_called, ARRAY_SIZE(bufs),
		     "Incorrect destroy callback count");
}

static void net_buf_test_2(void)
{
	struct net_buf *frag, *head;
	struct k_fifo fifo;
	int i;

	head = net_buf_alloc(&bufs_pool, K_NO_WAIT);
	zassert_not_null(head, "Failed to get fragment list head");

	frag = head;
	for (i = 0; i < bufs_pool.buf_count - 1; i++) {
		frag->frags = net_buf_alloc(&bufs_pool, K_NO_WAIT);
		zassert_not_null(frag->frags, "Failed to get fragment");
		frag = frag->frags;
	}

	k_fifo_init(&fifo);
	net_buf_put(&fifo, head);
	head = net_buf_get(&fifo, K_NO_WAIT);

	destroy_called = 0;
	net_buf_unref(head);
	zassert_equal(destroy_called, bufs_pool.buf_count,
		     "Incorrect fragment destroy callback count");
}

static void test_3_thread(void *arg1, void *arg2, void *arg3)
{
	struct k_fifo *fifo = (struct k_fifo *)arg1;
	struct k_sem *sema = (struct k_sem *)arg2;
	struct net_buf *buf;

	k_sem_give(sema);

	buf = net_buf_get(fifo, TEST_TIMEOUT);
	zassert_not_null(buf, "Unable to get buffer");

	destroy_called = 0;
	net_buf_unref(buf);
	zassert_equal(destroy_called, bufs_pool.buf_count,
		     "Incorrect destroy callback count");

	k_sem_give(sema);
}

static K_THREAD_STACK_DEFINE(test_3_thread_stack, 1024);

static void net_buf_test_3(void)
{
	static struct k_thread test_3_thread_data;
	struct net_buf *frag, *head;
	struct k_fifo fifo;
	struct k_sem sema;
	int i;

	head = net_buf_alloc(&bufs_pool, K_NO_WAIT);
	zassert_not_null(head, "Failed to get fragment list head");

	frag = head;
	for (i = 0; i < bufs_pool.buf_count - 1; i++) {
		frag->frags = net_buf_alloc(&bufs_pool, K_NO_WAIT);
		zassert_not_null(frag->frags, "Failed to get fragment");
		frag = frag->frags;
	}

	k_fifo_init(&fifo);
	k_sem_init(&sema, 0, UINT_MAX);

	k_thread_create(&test_3_thread_data, test_3_thread_stack,
			K_THREAD_STACK_SIZEOF(test_3_thread_stack),
			(k_thread_entry_t) test_3_thread, &fifo, &sema, NULL,
			K_PRIO_COOP(7), 0, 0);

	zassert_true(k_sem_take(&sema, TEST_TIMEOUT) == 0,
		    "Timeout while waiting for semaphore");

	net_buf_put(&fifo, head);

	zassert_true(k_sem_take(&sema, TEST_TIMEOUT) == 0,
		    "Timeout while waiting for semaphore");
}

static void net_buf_test_4(void)
{
	struct net_buf *frags[frags_pool.buf_count];
	struct net_buf *buf, *frag;
	int i, removed;

	/* Create a buf that does not have any data to store, it just
	 * contains link to fragments.
	 */
	buf = net_buf_alloc(&no_data_pool, K_FOREVER);

	zassert_equal(buf->size, 0, "Invalid buffer size");

	/* Test the fragments by appending after last fragment */
	for (i = 0; i < frags_pool.buf_count - 1; i++) {
		frag = net_buf_alloc(&frags_pool, K_FOREVER);
		net_buf_frag_add(buf, frag);
		frags[i] = frag;
	}

	/* And one as a first fragment */
	frag = net_buf_alloc(&frags_pool, K_FOREVER);
	net_buf_frag_insert(buf, frag);
	frags[i] = frag;

	frag = buf->frags;

	zassert_equal(net_buf_pool_get(frag->pool_id)->user_data_size, 0,
		      "Invalid user data size");

	i = 0;
	while (frag) {
		frag = frag->frags;
		i++;
	}

	zassert_equal(i, frags_pool.buf_count, "Incorrect fragment count");

	/* Remove about half of the fragments and verify count */
	i = removed = 0;
	frag = buf->frags;
	while (frag) {
		struct net_buf *next = frag->frags;

		if ((i % 2) && next) {
			net_buf_frag_del(frag, next);
			net_buf_unref(next);
			removed++;
		} else {
			frag = next;
		}
		i++;
	}

	i = 0;
	frag = buf->frags;
	while (frag) {
		frag = frag->frags;
		i++;
	}

	zassert_equal(i + removed, frags_pool.buf_count,
		     "Incorrect removed fragment count");

	removed = 0;

	while (buf->frags) {
		struct net_buf *frag = buf->frags;

		net_buf_frag_del(buf, frag);
		net_buf_unref(frag);
		removed++;
	}

	zassert_equal(removed, i, "Incorrect removed fragment count");
	zassert_equal(frag_destroy_called, frags_pool.buf_count,
		     "Incorrect frag destroy callback count");

	/* Add the fragments back and verify that they are properly unref
	 * by freeing the top buf.
	 */
	for (i = 0; i < frags_pool.buf_count - 3; i++) {
		net_buf_frag_add(buf, net_buf_alloc(&frags_pool, K_FOREVER));
	}

	/* Create a fragment list and add it to frags list after first
	 * element
	 */
	frag = net_buf_alloc(&frags_pool, K_FOREVER);
	net_buf_frag_add(frag, net_buf_alloc(&frags_pool, K_FOREVER));
	net_buf_frag_insert(frag, net_buf_alloc(&frags_pool, K_FOREVER));
	net_buf_frag_insert(buf->frags->frags, frag);

	i = 0;
	frag = buf->frags;
	while (frag) {
		frag = frag->frags;
		i++;
	}

	zassert_equal(i, frags_pool.buf_count, "Incorrect fragment count");

	frag_destroy_called = 0;

	net_buf_unref(buf);

	zassert_equal(frag_destroy_called, frags_pool.buf_count,
		     "Incorrect frag destroy callback count");
}

static void net_buf_test_big_buf(void)
{
	struct net_buf *big_frags[big_frags_pool.buf_count];
	struct net_buf *buf, *frag;
	struct ipv6_hdr *ipv6;
	struct udp_hdr *udp;
	int i, len;

	frag_destroy_called = 0;

	buf = net_buf_alloc(&no_data_pool, K_FOREVER);

	/* We reserve some space in front of the buffer for protocol
	 * headers (IPv6 + UDP). Link layer headers are ignored in
	 * this example.
	 */
#define PROTO_HEADERS (sizeof(struct ipv6_hdr) + sizeof(struct udp_hdr))
	frag = net_buf_alloc(&big_frags_pool, K_FOREVER);
	net_buf_reserve(frag, PROTO_HEADERS);
	big_frags[0] = frag;

	/* First add some application data */
	len = strlen(example_data);
	for (i = 0; i < 2; i++) {
		zassert_true(net_buf_tailroom(frag) >= len,
			    "Allocated buffer is too small");
		memcpy(net_buf_add(frag, len), example_data, len);
	}

	ipv6 = (struct ipv6_hdr *)(frag->data - net_buf_headroom(frag));
	udp = (struct udp_hdr *)((void *)ipv6 + sizeof(*ipv6));

	net_buf_frag_add(buf, frag);
	net_buf_unref(buf);

	zassert_equal(frag_destroy_called, big_frags_pool.buf_count,
		     "Incorrect frag destroy callback count");
}

static void net_buf_test_multi_frags(void)
{
	struct net_buf *frags[frags_pool.buf_count];
	struct net_buf *buf;
	struct ipv6_hdr *ipv6;
	struct udp_hdr *udp;
	int i, len, avail = 0, occupied = 0;

	frag_destroy_called = 0;

	/* Example of multi fragment scenario with IPv6 */
	buf = net_buf_alloc(&no_data_pool, K_FOREVER);

	/* We reserve some space in front of the buffer for link layer headers.
	 * In this example, we use min MTU (81 bytes) defined in rfc 4944 ch. 4
	 *
	 * Note that with IEEE 802.15.4 we typically cannot have zero-copy
	 * in sending side because of the IPv6 header compression.
	 */

#define LL_HEADERS (127 - 81)
	for (i = 0; i < frags_pool.buf_count - 1; i++) {
		frags[i] = net_buf_alloc(&frags_pool, K_FOREVER);
		net_buf_reserve(frags[i], LL_HEADERS);
		avail += net_buf_tailroom(frags[i]);
		net_buf_frag_add(buf, frags[i]);
	}

	/* Place the IP + UDP header in the first fragment */
	frags[i] = net_buf_alloc(&frags_pool, K_FOREVER);
	net_buf_reserve(frags[i], LL_HEADERS + (sizeof(struct ipv6_hdr) +
						sizeof(struct udp_hdr)));
	avail += net_buf_tailroom(frags[i]);
	net_buf_frag_insert(buf, frags[i]);

	/* First add some application data */
	len = strlen(example_data);
	for (i = 0; i < frags_pool.buf_count - 1; i++) {
		zassert_true(net_buf_tailroom(frags[i]) >= len,
			    "Allocated buffer is too small");
		memcpy(net_buf_add(frags[i], len), example_data, len);
		occupied += frags[i]->len;
	}

	ipv6 = (struct ipv6_hdr *)(frags[i]->data - net_buf_headroom(frags[i]));
	udp = (struct udp_hdr *)((void *)ipv6 + sizeof(*ipv6));

	net_buf_unref(buf);

	zassert_equal(frag_destroy_called, frags_pool.buf_count,
		     "Incorrect big frag destroy callback count");
}

void test_main(void)
{
	ztest_test_suite(net_buf_test,
			 ztest_unit_test(net_buf_test_1),
			 ztest_unit_test(net_buf_test_2),
			 ztest_unit_test(net_buf_test_3),
			 ztest_unit_test(net_buf_test_4),
			 ztest_unit_test(net_buf_test_big_buf),
			 ztest_unit_test(net_buf_test_multi_frags)
			 );

	ztest_run_test_suite(net_buf_test);
}
