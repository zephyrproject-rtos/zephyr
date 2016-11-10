/* main.c - Application main entry point */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <misc/printk.h>

#include <net/buf.h>
#include <net/net_ip.h>

#include <ztest.h>

#define TEST_TIMEOUT SECONDS(1)

struct bt_data {
	void *hci_sync;

	union {
		uint16_t hci_opcode;
		uint16_t acl_handle;
	};

	uint8_t type;
};

struct ipv6_hdr {
	uint8_t vtc;
	uint8_t tcflow;
	uint16_t flow;
	uint8_t len[2];
	uint8_t nexthdr;
	uint8_t hop_limit;
	struct in6_addr src;
	struct in6_addr dst;
} __attribute__((__packed__));

struct udp_hdr {
	uint16_t src_port;
	uint16_t dst_port;
	uint16_t len;
	uint16_t chksum;
} __attribute__((__packed__));

static int destroy_called;
static int frag_destroy_called;

static struct nano_fifo bufs_fifo;
static struct nano_fifo no_data_buf_fifo;
static struct nano_fifo frags_fifo;
static struct nano_fifo big_frags_fifo;

static void buf_destroy(struct net_buf *buf)
{
	destroy_called++;
	assert_equal(buf->free, &bufs_fifo, "Invalid free pointer in buffer");
	nano_fifo_put(buf->free, buf);
}

static void frag_destroy(struct net_buf *buf)
{
	frag_destroy_called++;
	assert_equal(buf->free, &frags_fifo,
		     "Invalid free frag pointer in buffer");
	nano_fifo_put(buf->free, buf);
}

static void frag_destroy_big(struct net_buf *buf)
{
	frag_destroy_called++;
	assert_equal(buf->free, &big_frags_fifo,
		     "Invalid free big frag pointer in buffer");
	nano_fifo_put(buf->free, buf);
}

static NET_BUF_POOL(bufs_pool, 22, 74, &bufs_fifo, buf_destroy,
		    sizeof(struct bt_data));

static NET_BUF_POOL(no_data_buf_pool, 1, 0, &no_data_buf_fifo, NULL,
		    sizeof(struct bt_data));

static NET_BUF_POOL(frags_pool, 13, 128, &frags_fifo, frag_destroy, 0);

static NET_BUF_POOL(big_pool, 1, 1280, &big_frags_fifo, frag_destroy_big, 0);

static const char example_data[] = "0123456789"
				   "abcdefghijklmnopqrstuvxyz"
				   "!#¤%&/()=?";

static void net_buf_test_1(void)
{
	struct net_buf *bufs[ARRAY_SIZE(bufs_pool)];
	struct net_buf *buf;
	int i;

	for (i = 0; i < ARRAY_SIZE(bufs_pool); i++) {
		buf = net_buf_get_timeout(&bufs_fifo, 0, K_NO_WAIT);
		assert_not_null(buf, "Failed to get buffer");
		bufs[i] = buf;
	}

	for (i = 0; i < ARRAY_SIZE(bufs_pool); i++) {
		net_buf_unref(bufs[i]);
	}

	assert_equal(destroy_called, ARRAY_SIZE(bufs_pool),
		     "Incorrect destroy callback count");
}

static void net_buf_test_2(void)
{
	struct net_buf *frag, *head;
	struct nano_fifo fifo;
	int i;

	head = net_buf_get_timeout(&bufs_fifo, 0, K_NO_WAIT);
	assert_not_null(head, "Failed to get fragment list head");

	frag = head;
	for (i = 0; i < ARRAY_SIZE(bufs_pool) - 1; i++) {
		frag->frags = net_buf_get_timeout(&bufs_fifo, 0, K_NO_WAIT);
		assert_not_null(frag->frags, "Failed to get fragment");
		frag = frag->frags;
	}

	nano_fifo_init(&fifo);
	net_buf_put(&fifo, head);
	head = net_buf_get_timeout(&fifo, 0, K_NO_WAIT);

	destroy_called = 0;
	net_buf_unref(head);
	assert_equal(destroy_called, ARRAY_SIZE(bufs_pool),
		     "Incorrect fragment destroy callback count");
}

