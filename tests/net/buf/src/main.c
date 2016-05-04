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
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <misc/printk.h>

#include <net/buf.h>
#include <net/net_ip.h>

#if defined(CONFIG_NET_BUF_DEBUG)
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

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

	if (buf->free != &bufs_fifo) {
		printk("Invalid free pointer in buffer!\n");
	}

	DBG("destroying %p\n", buf);

	nano_fifo_put(buf->free, buf);
}

static void frag_destroy(struct net_buf *buf)
{
	frag_destroy_called++;

	if (buf->free != &frags_fifo) {
		printk("Invalid free frag pointer in buffer!\n");
	} else {
		nano_fifo_put(buf->free, buf);
	}
}

static void frag_destroy_big(struct net_buf *buf)
{
	frag_destroy_called++;

	if (buf->free != &big_frags_fifo) {
		printk("Invalid free big frag pointer in buffer!\n");
	} else {
		nano_fifo_put(buf->free, buf);
	}
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

static bool net_buf_test_1(void)
{
	struct net_buf *bufs[ARRAY_SIZE(bufs_pool)];
	struct net_buf *buf;
	int i;

	for (i = 0; i < ARRAY_SIZE(bufs_pool); i++) {
		buf = net_buf_get_timeout(&bufs_fifo, 0, TICKS_NONE);
		if (!buf) {
			printk("Failed to get buffer!\n");
			return false;
		}

		bufs[i] = buf;
	}

	for (i = 0; i < ARRAY_SIZE(bufs_pool); i++) {
		net_buf_unref(bufs[i]);
	}

	if (destroy_called != ARRAY_SIZE(bufs_pool)) {
		printk("Incorrect destroy callback count: %d\n",
		       destroy_called);
		return false;
	}

	return true;
}

static bool net_buf_test_2(void)
{
	struct net_buf *frag, *head;
	struct nano_fifo fifo;
	int i;

	head = net_buf_get_timeout(&bufs_fifo, 0, TICKS_NONE);
	if (!head) {
		printk("Failed to get fragment list head!\n");
		return false;
	}

	DBG("Fragment list head %p\n", head);

	frag = head;
	for (i = 0; i < ARRAY_SIZE(bufs_pool) - 1; i++) {
		frag->frags = net_buf_get_timeout(&bufs_fifo, 0, TICKS_NONE);
		if (!frag->frags) {
			printk("Failed to get fragment!\n");
			return false;
		}
		DBG("%p -> %p\n", frag, frag->frags);
		frag = frag->frags;
	}

	DBG("%p -> %p\n", frag, frag->frags);

	nano_fifo_init(&fifo);
	net_buf_put(&fifo, head);
	head = net_buf_get_timeout(&fifo, 0, TICKS_NONE);

	destroy_called = 0;
	net_buf_unref(head);

	if (destroy_called != ARRAY_SIZE(bufs_pool)) {
		printk("Incorrect fragment destroy callback count: %d\n",
		       destroy_called);
		return false;
	}

	return true;
}

static void test_3_fiber(int arg1, int arg2)
{
	struct nano_fifo *fifo = (struct nano_fifo *)arg1;
	struct nano_sem *sema = (struct nano_sem *)arg2;
	struct net_buf *buf;

	nano_sem_give(sema);

	buf = net_buf_get_timeout(fifo, 0, TEST_TIMEOUT);
	if (!buf) {
		printk("test_3_fiber: Unable to get initial buffer\n");
		return;
	}

	DBG("Got buffer %p from FIFO\n", buf);

	destroy_called = 0;
	net_buf_unref(buf);

	if (destroy_called != ARRAY_SIZE(bufs_pool)) {
		printk("Incorrect fragment destroy callback count: %d "
		       "should be %d\n", destroy_called,
		       ARRAY_SIZE(bufs_pool));
		return;
	}

	nano_sem_give(sema);
}

static bool net_buf_test_3(void)
{
	static char __stack test_3_fiber_stack[1024];
	struct net_buf *frag, *head;
	struct nano_fifo fifo;
	struct nano_sem sema;
	int i;

	head = net_buf_get_timeout(&bufs_fifo, 0, TICKS_NONE);
	if (!head) {
		printk("Failed to get fragment list head!\n");
		return false;
	}

	DBG("Fragment list head %p\n", head);

	frag = head;
	for (i = 0; i < ARRAY_SIZE(bufs_pool) - 1; i++) {
		frag->frags = net_buf_get_timeout(&bufs_fifo, 0, TICKS_NONE);
		if (!frag->frags) {
			printk("Failed to get fragment!\n");
			return false;
		}
		DBG("%p -> %p\n", frag, frag->frags);
		frag = frag->frags;
	}

	DBG("%p -> %p\n", frag, frag->frags);

	nano_fifo_init(&fifo);
	nano_sem_init(&sema);

	fiber_start(test_3_fiber_stack, sizeof(test_3_fiber_stack),
		    test_3_fiber, (int)&fifo, (int)&sema, 7, 0);

	if (!nano_sem_take(&sema, TEST_TIMEOUT)) {
		printk("Timeout 1 while waiting for semaphore\n");
		return false;
	}

	DBG("calling net_buf_put\n");

	net_buf_put(&fifo, head);

	if (!nano_sem_take(&sema, TEST_TIMEOUT)) {
		printk("Timeout 2 while waiting for semaphore\n");
		return false;
	}

	return true;
}

static bool net_buf_test_4(void)
{
	struct net_buf *frags[ARRAY_SIZE(frags_pool)];
	struct net_buf *buf, *frag;
	int i, removed;

	DBG("sizeof(struct net_buf) = %u\n", sizeof(struct net_buf));
	DBG("sizeof(frags_pool)     = %u\n", sizeof(frags_pool));

	net_buf_pool_init(no_data_buf_pool);
	net_buf_pool_init(frags_pool);

	/* Create a buf that does not have any data to store, it just
	 * contains link to fragments.
	 */
	buf = net_buf_get(&no_data_buf_fifo, 0);

	if (buf->size != 0) {
		DBG("Invalid buf size %d\n", buf->size);
		return false;
	}

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

	if (frag->user_data_size != 0) {
		DBG("Invalid user data size %d\n", frag->user_data_size);
		return false;
	}

	i = 0;
	while (frag) {
		frag = frag->frags;
		i++;
	}

	if (i != ARRAY_SIZE(frags_pool)) {
		DBG("Incorrect fragment count: %d vs %d\n",
		    i, ARRAY_SIZE(frags_pool));
		return false;
	}

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

	if ((i + removed) != ARRAY_SIZE(frags_pool)) {
		DBG("Incorrect removed fragment count: %d vs %d\n",
		       i + removed, ARRAY_SIZE(frags_pool));
		return false;
	}

	removed = 0;

	while (buf->frags) {
		struct net_buf *frag = buf->frags;

		net_buf_frag_del(buf, frag);
		net_buf_unref(frag);
		removed++;
	}

	if (removed != i) {
		DBG("Not all fragments were removed: %d vs %d\n",
		       i, removed);
		return false;
	}

	if (frag_destroy_called != ARRAY_SIZE(frags_pool)) {
		DBG("Incorrect frag destroy callback count: %d vs %d\n",
		    frag_destroy_called, ARRAY_SIZE(frags_pool));
		return false;
	}

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

	if (i != ARRAY_SIZE(frags_pool)) {
		DBG("Incorrect fragment count: %d vs %d\n",
		    i, ARRAY_SIZE(frags_pool));
		return false;
	}

	frag_destroy_called = 0;

	net_buf_unref(buf);

	if (frag_destroy_called != 0) {
		DBG("Wrong frag destroy callback count\n");
		return false;
	}

	for (i = 0; i < ARRAY_SIZE(frags_pool); i++) {
		net_buf_unref(frags[i]);
	}

	if (frag_destroy_called != ARRAY_SIZE(frags_pool)) {
		DBG("Incorrect frag destroy count: %d vs %d\n",
		    frag_destroy_called, ARRAY_SIZE(frags_pool));
		return false;
	}

	return true;
}

static bool net_buf_test_big_buf(void)
{
	struct net_buf *big_frags[ARRAY_SIZE(big_pool)];
	struct net_buf *buf, *frag;
	struct ipv6_hdr *ipv6;
	struct udp_hdr *udp;
	int i, len;

	DBG("sizeof(big_pool)       = %u\n", sizeof(big_pool));
	net_buf_pool_init(big_pool);

	frag_destroy_called = 0;

	/* Example of one big fragment that can hold full IPv6 packet */
	buf = net_buf_get(&no_data_buf_fifo, 0);

	/* We reserve some space in front of the buffer for protocol
	 * headers (IPv6 + UDP). Link layer headers are ignored in
	 * this example.
	 */
#define PROTO_HEADERS (sizeof(struct ipv6_hdr) + sizeof(struct udp_hdr))
	frag = net_buf_get(&big_frags_fifo, PROTO_HEADERS);
	big_frags[0] = frag;

	DBG("There is %d bytes available for user data.\n",
	    net_buf_tailroom(frag));

	/* First add some application data */
	len = strlen(example_data);
	for (i = 0; i < 2; i++) {
		DBG("Adding data: %s\n", example_data);
		if (net_buf_tailroom(frag) < len) {
			printk("No enough space in the buffer\n");
			return -1;
		}
		memcpy(net_buf_add(frag, len), example_data, len);
	}
	DBG("Full data (len %d): %s\n", frag->len, frag->data);

	ipv6 = (struct ipv6_hdr *)(frag->data - net_buf_headroom(frag));
	udp = (struct udp_hdr *)((void *)ipv6 + sizeof(*ipv6));

	DBG("IPv6 hdr starts %p, UDP hdr starts %p, user data %p\n",
	    ipv6, udp, frag->data);

	net_buf_frag_add(buf, frag);
	net_buf_unref(buf);

	if (frag_destroy_called != 0) {
		printk("Wrong big frag destroy callback count\n");
		return false;
	}

	for (i = 0; i < ARRAY_SIZE(big_pool); i++) {
		net_buf_unref(big_frags[i]);
	}

	if (frag_destroy_called != ARRAY_SIZE(big_pool)) {
		printk("Incorrect big frag destroy count: %d vs %d\n",
		       frag_destroy_called, ARRAY_SIZE(big_pool));
		return false;
	}

	return true;
}

static bool net_buf_test_multi_frags(void)
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

	DBG("There is %d bytes available for user data in %d buffers.\n",
	    avail, i);

	/* First add some application data */
	len = strlen(example_data);
	for (i = 0; i < ARRAY_SIZE(frags_pool) - 1; i++) {
		DBG("Adding data: %s\n", example_data);
		if (net_buf_tailroom(frags[i]) < len) {
			printk("No enough space in the buffer\n");
			return false;
		}
		memcpy(net_buf_add(frags[i], len), example_data, len);
		occupied += frags[i]->len;
	}
	DBG("Full data len %d\n", occupied);

	ipv6 = (struct ipv6_hdr *)(frags[i]->data - net_buf_headroom(frags[i]));
	udp = (struct udp_hdr *)((void *)ipv6 + sizeof(*ipv6));

	DBG("Frag %p IPv6 hdr starts %p, UDP hdr starts %p\n",
	    frags[i], ipv6, udp);
	for (i = 0; i < ARRAY_SIZE(frags_pool); i++) {
		DBG("[%d] frag %p user data (%d) at %p\n",
		       i, frags[i], frags[i]->len, frags[i]->data);
	}

	net_buf_unref(buf);

	if (frag_destroy_called != 0) {
		printk("Wrong big frag destroy callback count\n");
		return false;
	}

	for (i = 0; i < ARRAY_SIZE(frags_pool); i++) {
		net_buf_unref(frags[i]);
	}

	if (frag_destroy_called != ARRAY_SIZE(frags_pool)) {
		printk("Incorrect big frag destroy count: %d vs %d\n",
		       frag_destroy_called, ARRAY_SIZE(frags_pool));
		return false;
	}

	return true;
}

static const struct {
	const char *name;
	bool (*func)(void);
} tests[] = {
	{ "Test 1", net_buf_test_1, },
	{ "Test 2", net_buf_test_2, },
	{ "Test 3", net_buf_test_3, },
	{ "Test 4", net_buf_test_4, },
	{ "Test 5 big buf", net_buf_test_big_buf, },
	{ "Test 6 multi frag", net_buf_test_multi_frags, },
};

void main(void)
{
	int count, pass;

	printk("sizeof(struct net_buf) = %u\n", sizeof(struct net_buf));
	printk("sizeof(bufs_pool)      = %u\n", sizeof(bufs_pool));

	net_buf_pool_init(bufs_pool);

	for (count = 0, pass = 0; count < ARRAY_SIZE(tests); count++) {
		printk("Running %s... ", tests[count].name);
		if (!tests[count].func()) {
			printk("failed!\n");
		} else {
			printk("passed!\n");
			pass++;
		}
	}

	printk("%d / %d tests passed\n", pass, count);
}
