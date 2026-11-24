/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest_assert.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/random/random.h>

#include <zephyr/ztest.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_PKT_LOG_LEVEL);

#include "net_private.h"

static uint8_t mac_addr[sizeof(struct net_eth_addr)];
static struct net_if *eth_if;
static uint8_t small_buffer[CONFIG_NET_BUF_DATA_SIZE * 2 + 3];

/************************\
 * FAKE ETHERNET DEVICE *
\************************/

static void fake_dev_iface_init(struct net_if *iface)
{
	if (mac_addr[2] == 0U) {
		/* 00-00-5E-00-53-xx Documentation RFC 7042 */
		mac_addr[0] = 0x00;
		mac_addr[1] = 0x00;
		mac_addr[2] = 0x5E;
		mac_addr[3] = 0x00;
		mac_addr[4] = 0x53;
		mac_addr[5] = sys_rand8_get();
	}

	net_if_set_link_addr(iface, mac_addr, 6, NET_LINK_ETHERNET);

	eth_if = iface;
}

static int fake_dev_send(const struct device *dev, struct net_pkt *pkt)
{
	return 0;
}

int fake_dev_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

#if defined(CONFIG_NET_L2_ETHERNET)
static DEVICE_API(ethernet, fake_dev_api) = {
	.iface_api.init = fake_dev_iface_init,
	.send = fake_dev_send,
};

#define _ETH_L2_LAYER ETHERNET_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(ETHERNET_L2)
#define L2_HDR_SIZE sizeof(struct net_eth_hdr)
#else
static DEVICE_API(dummy, fake_dev_api) = {
	.iface_api.init = fake_dev_iface_init,
	.send = fake_dev_send,
};

#define _ETH_L2_LAYER DUMMY_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(DUMMY_L2)
#define L2_HDR_SIZE 0
#endif

NET_DEVICE_INIT(fake_dev, "fake_dev",
		fake_dev_init, NULL, NULL, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&fake_dev_api, _ETH_L2_LAYER, _ETH_L2_CTX_TYPE,
		NET_ETH_MTU);

/*********************\
 * UTILITY FUNCTIONS *
\*********************/

static bool pkt_is_of_size(struct net_pkt *pkt, size_t size)
{
	return (net_pkt_available_buffer(pkt) == size);
}

static void pkt_print_cursor(struct net_pkt *pkt)
{
	if (!pkt || !pkt->cursor.buf || !pkt->cursor.pos) {
		printk("Unknown position\n");
	} else {
		printk("Position %zu (%p) in net_buf %p (data %p)\n",
		       pkt->cursor.pos - pkt->cursor.buf->data,
		       pkt->cursor.pos, pkt->cursor.buf,
		       pkt->cursor.buf->data);
	}
}


/*****************************\
 * HOW TO ALLOCATE - 2 TESTS *
\*****************************/
ZTEST(net_pkt_test_suite, test_net_pkt_allocate_wo_buffer)
{
	struct net_pkt *pkt;

	/* How to allocate a packet, with no buffer */
	pkt = net_pkt_alloc(K_NO_WAIT);
	zassert_true(pkt != NULL, "Pkt not allocated");

	/* Freeing the packet */
	net_pkt_unref(pkt);
	zassert_true(atomic_get(&pkt->atomic_ref) == 0,
		     "Pkt not properly unreferenced");

	/* Note that, if you already know the iface to which the packet
	 * belongs to, you will be able to use net_pkt_alloc_on_iface().
	 */
	pkt = net_pkt_alloc_on_iface(eth_if, K_NO_WAIT);
	zassert_true(pkt != NULL, "Pkt not allocated");

	net_pkt_unref(pkt);
	zassert_true(atomic_get(&pkt->atomic_ref) == 0,
		     "Pkt not properly unreferenced");
}

ZTEST(net_pkt_test_suite, test_net_pkt_allocate_with_buffer)
{
	struct net_pkt *pkt;

	/* How to allocate a packet, with buffer
	 * a) - with a size that will fit MTU, let's say 512 bytes
	 * Note: we don't care of the family/protocol for now
	 */
	pkt = net_pkt_alloc_with_buffer(eth_if, 512,
					NET_AF_UNSPEC, 0, K_NO_WAIT);
	zassert_true(pkt != NULL, "Pkt not allocated");

	/* Did we get the requested size? */
	zassert_true(pkt_is_of_size(pkt, 512), "Pkt size is not right");

	/* Freeing the packet */
	net_pkt_unref(pkt);
	zassert_true(atomic_get(&pkt->atomic_ref) == 0,
		     "Pkt not properly unreferenced");

	/*
	 * b) - with a size that will not fit MTU, let's say 1800 bytes
	 * Note: again we don't care of family/protocol for now.
	 */
	pkt = net_pkt_alloc_with_buffer(eth_if, 1800,
					NET_AF_UNSPEC, 0, K_NO_WAIT);
	zassert_true(pkt != NULL, "Pkt not allocated");

	zassert_false(pkt_is_of_size(pkt, 1800), "Pkt size is not right");
	zassert_true(pkt_is_of_size(pkt, net_if_get_mtu(eth_if) + L2_HDR_SIZE),
		     "Pkt size is not right");

	/* Freeing the packet */
	net_pkt_unref(pkt);
	zassert_true(atomic_get(&pkt->atomic_ref) == 0,
		     "Pkt not properly unreferenced");

	/*
	 * c) - Now with 512 bytes but on IPv4/UDP
	 */
	pkt = net_pkt_alloc_with_buffer(eth_if, 512, NET_AF_INET,
					NET_IPPROTO_UDP, K_NO_WAIT);
	zassert_true(pkt != NULL, "Pkt not allocated");

	/* Because 512 + NET_IPV4UDPH_LEN fits MTU, total must be that one */
	zassert_true(pkt_is_of_size(pkt, 512 + NET_IPV4UDPH_LEN),
		     "Pkt overall size does not match");

	/* Freeing the packet */
	net_pkt_unref(pkt);
	zassert_true(atomic_get(&pkt->atomic_ref) == 0,
		     "Pkt not properly unreferenced");

	/*
	 * c) - Now with 1800 bytes but on IPv4/UDP
	 */
	pkt = net_pkt_alloc_with_buffer(eth_if, 1800, NET_AF_INET,
					NET_IPPROTO_UDP, K_NO_WAIT);
	zassert_true(pkt != NULL, "Pkt not allocated");

	/* Because 1800 + NET_IPV4UDPH_LEN won't fit MTU, payload size
	 * should be MTU
	 */
	zassert_true(net_pkt_available_buffer(pkt) ==
		     net_if_get_mtu(eth_if),
		     "Payload buf size does not match for ipv4/udp");

	/* Freeing the packet */
	net_pkt_unref(pkt);
	zassert_true(atomic_get(&pkt->atomic_ref) == 0,
		     "Pkt not properly unreferenced");

	/* d) - with a zero payload but NET_AF_INET family
	 */
	pkt = net_pkt_alloc_with_buffer(eth_if, 0,
					NET_AF_INET, 0, K_NO_WAIT);
	zassert_true(pkt != NULL, "Pkt not allocated");

	/* Did we get the requested size? */
	zassert_true(pkt_is_of_size(pkt, NET_IPV4H_LEN),
		     "Pkt size is not right");

	/* Freeing the packet */
	net_pkt_unref(pkt);
	zassert_true(atomic_get(&pkt->atomic_ref) == 0,
		     "Pkt not properly unreferenced");

	/* e) - with a zero payload but NET_AF_PACKET family
	 */
	pkt = net_pkt_alloc_with_buffer(eth_if, 0,
					NET_AF_PACKET, 0, K_NO_WAIT);
	zassert_true(pkt != NULL, "Pkt not allocated");

	/* Did we get the requested size? */
	zassert_true(pkt_is_of_size(pkt, 0), "Pkt size is not right");

	/* Freeing the packet */
	net_pkt_unref(pkt);
	zassert_true(atomic_get(&pkt->atomic_ref) == 0,
		     "Pkt not properly unreferenced");
}

/********************************\
 * HOW TO R/W A PACKET -  TESTS *
\********************************/

