/* main.c - Application main entry point */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <zephyr/sys/printk.h>

#include <zephyr/net/buf.h>

#include <zephyr/ztest.h>

#define TEST_TIMEOUT K_SECONDS(1)

#define USER_DATA_HEAP	4
#define USER_DATA_FIXED	0
#define USER_DATA_VAR	63

struct bt_data {
	void *hci_sync;

	union {
		uint16_t hci_opcode;
		uint16_t acl_handle;
	};

	uint8_t type;
};

struct in6_addr {
	union {
		uint8_t u6_addr8[16];
		uint16_t u6_addr16[8];          /* In big endian */
		uint32_t u6_addr32[4];          /* In big endian */
	} in6_u;
#define s6_addr         in6_u.u6_addr8
#define s6_addr16       in6_u.u6_addr16
#define s6_addr32       in6_u.u6_addr32
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

static void buf_destroy(struct net_buf *buf);
static void fixed_destroy(struct net_buf *buf);
static void var_destroy(struct net_buf *buf);

NET_BUF_POOL_HEAP_DEFINE(bufs_pool, 10, USER_DATA_HEAP, buf_destroy);
NET_BUF_POOL_FIXED_DEFINE(fixed_pool, 10, 128, USER_DATA_FIXED, fixed_destroy);
NET_BUF_POOL_VAR_DEFINE(var_pool, 10, 1024, USER_DATA_VAR, var_destroy);

static void buf_destroy(struct net_buf *buf)
{
	struct net_buf_pool *pool = net_buf_pool_get(buf->pool_id);

	destroy_called++;
	zassert_equal(pool, &bufs_pool, "Invalid free pointer in buffer");
	net_buf_destroy(buf);
}

static void fixed_destroy(struct net_buf *buf)
{
	struct net_buf_pool *pool = net_buf_pool_get(buf->pool_id);

	destroy_called++;
	zassert_equal(pool, &fixed_pool, "Invalid free pointer in buffer");
	net_buf_destroy(buf);
}

static void var_destroy(struct net_buf *buf)
{
	struct net_buf_pool *pool = net_buf_pool_get(buf->pool_id);

	destroy_called++;
	zassert_equal(pool, &var_pool, "Invalid free pointer in buffer");
	net_buf_destroy(buf);
}

static const char example_data[] = "0123456789"
				   "abcdefghijklmnopqrstuvxyz"
				   "!#¤%&/()=?";

ZTEST(net_buf_tests, test_net_buf_1)
{
	struct net_buf *bufs[bufs_pool.buf_count];
	struct net_buf *buf;
	int i;

	for (i = 0; i < bufs_pool.buf_count; i++) {
		buf = net_buf_alloc_len(&bufs_pool, 74, K_NO_WAIT);
		zassert_not_null(buf, "Failed to get buffer");
		bufs[i] = buf;
	}

	for (i = 0; i < ARRAY_SIZE(bufs); i++) {
		net_buf_unref(bufs[i]);
	}

	zassert_equal(destroy_called, ARRAY_SIZE(bufs),
		      "Incorrect destroy callback count");
}

ZTEST(net_buf_tests, test_net_buf_2)
{
	struct net_buf *frag, *head;
	static struct k_fifo fifo;
	int i;

	head = net_buf_alloc_len(&bufs_pool, 74, K_NO_WAIT);
	zassert_not_null(head, "Failed to get fragment list head");

	frag = head;
	for (i = 0; i < bufs_pool.buf_count - 1; i++) {
		frag->frags = net_buf_alloc_len(&bufs_pool, 74, K_NO_WAIT);
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

ZTEST(net_buf_tests, test_net_buf_3)
{
	static struct k_thread test_3_thread_data;
	struct net_buf *frag, *head;
	static struct k_fifo fifo;
	static struct k_sem sema;
	int i;

	head = net_buf_alloc_len(&bufs_pool, 74, K_NO_WAIT);
	zassert_not_null(head, "Failed to get fragment list head");

	frag = head;
	for (i = 0; i < bufs_pool.buf_count - 1; i++) {
		frag->frags = net_buf_alloc_len(&bufs_pool, 74, K_NO_WAIT);
		zassert_not_null(frag->frags, "Failed to get fragment");
		frag = frag->frags;
	}

	k_fifo_init(&fifo);
	k_sem_init(&sema, 0, UINT_MAX);

	k_thread_create(&test_3_thread_data, test_3_thread_stack,
			K_THREAD_STACK_SIZEOF(test_3_thread_stack),
			(k_thread_entry_t) test_3_thread, &fifo, &sema, NULL,
			K_PRIO_COOP(7), 0, K_NO_WAIT);

	zassert_true(k_sem_take(&sema, TEST_TIMEOUT) == 0,
		     "Timeout while waiting for semaphore");

	net_buf_put(&fifo, head);

	zassert_true(k_sem_take(&sema, TEST_TIMEOUT) == 0,
		     "Timeout while waiting for semaphore");
}

ZTEST(net_buf_tests, test_net_buf_4)
{
	struct net_buf *frags[bufs_pool.buf_count - 1];
	struct net_buf *buf, *frag;
	int i, removed;

	destroy_called = 0;

	/* Create a buf that does not have any data to store, it just
	 * contains link to fragments.
	 */
	buf = net_buf_alloc_len(&bufs_pool, 0, K_FOREVER);

	zassert_equal(buf->size, 0, "Invalid buffer size");

	/* Test the fragments by appending after last fragment */
	for (i = 0; i < bufs_pool.buf_count - 2; i++) {
		frag = net_buf_alloc_len(&bufs_pool, 74, K_FOREVER);
		net_buf_frag_add(buf, frag);
		frags[i] = frag;
	}

	/* And one as a first fragment */
	frag = net_buf_alloc_len(&bufs_pool, 74, K_FOREVER);
	net_buf_frag_insert(buf, frag);
	frags[i] = frag;

	frag = buf->frags;

	i = 0;
	while (frag) {
		frag = frag->frags;
		i++;
	}

	zassert_equal(i, bufs_pool.buf_count - 1, "Incorrect fragment count");

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

	zassert_equal(1 + i + removed, bufs_pool.buf_count,
		      "Incorrect removed fragment count");

	removed = 0;

	while (buf->frags) {
		struct net_buf *frag = buf->frags;

		net_buf_frag_del(buf, frag);
		net_buf_unref(frag);
		removed++;
	}

	zassert_equal(removed, i, "Incorrect removed fragment count");
	zassert_equal(destroy_called, bufs_pool.buf_count - 1,
		      "Incorrect frag destroy callback count");

	/* Add the fragments back and verify that they are properly unref
	 * by freeing the top buf.
	 */
	for (i = 0; i < bufs_pool.buf_count - 4; i++) {
		net_buf_frag_add(buf,
				 net_buf_alloc_len(&bufs_pool, 74, K_FOREVER));
	}

	/* Create a fragment list and add it to frags list after first
	 * element
	 */
	frag = net_buf_alloc_len(&bufs_pool, 74, K_FOREVER);
	net_buf_frag_add(frag, net_buf_alloc_len(&bufs_pool, 74, K_FOREVER));
	net_buf_frag_insert(frag, net_buf_alloc_len(&bufs_pool, 74, K_FOREVER));
	net_buf_frag_insert(buf->frags->frags, frag);

	i = 0;
	frag = buf->frags;
	while (frag) {
		frag = frag->frags;
		i++;
	}

	zassert_equal(i, bufs_pool.buf_count - 1, "Incorrect fragment count");

	destroy_called = 0;

	net_buf_unref(buf);

	zassert_equal(destroy_called, bufs_pool.buf_count,
		      "Incorrect frag destroy callback count");
}

ZTEST(net_buf_tests, test_net_buf_big_buf)
{
	struct net_buf *big_frags[bufs_pool.buf_count];
	struct net_buf *buf, *frag;
	struct ipv6_hdr *ipv6;
	struct udp_hdr *udp;
	int i, len;

	destroy_called = 0;

	buf = net_buf_alloc_len(&bufs_pool, 0, K_FOREVER);

	/* We reserve some space in front of the buffer for protocol
	 * headers (IPv6 + UDP). Link layer headers are ignored in
	 * this example.
	 */
#define PROTO_HEADERS (sizeof(struct ipv6_hdr) + sizeof(struct udp_hdr))
	frag = net_buf_alloc_len(&bufs_pool, 1280, K_FOREVER);
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
	udp = (struct udp_hdr *)((uint8_t *)ipv6 + sizeof(*ipv6));

	net_buf_frag_add(buf, frag);
	net_buf_unref(buf);

	zassert_equal(destroy_called, 2, "Incorrect destroy callback count");
}

ZTEST(net_buf_tests, test_net_buf_multi_frags)
{
	struct net_buf *frags[bufs_pool.buf_count];
	struct net_buf *buf;
	struct ipv6_hdr *ipv6;
	struct udp_hdr *udp;
	int i, len, avail = 0, occupied = 0;

	destroy_called = 0;

	/* Example of multi fragment scenario with IPv6 */
	buf = net_buf_alloc_len(&bufs_pool, 0, K_FOREVER);

	/* We reserve some space in front of the buffer for link layer headers.
	 * In this example, we use min MTU (81 bytes) defined in rfc 4944 ch. 4
	 *
	 * Note that with IEEE 802.15.4 we typically cannot have zero-copy
	 * in sending side because of the IPv6 header compression.
	 */

#define LL_HEADERS (127 - 81)
	for (i = 0; i < bufs_pool.buf_count - 2; i++) {
		frags[i] = net_buf_alloc_len(&bufs_pool, 128, K_FOREVER);
		net_buf_reserve(frags[i], LL_HEADERS);
		avail += net_buf_tailroom(frags[i]);
		net_buf_frag_add(buf, frags[i]);
	}

	/* Place the IP + UDP header in the first fragment */
	frags[i] = net_buf_alloc_len(&bufs_pool, 128, K_FOREVER);
	net_buf_reserve(frags[i], LL_HEADERS + (sizeof(struct ipv6_hdr) +
						sizeof(struct udp_hdr)));
	avail += net_buf_tailroom(frags[i]);
	net_buf_frag_insert(buf, frags[i]);

	/* First add some application data */
	len = strlen(example_data);
	for (i = 0; i < bufs_pool.buf_count - 2; i++) {
		zassert_true(net_buf_tailroom(frags[i]) >= len,
			     "Allocated buffer is too small");
		memcpy(net_buf_add(frags[i], len), example_data, len);
		occupied += frags[i]->len;
	}

	ipv6 = (struct ipv6_hdr *)(frags[i]->data - net_buf_headroom(frags[i]));
	udp = (struct udp_hdr *)((uint8_t *)ipv6 + sizeof(*ipv6));

	net_buf_unref(buf);

	zassert_equal(destroy_called, bufs_pool.buf_count,
		      "Incorrect frag destroy callback count");
}

ZTEST(net_buf_tests, test_net_buf_clone)
{
	struct net_buf *buf, *clone;

	destroy_called = 0;

	buf = net_buf_alloc_len(&bufs_pool, 74, K_NO_WAIT);
	zassert_not_null(buf, "Failed to get buffer");

	clone = net_buf_clone(buf, K_NO_WAIT);
	zassert_not_null(clone, "Failed to get clone buffer");
	zassert_equal(buf->data, clone->data, "Incorrect clone data pointer");

	net_buf_unref(buf);
	net_buf_unref(clone);

	zassert_equal(destroy_called, 2, "Incorrect destroy callback count");
}

ZTEST(net_buf_tests, test_net_buf_fixed_pool)
{
	struct net_buf *buf;

	destroy_called = 0;

	buf = net_buf_alloc_len(&fixed_pool, 20, K_NO_WAIT);
	zassert_not_null(buf, "Failed to get buffer");

	net_buf_unref(buf);

	zassert_equal(destroy_called, 1, "Incorrect destroy callback count");
}

ZTEST(net_buf_tests, test_net_buf_var_pool)
{
	struct net_buf *buf1, *buf2, *buf3;

	destroy_called = 0;

	buf1 = net_buf_alloc_len(&var_pool, 20, K_NO_WAIT);
	zassert_not_null(buf1, "Failed to get buffer");

	buf2 = net_buf_alloc_len(&var_pool, 200, K_NO_WAIT);
	zassert_not_null(buf2, "Failed to get buffer");

	buf3 = net_buf_clone(buf2, K_NO_WAIT);
	zassert_not_null(buf3, "Failed to clone buffer");
	zassert_equal(buf3->data, buf2->data, "Cloned data doesn't match");

	net_buf_unref(buf1);
	net_buf_unref(buf2);
	net_buf_unref(buf3);

	zassert_equal(destroy_called, 3, "Incorrect destroy callback count");
}

ZTEST(net_buf_tests, test_net_buf_byte_order)
{
	struct net_buf *buf;
	uint8_t le16[2] = { 0x02, 0x01 };
	uint8_t be16[2] = { 0x01, 0x02 };
	uint8_t le24[3] = { 0x03, 0x02, 0x01 };
	uint8_t be24[3] = { 0x01, 0x02, 0x03 };
	uint8_t le32[4] = { 0x04, 0x03, 0x02, 0x01 };
	uint8_t be32[4] = { 0x01, 0x02, 0x03, 0x04 };
	uint8_t le48[6] = { 0x06, 0x05, 0x04, 0x03, 0x02, 0x01 };
	uint8_t be48[6] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };
	uint8_t le64[8] = { 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01 };
	uint8_t be64[8] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
	uint16_t u16;
	uint32_t u32;
	uint64_t u64;

	buf = net_buf_alloc_len(&fixed_pool, 16, K_FOREVER);
	zassert_not_null(buf, "Failed to get buffer");

	/* add/pull byte order */
	net_buf_add_mem(buf, &le16, sizeof(le16));
	net_buf_add_mem(buf, &be16, sizeof(be16));

	u16 = net_buf_pull_le16(buf);
	zassert_equal(u16, net_buf_pull_be16(buf),
		      "Invalid 16 bits byte order");

	net_buf_reset(buf);

	net_buf_add_le16(buf, u16);
	net_buf_add_be16(buf, u16);

	zassert_mem_equal(le16, net_buf_pull_mem(buf, sizeof(le16)),
			  sizeof(le16), "Invalid 16 bits byte order");
	zassert_mem_equal(be16, net_buf_pull_mem(buf, sizeof(be16)),
			  sizeof(be16), "Invalid 16 bits byte order");

	net_buf_reset(buf);

	net_buf_add_mem(buf, &le24, sizeof(le24));
	net_buf_add_mem(buf, &be24, sizeof(be24));

	u32 = net_buf_pull_le24(buf);
	zassert_equal(u32, net_buf_pull_be24(buf),
		      "Invalid 24 bits byte order");

	net_buf_reset(buf);

	net_buf_add_le24(buf, u32);
	net_buf_add_be24(buf, u32);

	zassert_mem_equal(le24, net_buf_pull_mem(buf, sizeof(le24)),
			  sizeof(le24), "Invalid 24 bits byte order");
	zassert_mem_equal(be24, net_buf_pull_mem(buf, sizeof(be24)),
			  sizeof(be24), "Invalid 24 bits byte order");

	net_buf_reset(buf);

	net_buf_add_mem(buf, &le32, sizeof(le32));
	net_buf_add_mem(buf, &be32, sizeof(be32));

	u32 = net_buf_pull_le32(buf);
	zassert_equal(u32, net_buf_pull_be32(buf),
		      "Invalid 32 bits byte order");

	net_buf_reset(buf);

	net_buf_add_le32(buf, u32);
	net_buf_add_be32(buf, u32);

	zassert_mem_equal(le32, net_buf_pull_mem(buf, sizeof(le32)),
			  sizeof(le32), "Invalid 32 bits byte order");
	zassert_mem_equal(be32, net_buf_pull_mem(buf, sizeof(be32)),
			  sizeof(be32), "Invalid 32 bits byte order");

	net_buf_reset(buf);

	net_buf_add_mem(buf, &le48, sizeof(le48));
	net_buf_add_mem(buf, &be48, sizeof(be48));

	u64 = net_buf_pull_le48(buf);
	zassert_equal(u64, net_buf_pull_be48(buf),
		      "Invalid 48 bits byte order");

	net_buf_reset(buf);

	net_buf_add_le48(buf, u64);
	net_buf_add_be48(buf, u64);

	zassert_mem_equal(le48, net_buf_pull_mem(buf, sizeof(le48)),
			  sizeof(le48), "Invalid 48 bits byte order");
	zassert_mem_equal(be48, net_buf_pull_mem(buf, sizeof(be48)),
			  sizeof(be48), "Invalid 48 bits byte order");

	net_buf_reset(buf);

	net_buf_add_mem(buf, &le64, sizeof(le64));
	net_buf_add_mem(buf, &be64, sizeof(be64));

	u64 = net_buf_pull_le64(buf);
	zassert_equal(u64, net_buf_pull_be64(buf),
		      "Invalid 64 bits byte order");

	net_buf_reset(buf);

	net_buf_add_le64(buf, u64);
	net_buf_add_be64(buf, u64);

	zassert_mem_equal(le64, net_buf_pull_mem(buf, sizeof(le64)),
			  sizeof(le64), "Invalid 64 bits byte order");
	zassert_mem_equal(be64, net_buf_pull_mem(buf, sizeof(be64)),
			  sizeof(be64), "Invalid 64 bits byte order");

	/* push/remove byte order */
	net_buf_reset(buf);
	net_buf_reserve(buf, 16);

	net_buf_push_mem(buf, &le16, sizeof(le16));
	net_buf_push_mem(buf, &be16, sizeof(be16));

	u16 = net_buf_remove_le16(buf);
	zassert_equal(u16, net_buf_remove_be16(buf),
		      "Invalid 16 bits byte order");

	net_buf_reset(buf);
	net_buf_reserve(buf, 16);

	net_buf_push_le16(buf, u16);
	net_buf_push_be16(buf, u16);

	zassert_mem_equal(le16, net_buf_remove_mem(buf, sizeof(le16)),
			  sizeof(le16),  "Invalid 16 bits byte order");
	zassert_mem_equal(be16, net_buf_remove_mem(buf, sizeof(be16)),
			  sizeof(be16),  "Invalid 16 bits byte order");

	net_buf_reset(buf);
	net_buf_reserve(buf, 16);

	net_buf_push_mem(buf, &le24, sizeof(le24));
	net_buf_push_mem(buf, &be24, sizeof(be24));

	u32 = net_buf_remove_le24(buf);
	zassert_equal(u32, net_buf_remove_be24(buf),
		      "Invalid 24 bits byte order");

	net_buf_reset(buf);
	net_buf_reserve(buf, 16);

	net_buf_push_le24(buf, u32);
	net_buf_push_be24(buf, u32);

	zassert_mem_equal(le24, net_buf_remove_mem(buf, sizeof(le24)),
			  sizeof(le24),  "Invalid 24 bits byte order");
	zassert_mem_equal(be24, net_buf_remove_mem(buf, sizeof(be24)),
			  sizeof(be24),  "Invalid 24 bits byte order");

	net_buf_reset(buf);
	net_buf_reserve(buf, 16);

	net_buf_push_mem(buf, &le32, sizeof(le32));
	net_buf_push_mem(buf, &be32, sizeof(be32));

	u32 = net_buf_remove_le32(buf);
	zassert_equal(u32, net_buf_remove_be32(buf),
		      "Invalid 32 bits byte order");

	net_buf_reset(buf);
	net_buf_reserve(buf, 16);

	net_buf_push_le32(buf, u32);
	net_buf_push_be32(buf, u32);

	zassert_mem_equal(le32, net_buf_remove_mem(buf, sizeof(le32)),
			  sizeof(le32), "Invalid 32 bits byte order");
	zassert_mem_equal(be32, net_buf_remove_mem(buf, sizeof(be32)),
			  sizeof(be32), "Invalid 32 bits byte order");

	net_buf_reset(buf);
	net_buf_reserve(buf, 16);

	net_buf_push_mem(buf, &le48, sizeof(le48));
	net_buf_push_mem(buf, &be48, sizeof(be48));

	u64 = net_buf_remove_le48(buf);
	zassert_equal(u64, net_buf_remove_be48(buf),
		      "Invalid 48 bits byte order");

	net_buf_reset(buf);
	net_buf_reserve(buf, 16);

	net_buf_push_le48(buf, u64);
	net_buf_push_be48(buf, u64);

	zassert_mem_equal(le48, net_buf_remove_mem(buf, sizeof(le48)),
			  sizeof(le48),  "Invalid 48 bits byte order");
	zassert_mem_equal(be48, net_buf_remove_mem(buf, sizeof(be48)),
			  sizeof(be48),  "Invalid 48 bits byte order");

	net_buf_reset(buf);
	net_buf_reserve(buf, 16);

	net_buf_push_mem(buf, &le64, sizeof(le64));
	net_buf_push_mem(buf, &be64, sizeof(be64));

	u64 = net_buf_remove_le64(buf);
	zassert_equal(u64, net_buf_remove_be64(buf),
		      "Invalid 64 bits byte order");

	net_buf_reset(buf);
	net_buf_reserve(buf, 16);

	net_buf_push_le64(buf, u64);
	net_buf_push_be64(buf, u64);

	zassert_mem_equal(le64, net_buf_remove_mem(buf, sizeof(le64)),
			  sizeof(le64), "Invalid 64 bits byte order");
	zassert_mem_equal(be64, net_buf_remove_mem(buf, sizeof(be64)),
			  sizeof(be64), "Invalid 64 bits byte order");

	net_buf_unref(buf);
}

ZTEST(net_buf_tests, test_net_buf_user_data)
{
	struct net_buf *buf;

	/* Fixed Pool */
	buf = net_buf_alloc(&fixed_pool, K_NO_WAIT);
	zassert_not_null(buf, "Failed to get buffer");

	zassert_equal(USER_DATA_FIXED, fixed_pool.user_data_size,
		"Bad user_data_size");
	zassert_equal(USER_DATA_FIXED, buf->user_data_size,
		"Bad user_data_size");

	net_buf_unref(buf);

	/* Heap Pool */
	buf = net_buf_alloc_len(&bufs_pool, 20, K_NO_WAIT);
	zassert_not_null(buf, "Failed to get buffer");

	zassert_equal(USER_DATA_HEAP, bufs_pool.user_data_size,
		"Bad user_data_size");
	zassert_equal(USER_DATA_HEAP, buf->user_data_size,
		"Bad user_data_size");

	net_buf_unref(buf);

	/* Var Pool */
	buf = net_buf_alloc_len(&var_pool, 20, K_NO_WAIT);
	zassert_not_null(buf, "Failed to get buffer");

	zassert_equal(USER_DATA_VAR, var_pool.user_data_size,
		"Bad user_data_size");
	zassert_equal(USER_DATA_VAR, buf->user_data_size,
		"Bad user_data_size");

	net_buf_unref(buf);
}

ZTEST_SUITE(net_buf_tests, NULL, NULL, NULL, NULL, NULL);