static void test_3_fiber(int arg1, int arg2)
{
	struct nano_fifo *fifo = (struct nano_fifo *)arg1;
	struct nano_sem *sema = (struct nano_sem *)arg2;
	struct net_buf *buf;

	nano_sem_give(sema);

	buf = net_buf_get_timeout(fifo, 0, TEST_TIMEOUT);
	assert_not_null(buf, "Unable to get buffer");

	destroy_called = 0;
	net_buf_unref(buf);
	assert_equal(destroy_called, ARRAY_SIZE(bufs_pool),
		     "Incorrect destroy callback count");

	nano_sem_give(sema);
}

static void net_buf_test_3(void)
{
	static char __stack test_3_fiber_stack[1024];
	struct net_buf *frag, *head;
	struct nano_fifo fifo;
	struct nano_sem sema;
	int i;

	head = net_buf_get_timeout(&bufs_fifo, 0, K_NO_WAIT);
	assert_not_null(head, "Failed to get fragment list head");

	frag = head;
	for (i = 0; i < ARRAY_SIZE(bufs_pool) - 1; i++) {
		frag->frags = net_buf_get_timeout(&bufs_fifo, 0, K_NO_WAIT);
		assert_not_null(frag->frags, "Failed to get fragment");
		frag = frag->frags;
	}

	nano_fifo_init(&fifo);
	nano_sem_init(&sema);

	fiber_start(test_3_fiber_stack, sizeof(test_3_fiber_stack),
		    test_3_fiber, (int)&fifo, (int)&sema, 7, 0);

	assert_true(nano_sem_take(&sema, TEST_TIMEOUT),
		    "Timeout while waiting for semaphore");

	net_buf_put(&fifo, head);

	assert_true(nano_sem_take(&sema, TEST_TIMEOUT),
		    "Timeout while waiting for semaphore");
}