ZTEST(net_pkt_test_suite, test_net_pkt_basics_of_rw)
{
	struct net_pkt_cursor backup;
	struct net_pkt *pkt;
	uint16_t value16;
	int ret;

	pkt = net_pkt_alloc_with_buffer(eth_if, 512,
					NET_AF_UNSPEC, 0, K_NO_WAIT);
	zassert_true(pkt != NULL, "Pkt not allocated");

	/* Once newly allocated with buffer,
	 * a packet has no data accounted for in its buffer
	 */
	zassert_true(net_pkt_get_len(pkt) == 0,
		     "Pkt initial length should be 0");

	/* This is done through net_buf which can distinguish
	 * the size of a buffer from the length of the data in it.
	 */

	/* Let's subsequently write 1 byte, then 2 bytes and 4 bytes
	 * We write values made of 0s
	 */
	ret = net_pkt_write_u8(pkt, 0);
	zassert_true(ret == 0, "Pkt write failed");

	/* Length should be 1 now */
	zassert_true(net_pkt_get_len(pkt) == 1, "Pkt length mismatch");

	ret = net_pkt_write_be16(pkt, 0);
	zassert_true(ret == 0, "Pkt write failed");

	/* Length should be 3 now */
	zassert_true(net_pkt_get_len(pkt) == 3, "Pkt length mismatch");

	/* Verify that the data is properly written to net_buf */
	net_pkt_cursor_backup(pkt, &backup);
	net_pkt_cursor_init(pkt);
	net_pkt_set_overwrite(pkt, true);
	net_pkt_skip(pkt, 1);
	net_pkt_read_be16(pkt, &value16);
	zassert_equal(value16, 0, "Invalid value %d read, expected %d",
		      value16, 0);

	/* Then write new value, overwriting the old one */
	net_pkt_cursor_init(pkt);
	net_pkt_skip(pkt, 1);
	ret = net_pkt_write_be16(pkt, 42);
	zassert_true(ret == 0, "Pkt write failed");

	/* And re-read the value again */
	net_pkt_cursor_init(pkt);
	net_pkt_skip(pkt, 1);
	ret = net_pkt_read_be16(pkt, &value16);
	zassert_true(ret == 0, "Pkt read failed");
	zassert_equal(value16, 42, "Invalid value %d read, expected %d",
		      value16, 42);

	net_pkt_set_overwrite(pkt, false);
	net_pkt_cursor_restore(pkt, &backup);

	ret = net_pkt_write_be32(pkt, 0);
	zassert_true(ret == 0, "Pkt write failed");

	/* Length should be 7 now */
	zassert_true(net_pkt_get_len(pkt) == 7, "Pkt length mismatch");

	/* All these writing functions use net_ptk_write(), which works
	 * this way:
	 */
	ret = net_pkt_write(pkt, small_buffer, 9);
	zassert_true(ret == 0, "Pkt write failed");

	/* Length should be 16 now */
	zassert_true(net_pkt_get_len(pkt) == 16, "Pkt length mismatch");

	/* Now let's say you want to memset some data */
	ret = net_pkt_memset(pkt, 0, 4);
	zassert_true(ret == 0, "Pkt memset failed");

	/* Length should be 20 now */
	zassert_true(net_pkt_get_len(pkt) == 20, "Pkt length mismatch");

	/* So memset affects the length exactly as write does */

	/* Sometimes you might want to advance in the buffer without caring
	 * what's written there since you'll eventually come back for that.
	 * net_pkt_skip() is used for it.
	 * Note: usually you will not have to use that function a lot yourself.
	 */
	ret = net_pkt_skip(pkt, 20);
	zassert_true(ret == 0, "Pkt skip failed");

	/* Length should be 40 now */
	zassert_true(net_pkt_get_len(pkt) == 40, "Pkt length mismatch");

	/* Again, skip affected the length also, like a write
	 * But wait a minute: how to get back then, in order to write at
	 * the position we just skipped?
	 *
	 * So let's introduce the concept of buffer cursor. (which could
	 * be named 'cursor' if such name has more relevancy. Basically, each
	 * net_pkt embeds such 'cursor': it's like a head of a tape
	 * recorder/reader, it holds the current position in the buffer where
	 * you can r/w. All operations use and update it below.
	 * There is, however, a catch: buffer is described through net_buf
	 * and these are like a simple linked-list.
	 * Which means that unlike a tape recorder/reader: you are not
	 * able to go backward. Only back from starting point and forward.
	 * Thus why there is a net_pkt_cursor_init(pkt) which will let you going
	 * back from the start. We could hold more info in order to avoid that,
	 * but that would mean growing each an every net_buf.
	 */
	net_pkt_cursor_init(pkt);

	/* But isn't it so that if I want to go at the previous position I
	 * skipped, I'll use skip again but then won't it affect again the
	 * length?
	 * Answer is yes. Hopefully there is a mean to avoid that. Basically
	 * for data that already "exists" in the buffer (aka: data accounted
	 * for in the buffer, through the length) you'll need to set the packet
	 * to overwrite: all subsequent operations will then work on existing
	 * data and will not affect the length (it won't add more data)
	 */
	net_pkt_set_overwrite(pkt, true);

	zassert_true(net_pkt_is_being_overwritten(pkt),
		     "Pkt is not set to overwrite");

	/* Ok so previous skipped position was at offset 20 */
	ret = net_pkt_skip(pkt, 20);
	zassert_true(ret == 0, "Pkt skip failed");

	/* Length should _still_ be 40 */
	zassert_true(net_pkt_get_len(pkt) == 40, "Pkt length mismatch");

	/* And you can write stuff */
	ret = net_pkt_write_le32(pkt, 0);
	zassert_true(ret == 0, "Pkt write failed");

	/* Again, length should _still_ be 40 */
	zassert_true(net_pkt_get_len(pkt) == 40, "Pkt length mismatch");

	/* Let's memset the rest */
	ret = net_pkt_memset(pkt, 0, 16);
	zassert_true(ret == 0, "Pkt memset failed");

	/* Again, length should _still_ be 40 */
	zassert_true(net_pkt_get_len(pkt) == 40, "Pkt length mismatch");

	/* We are now back at the end of the existing data in the buffer
	 * Since overwrite is still on, we should not be able to r/w
	 * anything.
	 * This is completely nominal, as being set, overwrite allows r/w only
	 * on existing data in the buffer:
	 */
	ret = net_pkt_write_be32(pkt, 0);
	zassert_true(ret != 0, "Pkt write succeeded where it shouldn't have");

	/* Logically, in order to be able to add new data in the buffer,
	 * overwrite should be disabled:
	 */
	net_pkt_set_overwrite(pkt, false);

	/* But it will fail: */
	ret = net_pkt_write_le32(pkt, 0);
	zassert_true(ret != 0, "Pkt write succeeded?");

	/* Why is that?
	 * This is because in case of r/w error: the iterator is invalidated.
	 * This a design choice, once you get a r/w error it means your code
	 * messed up requesting smaller buffer than you actually needed, or
	 * writing too much data than it should have been etc...).
	 * So you must drop your packet entirely.
	 */

	/* Freeing the packet */
	net_pkt_unref(pkt);
	zassert_true(atomic_get(&pkt->atomic_ref) == 0,
		     "Pkt not properly unreferenced");
}

ZTEST(net_pkt_test_suite, test_net_pkt_advanced_basics)
{
	struct net_pkt_cursor backup;
	struct net_pkt *pkt;
	int ret;

	pkt = net_pkt_alloc_with_buffer(eth_if, 512,
					NET_AF_INET, NET_IPPROTO_UDP, K_NO_WAIT);
	zassert_true(pkt != NULL, "Pkt not allocated");

	pkt_print_cursor(pkt);

	/* As stated earlier, initializing the cursor, is the way to go
	 * back from the start in the buffer (either header or payload then).
	 * We also showed that using net_pkt_skip() could be used to move
	 * forward in the buffer.
	 * But what if you are far in the buffer, you need to go backward,
	 * and back again to your previous position?
	 * You could certainly do:
	 */
	ret = net_pkt_write(pkt, small_buffer, 20);
	zassert_true(ret == 0, "Pkt write failed");

	pkt_print_cursor(pkt);

	net_pkt_cursor_init(pkt);

	pkt_print_cursor(pkt);

	/* ... do something here ... */

	/* And finally go back with overwrite/skip: */
	net_pkt_set_overwrite(pkt, true);
	ret = net_pkt_skip(pkt, 20);
	zassert_true(ret == 0, "Pkt skip failed");
	net_pkt_set_overwrite(pkt, false);

	pkt_print_cursor(pkt);

	/* In this example, do not focus on the 20 bytes. It is just for
	 * the sake of the example.
	 * The other method is backup/restore the packet cursor.
	 */
	net_pkt_cursor_backup(pkt, &backup);

	net_pkt_cursor_init(pkt);

	/* ... do something here ... */

	/* and restore: */
	net_pkt_cursor_restore(pkt, &backup);

	pkt_print_cursor(pkt);

	/* Another feature, is how you access your data. Earlier was
	 * presented basic r/w functions. But sometime you might want to
	 * access your data directly through a structure/type etc...
	 * Due to the "fragmented" possible nature of your buffer, you
	 * need to know if the data you are trying to access is in
	 * contiguous area.
	 * For this, you'll use:
	 */
	ret = (int) net_pkt_is_contiguous(pkt, 4);
	zassert_true(ret == 1, "Pkt contiguity check failed");

	/* If that's successful you should be able to get the actual
	 * position in the buffer and cast it to the type you want.
	 */
	{
		uint32_t *val = (uint32_t *)net_pkt_cursor_get_pos(pkt);

		*val = 0U;
		/* etc... */
	}

	/* However, to advance your cursor, since none of the usual r/w
	 * functions got used: net_pkt_skip() should be called relevantly:
	 */
	net_pkt_skip(pkt, 4);

	/* Freeing the packet */
	net_pkt_unref(pkt);
	zassert_true(atomic_get(&pkt->atomic_ref) == 0,
		     "Pkt not properly unreferenced");

	/* Obviously one will very rarely use these 2 last low level functions
	 * - net_pkt_is_contiguous()
	 * - net_pkt_cursor_update()
	 *
	 * Let's see why next.
	 */
}

ZTEST(net_pkt_test_suite, test_net_pkt_easier_rw_usage)
{
	struct net_pkt *pkt;
	int ret;

	pkt = net_pkt_alloc_with_buffer(eth_if, 512,
					NET_AF_INET, NET_IPPROTO_UDP, K_NO_WAIT);
	zassert_true(pkt != NULL, "Pkt not allocated");

	/* In net core, all goes down in fine to header manipulation.
	 * Either it's an IP header, UDP, ICMP, TCP one etc...
	 * One would then prefer to access those directly via there
	 * descriptors (struct net_udp_hdr, struct net_icmp_hdr, ...)
	 * rather than building it byte by bytes etc...
	 *
	 * As seen earlier, it is possible to cast on current position.
	 * However, due to the "fragmented" possible nature of the buffer,
	 * it should also be possible to handle the case the data being
	 * accessed is scattered on 1+ net_buf.
	 *
	 * To avoid redoing the contiguity check, cast or copy on failure,
	 * a complex type named struct net_pkt_header_access exists.
	 * It solves both cases (accessing data contiguous or not), without
	 * the need for runtime allocation (all is on stack)
	 */
	{
		NET_PKT_DATA_ACCESS_DEFINE(ip_access, struct net_ipv4_hdr);
		struct net_ipv4_hdr *ip_hdr;

		ip_hdr = (struct net_ipv4_hdr *)
			net_pkt_get_data(pkt, &ip_access);
		zassert_not_null(ip_hdr, "Accessor failed");

		ip_hdr->tos = 0x00;

		ret = net_pkt_set_data(pkt, &ip_access);
		zassert_true(ret == 0, "Accessor failed");

		zassert_true(net_pkt_get_len(pkt) == NET_IPV4H_LEN,
			     "Pkt length mismatch");
	}

	/* As you can notice: get/set take also care of handling the cursor
	 * and updating the packet length relevantly thus why packet length
	 * has properly grown.
	 */

	/* Freeing the packet */
	net_pkt_unref(pkt);
	zassert_true(atomic_get(&pkt->atomic_ref) == 0,
		     "Pkt not properly unreferenced");
}

uint8_t b5_data[10] = "qrstuvwxyz";
struct net_buf b5 = {
	.ref   = 1,
	.data  = b5_data,
	.len   = 0,
	.size  = 0,
	.__buf  = b5_data,
};

uint8_t b4_data[4] = "mnop";
struct net_buf b4 = {
	.frags = &b5,
	.ref   = 1,
	.data  = b4_data,
	.len   = sizeof(b4_data) - 2,
	.size  = sizeof(b4_data),
	.__buf  = b4_data,
};

struct net_buf b3 = {
	.frags = &b4,
	.ref   = 1,
	.data  = NULL,
	.__buf  = NULL,
};

uint8_t b2_data[8] = "efghijkl";
struct net_buf b2 = {
	.frags = &b3,
	.ref   = 1,
	.data  = b2_data,
	.len   = 0,
	.size  = sizeof(b2_data),
	.__buf  = b2_data,
};

uint8_t b1_data[4] = "abcd";
struct net_buf b1 = {
	.frags = &b2,
	.ref   = 1,
	.data  = b1_data,
	.len   = sizeof(b1_data) - 2,
	.size  = sizeof(b1_data),
	.__buf  = b1_data,
};

ZTEST(net_pkt_test_suite, test_net_pkt_copy)
{
	struct net_pkt *pkt_src;
	struct net_pkt *pkt_dst;

	pkt_src = net_pkt_alloc_on_iface(eth_if, K_NO_WAIT);
	zassert_true(pkt_src != NULL, "Pkt not allocated");

	pkt_print_cursor(pkt_src);

	/* Let's append the buffers */
	net_pkt_append_buffer(pkt_src, &b1);

	net_pkt_set_overwrite(pkt_src, true);

	/* There should be some space left */
	zassert_true(net_pkt_available_buffer(pkt_src) != 0, "No space left?");
	/* Length should be 4 */
	zassert_true(net_pkt_get_len(pkt_src) == 4, "Wrong length");

	/* Actual space left is 12 (in b1, b2 and b4) */
	zassert_true(net_pkt_available_buffer(pkt_src) == 12,
		     "Wrong space left?");

	pkt_print_cursor(pkt_src);

	/* Now let's clone the pkt
	 * This will test net_pkt_copy_new() as it uses it for the buffers
	 */
	pkt_dst = net_pkt_clone(pkt_src, K_NO_WAIT);
	zassert_true(pkt_dst != NULL, "Pkt not clone");

	/* Cloning does not take into account left space,
	 * but only occupied one
	 */
	zassert_true(net_pkt_available_buffer(pkt_dst) == 0, "Space left");
	zassert_true(net_pkt_get_len(pkt_src) == net_pkt_get_len(pkt_dst),
		     "Not same amount?");

	/* It also did not care to copy the net_buf itself, only the content
	 * so, knowing that the base buffer size is bigger than necessary,
	 * pkt_dst has only one net_buf
	 */
	zassert_true(pkt_dst->buffer->frags == NULL, "Not only one buffer?");

	/* Freeing the packet */
	pkt_src->buffer = NULL;
	net_pkt_unref(pkt_src);
	zassert_true(atomic_get(&pkt_src->atomic_ref) == 0,
		     "Pkt not properly unreferenced");
	net_pkt_unref(pkt_dst);
	zassert_true(atomic_get(&pkt_dst->atomic_ref) == 0,
		     "Pkt not properly unreferenced");
}

#define PULL_TEST_PKT_DATA_SIZE 600

ZTEST(net_pkt_test_suite, test_net_pkt_pull)
{
	const int PULL_AMOUNT = 8;
	const int LARGE_PULL_AMOUNT = 200;
	struct net_pkt *dummy_pkt;
	static uint8_t pkt_data[PULL_TEST_PKT_DATA_SIZE];
	static uint8_t pkt_data_readback[PULL_TEST_PKT_DATA_SIZE];
	size_t len;
	int i, ret;

	for (i = 0; i < PULL_TEST_PKT_DATA_SIZE; ++i) {
		pkt_data[i] = i & 0xff;
	}

	dummy_pkt = net_pkt_alloc_with_buffer(eth_if,
					      PULL_TEST_PKT_DATA_SIZE,
					      NET_AF_UNSPEC,
					      0,
					      K_NO_WAIT);
	zassert_true(dummy_pkt != NULL, "Pkt not allocated");

	zassert_true(net_pkt_write(dummy_pkt,
				   pkt_data,
				   PULL_TEST_PKT_DATA_SIZE) == 0,
		     "Write packet failed");

	net_pkt_cursor_init(dummy_pkt);
	net_pkt_pull(dummy_pkt, PULL_AMOUNT);
	zassert_equal(net_pkt_get_len(dummy_pkt),
		      PULL_TEST_PKT_DATA_SIZE - PULL_AMOUNT,
		      "Pull failed to set new size");
	zassert_true(net_pkt_read(dummy_pkt,
				  pkt_data_readback,
				  PULL_TEST_PKT_DATA_SIZE - PULL_AMOUNT) == 0,
		     "Read packet failed");
	zassert_mem_equal(pkt_data_readback,
			  &pkt_data[PULL_AMOUNT],
			  PULL_TEST_PKT_DATA_SIZE - PULL_AMOUNT,
			  "Packet data changed");

	net_pkt_cursor_init(dummy_pkt);
	net_pkt_pull(dummy_pkt, LARGE_PULL_AMOUNT);
	zassert_equal(net_pkt_get_len(dummy_pkt),
		      PULL_TEST_PKT_DATA_SIZE - PULL_AMOUNT -
		      LARGE_PULL_AMOUNT,
		      "Large pull failed to set new size (%zu vs %d)",
		      net_pkt_get_len(dummy_pkt),
		      PULL_TEST_PKT_DATA_SIZE - PULL_AMOUNT -
		      LARGE_PULL_AMOUNT);

	net_pkt_cursor_init(dummy_pkt);
	net_pkt_pull(dummy_pkt, net_pkt_get_len(dummy_pkt));
	zassert_equal(net_pkt_get_len(dummy_pkt), 0,
		      "Full pull failed to set new size (%zu)",
		      net_pkt_get_len(dummy_pkt));

	net_pkt_cursor_init(dummy_pkt);
	ret = net_pkt_pull(dummy_pkt, 1);
	zassert_equal(ret, -ENOBUFS, "Did not return error");
	zassert_equal(net_pkt_get_len(dummy_pkt), 0,
		      "Empty pull set new size (%zu)",
		      net_pkt_get_len(dummy_pkt));

	net_pkt_unref(dummy_pkt);

	dummy_pkt = net_pkt_alloc_with_buffer(eth_if,
					      PULL_TEST_PKT_DATA_SIZE,
					      NET_AF_UNSPEC,
					      0,
					      K_NO_WAIT);
	zassert_true(dummy_pkt != NULL, "Pkt not allocated");

	zassert_true(net_pkt_write(dummy_pkt,
				   pkt_data,
				   PULL_TEST_PKT_DATA_SIZE) == 0,
		     "Write packet failed");

	net_pkt_cursor_init(dummy_pkt);
	ret = net_pkt_pull(dummy_pkt, net_pkt_get_len(dummy_pkt) + 1);
	zassert_equal(ret, -ENOBUFS, "Did not return error");
	zassert_equal(net_pkt_get_len(dummy_pkt), 0,
		      "Not empty after full pull (%zu)",
		      net_pkt_get_len(dummy_pkt));

	net_pkt_unref(dummy_pkt);

	dummy_pkt = net_pkt_alloc_with_buffer(eth_if,
					      PULL_TEST_PKT_DATA_SIZE,
					      NET_AF_UNSPEC,
					      0,
					      K_NO_WAIT);
	zassert_true(dummy_pkt != NULL, "Pkt not allocated");

	zassert_true(net_pkt_write(dummy_pkt,
				   pkt_data,
				   PULL_TEST_PKT_DATA_SIZE) == 0,
		     "Write packet failed");

	net_pkt_cursor_init(dummy_pkt);
	len = net_pkt_get_len(dummy_pkt);

	for (i = 0; i < len; i++) {
		ret = net_pkt_pull(dummy_pkt, 1);
		zassert_equal(ret, 0, "Did return error");
	}

	ret = net_pkt_pull(dummy_pkt, 1);
	zassert_equal(ret, -ENOBUFS, "Did not return error");

	zassert_equal(dummy_pkt->buffer, NULL, "buffer list not empty");

	net_pkt_unref(dummy_pkt);
}

/* Three buffers: two full ones and a partial third */
#define PULL_MID_TEST_PKT_DATA_SIZE ((2 * CONFIG_NET_BUF_DATA_SIZE) + 64)

/* Pulling from a cursor inside the packet removes exactly the requested bytes
 * at the cursor and keeps the data in front of it and after it, wherever the
 * pulled range starts and ends relative to the buffer boundaries.
 */