static void net_buf_test_4(void)
{
	struct net_buf *frags[ARRAY_SIZE(frags_pool)];
	struct net_buf *buf, *frag;
	int i, removed;

	net_buf_pool_init(no_data_buf_pool);
	net_buf_pool_init(frags_pool);

	/* Create a buf that does not have any data to store, it just
	 * contains link to fragments.
	 */
	buf = net_buf_get(&no_data_buf_fifo, 0);

	assert_equal(buf->size, 0, "Invalid buffer size");

	/* Test the fragments by appending after last fragment */
	for (i = 0; i < ARRAY_SIZE(frags_pool) - 1; i++) {
		frag = net_buf_get(&frags_fifo, 0);
		net_buf_frag_add(buf, frag);
		frags[i] = frag;
	}

	/* And one as a first fragment */
	frag = net_buf_get(&frags_fifo, 0);
	net_buf_frag_insert(buf, frag);
	frags[i] = frag;

	frag = buf->frags;

	assert_equal(frag->user_data_size, 0, "Invalid user data size");

	i = 0;
	while (frag) {
		frag = frag->frags;
		i++;
	}

	assert_equal(i, ARRAY_SIZE(frags_pool), "Incorrect fragment count");

	/* Remove about half of the fragments and verify count */
	i = removed = 0;
	frag = buf->frags;
	while (frag) {
		struct net_buf *next = frag->frags;

		if ((i % 2) && next) {
			net_buf_frag_del(frag, next);
			net_buf_unref(next);
			removed++;
		} else  {
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

	assert_equal(i + removed, ARRAY_SIZE(frags_pool),
		     "Incorrect removed fragment count");

	removed = 0;

	while (buf->frags) {
		struct net_buf *frag = buf->frags;

		net_buf_frag_del(buf, frag);
		net_buf_unref(frag);
		removed++;
	}

	assert_equal(removed, i, "Incorrect removed fragment count");
	assert_equal(frag_destroy_called, ARRAY_SIZE(frags_pool),
		     "Incorrect frag destroy callback count");

	/* Add the fragments back and verify that they are properly unref
	 * by freeing the top buf.
	 */
	for (i = 0; i < ARRAY_SIZE(frags_pool) - 3; i++) {
		net_buf_frag_add(buf, net_buf_get(&frags_fifo, 0));
	}

	/* Create a fragment list and add it to frags list after first
	 * element
	 */
	frag = net_buf_get(&frags_fifo, 0);
	net_buf_frag_add(frag, net_buf_get(&frags_fifo, 0));
	net_buf_frag_insert(frag, net_buf_get(&frags_fifo, 0));
	net_buf_frag_insert(buf->frags->frags, frag);

	i = 0;
	frag = buf->frags;
	while (frag) {
		frag = frag->frags;
		i++;
	}

	assert_equal(i, ARRAY_SIZE(frags_pool),
		     "Incorrect fragment count");

	frag_destroy_called = 0;

	net_buf_unref(buf);

	assert_equal(frag_destroy_called, ARRAY_SIZE(frags_pool),
		     "Incorrect frag destroy callback count");
}

static void net_buf_test_big_buf(void)
{
	struct net_buf *big_frags[ARRAY_SIZE(big_pool)];
	struct net_buf *buf, *frag;
	struct ipv6_hdr *ipv6;
	struct udp_hdr *udp;
	int i, len;

	net_buf_pool_init(big_pool);

	frag_destroy_called = 0;

	buf = net_buf_get(&no_data_buf_fifo, 0);

	/* We reserve some space in front of the buffer for protocol
	 * headers (IPv6 + UDP). Link layer headers are ignored in
	 * this example.
	 */
#define PROTO_HEADERS (sizeof(struct ipv6_hdr) + sizeof(struct udp_hdr))
	frag = net_buf_get(&big_frags_fifo, PROTO_HEADERS);
	big_frags[0] = frag;

	/* First add some application data */
	len = strlen(example_data);
	for (i = 0; i < 2; i++) {
		assert_true(net_buf_tailroom(frag) >= len,
			    "Allocated buffer is too small");
		memcpy(net_buf_add(frag, len), example_data, len);
	}

	ipv6 = (struct ipv6_hdr *)(frag->data - net_buf_headroom(frag));
	udp = (struct udp_hdr *)((void *)ipv6 + sizeof(*ipv6));

	net_buf_frag_add(buf, frag);
	net_buf_unref(buf);

	assert_equal(frag_destroy_called, ARRAY_SIZE(big_pool),
		     "Incorrect frag destroy callback count");
}

static void net_buf_test_multi_frags(void)
{
	struct net_buf *frags[ARRAY_SIZE(frags_pool)];
	struct net_buf *buf;
	struct ipv6_hdr *ipv6;
	struct udp_hdr *udp;
	int i, len, avail = 0, occupied = 0;

	frag_destroy_called = 0;

	/* Example of multi fragment scenario with IPv6 */
	buf = net_buf_get(&no_data_buf_fifo, 0);

	/* We reserve some space in front of the buffer for link layer headers.
	 * In this example, we use min MTU (81 bytes) defined in rfc 4944 ch. 4
	 *
	 * Note that with IEEE 802.15.4 we typically cannot have zero-copy
	 * in sending side because of the IPv6 header compression.
	 */

#define LL_HEADERS (127 - 81)
	for (i = 0; i < ARRAY_SIZE(frags_pool) - 1; i++) {
		frags[i] = net_buf_get(&frags_fifo, LL_HEADERS);
		avail += net_buf_tailroom(frags[i]);
		net_buf_frag_add(buf, frags[i]);
	}

	/* Place the IP + UDP header in the first fragment */
	frags[i] = net_buf_get(&frags_fifo,
			       LL_HEADERS +
			       (sizeof(struct ipv6_hdr) +
				sizeof(struct udp_hdr)));
	avail += net_buf_tailroom(frags[i]);
	net_buf_frag_insert(buf, frags[i]);

	/* First add some application data */
	len = strlen(example_data);
	for (i = 0; i < ARRAY_SIZE(frags_pool) - 1; i++) {
		assert_true(net_buf_tailroom(frags[i]) >= len,
			    "Allocated buffer is too small");
		memcpy(net_buf_add(frags[i], len), example_data, len);
		occupied += frags[i]->len;
	}

	ipv6 = (struct ipv6_hdr *)(frags[i]->data - net_buf_headroom(frags[i]));
	udp = (struct udp_hdr *)((void *)ipv6 + sizeof(*ipv6));

	net_buf_unref(buf);

	assert_equal(frag_destroy_called, ARRAY_SIZE(frags_pool),
		     "Incorrect big frag destroy callback count");
}

void test_main(void)
{
	net_buf_pool_init(bufs_pool);

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