ZTEST(net_pkt_test_suite, test_net_pkt_pull_from_cursor)
{
	static uint8_t pkt_data[PULL_MID_TEST_PKT_DATA_SIZE];
	static uint8_t expected[PULL_MID_TEST_PKT_DATA_SIZE];
	static uint8_t readback[PULL_MID_TEST_PKT_DATA_SIZE];
	struct {
		size_t skip;
		size_t pull;
	} cases[8];
	struct net_pkt *pkt;
	size_t total = sizeof(pkt_data);
	size_t first_len;
	size_t second_len;
	size_t len;
	size_t i;
	int ret;

	for (i = 0; i < total; i++) {
		pkt_data[i] = (uint8_t)(i * 7U);
	}

	/* Find the buffer layout the allocator gives a packet of this size */
	pkt = net_pkt_alloc_with_buffer(eth_if, total, NET_AF_UNSPEC, 0, K_NO_WAIT);
	zassert_not_null(pkt, "Pkt not allocated");
	zassert_ok(net_pkt_write(pkt, pkt_data, total), "Write packet failed");
	zassert_not_null(pkt->buffer->frags, "Packet needs at least three buffers");
	zassert_not_null(pkt->buffer->frags->frags, "Packet needs at least three buffers");
	first_len = pkt->buffer->len;
	second_len = pkt->buffer->frags->len;
	net_pkt_unref(pkt);

	zassert_true(first_len > 16U, "First buffer too short (%zu)", first_len);
	zassert_true((first_len + second_len + 8U) < total, "Packet too short");

	/* Inside the first buffer */
	cases[0].skip = 8U;
	cases[0].pull = 8U;
	/* Up to the end of the first buffer */
	cases[1].skip = first_len - 8U;
	cases[1].pull = 8U;
	/* Across the end of the first buffer */
	cases[2].skip = first_len - 2U;
	cases[2].pull = 8U;
	/* The front of the second buffer */
	cases[3].skip = first_len;
	cases[3].pull = 8U;
	/* All of the second buffer */
	cases[4].skip = first_len;
	cases[4].pull = second_len;
	/* The tail of the first, all of the second and the front of the third */
	cases[5].skip = first_len - 4U;
	cases[5].pull = second_len + 8U;
	/* From the front of the second buffer to the end of the packet */
	cases[6].skip = first_len;
	cases[6].pull = total - first_len;
	/* From inside the first buffer to the end of the packet */
	cases[7].skip = first_len - 4U;
	cases[7].pull = total - first_len + 4U;

	ARRAY_FOR_EACH(cases, c) {
		size_t skip = cases[c].skip;
		size_t pull = cases[c].pull;

		memcpy(expected, pkt_data, skip);
		memcpy(expected + skip, pkt_data + skip + pull, total - skip - pull);

		pkt = net_pkt_alloc_with_buffer(eth_if, total, NET_AF_UNSPEC, 0, K_NO_WAIT);
		zassert_not_null(pkt, "Pkt not allocated");
		zassert_ok(net_pkt_write(pkt, pkt_data, total), "Write packet failed");

		net_pkt_cursor_init(pkt);
		net_pkt_set_overwrite(pkt, true);
		zassert_ok(net_pkt_skip(pkt, skip), "Skip failed");

		ret = net_pkt_pull(pkt, pull);
		zexpect_ok(ret, "Pull of %zu at %zu failed (%d)", pull, skip, ret);

		len = net_pkt_get_len(pkt);
		zexpect_equal(len, total - pull, "Pull of %zu at %zu left %zu bytes", pull, skip,
			      len);

		/* Compare what is there, so a short packet fails on content too */
		net_pkt_cursor_init(pkt);
		memset(readback, 0, sizeof(readback));
		len = MIN(len, total - pull);
		ret = net_pkt_read(pkt, readback, len);
		zexpect_ok(ret, "Read after pull of %zu at %zu failed", pull, skip);
		zexpect_mem_equal(readback, expected, total - pull,
				  "Pull of %zu at %zu changed other data", pull, skip);

		net_pkt_unref(pkt);
	}
}

ZTEST(net_pkt_test_suite, test_net_pkt_clone)
{
	uint8_t buf[26] = {"abcdefghijklmnopqrstuvwxyz"};
	struct net_pkt *pkt;
	struct net_pkt *cloned_pkt;
	int ret;

	pkt = net_pkt_alloc_with_buffer(eth_if, 64,
					NET_AF_UNSPEC, 0, K_NO_WAIT);
	zassert_true(pkt != NULL, "Pkt not allocated");

	ret = net_pkt_write(pkt, buf, sizeof(buf));
	zassert_true(ret == 0, "Pkt write failed");

	zassert_true(net_pkt_get_len(pkt) == sizeof(buf),
		     "Pkt length mismatch");

	net_pkt_cursor_init(pkt);
	net_pkt_set_overwrite(pkt, true);
	net_pkt_skip(pkt, 6);
	zassert_true(sizeof(buf) - 6 == net_pkt_remaining_data(pkt),
		     "Pkt remaining data mismatch");

	memcpy(net_pkt_lladdr_src(pkt)->addr,
	       pkt->buffer->data,
	       NET_LINK_ADDR_MAX_LENGTH);
	net_pkt_lladdr_src(pkt)->len = NET_LINK_ADDR_MAX_LENGTH;
	net_pkt_lladdr_src(pkt)->type = NET_LINK_ETHERNET;
	zassert_mem_equal(net_pkt_lladdr_src(pkt)->addr, buf, NET_LINK_ADDR_MAX_LENGTH);
	memcpy(net_pkt_lladdr_dst(pkt)->addr,
	       net_pkt_cursor_get_pos(pkt),
	       NET_LINK_ADDR_MAX_LENGTH);
	net_pkt_lladdr_dst(pkt)->len = NET_LINK_ADDR_MAX_LENGTH;
	net_pkt_lladdr_dst(pkt)->type = NET_LINK_ETHERNET;
	zassert_mem_equal(net_pkt_lladdr_dst(pkt)->addr, &buf[6], NET_LINK_ADDR_MAX_LENGTH);

	net_pkt_set_family(pkt, NET_AF_INET6);
	net_pkt_set_captured(pkt, true);
	net_pkt_set_eof(pkt, true);
	net_pkt_set_tx_timestamping(pkt, true);
	net_pkt_set_rx_timestamping(pkt, true);
	net_pkt_set_forwarding(pkt, true);

	net_pkt_set_l2_bridged(pkt, true);
	net_pkt_set_l2_processed(pkt, true);
	net_pkt_set_ll_proto_type(pkt, ETH_P_IEEE802154);

	net_pkt_set_overwrite(pkt, false);
	cloned_pkt = net_pkt_clone(pkt, K_NO_WAIT);
	zassert_true(cloned_pkt != NULL, "Pkt not cloned");

	zassert_true(net_pkt_get_len(cloned_pkt) == sizeof(buf),
		     "Cloned pkt length mismatch");

	zassert_true(sizeof(buf) - 6 == net_pkt_remaining_data(pkt),
		     "Pkt remaining data mismatch");

	zassert_true(sizeof(buf) - 6 == net_pkt_remaining_data(cloned_pkt),
		     "Cloned pkt remaining data mismatch");

	zassert_false(net_pkt_is_being_overwritten(cloned_pkt),
		     "Cloned pkt overwrite flag not restored");

	zassert_false(net_pkt_is_being_overwritten(pkt),
		     "Pkt overwrite flag not restored");

	zassert_equal(net_pkt_family(cloned_pkt), NET_AF_INET6,
		     "Address family value mismatch");

	zassert_true(net_pkt_is_captured(cloned_pkt),
		     "Cloned pkt captured flag mismatch");

	zassert_true(net_pkt_eof(cloned_pkt),
		     "Cloned pkt eof flag mismatch");

#if CONFIG_NET_PKT_TIMESTAMP
	zassert_true(net_pkt_is_tx_timestamping(cloned_pkt),
		     "Cloned pkt tx_timestamping flag mismatch");

	zassert_true(net_pkt_is_rx_timestamping(cloned_pkt),
		     "Cloned pkt rx_timestamping flag mismatch");
#endif

	zassert_true(net_pkt_forwarding(cloned_pkt),
		     "Cloned pkt forwarding flag mismatch");

	zassert_true(net_pkt_is_l2_bridged(cloned_pkt),
		     "Cloned pkt l2_bridged flag mismatch");

	zassert_true(net_pkt_is_l2_processed(cloned_pkt),
		     "Cloned pkt l2_processed flag mismatch");

	zassert_mem_equal(net_pkt_lladdr_src(cloned_pkt)->addr, buf, NET_LINK_ADDR_MAX_LENGTH);
	zassert_true(memcmp(net_pkt_lladdr_src(cloned_pkt)->addr,
			    cloned_pkt->buffer->data,
			    NET_LINK_ADDR_MAX_LENGTH) == 0,
		     "Cloned pkt ll src addr mismatch");

	zassert_mem_equal(net_pkt_lladdr_dst(cloned_pkt)->addr, &buf[6], NET_LINK_ADDR_MAX_LENGTH);
	zassert_true(memcmp(net_pkt_lladdr_dst(cloned_pkt)->addr,
			    net_pkt_cursor_get_pos(cloned_pkt),
			    NET_LINK_ADDR_MAX_LENGTH) == 0,
		     "Cloned pkt ll dst addr mismatch");

	zassert_equal(net_pkt_ll_proto_type(cloned_pkt), ETH_P_IEEE802154,
		     "Address ll_proto_type value mismatch");

	net_pkt_unref(pkt);
	net_pkt_unref(cloned_pkt);
}

/* A TX clone is allocated for the given interface: net_pkt_tx_clone() lets
 * the L2 reserve its header space, net_pkt_tx_clone_raw() copies the buffer
 * as is.
 */
ZTEST(net_pkt_test_suite, test_net_pkt_tx_clone)
{
	uint8_t buf[26] = {"abcdefghijklmnopqrstuvwxyz"};
	uint8_t read_buf[sizeof(buf)];
	struct net_pkt *cloned_pkt;
	struct net_pkt *pkt;
	int ret;

	pkt = net_pkt_rx_alloc_with_buffer(eth_if, sizeof(buf), NET_AF_UNSPEC, 0, K_NO_WAIT);
	zassert_not_null(pkt, "Pkt not allocated");

	ret = net_pkt_write(pkt, buf, sizeof(buf));
	zassert_equal(ret, 0, "Pkt write failed");

	cloned_pkt = net_pkt_tx_clone_raw(pkt, eth_if, K_NO_WAIT);
	zassert_not_null(cloned_pkt, "Pkt not cloned");
	zassert_equal(net_pkt_iface(cloned_pkt), eth_if, "Cloned pkt iface mismatch");
	zassert_equal(net_pkt_get_len(cloned_pkt), sizeof(buf), "Cloned pkt length mismatch");
	zassert_equal(net_buf_headroom(cloned_pkt->buffer), 0, "Raw clone has headroom");

	net_pkt_cursor_init(cloned_pkt);
	ret = net_pkt_read(cloned_pkt, read_buf, sizeof(read_buf));
	zassert_equal(ret, 0, "Cloned pkt read failed");
	zassert_mem_equal(read_buf, buf, sizeof(buf), "Cloned pkt data mismatch");

	net_pkt_unref(cloned_pkt);

	cloned_pkt = net_pkt_tx_clone(pkt, eth_if, K_NO_WAIT);
	zassert_not_null(cloned_pkt, "Pkt not cloned");
	zassert_equal(net_pkt_iface(cloned_pkt), eth_if, "Cloned pkt iface mismatch");
	zassert_equal(net_pkt_get_len(cloned_pkt), sizeof(buf), "Cloned pkt length mismatch");

	if (IS_ENABLED(CONFIG_NET_L2_ETHERNET_RESERVE_HEADER)) {
		zassert_true(net_buf_headroom(cloned_pkt->buffer) >= sizeof(struct net_eth_hdr),
			     "No link layer header space reserved");
	} else {
		zassert_equal(net_buf_headroom(cloned_pkt->buffer), 0, "Clone has headroom");
	}

	net_pkt_cursor_init(cloned_pkt);
	ret = net_pkt_read(cloned_pkt, read_buf, sizeof(read_buf));
	zassert_equal(ret, 0, "Cloned pkt read failed");
	zassert_mem_equal(read_buf, buf, sizeof(buf), "Cloned pkt data mismatch");

	net_pkt_unref(cloned_pkt);
	net_pkt_unref(pkt);
}

NET_BUF_POOL_FIXED_DEFINE(test_net_pkt_headroom_pool, 4, 2, 4, NULL);

ZTEST(net_pkt_test_suite, test_net_pkt_headroom)
{
	struct net_pkt *pkt;
	struct net_buf *frag1;
	struct net_buf *frag2;
	struct net_buf *frag3;
	struct net_buf *frag4;

	/*
	 * Create a net_pkt; append net_bufs with reserved bytes (headroom).
	 *
	 * Layout to be crafted before writing to the net_buf: "HA|HH|HA|AA"
	 *  H: Headroom
	 *  |: net_buf/fragment delimiter
	 *  A: available byte
	 */
	pkt = net_pkt_alloc_on_iface(eth_if, K_NO_WAIT);
	zassert_true(pkt != NULL, "Pkt not allocated");

	/* 1st fragment has 1 byte headroom and one byte available: "HA" */
	frag1 = net_buf_alloc_len(&test_net_pkt_headroom_pool, 2, K_NO_WAIT);
	net_buf_reserve(frag1, 1);
	net_pkt_append_buffer(pkt, frag1);
	zassert_equal(net_pkt_available_buffer(pkt), 1, "Wrong space left");
	zassert_equal(net_pkt_get_len(pkt), 0, "Length mismatch");

	/* 2nd fragment affecting neither size nor length: "HH" */
	frag2 = net_buf_alloc_len(&test_net_pkt_headroom_pool, 2, K_NO_WAIT);
	net_buf_reserve(frag2, 2);
	net_pkt_append_buffer(pkt, frag2);
	zassert_equal(net_pkt_available_buffer(pkt), 1, "Wrong space left");
	zassert_equal(net_pkt_get_len(pkt), 0, "Length mismatch");

	/* 3rd fragment has 1 byte headroom and one byte available: "HA" */
	frag3 = net_buf_alloc_len(&test_net_pkt_headroom_pool, 2, K_NO_WAIT);
	net_buf_reserve(frag3, 1);
	net_pkt_append_buffer(pkt, frag3);
	zassert_equal(net_pkt_available_buffer(pkt), 2, "Wrong space left");
	zassert_equal(net_pkt_get_len(pkt), 0, "Length mismatch");

	/* 4th fragment has no headroom and two available bytes: "AA" */
	frag4 = net_buf_alloc_len(&test_net_pkt_headroom_pool, 2, K_NO_WAIT);
	net_pkt_append_buffer(pkt, frag4);
	zassert_equal(net_pkt_available_buffer(pkt), 4, "Wrong space left");
	zassert_equal(net_pkt_get_len(pkt), 0, "Length mismatch");

	/* Writing net_pkt via cursor, spanning all 4 fragments */
	net_pkt_cursor_init(pkt);
	zassert_true(net_pkt_write(pkt, "1234", 4) == 0, "Pkt write failed");

	/* Expected layout across all four fragments: "H1|HH|H2|34" */
	zassert_equal(frag1->size, 2, "Size mismatch");
	zassert_equal(frag1->len, 1, "Length mismatch");
	zassert_equal(frag2->size, 2, "Size mismatch");
	zassert_equal(frag2->len, 0, "Length mismatch");
	zassert_equal(frag3->size, 2, "Size mismatch");
	zassert_equal(frag3->len, 1, "Length mismatch");
	zassert_equal(frag4->size, 2, "Size mismatch");
	zassert_equal(frag4->len, 2, "Length mismatch");
	net_pkt_cursor_init(pkt);
	zassert_true(net_pkt_read(pkt, small_buffer, 4) == 0, "Read failed");
	zassert_mem_equal(small_buffer, "1234", 4, "Data mismatch");

	/* Making use of the headrooms */
	net_buf_push_u8(frag3, 'D');
	net_buf_push_u8(frag2, 'C');
	net_buf_push_u8(frag2, 'B');
	net_buf_push_u8(frag1, 'A');
	net_pkt_cursor_init(pkt);
	zassert_true(net_pkt_read(pkt, small_buffer, 8) == 0, "Read failed");
	zassert_mem_equal(small_buffer, "A1BCD234", 8, "Data mismatch");

	net_pkt_unref(pkt);
}

NET_BUF_POOL_VAR_DEFINE(test_net_pkt_headroom_copy_pool, 2, 128, 4, NULL);

ZTEST(net_pkt_test_suite, test_net_pkt_headroom_copy)
{
	struct net_pkt *pkt_src;
	struct net_pkt *pkt_dst;
	struct net_buf *frag1_dst;
	struct net_buf *frag2_dst;
	int res;

	/* Create et_pkt containing the bytes "0123" */
	pkt_src = net_pkt_alloc_with_buffer(eth_if, 4,
					NET_AF_UNSPEC, 0, K_NO_WAIT);
	zassert_true(pkt_src != NULL, "Pkt not allocated");
	res = net_pkt_write(pkt_src, "0123", 4);
	zassert_equal(res, 0, "Pkt write failed");

	/* Create net_pkt consisting of net_buf fragments with reserved bytes */
	pkt_dst = net_pkt_alloc_on_iface(eth_if, K_NO_WAIT);
	zassert_true(pkt_src != NULL, "Pkt not allocated");

	frag1_dst = net_buf_alloc_len(&test_net_pkt_headroom_copy_pool, 2,
				      K_NO_WAIT);
	net_buf_reserve(frag1_dst, 1);
	net_pkt_append_buffer(pkt_dst, frag1_dst);
	frag2_dst = net_buf_alloc_len(&test_net_pkt_headroom_copy_pool, 4,
				      K_NO_WAIT);
	net_buf_reserve(frag2_dst, 1);
	net_pkt_append_buffer(pkt_dst, frag2_dst);
	zassert_equal(net_pkt_available_buffer(pkt_dst), 4, "Wrong space left");
	zassert_equal(net_pkt_get_len(pkt_dst), 0, "Length mismatch");

	/* Copy to net_pkt which contains fragments with reserved bytes */
	net_pkt_cursor_init(pkt_src);
	net_pkt_cursor_init(pkt_dst);
	res = net_pkt_copy(pkt_dst, pkt_src, 4);
	zassert_equal(res, 0, "Pkt copy failed");
	zassert_equal(net_pkt_available_buffer(pkt_dst), 0, "Wrong space left");
	zassert_equal(net_pkt_get_len(pkt_dst), 4, "Length mismatch");

	net_pkt_cursor_init(pkt_dst);
	zassert_true(net_pkt_read(pkt_dst, small_buffer, 4) == 0,
		     "Pkt read failed");
	zassert_mem_equal(small_buffer, "0123", 4, "Data mismatch");

	net_pkt_unref(pkt_dst);
	net_pkt_unref(pkt_src);
}

ZTEST(net_pkt_test_suite, test_net_pkt_get_contiguous_len)
{
	size_t cont_len;
	int res;
	/* Allocate pkt with 2 fragments */
	struct net_pkt *pkt = net_pkt_rx_alloc_with_buffer(
					   NULL, CONFIG_NET_BUF_DATA_SIZE * 2,
					   NET_AF_UNSPEC, 0, K_NO_WAIT);

	zassert_not_null(pkt, "Pkt not allocated");

	net_pkt_cursor_init(pkt);

	cont_len = net_pkt_get_contiguous_len(pkt);
	zassert_equal(CONFIG_NET_BUF_DATA_SIZE, cont_len,
		      "Expected one complete available net_buf");

	net_pkt_set_overwrite(pkt, false);

	/* now write 3 byte into the pkt */
	for (int i = 0; i < 3; ++i) {
		res = net_pkt_write_u8(pkt, 0xAA);
		zassert_equal(0, res, "Write packet failed");
	}

	cont_len = net_pkt_get_contiguous_len(pkt);
	zassert_equal(CONFIG_NET_BUF_DATA_SIZE - 3, cont_len,
		      "Expected a three byte reduction");

	/* Fill the first fragment up until only 3 bytes are free */
	for (int i = 0; i < CONFIG_NET_BUF_DATA_SIZE - 6; ++i) {
		res = net_pkt_write_u8(pkt, 0xAA);
		zassert_equal(0, res, "Write packet failed");
	}

	cont_len = net_pkt_get_contiguous_len(pkt);
	zassert_equal(3, cont_len, "Expected only three bytes are available");

	/* Fill the complete first fragment, so the cursor points to the second
	 * fragment.
	 */
	for (int i = 0; i < 3; ++i) {
		res = net_pkt_write_u8(pkt, 0xAA);
		zassert_equal(0, res, "Write packet failed");
	}

	cont_len = net_pkt_get_contiguous_len(pkt);
	zassert_equal(CONFIG_NET_BUF_DATA_SIZE, cont_len,
		      "Expected next full net_buf is available");

	/* Fill the last fragment */
	for (int i = 0; i < CONFIG_NET_BUF_DATA_SIZE; ++i) {
		res = net_pkt_write_u8(pkt, 0xAA);
		zassert_equal(0, res, "Write packet failed");
	}

	cont_len = net_pkt_get_contiguous_len(pkt);
	zassert_equal(0, cont_len, "Expected no available space");

	net_pkt_unref(pkt);
}

ZTEST(net_pkt_test_suite, test_net_pkt_remove_tail)
{
	struct net_pkt *pkt;
	int err;

	pkt = net_pkt_alloc_with_buffer(NULL,
					CONFIG_NET_BUF_DATA_SIZE * 2 + 3,
					NET_AF_UNSPEC, 0, K_NO_WAIT);
	zassert_true(pkt != NULL, "Pkt not allocated");

	net_pkt_cursor_init(pkt);
	net_pkt_write(pkt, small_buffer, CONFIG_NET_BUF_DATA_SIZE * 2 + 3);

	zassert_equal(net_pkt_get_len(pkt), CONFIG_NET_BUF_DATA_SIZE * 2 + 3,
		      "Pkt length is invalid");
	zassert_equal(pkt->frags->frags->frags->len, 3,
		      "3rd buffer length is invalid");

	/* Remove some bytes from last buffer */
	err = net_pkt_remove_tail(pkt, 2);
	zassert_equal(err, 0, "Failed to remove tail");

	zassert_equal(net_pkt_get_len(pkt), CONFIG_NET_BUF_DATA_SIZE * 2 + 1,
		      "Pkt length is invalid");
	zassert_not_equal(pkt->frags->frags->frags, NULL,
			  "3rd buffer was removed");
	zassert_equal(pkt->frags->frags->frags->len, 1,
		      "3rd buffer length is invalid");

	/* Remove last byte from last buffer */
	err = net_pkt_remove_tail(pkt, 1);
	zassert_equal(err, 0, "Failed to remove tail");

	zassert_equal(net_pkt_get_len(pkt), CONFIG_NET_BUF_DATA_SIZE * 2,
		      "Pkt length is invalid");
	zassert_equal(pkt->frags->frags->frags, NULL,
		      "3rd buffer was not removed");
	zassert_equal(pkt->frags->frags->len, CONFIG_NET_BUF_DATA_SIZE,
		      "2nd buffer length is invalid");

	/* Remove 2nd buffer and one byte from 1st buffer */
	err = net_pkt_remove_tail(pkt, CONFIG_NET_BUF_DATA_SIZE + 1);
	zassert_equal(err, 0, "Failed to remove tail");

	zassert_equal(net_pkt_get_len(pkt), CONFIG_NET_BUF_DATA_SIZE - 1,
		      "Pkt length is invalid");
	zassert_equal(pkt->frags->frags, NULL,
		      "2nd buffer was not removed");
	zassert_equal(pkt->frags->len, CONFIG_NET_BUF_DATA_SIZE - 1,
		      "1st buffer length is invalid");

	net_pkt_unref(pkt);

	pkt = net_pkt_rx_alloc_with_buffer(NULL,
					   CONFIG_NET_BUF_DATA_SIZE * 2 + 3,
					   NET_AF_UNSPEC, 0, K_NO_WAIT);

	net_pkt_cursor_init(pkt);
	net_pkt_write(pkt, small_buffer, CONFIG_NET_BUF_DATA_SIZE * 2 + 3);

	zassert_equal(net_pkt_get_len(pkt), CONFIG_NET_BUF_DATA_SIZE * 2 + 3,
		      "Pkt length is invalid");
	zassert_equal(pkt->frags->frags->frags->len, 3,
		      "3rd buffer length is invalid");

	/* Remove bytes spanning 3 buffers */
	err = net_pkt_remove_tail(pkt, CONFIG_NET_BUF_DATA_SIZE + 5);
	zassert_equal(err, 0, "Failed to remove tail");

	zassert_equal(net_pkt_get_len(pkt), CONFIG_NET_BUF_DATA_SIZE - 2,
		      "Pkt length is invalid");
	zassert_equal(pkt->frags->frags, NULL,
		      "2nd buffer was not removed");
	zassert_equal(pkt->frags->len, CONFIG_NET_BUF_DATA_SIZE - 2,
		      "1st buffer length is invalid");

	/* Try to remove more bytes than packet has */
	err = net_pkt_remove_tail(pkt, CONFIG_NET_BUF_DATA_SIZE);
	zassert_equal(err, -EINVAL,
		      "Removing more bytes than available should fail");

	net_pkt_unref(pkt);
}

ZTEST(net_pkt_test_suite, test_net_pkt_shallow_clone_noleak_buf)
{
	const int bufs_to_allocate = 3;
	const size_t pkt_size = CONFIG_NET_BUF_DATA_SIZE * bufs_to_allocate;
	struct net_pkt *pkt, *shallow_pkt;
	struct net_buf_pool *tx_data;

	pkt = net_pkt_alloc_with_buffer(NULL, pkt_size,
					NET_AF_UNSPEC, 0, K_NO_WAIT);

	zassert_true(pkt != NULL, "Pkt not allocated");

	net_pkt_get_info(NULL, NULL, NULL, &tx_data);
	zassert_equal(atomic_get(&tx_data->avail_count), tx_data->buf_count - bufs_to_allocate,
		      "Incorrect net buf allocation");

	shallow_pkt = net_pkt_shallow_clone(pkt, K_NO_WAIT);
	zassert_true(shallow_pkt != NULL, "Pkt not allocated");
	zassert_equal(atomic_get(&tx_data->avail_count), tx_data->buf_count - bufs_to_allocate,
		      "Incorrect available net buf count");

	net_pkt_unref(pkt);
	zassert_equal(atomic_get(&tx_data->avail_count), tx_data->buf_count - bufs_to_allocate,
		      "Incorrect available net buf count");

	net_pkt_unref(shallow_pkt);
	zassert_equal(atomic_get(&tx_data->avail_count), tx_data->buf_count,
		      "Leak detected");

}

void test_net_pkt_shallow_clone_append_buf(int extra_frag_refcounts)
{
	const int bufs_to_allocate = 3;
	const int bufs_frag = 2;

	zassert_true(bufs_frag + bufs_to_allocate < CONFIG_NET_BUF_DATA_SIZE,
		     "Total bufs to allocate must less than available space");

	const size_t pkt_size = CONFIG_NET_BUF_DATA_SIZE * bufs_to_allocate;

	struct net_pkt *pkt, *shallow_pkt;
	struct net_buf *frag_head;
	struct net_buf *frag;
	struct net_buf_pool *tx_data;

	pkt = net_pkt_alloc_with_buffer(NULL, pkt_size, NET_AF_UNSPEC, 0, K_NO_WAIT);
	zassert_true(pkt != NULL, "Pkt not allocated");

	net_pkt_get_info(NULL, NULL, NULL, &tx_data);
	zassert_equal(atomic_get(&tx_data->avail_count), tx_data->buf_count
		      - bufs_to_allocate, "Incorrect net buf allocation");

	shallow_pkt = net_pkt_shallow_clone(pkt, K_NO_WAIT);
	zassert_true(shallow_pkt != NULL, "Pkt not allocated");

	/* allocate buffers for the frag */
	for (int i = 0; i < bufs_frag; i++) {
		frag = net_buf_alloc_len(tx_data, CONFIG_NET_BUF_DATA_SIZE, K_NO_WAIT);
		zassert_true(frag != NULL, "Frag not allocated");
		net_pkt_append_buffer(pkt, frag);
		if (i == 0) {
			frag_head = frag;
		}
	}

	zassert_equal(atomic_get(&tx_data->avail_count), tx_data->buf_count
		      - bufs_to_allocate - bufs_frag, "Incorrect net buf allocation");

	/* Note: if the frag is appended to a net buf, then the nut buf */
	/* takes ownership of one ref count. Otherwise net_buf_unref() must */
	/* be called on the frag to free the buffers. */

	for (int i = 0; i < extra_frag_refcounts; i++) {
		frag_head = net_buf_ref(frag_head);
	}

	net_pkt_unref(pkt);

	/* we shouldn't have freed any buffers yet */
	zassert_equal(atomic_get(&tx_data->avail_count), tx_data->buf_count
		      - bufs_to_allocate - bufs_frag,
		      "Incorrect net buf allocation");

	net_pkt_unref(shallow_pkt);

	if (extra_frag_refcounts == 0) {
		/* if no extra ref counts to frag were added then we should free */
		/* all the buffers at this point */
		zassert_equal(atomic_get(&tx_data->avail_count), tx_data->buf_count,
			      "Leak detected");
	} else {
		/* otherwise only bufs_frag should be available, and frag could */
		/* still used at this point */
		zassert_equal(atomic_get(&tx_data->avail_count),
			      tx_data->buf_count - bufs_frag, "Leak detected");
	}

	for (int i = 0; i < extra_frag_refcounts; i++) {
		net_buf_unref(frag_head);
	}

	/* all the buffers should be freed now */
	zassert_equal(atomic_get(&tx_data->avail_count), tx_data->buf_count,
		      "Leak detected");
}

ZTEST(net_pkt_test_suite, test_net_pkt_shallow_clone_append_buf_0)
{
	test_net_pkt_shallow_clone_append_buf(0);
}

ZTEST(net_pkt_test_suite, test_net_pkt_shallow_clone_append_buf_1)
{
	test_net_pkt_shallow_clone_append_buf(1);
}

ZTEST(net_pkt_test_suite, test_net_pkt_shallow_clone_append_buf_2)
{
	test_net_pkt_shallow_clone_append_buf(2);
}

/* Buffers held to keep the data pool empty while a blocking allocation runs. */
static struct net_buf *pool_hog;

static struct net_buf *drain_data_pool(struct net_buf_pool *pool)
{
	struct net_buf *head = NULL;
	struct net_buf *buf;

	while ((buf = net_buf_alloc_len(pool, CONFIG_NET_BUF_DATA_SIZE,
					K_NO_WAIT)) != NULL) {
		buf->frags = head;
		head = buf;
	}

	return head;
}

static void release_pool_hog(struct k_work *work)
{
	ARG_UNUSED(work);

	if (pool_hog != NULL) {
		net_buf_unref(pool_hog);
		pool_hog = NULL;
	}
}

static K_WORK_DELAYABLE_DEFINE(pool_hog_work, release_pool_hog);

/* The allocator tries a non-blocking allocation before waiting. Check that a
 * caller which asked to wait still waits, and still gets its buffers once they
 * are returned to the pool.
 */
ZTEST(net_pkt_test_suite, test_net_pkt_alloc_buffer_blocking_succeeds)
{
	struct net_buf_pool *tx_data;
	struct net_pkt *pkt;
	int64_t start;

	net_pkt_get_info(NULL, NULL, NULL, &tx_data);

	pool_hog = drain_data_pool(tx_data);
	zassert_not_null(pool_hog, "Data pool was already empty");
	zassert_equal(atomic_get(&tx_data->avail_count), 0,
		      "Data pool not drained");

	k_work_schedule(&pool_hog_work, K_MSEC(50));

	start = k_uptime_get();
	pkt = net_pkt_alloc_with_buffer(NULL, CONFIG_NET_BUF_DATA_SIZE * 3,
					NET_AF_UNSPEC, 0, K_MSEC(2000));

	zassert_not_null(pkt, "Blocking allocation failed even though the "
			      "buffers were returned to the pool");
	zassert_true(k_uptime_get() - start >= 50,
		     "Allocation returned without waiting");

	net_pkt_unref(pkt);
	release_pool_hog(NULL);
}

/* A caller that asked to wait for a given time must not wait appreciably
 * longer. The upper bound is the point of this test: it fails if the deadline
 * is ever restarted per fragment instead of being held across the chain.
 */
ZTEST(net_pkt_test_suite, test_net_pkt_alloc_buffer_blocking_times_out)
{
	struct net_buf_pool *tx_data;
	struct net_pkt *pkt;
	int64_t elapsed;

	net_pkt_get_info(NULL, NULL, NULL, &tx_data);

	pool_hog = drain_data_pool(tx_data);
	zassert_not_null(pool_hog, "Data pool was already empty");

	elapsed = k_uptime_get();
	pkt = net_pkt_alloc_with_buffer(NULL, CONFIG_NET_BUF_DATA_SIZE * 3,
					NET_AF_UNSPEC, 0, K_MSEC(100));
	elapsed = k_uptime_get() - elapsed;

	zassert_is_null(pkt, "Allocation succeeded with an empty pool");
	zassert_true(elapsed >= 100, "Allocation gave up after %lld ms",
		     elapsed);
	zassert_true(elapsed < 300, "Allocation waited %lld ms for a 100 ms "
		     "timeout", elapsed);

	release_pool_hog(NULL);
}

/* Buffers handed back one at a time, so that a multi fragment allocation has
 * to wait more than once.
 */
static struct net_buf *trickle_hog;

static void trickle_release(struct k_work *work)
{
	struct net_buf *buf = trickle_hog;

	if (buf == NULL) {
		return;
	}

	trickle_hog = buf->frags;
	buf->frags = NULL;
	net_buf_unref(buf);

	if (trickle_hog != NULL) {
		k_work_reschedule(k_work_delayable_from_work(work), K_MSEC(60));
	}
}

static K_WORK_DELAYABLE_DEFINE(trickle_work, trickle_release);

/* The timeout is a budget for the whole allocation, not for each fragment of
 * it. With one buffer coming back every 60 ms, a three fragment allocation
 * given 100 ms must give up: it can only ever get the first fragment. If the
 * deadline were restarted for each fragment it would instead collect all
 * three and return after about 180 ms.
 */
ZTEST(net_pkt_test_suite, test_net_pkt_alloc_buffer_deadline_spans_chain)
{
	struct net_buf_pool *tx_data;
	struct net_pkt *pkt;
	int64_t elapsed;

	net_pkt_get_info(NULL, NULL, NULL, &tx_data);

	trickle_hog = drain_data_pool(tx_data);
	zassert_not_null(trickle_hog, "Data pool was already empty");

	k_work_schedule(&trickle_work, K_MSEC(60));

	elapsed = k_uptime_get();
	pkt = net_pkt_alloc_with_buffer(NULL, CONFIG_NET_BUF_DATA_SIZE * 3,
					NET_AF_UNSPEC, 0, K_MSEC(100));
	elapsed = k_uptime_get() - elapsed;

	if (pkt != NULL) {
		net_pkt_unref(pkt);
	}

	(void)k_work_cancel_delayable(&trickle_work);
	if (trickle_hog != NULL) {
		net_buf_unref(trickle_hog);
		trickle_hog = NULL;
	}

	zassert_is_null(pkt, "Allocation collected every fragment in %lld ms "
			"despite a 100 ms timeout, so the deadline is being "
			"restarted per fragment", elapsed);
	zassert_true(elapsed < 150, "Allocation waited %lld ms for a 100 ms "
		     "timeout", elapsed);
}

/* When only part of the chain can be allocated the buffers taken so far must
 * go back to the pool.
 */
ZTEST(net_pkt_test_suite, test_net_pkt_alloc_buffer_partial_chain)
{
	struct net_buf_pool *tx_data;
	struct net_buf *spare;
	struct net_pkt *pkt;

	net_pkt_get_info(NULL, NULL, NULL, &tx_data);

	pool_hog = drain_data_pool(tx_data);
	zassert_not_null(pool_hog, "Data pool was already empty");

	/* Hand one buffer back, so the first fragment succeeds and the
	 * second one has to give up.
	 */
	spare = pool_hog;
	pool_hog = spare->frags;
	spare->frags = NULL;
	net_buf_unref(spare);

	zassert_equal(atomic_get(&tx_data->avail_count), 1,
		      "Expected exactly one buffer to be available");

	pkt = net_pkt_alloc_with_buffer(NULL, CONFIG_NET_BUF_DATA_SIZE * 3,
					NET_AF_UNSPEC, 0, K_MSEC(100));

	zassert_is_null(pkt, "Allocation succeeded without enough buffers");
	zassert_equal(atomic_get(&tx_data->avail_count), 1,
		      "Partially allocated chain was not returned to the pool");

	release_pool_hog(NULL);
}

/* A non-blocking caller must still fail immediately on an empty pool, and must
 * not leak the buffer it probed for.
 */
ZTEST(net_pkt_test_suite, test_net_pkt_alloc_buffer_nowait_exhausted)
{
	struct net_buf_pool *tx_data;
	struct net_pkt *pkt;

	net_pkt_get_info(NULL, NULL, NULL, &tx_data);

	pool_hog = drain_data_pool(tx_data);
	zassert_not_null(pool_hog, "Data pool was already empty");

	pkt = net_pkt_alloc_with_buffer(NULL, CONFIG_NET_BUF_DATA_SIZE,
					NET_AF_UNSPEC, 0, K_NO_WAIT);

	zassert_is_null(pkt, "Allocation succeeded with an empty pool");
	zassert_equal(atomic_get(&tx_data->avail_count), 0,
		      "A failed allocation returned a buffer to the pool");

	release_pool_hog(NULL);
}

ZTEST(net_pkt_test_suite, test_net_pkt_frag_double_free)
{
	struct net_buf_pool *tx_data;
	struct net_buf *frag;
	atomic_val_t avail;
	uint8_t pool_id;
	uint8_t user_data_size;

	if (IS_ENABLED(CONFIG_ASSERT)) {
		/* An assertion catches the double free before it returns */
		ztest_test_skip();
	}

	net_pkt_get_info(NULL, NULL, NULL, &tx_data);

	frag = net_pkt_get_reserve_tx_data(16, K_NO_WAIT);
	zassert_not_null(frag, "Failed to get fragment");

	pool_id = frag->pool_id;
	user_data_size = frag->user_data_size;

	net_pkt_frag_unref(frag);
	zassert_equal(frag->ref, 0U, "Freed fragment has a reference");
	avail = atomic_get(&tx_data->avail_count);

	/* A second unref has to leave the freed fragment as it is */
	net_pkt_frag_unref(frag);
	zexpect_equal(atomic_get(&tx_data->avail_count), avail, "Fragment freed twice");
	zexpect_equal(frag->ref, 0U, "Reference count changed to %u", frag->ref);
	zexpect_equal(frag->flags, 0U, "Flags changed to 0x%02x", frag->flags);
	zexpect_equal(frag->pool_id, pool_id, "Pool id changed from %u to %u", pool_id,
		      frag->pool_id);
	zexpect_equal(frag->user_data_size, user_data_size, "User data size changed from %u to %u",
		      user_data_size, frag->user_data_size);
}

ZTEST(net_pkt_test_suite, test_net_pkt_frag_ref_saturated)
{
	struct net_buf_pool *tx_data;
	struct net_buf *frag;
	atomic_val_t avail;

	if (IS_ENABLED(CONFIG_ASSERT)) {
		/* An assertion catches the saturation before it returns */
		ztest_test_skip();
	}

	net_pkt_get_info(NULL, NULL, NULL, &tx_data);

	frag = net_pkt_get_reserve_tx_data(16, K_NO_WAIT);
	zassert_not_null(frag, "Failed to get fragment");

	/* One reference more than the count can hold */
	for (size_t i = 0; i < UINT8_MAX; i++) {
		zassert_equal_ptr(net_pkt_frag_ref(frag), frag);
	}

	avail = atomic_get(&tx_data->avail_count);

	/* The count no longer tells when the last reference is dropped, so the
	 * fragment is never freed.
	 */
	for (size_t i = 0; i <= UINT8_MAX; i++) {
		net_pkt_frag_unref(frag);
	}

	zassert_equal(atomic_get(&tx_data->avail_count), avail,
		      "Fragment freed with a saturated reference count");
	zassert_equal(frag->ref, UINT8_MAX, "Reference count changed to %u", frag->ref);
	zexpect_equal(frag->flags, 0U, "Flags changed to 0x%02x", frag->flags);

	/* All references have been dropped: give the fragment back to its pool */
	frag->ref = 1U;
	net_pkt_frag_unref(frag);
	zassert_equal(atomic_get(&tx_data->avail_count), avail + 1, "Fragment not freed");
}

#if defined(CONFIG_NET_PKT_TX_VIEWS)
#define VIEW_FRAG_LEN 40U
#define VIEW_FRAGS    3U

static uint8_t view_pattern[VIEW_FRAG_LEN * VIEW_FRAGS];

/* A TX packet with VIEW_FRAGS fragments of VIEW_FRAG_LEN bytes of a pattern */
static struct net_pkt *view_src_alloc(struct net_buf *frags[VIEW_FRAGS])
{
	struct net_pkt *pkt;

	for (size_t i = 0; i < sizeof(view_pattern); i++) {
		view_pattern[i] = (uint8_t)(i * 7U + 1U);
	}

	pkt = net_pkt_alloc(K_NO_WAIT);
	zassert_not_null(pkt, "Failed to get packet");

	for (size_t i = 0; i < VIEW_FRAGS; i++) {
		frags[i] = net_pkt_get_reserve_tx_data(VIEW_FRAG_LEN, K_NO_WAIT);
		zassert_not_null(frags[i], "Failed to get fragment");
		zassert_true(net_buf_tailroom(frags[i]) >= VIEW_FRAG_LEN, "Fragment too small");

		net_buf_add_mem(frags[i], &view_pattern[i * VIEW_FRAG_LEN], VIEW_FRAG_LEN);
		net_pkt_frag_add(pkt, frags[i]);
	}

	return pkt;
}

static size_t view_count(struct net_buf *views)
{
	size_t count = 0U;

	for (struct net_buf *buf = views; buf != NULL; buf = buf->frags) {
		count++;
	}

	return count;
}

static void view_expect_data(struct net_buf *views, size_t pos, size_t len)
{
	uint8_t data[sizeof(view_pattern)];
	size_t copied;

	copied = net_buf_linearize(data, sizeof(data), views, 0, len);
	zexpect_equal(copied, len, "Copied %zu bytes instead of %zu", copied, len);
	zexpect_mem_equal(data, &view_pattern[pos], len, "View data differs");
}

ZTEST(net_pkt_test_suite, test_net_pkt_view_data)
{
	struct net_buf *frags[VIEW_FRAGS];
	struct net_pkt *src;
	struct net_buf *views;

	src = view_src_alloc(frags);

	/* From inside the first fragment to inside the last one */
	views = net_pkt_get_view(src, VIEW_FRAG_LEN - 10U, VIEW_FRAG_LEN + 20U, VIEW_FRAGS);
	zassert_not_null(views, "No views");

	zexpect_equal(view_count(views), 3U, "Got %zu views", view_count(views));
	zexpect_equal(net_buf_frags_len(views), VIEW_FRAG_LEN + 20U, "Views of %zu bytes",
		      net_buf_frags_len(views));
	view_expect_data(views, VIEW_FRAG_LEN - 10U, VIEW_FRAG_LEN + 20U);

	for (struct net_buf *view = views; view != NULL; view = view->frags) {
		zexpect_equal(net_buf_headroom(view), 0U, "View has headroom");
		zexpect_equal(net_buf_tailroom(view), 0U, "View has tailroom");
	}

	/* The source is left as it was */
	zexpect_equal_ptr(src->buffer, frags[0], "Source buffer changed");
	zexpect_equal(net_pkt_get_len(src), sizeof(view_pattern), "Source length changed");
	zexpect_equal(frags[1]->len, VIEW_FRAG_LEN, "Source fragment changed");

	net_pkt_frag_unref(views);

	/* Exactly one whole fragment */
	views = net_pkt_get_view(src, VIEW_FRAG_LEN, VIEW_FRAG_LEN, 1U);
	zassert_not_null(views, "No view of one fragment");
	zexpect_equal(view_count(views), 1U, "Got %zu views", view_count(views));
	view_expect_data(views, VIEW_FRAG_LEN, VIEW_FRAG_LEN);
	net_pkt_frag_unref(views);

	for (size_t i = 0; i < VIEW_FRAGS; i++) {
		zexpect_equal(frags[i]->ref, 1U, "Fragment %zu has %u references", i,
			      frags[i]->ref);
	}

	net_pkt_unref(src);
}

ZTEST(net_pkt_test_suite, test_net_pkt_view_outlives_source)
{
	struct net_buf *frags[VIEW_FRAGS];
	struct net_buf_pool *tx_data;
	struct net_buf_pool *view_pool;
	atomic_val_t data_avail;
	atomic_val_t view_avail;
	struct net_pkt *src;
	struct net_buf *views;
	struct net_buf *reused[CONFIG_NET_BUF_TX_COUNT];
	struct net_buf *last;
	uint8_t more[VIEW_FRAG_LEN / 2U];
	size_t count = 0U;
	size_t fill;

	net_pkt_get_info(NULL, NULL, NULL, &tx_data);
	data_avail = atomic_get(&tx_data->avail_count);

	src = view_src_alloc(frags);

	views = net_pkt_get_view(src, 0U, sizeof(view_pattern), VIEW_FRAGS);
	zassert_not_null(views, "No views");

	view_pool = net_buf_pool_get(views->pool_id);
	view_avail = atomic_get(&view_pool->avail_count);

	/* Drop the data in front of the last fragment, as TCP does when the
	 * peer acknowledges it, then append to the last fragment.
	 */
	net_pkt_cursor_init(src);
	net_pkt_set_overwrite(src, true);
	zassert_ok(net_pkt_pull(src, 2U * VIEW_FRAG_LEN + 5U), "Pull failed");
	net_pkt_trim_buffer(src);
	zexpect_equal_ptr(src->buffer, frags[2], "Pulled fragments still linked");

	last = net_buf_frag_last(src->buffer);
	memset(more, 0xaa, sizeof(more));
	if (net_buf_tailroom(last) >= sizeof(more)) {
		net_buf_add_mem(last, more, sizeof(more));
	}

	view_expect_data(views, 0U, sizeof(view_pattern));

	/* The source goes away, the views keep the data. Filling every free
	 * fragment of the pool must not reach the data the views point into.
	 */
	net_pkt_unref(src);
	zexpect_equal(atomic_get(&tx_data->avail_count), data_avail - VIEW_FRAGS,
		      "Fragments freed while views point into them");

	while (count < ARRAY_SIZE(reused)) {
		reused[count] = net_pkt_get_reserve_tx_data(1U, K_NO_WAIT);
		if (reused[count] == NULL) {
			break;
		}

		fill = net_buf_tailroom(reused[count]);
		memset(net_buf_add(reused[count], fill), 0x55, fill);
		count++;
	}

	view_expect_data(views, 0U, sizeof(view_pattern));

	while (count > 0U) {
		net_pkt_frag_unref(reused[--count]);
	}

	net_pkt_frag_unref(views);
	zexpect_equal(atomic_get(&tx_data->avail_count), data_avail, "Fragments not freed");
	zexpect_equal(atomic_get(&view_pool->avail_count), view_avail + VIEW_FRAGS,
		      "Views not freed");
}

ZTEST(net_pkt_test_suite, test_net_pkt_view_refused)
{
	struct net_buf *frags[VIEW_FRAGS];
	struct net_buf_pool *view_pool;
	struct net_buf *held[CONFIG_NET_BUF_TX_VIEW_COUNT];
	atomic_val_t view_avail;
	struct net_pkt *src;
	struct net_buf *views;
	size_t count = 0U;

	src = view_src_alloc(frags);

	views = net_pkt_get_view(src, 0U, 1U, 1U);
	zassert_not_null(views, "No view");
	view_pool = net_buf_pool_get(views->pool_id);
	net_pkt_frag_unref(views);
	view_avail = atomic_get(&view_pool->avail_count);

	zexpect_is_null(net_pkt_get_view(src, 0U, 0U, VIEW_FRAGS), "View of no data");
	zexpect_is_null(net_pkt_get_view(src, 1U, sizeof(view_pattern), VIEW_FRAGS),
			"View past the end");
	zexpect_is_null(net_pkt_get_view(src, sizeof(view_pattern), 1U, VIEW_FRAGS),
			"View from the end");
	zexpect_is_null(net_pkt_get_view(src, VIEW_FRAG_LEN - 1U, 2U, 1U),
			"Two views where one is allowed");

	frags[1]->ref = UINT8_MAX - 1U;
	zexpect_is_null(net_pkt_get_view(src, VIEW_FRAG_LEN, 1U, 1U),
			"View of a fragment with a nearly saturated count");
	frags[1]->ref = 1U;

	zexpect_equal(atomic_get(&view_pool->avail_count), view_avail, "Views leaked");

	/* Leave two views free, then ask for three */
	while (atomic_get(&view_pool->avail_count) > 2) {
		held[count] = net_pkt_get_view(src, 0U, 1U, 1U);
		zassert_not_null(held[count], "No view left after %zu", count);
		count++;
	}

	zexpect_is_null(net_pkt_get_view(src, 0U, sizeof(view_pattern), VIEW_FRAGS),
			"Got three views with two free");
	zexpect_equal(atomic_get(&view_pool->avail_count), 2, "Views leaked on failure");
	zexpect_equal(frags[1]->ref, 1U, "Reference leaked on failure");
	zexpect_equal(frags[2]->ref, 1U, "Reference leaked on failure");

	while (count > 0U) {
		net_pkt_frag_unref(held[--count]);
	}

	zexpect_equal(frags[0]->ref, 1U, "Reference leaked");
	zexpect_equal(atomic_get(&view_pool->avail_count), view_avail, "Views leaked");

	net_pkt_unref(src);
}
#endif /* CONFIG_NET_PKT_TX_VIEWS */

ZTEST_SUITE(net_pkt_test_suite, NULL, NULL, NULL, NULL, NULL);
